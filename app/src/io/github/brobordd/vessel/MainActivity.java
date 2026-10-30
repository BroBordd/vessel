/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
package io.github.brobordd.vessel;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.pm.ConfigurationInfo;
import android.graphics.Bitmap;
import android.content.res.Configuration;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTimestamp;
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
import java.nio.ByteBuffer;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;

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
    volatile OutputStream stdin;
    ExecutorService inputQ = Executors.newSingleThreadExecutor();
    volatile boolean running;     // the game process is meant to be alive: the audio thread and the CPU blitter keep going
    volatile boolean paused;      // the app is in the background: the audio waits, and the game was told to wait too

    /** The size in pixels of the picture the game is drawing NOW. The first surface decides the size the game starts with. On the GPU path the game then
     *  follows the screen (game/screen.h: it takes the screen size / SCALE after every "r" line) and GfxRenderer updates these from the frames' own
     *  size; until a frame of the new size arrives the old picture stays, letterboxed by fit(). On the CPU path it never changes (always letterboxed).
     *  SCALE must equal SCREEN_SCALE in game/screen.h. */
    volatile int w, h;
    volatile int sw, sh;                          // the surface (screen) size now
    volatile int viewX, viewY;                    // where the game's picture sits on the screen, in screen pixels
    volatile float viewScale = SCALE;             // screen pixels per game pixel

    // CPU path
    MappedByteBuffer fbMap;
    volatile int blitGen;                         // a blitter thread runs while its generation is the current one

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
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        hideSystemUi();
    }

    void hideSystemUi() {
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) hideSystemUi();             // the bars come back after a dialog, a notification shade, a rotation...
    }

    @Override
    public void onConfigurationChanged(Configuration c) {
        super.onConfigurationChanged(c);          // (we handle the changes ourselves, see the manifest: the activity is never recreated)
        hideSystemUi();
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

    /** The game process lives as long as the activity does: going to the home screen only PAUSES it (the audio stops, the game waits, the
     *  surface may go away), and coming back carries on exactly where it was. */
    @Override
    protected void onPause() {
        paused = true;
        send("p 1\n");                            // the game pauses the world and stops drawing
        if (gv != null) gv.onPause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        paused = false;                           // the audio thread wakes up first, so the game's audio is not stuck when it hears us
        hideSystemUi();
        if (gv != null) gv.onResume();
        send("p 0\n");
        if (gpu && w > 0 && !alive()) startGpu(w, h);          // the system killed the game while we were away: start it again
    }

    boolean alive() {
        Process p = proc;
        if (p == null) return false;
        try { p.exitValue(); return false; } catch (IllegalThreadStateException e) { return true; }
    }

    /** The back button never closes the app: the game gets it ("k 4") and closes a window, or pauses. If it has nothing to do with it, it
     *  answers "back: unhandled" and the app goes to the background instead (see spawn). */
    @Override
    public void onBackPressed() {
        if (!alive()) { super.onBackPressed(); return; }      // no game running (not installed?): do not trap the user
        send("k 4\n");
    }

    /** Sends one line to the game's stdin (on a worker thread, never blocks the UI). */
    void send(final String line) {
        try {
            inputQ.execute(new Runnable() {
                public void run() {
                    try {
                        OutputStream o = stdin;
                        if (o != null) { o.write(line.getBytes()); o.flush(); }
                    } catch (Exception ignored) {}
                }
            });
        } catch (RejectedExecutionException ignored) {}
    }

    /** Works out where the game's picture goes on a surface of this size: as big as fits, centred, nothing cut off or stretched. */
    void fit(int surfW, int surfH) {
        sw = surfW; sh = surfH;
        final int gw = w, gh = h;                 // (the GL thread may change them: read once)
        if (gw <= 0 || gh <= 0 || surfW <= 0 || surfH <= 0) return;
        if (gw * SCALE <= surfW && gh * SCALE <= surfH && surfW - gw * SCALE < SCALE && surfH - gh * SCALE < SCALE) {
            viewScale = SCALE;                    // the game's size follows this screen: exactly SCALE screen pixels per game pixel, no bars (at most a pixel of border)
            viewX = (surfW - gw * SCALE) / 2;
            viewY = (surfH - gh * SCALE) / 2;
            return;
        }
        float s = Math.min((float) surfW / gw, (float) surfH / gh);   // fallback: as big as fits, centred, nothing cut off or stretched
        viewScale = s;
        viewX = (int) ((surfW - gw * s) / 2);
        viewY = (int) ((surfH - gh * s) / 2);
    }

    /** The screen was sized or turned (called for every new surface size): tells the game, which re-lays itself out for it when it can (see game/screen.h). */
    void onViewSized(int surfW, int surfH) {
        send("r " + (surfW > surfH ? 1 : 0) + " " + surfW + " " + surfH + "\n");
    }

    @Override public void surfaceCreated(SurfaceHolder holder) {}

    @Override
    public void surfaceChanged(SurfaceHolder holder, int fmt, int width, int height) {
        if (gpu) return;                          // the GPU path is driven by GfxRenderer.onSurfaceChanged
        // CPU path: the game's size is the first surface's and NEVER changes (one shared W*H framebuffer, game never resizes: screen_resizable() is 0 there);
        // a rotation only re-fits (letterbox) below
        if (w == 0) { w = width / SCALE; h = height / SCALE; }   // the first surface decides the game's size
        fit(width, height);
        if (!alive()) start(holder);              // first time (or the game died): start it
        startBlit(holder);                        // a new surface (after the background, a rotation): a new blitter for it
        onViewSized(width, height);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        blitGen++;                                // the blitter of this surface stops; the game itself carries on (paused if we are in the background)
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
                AudioTrack at = null, fx = null;
                try {
                    int min = AudioTrack.getMinBufferSize(RATE,
                            AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);
                    // 2x the minimum (was 4x): less sound queued up = less delay between the game
                    // mixing a sound and you hearing it (the dialog blips line up with the letters)
                    // TWO tracks, identical, fed in step from one stream (8 bytes a frame: music L R, effects L R).
                    // the music track carries the volume knob (AudioTrack.setVolume acts at the end of the pipeline: heard at once);
                    // the effects ignore it, as before.
                    at = new AudioTrack(AudioManager.STREAM_MUSIC, RATE,
                            AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT,
                            min * 2, AudioTrack.MODE_STREAM);
                    fx = new AudioTrack(AudioManager.STREAM_MUSIC, RATE,
                            AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT,
                            min * 2, AudioTrack.MODE_STREAM);
                    at.setVolume(musicVol);
                    musicTrack = at;
                    at.play();
                    fx.play();
                    FileInputStream in = new FileInputStream(fifo); // blocks until game opens it
                    byte[] buf = new byte[4096];
                    byte[] bm = new byte[2048], bf = new byte[2048];
                    int have = 0;                          // bytes of an unfinished frame kept from the last read
                    int n;
                    // the speaker's own clock: AudioTrack says "frame F was at the speaker at time T" (System.nanoTime clock). the game
                    // uses it to show the music window for what is HEARD (a guess at the delay is never right). 25 times a second is plenty.
                    AudioTimestamp ts = new AudioTimestamp();
                    long lastStampAt = 0, lastStampNs = 0;
                    while (running && (n = in.read(buf, have, buf.length - have)) > 0) {
                        if (paused) {                      // in the background: stop consuming, the game's audio then waits on the pipe
                            at.pause(); fx.pause();
                            while (paused && running) {
                                try { Thread.sleep(40); } catch (InterruptedException ie) { break; }
                            }
                            if (running) { at.play(); fx.play(); }
                        }
                        int total = have + n, frames = total / 8;
                        for (int i = 0; i < frames; i++) {
                            int s = i * 8, d = i * 4;
                            bm[d] = buf[s]; bm[d + 1] = buf[s + 1]; bm[d + 2] = buf[s + 2]; bm[d + 3] = buf[s + 3];
                            bf[d] = buf[s + 4]; bf[d + 1] = buf[s + 5]; bf[d + 2] = buf[s + 6]; bf[d + 3] = buf[s + 7];
                        }
                        have = total - frames * 8;
                        for (int i = 0; i < have; i++) buf[i] = buf[frames * 8 + i];
                        if (frames > 0) {
                            at.write(bm, 0, frames * 4);
                            fx.write(bf, 0, frames * 4);
                        }
                        long nowNs = System.nanoTime();
                        if (nowNs - lastStampAt >= 40000000L) {
                            lastStampAt = nowNs;
                            try {
                                if (at.getTimestamp(ts) && ts.framePosition > 0 && ts.nanoTime != lastStampNs) {
                                    lastStampNs = ts.nanoTime;
                                    send("a " + ts.framePosition + " " + ts.nanoTime + "\n");
                                }
                            } catch (Exception ignored) {}
                        }
                    }
                    in.close();
                } catch (Exception e) {
                    Log.e("game", "audio", e);
                } finally {
                    musicTrack = null;
                    if (at != null) { at.stop(); at.release(); }
                    if (fx != null) { fx.stop(); fx.release(); }
                }
            }
        }).start();
    }

    // the music volume (0..1), told by the game ("vol 0.65"): put straight on the music track
    volatile float musicVol = 0.65f;
    volatile AudioTrack musicTrack;
    void setMusicVol(float v) {
        if (v < 0f) v = 0f;
        if (v > 1f) v = 1f;
        musicVol = v;
        AudioTrack t = musicTrack;
        if (t != null) { try { t.setVolume(v); } catch (Exception ignored) {} }
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
                    while ((l = r.readLine()) != null) {
                        if (l.startsWith("vol ")) {                  // the music volume: onto the music track at once (not logged)
                            try { setMusicVol(Float.parseFloat(l.substring(4).trim())); } catch (Exception ignored) {}
                            continue;
                        }
                        Log.i("game", l);
                        if (l.equals("back: unhandled")) {           // the game had nothing for the back button to do: go to the home screen
                            runOnUiThread(new Runnable() { public void run() { moveTaskToBack(true); } });
                        }
                    }
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
        if (alive()) return;                      // already running (a rotation, or back from the background): nothing to do
        if (w == 0) { w = gw; h = gh; }           // the first surface decides the game's size, for good
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

    /** CPU path: starts the game, which draws pixels into a shared file. (startBlit puts them on the screen.) */
    void start(SurfaceHolder holder) {
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
            fbMap = raf.getChannel().map(FileChannel.MapMode.READ_WRITE, 0, (long) w * h * 4);
            raf.close();

            running = true;
            startAudio();

            proc = spawn(bin, "cpu");
        } catch (Exception e) {
            Log.e("game", "start failed", e);
        }
    }

    /** CPU path: a thread copies the game's pixels to this surface, letterboxed. One per surface; a newer generation stops the older one. */
    void startBlit(final SurfaceHolder holder) {
        if (fbMap == null) return;
        final int gen = ++blitGen;
        final MappedByteBuffer map = fbMap;
        final int gw = w, gh = h;
        new Thread(new Runnable() {
            public void run() {
                Bitmap bmp = Bitmap.createBitmap(gw, gh, Bitmap.Config.ARGB_8888);
                while (running && gen == blitGen) {
                    ByteBuffer src = map.duplicate();
                    src.rewind();
                    bmp.copyPixelsFromBuffer(src);
                    Rect dst = new Rect(viewX, viewY, viewX + Math.round(gw * viewScale), viewY + Math.round(gh * viewScale));
                    // GPU-backed canvas (API 26+); CPU lockCanvas() was the bottleneck.
                    // Posting is paced by the buffer queue (vsync), so no sleep there.
                    boolean hw = Build.VERSION.SDK_INT >= 26;
                    Canvas c = hw ? holder.lockHardwareCanvas() : holder.lockCanvas();
                    if (c != null) {
                        c.drawColor(Color.BLACK);                   // the bars round the picture
                        c.drawBitmap(bmp, null, dst, null);
                        holder.unlockCanvasAndPost(c);
                    }
                    if (!hw) {
                        try { Thread.sleep(16); } catch (InterruptedException e) { return; }
                    }
                }
            }
        }).start();
    }

    // --- touch: every finger is tracked by its own id ---
    // the game's classic input is ONE pointer: "t <action> <x> <y>" (0 down, 1 up, 2 move, 3 cancel). that stays the PRIMARY finger: the first one
    // down, until it lifts (a second finger can no longer make the primary jump to somewhere else, as it did when only pointer 0 was read).
    // every OTHER finger is sent as "m <id> <action> <x> <y>" (same actions, id = Android's pointer id), so the game can use two thumbs.
    // a finger that goes down while no primary is held becomes the primary. (older games ignore "m" lines.)
    private int primaryId = -1;

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        final float k = viewScale;                 // screen pixels -> game pixels: undo the letterbox
        final int act = e.getActionMasked();
        final StringBuilder sb = new StringBuilder();
        switch (act) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN: {
                int i = e.getActionIndex();
                int id = e.getPointerId(i);
                if (act == MotionEvent.ACTION_DOWN) primaryId = -1;          // a fresh gesture: nobody is held
                if (primaryId < 0) { primaryId = id; touchLine(sb, "t ", id, 0, e.getX(i), e.getY(i), k, false); }
                else touchLine(sb, "m ", id, 0, e.getX(i), e.getY(i), k, true);
                break;
            }
            case MotionEvent.ACTION_MOVE:
                for (int i = 0; i < e.getPointerCount(); i++) {
                    int id = e.getPointerId(i);
                    if (id == primaryId) touchLine(sb, "t ", id, 2, e.getX(i), e.getY(i), k, false);
                    else touchLine(sb, "m ", id, 2, e.getX(i), e.getY(i), k, true);
                }
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP: {
                int i = e.getActionIndex();
                int id = e.getPointerId(i);
                if (id == primaryId) { touchLine(sb, "t ", id, 1, e.getX(i), e.getY(i), k, false); primaryId = -1; }
                else touchLine(sb, "m ", id, 1, e.getX(i), e.getY(i), k, true);
                if (act == MotionEvent.ACTION_UP) primaryId = -1;
                break;
            }
            case MotionEvent.ACTION_CANCEL:
                for (int i = 0; i < e.getPointerCount(); i++) {
                    int id = e.getPointerId(i);
                    if (id == primaryId) touchLine(sb, "t ", id, 3, e.getX(i), e.getY(i), k, false);
                    else touchLine(sb, "m ", id, 3, e.getX(i), e.getY(i), k, true);
                }
                primaryId = -1;
                break;
            default:
                return true;                       // hover etc.
        }
        if (sb.length() > 0) send(sb.toString());  // one write per event, however many fingers
        return true;
    }

    private void touchLine(StringBuilder sb, String tag, int id, int action, float x, float y, float k, boolean withId) {
        sb.append(tag);
        if (withId) sb.append(id).append(' ');
        sb.append(action).append(' ').append((int) ((x - viewX) / k)).append(' ').append((int) ((y - viewY) / k)).append('\n');
    }

    @Override
    protected void onDestroy() {
        running = false;
        if (proc != null) proc.destroy();
        inputQ.shutdownNow();
        super.onDestroy();
    }
}
