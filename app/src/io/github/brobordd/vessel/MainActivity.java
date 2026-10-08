/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
package io.github.brobordd.vessel;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.pm.ConfigurationInfo;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.system.Os;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends Activity implements SurfaceHolder.Callback {

    static final boolean BURNED_IN = false;
    /** true: the GPU draws the game (OpenGL ES 3.0, see GfxRenderer); falls back to the CPU path if the device has no ES 3.0.
     *  false: always the old CPU path. */
    static final boolean GPU = true;
    static final int SCALE = 2;
    static final int RATE = 44100;

    SurfaceView sv;           // CPU path
    GLSurfaceView gv;         // GPU path
    GfxRenderer gfx;
    boolean gpu;
    volatile Process proc;
    OutputStream stdin;
    ExecutorService inputQ = Executors.newSingleThreadExecutor();
    volatile boolean running;
    int w, h, sw, sh;

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        if (Build.VERSION.SDK_INT >= 28) {
            getWindow().getAttributes().layoutInDisplayCutoutMode =
                    WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }
        setVolumeControlStream(AudioManager.STREAM_MUSIC);
        gpu = GPU && hasEs3();
        if (gpu) {
            try {
                gfx = new GfxRenderer(this);
            } catch (Exception e) {
                Log.e("game", "gpu: setup failed, using the CPU path", e);
                gpu = false;
            }
        }
        View v;
        if (gpu) {
            gv = new GLSurfaceView(this);
            gv.setEGLContextClientVersion(3);
            gv.setEGLConfigChooser(8, 8, 8, 0, 0, 0);
            gv.setRenderer(gfx);
            gv.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
            gv.getHolder().addCallback(this);     // for surfaceDestroyed: the game stops with the surface
            v = gv;
        } else {
            sv = new SurfaceView(this);
            sv.getHolder().addCallback(this);
            v = sv;
        }
        Log.i("game", "render path: " + (gpu ? "gpu" : "cpu"));
        setContentView(v);
        v.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
    }

    boolean hasEs3() {
        try {
            ActivityManager am = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
            ConfigurationInfo ci = am.getDeviceConfigurationInfo();
            return ci.reqGlEsVersion >= 0x30000;
        } catch (Exception e) {
            return false;
        }
    }

    @Override
    protected void onPause() {
        if (gv != null) gv.onPause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (gv != null) gv.onResume();
    }

    @Override public void surfaceCreated(SurfaceHolder holder) {}

    @Override
    public void surfaceChanged(SurfaceHolder holder, int fmt, int width, int height) {
        if (gpu || proc != null) return;      // the GPU path starts the game from GfxRenderer.onSurfaceChanged
        sw = width; sh = height;
        w = width / SCALE; h = height / SCALE;
        start(holder);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        running = false;
        if (proc != null) { proc.destroy(); proc = null; }
    }

    // --- audio: read PCM (S16 stereo 44100) from the FIFO, play via AudioTrack ---
    void startAudio() {
        final File fifo = new File(getFilesDir(), "audio.pcm");
        try {
            fifo.delete();
            Os.mkfifo(fifo.getAbsolutePath(), 0600);
        } catch (Exception e) {
            Log.e("game", "mkfifo failed", e);
            return;
        }
        new Thread(new Runnable() {
            public void run() {
                AudioTrack at = null;
                try {
                    int min = AudioTrack.getMinBufferSize(RATE,
                            AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
                    // 2x the minimum (was 4x): less sound queued up = less delay between the game
                    // mixing a sound and you hearing it (the dialog blips line up with the letters)
                    at = new AudioTrack(AudioManager.STREAM_MUSIC, RATE,
                            AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT,
                            min * 2, AudioTrack.MODE_STREAM);
                    at.play();
                    FileInputStream in = new FileInputStream(fifo); // blocks until game opens it
                    byte[] buf = new byte[4096];
                    int n;
                    while (running && (n = in.read(buf)) > 0) at.write(buf, 0, n);
                    in.close();
                } catch (Exception e) {
                    Log.e("game", "audio", e);
                } finally {
                    if (at != null) { at.stop(); at.release(); }
                }
            }
        }).start();
    }

    File gameBinary() {
        return BURNED_IN
                ? new File(getApplicationInfo().nativeLibraryDir, "libgame.so")
                : new File(getFilesDir(), "game");
    }

    /** Starts the game process: "game <w> <h> <gpu|cpu>", stdout goes to logcat (tag "game"), touches go to its stdin. */
    Process spawn(File bin, String mode) throws Exception {
        final Process p = new ProcessBuilder(bin.getAbsolutePath(), "" + w, "" + h, mode)
                .directory(getFilesDir())
                .redirectErrorStream(true)
                .start();
        stdin = p.getOutputStream();
        new Thread(new Runnable() {
            public void run() {
                try {
                    BufferedReader r = new BufferedReader(
                            new InputStreamReader(p.getInputStream()));
                    String l;
                    while ((l = r.readLine()) != null) Log.i("game", l);
                    Log.i("game", "[exited " + p.waitFor() + "]");
                } catch (Exception e) {
                    Log.e("game", "reader", e);
                }
            }
        }).start();
        return p;
    }

    /** GPU path: called on the GL thread once the surface (and so the game's pixel size) is known. */
    void startGpu(int gw, int gh) {
        if (proc != null) return;
        w = gw; h = gh;
        final File bin = gameBinary();
        if (!bin.exists()) {
            Log.e("game", "NOT FOUND: " + bin);
            return;
        }
        bin.setExecutable(true, false);
        try {
            gfx.reset();                       // the game fills the shared file in as it starts
            running = true;
            startAudio();
            proc = spawn(bin, "gpu");
        } catch (Exception e) {
            Log.e("game", "start failed", e);
        }
    }

    /** CPU path: the game draws pixels into a shared file, a thread copies them to the screen. */
    void start(final SurfaceHolder holder) {
        final File bin = gameBinary();
        if (!bin.exists()) {
            Log.e("game", "NOT FOUND: " + bin);
            return;
        }
        bin.setExecutable(true, false);

        try {
            File fb = new File(getFilesDir(), "fb");
            RandomAccessFile raf = new RandomAccessFile(fb, "rw");
            raf.setLength((long) w * h * 4);
            final MappedByteBuffer map = raf.getChannel()
                    .map(FileChannel.MapMode.READ_WRITE, 0, (long) w * h * 4);

            running = true;
            startAudio();

            proc = spawn(bin, "cpu");

            new Thread(new Runnable() {
                public void run() {
                    Bitmap bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
                    Rect dst = new Rect(0, 0, sw, sh);
                    while (running) {
                        map.rewind();
                        bmp.copyPixelsFromBuffer(map);
                        // GPU-backed canvas (API 26+); CPU lockCanvas() was the bottleneck.
                        // Posting is paced by the buffer queue (vsync), so no sleep there.
                        boolean hw = Build.VERSION.SDK_INT >= 26;
                        Canvas c = hw ? holder.lockHardwareCanvas() : holder.lockCanvas();
                        if (c != null) {
                            c.drawBitmap(bmp, null, dst, null);
                            holder.unlockCanvasAndPost(c);
                        }
                        if (!hw) {
                            try { Thread.sleep(16); } catch (InterruptedException e) { return; }
                        }
                    }
                }
            }).start();
        } catch (Exception e) {
            Log.e("game", "start failed", e);
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        final String line = "t " + e.getActionMasked() + " "
                + (int) (e.getX() / SCALE) + " " + (int) (e.getY() / SCALE) + "\n";
        inputQ.execute(new Runnable() {
            public void run() {
                try {
                    if (stdin != null) { stdin.write(line.getBytes()); stdin.flush(); }
                } catch (Exception ignored) {}
            }
        });
        return true;
    }

    @Override
    protected void onDestroy() {
        running = false;
        if (proc != null) proc.destroy();
        inputQ.shutdownNow();
        super.onDestroy();
    }
}
