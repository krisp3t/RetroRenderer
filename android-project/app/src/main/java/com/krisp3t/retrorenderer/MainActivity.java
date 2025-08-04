package com.krisp3t.retrorenderer;

import android.content.res.AssetManager;
import android.os.Bundle;
import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        nativeSetAssetManager(getAssets());
    }

    // Declare the native function
    private static native void nativeSetAssetManager(AssetManager assetManager);

    @Override
    protected String[] getLibraries() {
        return new String[] { "retrorenderer" };
    }
}
