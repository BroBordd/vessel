/* Vessel - Copyright (C) 2026 BroBordd
 * SPDX-License-Identifier: GPL-3.0-only (see LICENSE) */
package io.github.brobordd.vessel;

import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.util.Log;

import java.io.File;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Draws the game on the GPU.
 *
 * The game process records its draw calls (filled rects, blend, clip, clear, one zoom pass) into a
 * shared file, see game/gfx.h for the layout. Every frame this class copies the newest complete
 * frame out of that file, uploads the rects as one vertex buffer and draws them with instanced quads.
 * Everything is rasterised at the game's own resolution into an offscreen "scene" texture (so the
 * pixels are the same ones the CPU renderer produced), then that texture is stretched to the screen
 * with nearest filtering.
 */
final class GfxRenderer implements GLSurfaceView.Renderer {

    // ---- shared file layout, must match game/gfx.h ----
    static final int MAGIC = 0x31584647;               // "GFX1"
    static final int HDR_BYTES = 64;
    static final int NSLOTS = 4;
    static final int CAP = 65536;                      // records per slot
    static final int SLOT_HDR = 16;
    static final int REC_BYTES = 16;
    static final int SLOT_BYTES = SLOT_HDR + CAP * REC_BYTES;
    static final int FILE_BYTES = HDR_BYTES + NSLOTS * SLOT_BYTES;

    static final int H_MAGIC = 0, H_FRAME = 6, H_LATEST = 5, H_READING = 7;     // header int indices
    static final int T_RECT = 0, T_BLEND = 1, T_CLIP = 2, T_TARGET = 3, T_COPY = 4, T_CLEAR = 5;

    private static final String TAG = "game";

    private static final String VS_RECT =
            "#version 300 es\n" +
            "layout(location = 0) in ivec4 aRec;\n" +
            "uniform vec2 uGame;\n" +
            "out vec4 vC;\n" +
            "void main() {\n" +
            "  vec2 corner = vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1));\n" +
            "  float x = float((aRec.x & 0xFFFF) - 32768);\n" +
            "  float y = float(((aRec.x >> 16) & 0xFFFF) - 32768);\n" +
            "  float w = float(aRec.y & 0xFFFF);\n" +
            "  float h = float((aRec.y >> 16) & 0xFFFF);\n" +
            "  vec2 p = vec2(x, y) + corner * vec2(w, h);\n" +
            "  gl_Position = vec4(p.x / uGame.x * 2.0 - 1.0, 1.0 - p.y / uGame.y * 2.0, 0.0, 1.0);\n" +
            "  int c = aRec.z;\n" +
            "  vC = vec4(float(c & 255), float((c >> 8) & 255), float((c >> 16) & 255), float((c >> 24) & 255)) / 255.0;\n" +
            "}\n";

    private static final String FS_RECT =
            "#version 300 es\n" +
            "precision mediump float;\n" +
            "in vec4 vC;\n" +
            "out vec4 o;\n" +
            "void main() { o = vC; }\n";

    // a textured quad: the zoom pass (texture -> scene) and the final stretch (scene -> screen)
    private static final String VS_TEX =
            "#version 300 es\n" +
            "uniform vec4 uSrc;\n" +        // source rect in game pixels: x, y, w, h
            "uniform vec2 uGame;\n" +
            "out vec2 vUV;\n" +
            "void main() {\n" +
            "  vec2 c = vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1));\n" +
            "  gl_Position = vec4(c.x * 2.0 - 1.0, 1.0 - c.y * 2.0, 0.0, 1.0);\n" +
            "  vec2 s = uSrc.xy + c * uSrc.zw;\n" +
            "  vUV = vec2(s.x / uGame.x, 1.0 - s.y / uGame.y);\n" +
            "}\n";

    private static final String FS_TEX =
            "#version 300 es\n" +
            "precision highp float;\n" +
            "uniform sampler2D uTex;\n" +
            "in vec2 vUV;\n" +
            "out vec4 o;\n" +
            "void main() { o = texture(uTex, vUV); }\n";

    private final MainActivity host;
    private final MappedByteBuffer map;
    private final IntBuffer ib;                        // the whole shared file as ints
    private final ByteBuffer src;                      // duplicate of map, used to position bulk copies
    private final ByteBuffer stage;                    // private copy of one frame's records
    private final IntBuffer stageIb;

    // GL objects (all created on the GL thread)
    private int progRect, progTex, vaoRect, vaoEmpty, vbo;
    private int uGameRect, uGameTex, uSrcTex, uTexSampler;
    private final int[] sceneTex = new int[1], sceneFbo = new int[1], zoomTex = new int[1], zoomFbo = new int[1];
    private boolean ready;
    private int sw, sh, gw, gh;                        // screen size, game size

    // replay state
    private boolean blendOn, scissorOn;
    private int lastGen;

    // stats
    private long statT0;
    private int statFrames, statRecs, statDraws, statDrawsLast, errChecks;

    GfxRenderer(MainActivity host) throws Exception {
        this.host = host;
        File f = new File(host.getFilesDir(), "gfx");
        f.delete();
        RandomAccessFile raf = new RandomAccessFile(f, "rw");
        raf.setLength(FILE_BYTES);
        map = raf.getChannel().map(FileChannel.MapMode.READ_WRITE, 0, FILE_BYTES);
        raf.close();
        map.order(ByteOrder.nativeOrder());
        ib = map.asIntBuffer();
        src = map.duplicate();
        src.order(ByteOrder.nativeOrder());
        stage = ByteBuffer.allocateDirect(CAP * REC_BYTES).order(ByteOrder.nativeOrder());
        stageIb = stage.asIntBuffer();
    }

    /** Forget everything about the previous game process. Called on the GL thread right before a new one starts. */
    void reset() {
        for (int i = 0; i < HDR_BYTES / 4; i++) ib.put(i, 0);
        lastGen = 0;
    }

    // ---------------------------------------------------------------- GLSurfaceView.Renderer

    @Override
    public void onSurfaceCreated(GL10 unused, EGLConfig config) {
        ready = false;
        sceneTex[0] = sceneFbo[0] = zoomTex[0] = zoomFbo[0] = 0;     // a new context starts empty
        try {
            progRect = program(VS_RECT, FS_RECT);
            progTex = program(VS_TEX, FS_TEX);
        } catch (RuntimeException e) {
            Log.e(TAG, "gpu: shader setup failed (set GPU = false in MainActivity to use the CPU path): " + e.getMessage());
            return;
        }
        uGameRect = GLES30.glGetUniformLocation(progRect, "uGame");
        uGameTex = GLES30.glGetUniformLocation(progTex, "uGame");
        uSrcTex = GLES30.glGetUniformLocation(progTex, "uSrc");
        uTexSampler = GLES30.glGetUniformLocation(progTex, "uTex");

        int[] o = new int[1];
        GLES30.glGenBuffers(1, o, 0); vbo = o[0];
        GLES30.glGenVertexArrays(1, o, 0); vaoRect = o[0];
        GLES30.glBindVertexArray(vaoRect);
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, vbo);
        GLES30.glEnableVertexAttribArray(0);
        GLES30.glVertexAttribDivisor(0, 1);
        GLES30.glBindVertexArray(0);
        GLES30.glGenVertexArrays(1, o, 0); vaoEmpty = o[0];

        GLES30.glDisable(GLES30.GL_DITHER);
        GLES30.glDisable(GLES30.GL_DEPTH_TEST);
        GLES30.glDisable(GLES30.GL_CULL_FACE);

        Log.i(TAG, "gpu: " + GLES30.glGetString(GLES30.GL_RENDERER) + " / " + GLES30.glGetString(GLES30.GL_VERSION));
    }

    @Override
    public void onSurfaceChanged(GL10 unused, int width, int height) {
        if (progRect == 0 || progTex == 0) return;
        sw = width; sh = height;
        gw = width / MainActivity.SCALE; gh = height / MainActivity.SCALE;
        if (sceneTex[0] != 0) {
            GLES30.glDeleteTextures(2, new int[] { sceneTex[0], zoomTex[0] }, 0);
            GLES30.glDeleteFramebuffers(2, new int[] { sceneFbo[0], zoomFbo[0] }, 0);
        }
        makeTarget(sceneTex, sceneFbo, gw, gh);
        makeTarget(zoomTex, zoomFbo, gw, gh);
        ready = true;
        host.startGpu(gw, gh);
    }

    @Override
    public void onDrawFrame(GL10 unused) {
        if (!ready) return;
        pollFrame();
        present();
        long now = System.nanoTime();
        if (statT0 == 0) statT0 = now;
        if (now - statT0 >= 2000000000L) {
            if (statFrames > 0)
                Log.i(TAG, "gpu: " + (statFrames * 1000000000L / (now - statT0)) + " game frames/s drawn, "
                        + (statRecs / statFrames) + " records, " + statDrawsLast + " draw calls per frame");
            statT0 = now; statFrames = 0; statRecs = 0;
        }
    }

    // ---------------------------------------------------------------- frame intake

    /** Copies the newest finished frame out of the shared file and, if there is one, replays it into the scene texture. */
    private void pollFrame() {
        if (ib.get(H_MAGIC) != MAGIC) return;
        int frame = ib.get(H_FRAME);
        if (frame == 0 || frame == lastGen) return;
        int s = ib.get(H_LATEST);
        if (s < 0 || s >= NSLOTS) return;

        ib.put(H_READING, s);                                        // asks the game not to reuse this slot
        int base = HDR_BYTES + s * SLOT_BYTES;
        int g1 = ib.get(base >> 2);
        int n = ib.get((base >> 2) + 1);
        boolean ok = g1 != 0 && n >= 0 && n <= CAP;
        if (ok) {
            src.clear();
            src.position(base + SLOT_HDR);
            src.limit(base + SLOT_HDR + n * REC_BYTES);
            stage.clear();
            stage.put(src);
            ok = ib.get(base >> 2) == g1;                            // slot untouched while we copied: the copy is whole
        }
        ib.put(H_READING, -1);
        if (!ok) return;                                             // try again next vsync

        lastGen = g1;
        stage.clear();
        replay(n);
        statFrames++; statRecs += n;
    }

    // ---------------------------------------------------------------- replay

    private void replay(int n) {
        statDraws = 0;
        if (n > 0) {
            stage.position(0);
            stage.limit(n * REC_BYTES);
            GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, vbo);
            GLES30.glBufferData(GLES30.GL_ARRAY_BUFFER, n * REC_BYTES, stage, GLES30.GL_STREAM_DRAW);
            stage.clear();
        }

        bindTarget(sceneFbo[0]);
        setBlend(false);
        setScissor(false, 0, 0, 0, 0);

        int run = -1;
        for (int i = 0; i < n; i++) {
            int t = stageIb.get(i * 4 + 3);
            if (t == T_RECT) { if (run < 0) run = i; continue; }
            if (run >= 0) { drawRects(run, i - run); run = -1; }
            int a = stageIb.get(i * 4), b = stageIb.get(i * 4 + 1), c = stageIb.get(i * 4 + 2);
            switch (t) {
                case T_BLEND:
                    setBlend(a != 0);
                    break;
                case T_CLIP:
                    setScissor(c != 0, (a & 0xFFFF) - 32768, (a >>> 16) - 32768, b & 0xFFFF, b >>> 16);
                    break;
                case T_TARGET:
                    bindTarget(a != 0 ? zoomFbo[0] : sceneFbo[0]);
                    break;
                case T_COPY:
                    copyZoom((a & 0xFFFF) - 32768, (a >>> 16) - 32768, b & 0xFFFF, b >>> 16);
                    break;
                case T_CLEAR:
                    clear(c);
                    break;
                default:
                    break;
            }
        }
        if (run >= 0) drawRects(run, n - run);

        bindTarget(sceneFbo[0]);
        statDrawsLast = statDraws;

        if (errChecks < 4 || (statFrames % 600) == 0) {
            errChecks++;
            int e = GLES30.glGetError();
            if (e != 0) Log.e(TAG, "gpu: GL error 0x" + Integer.toHexString(e));
        }
    }

    private void drawRects(int first, int count) {
        GLES30.glUseProgram(progRect);
        GLES30.glUniform2f(uGameRect, gw, gh);
        GLES30.glBindVertexArray(vaoRect);
        GLES30.glBindBuffer(GLES30.GL_ARRAY_BUFFER, vbo);
        GLES30.glVertexAttribIPointer(0, 4, GLES30.GL_INT, REC_BYTES, first * REC_BYTES);
        GLES30.glDrawArraysInstanced(GLES30.GL_TRIANGLE_STRIP, 0, 4, count);
        statDraws++;
    }

    private void copyZoom(int x, int y, int w, int h) {
        boolean wasBlend = blendOn;                                  // a texture copy replaces pixels, whatever the draw blend mode
        if (wasBlend) setBlend(false);
        drawTexture(zoomTex[0], x, y, w, h);
        if (wasBlend) setBlend(true);
    }

    private void drawTexture(int tex, int x, int y, int w, int h) {
        GLES30.glUseProgram(progTex);
        GLES30.glUniform2f(uGameTex, gw, gh);
        GLES30.glUniform4f(uSrcTex, x, y, w, h);
        GLES30.glActiveTexture(GLES30.GL_TEXTURE0);
        GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, tex);
        GLES30.glUniform1i(uTexSampler, 0);
        GLES30.glBindVertexArray(vaoEmpty);
        GLES30.glDrawArrays(GLES30.GL_TRIANGLE_STRIP, 0, 4);
        statDraws++;
    }

    private void clear(int rgba) {
        boolean wasScissor = scissorOn;                              // SDL_RenderClear ignores the clip rect
        if (wasScissor) GLES30.glDisable(GLES30.GL_SCISSOR_TEST);
        GLES30.glClearColor((rgba & 255) / 255f, ((rgba >> 8) & 255) / 255f,
                ((rgba >> 16) & 255) / 255f, ((rgba >>> 24) & 255) / 255f);
        GLES30.glClear(GLES30.GL_COLOR_BUFFER_BIT);
        if (wasScissor) GLES30.glEnable(GLES30.GL_SCISSOR_TEST);
    }

    private void setBlend(boolean on) {
        blendOn = on;
        if (on) {
            GLES30.glEnable(GLES30.GL_BLEND);
            // same maths as SDL's blend mode: colour by src alpha, alpha accumulates
            GLES30.glBlendFuncSeparate(GLES30.GL_SRC_ALPHA, GLES30.GL_ONE_MINUS_SRC_ALPHA,
                    GLES30.GL_ONE, GLES30.GL_ONE_MINUS_SRC_ALPHA);
        } else {
            GLES30.glDisable(GLES30.GL_BLEND);
        }
    }

    private void setScissor(boolean on, int x, int y, int w, int h) {
        scissorOn = on;
        if (on) {
            GLES30.glEnable(GLES30.GL_SCISSOR_TEST);
            GLES30.glScissor(x, gh - (y + h), Math.max(w, 0), Math.max(h, 0));   // game y is down, GL y is up
        } else {
            GLES30.glDisable(GLES30.GL_SCISSOR_TEST);
        }
    }

    private void bindTarget(int fbo) {
        GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, fbo);
        GLES30.glViewport(0, 0, gw, gh);
    }

    /** Stretches the scene texture over the whole screen. Runs every vsync, new game frame or not. */
    private void present() {
        GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, 0);
        GLES30.glViewport(0, 0, sw, sh);
        setBlend(false);
        setScissor(false, 0, 0, 0, 0);
        drawTexture(sceneTex[0], 0, 0, gw, gh);
    }

    // ---------------------------------------------------------------- GL helpers

    private void makeTarget(int[] tex, int[] fbo, int w, int h) {
        GLES30.glGenTextures(1, tex, 0);
        GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, tex[0]);
        GLES30.glTexImage2D(GLES30.GL_TEXTURE_2D, 0, GLES30.GL_RGBA8, w, h, 0,
                GLES30.GL_RGBA, GLES30.GL_UNSIGNED_BYTE, null);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_NEAREST);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_NEAREST);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_WRAP_S, GLES30.GL_CLAMP_TO_EDGE);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_WRAP_T, GLES30.GL_CLAMP_TO_EDGE);
        GLES30.glGenFramebuffers(1, fbo, 0);
        GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, fbo[0]);
        GLES30.glFramebufferTexture2D(GLES30.GL_FRAMEBUFFER, GLES30.GL_COLOR_ATTACHMENT0,
                GLES30.GL_TEXTURE_2D, tex[0], 0);
        int st = GLES30.glCheckFramebufferStatus(GLES30.GL_FRAMEBUFFER);
        if (st != GLES30.GL_FRAMEBUFFER_COMPLETE) Log.e(TAG, "gpu: framebuffer incomplete 0x" + Integer.toHexString(st));
        GLES30.glDisable(GLES30.GL_SCISSOR_TEST);
        GLES30.glClearColor(0f, 0f, 0f, 0f);                         // a fresh SDL surface is transparent black too
        GLES30.glClear(GLES30.GL_COLOR_BUFFER_BIT);
        GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, 0);
    }

    private static int program(String vs, String fs) {
        int v = shader(GLES30.GL_VERTEX_SHADER, vs);
        int f = shader(GLES30.GL_FRAGMENT_SHADER, fs);
        int p = GLES30.glCreateProgram();
        GLES30.glAttachShader(p, v);
        GLES30.glAttachShader(p, f);
        GLES30.glLinkProgram(p);
        int[] ok = new int[1];
        GLES30.glGetProgramiv(p, GLES30.GL_LINK_STATUS, ok, 0);
        if (ok[0] == 0) {
            String log = GLES30.glGetProgramInfoLog(p);
            GLES30.glDeleteProgram(p);
            throw new RuntimeException("link: " + log);
        }
        GLES30.glDeleteShader(v);
        GLES30.glDeleteShader(f);
        return p;
    }

    private static int shader(int type, String code) {
        int s = GLES30.glCreateShader(type);
        GLES30.glShaderSource(s, code);
        GLES30.glCompileShader(s);
        int[] ok = new int[1];
        GLES30.glGetShaderiv(s, GLES30.GL_COMPILE_STATUS, ok, 0);
        if (ok[0] == 0) {
            String log = GLES30.glGetShaderInfoLog(s);
            GLES30.glDeleteShader(s);
            throw new RuntimeException("compile: " + log);
        }
        return s;
    }
}
