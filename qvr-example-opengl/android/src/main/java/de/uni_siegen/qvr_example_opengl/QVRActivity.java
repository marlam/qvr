package de.uni_siegen.qvr_example_opengl;

import android.app.Activity;
import android.content.Context;
import android.view.View;
import android.view.KeyEvent;
import android.os.Bundle;
import com.google.vr.ndk.base.AndroidCompat;
import com.google.vr.ndk.base.GvrLayout;
import org.qtproject.qt5.android.bindings.QtActivity;

public class QVRActivity extends QtActivity
{
    private GvrLayout gvrLayout;
    private long nativeGvrContext;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        gvrLayout = new GvrLayout(this);
        nativeGvrContext = gvrLayout.getGvrApi().getNativeGvrContext();
        if (gvrLayout.setAsyncReprojectionEnabled(true)) {
            AndroidCompat.setSustainedPerformanceMode(this, true);
        }
        AndroidCompat.setVrModeEnabled(this, true);
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

    void setViewEtc()
    {
        gvrLayout.setPresentationView(getCurrentFocus());
        setContentView(gvrLayout);
    }
}
