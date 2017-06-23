package de.uni_siegen.libqvr;

import android.app.Activity;
import android.content.Context;
import android.view.View;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.os.Bundle;
import android.os.Vibrator;
import android.opengl.GLSurfaceView;
import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import org.qtproject.qt5.android.bindings.QtActivity;

public class QVRActivity extends QtActivity
{
    static {
        System.loadLibrary("gvr");
        System.loadLibrary("qvr");
    }

    private GvrLayout gvrLayout;
    private GLSurfaceView surfaceView;
    private long nativeGvrContext;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        gvrLayout = new GvrLayout(this);
        nativeGvrContext = gvrLayout.getGvrApi().getNativeGvrContext();
    }

    @Override
    protected void onPause() {
        gvrLayout.onPause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        gvrLayout.onResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        gvrLayout.shutdown();
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
        gvrLayout.onBackPressed();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        // Avoid accidental volume key presses while the phone is in the VR headset.
        if (event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_UP
                || event.getKeyCode() == KeyEvent.KEYCODE_VOLUME_DOWN) {
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    long getNativeGvrContext()
    {
        return nativeGvrContext;
    }

    void initializeVR()
    {
        surfaceView = new GLSurfaceView(this);
        surfaceView.setPreserveEGLContextOnPause(true);
        surfaceView.setEGLContextClientVersion(3);
        surfaceView.setEGLConfigChooser(8, 8, 8, 0, 0, 0);
        surfaceView.setRenderer(new GLSurfaceView.Renderer() {
            @Override public void onSurfaceCreated(GL10 gl, EGLConfig config) { nativeOnSurfaceCreated(); }
            @Override public void onSurfaceChanged(GL10 gl, int width, int height) {}
            @Override public void onDrawFrame(GL10 gl) { nativeOnDrawFrame(); }
        });
        surfaceView.setOnTouchListener(new View.OnTouchListener() {
            @Override public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_DOWN) {
                    ((Vibrator)getSystemService(Context.VIBRATOR_SERVICE)).vibrate(50);
                    surfaceView.queueEvent(new Runnable() {
                        @Override public void run() { nativeOnTriggerEvent(); }
                    });
                    return true;
                }
                return false;
            }
        });
        gvrLayout.setPresentationView(surfaceView);
        setContentView(gvrLayout);
        if (gvrLayout.setAsyncReprojectionEnabled(true)) {
            AndroidCompat.setSustainedPerformanceMode(this, true);
        }
        AndroidCompat.setVrModeEnabled(this, true);
    }

    private native void nativeOnSurfaceCreated();
    private native void nativeOnDrawFrame();
    private native void nativeOnTriggerEvent();
}
