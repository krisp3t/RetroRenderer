package com.krisp3t.retrorenderer;

import android.content.res.AssetManager;
import android.os.Bundle;
import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MainActivity extends SDLActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        nativeSetAssetManager(getAssets());
        copyAssetToInternalStorage("config_panel.ini", "config_panel.ini");
        setImGuiIniPath(getFilesDir().getAbsolutePath() + "/config_panel.ini");
    }
    private static native void nativeSetAssetManager(AssetManager assetManager);
    public native void setImGuiIniPath(String path);

    @Override
    protected String[] getLibraries() {
        return new String[] { "retrorenderer" };
    }

    private void copyAssetToInternalStorage(String assetName, String outputName) {
        File outFile = new File(getFilesDir(), outputName);
        if (outFile.exists()) return; // Don't overwrite if it already exists

        try (InputStream in = getAssets().open(assetName);
             OutputStream out = new FileOutputStream(outFile)) {
            byte[] buffer = new byte[1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
