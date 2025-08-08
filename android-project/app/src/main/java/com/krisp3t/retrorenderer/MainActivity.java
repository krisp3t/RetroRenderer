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
        copyAllAssetsToInternalStorage("", getFilesDir());
        setImGuiIniPath(getFilesDir().getAbsolutePath() + "/config_panel.ini");
    }
    private static native void nativeSetAssetManager(AssetManager assetManager);
    public native void setImGuiIniPath(String path);

    @Override
    protected String[] getLibraries() {
        return new String[] { "retrorenderer" };
    }

    private void copyAssetFile(String assetPath, File destFile) {
        if (destFile.exists()) return; // Don't overwrite existing

        try (InputStream in = getAssets().open(assetPath);
             OutputStream out = new FileOutputStream(destFile)) {
            byte[] buffer = new byte[4096];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void copyAllAssetsToInternalStorage(String assetPath, File outputDir) {
        AssetManager assetManager = getAssets();
        try {
            String[] assets = assetManager.list(assetPath);
            if (assets == null || assets.length == 0) {
                // File
                copyAssetFile(assetPath, new File(outputDir, new File(assetPath).getName()));
            } else {
                // Directory
                File dir = new File(outputDir, assetPath);
                if (!dir.exists()) {
                    dir.mkdirs();
                }
                for (String asset : assets) {
                    copyAllAssetsToInternalStorage(
                            assetPath.isEmpty() ? asset : assetPath + "/" + asset,
                            outputDir
                    );
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
