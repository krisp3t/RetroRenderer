package com.krisp3t.retrorenderer;

import android.content.Intent;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Bundle;
import org.libsdl.app.SDLActivity;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class MainActivity extends SDLActivity {
    // Requests
    private static final int PICK_SCENE_REQUEST = 1;
    private static final int PICK_TEXTURE_REQUEST = 2;
    // Native
    private static native void nativeSetAssetManager(AssetManager assetManager);
    public native void nativeSetImGuiIniPath(String path);
    public native void nativeSetAssetsPath(String path);
    private static native void nativeOnFilePicked(byte[] data, String extension);
    private static native void nativeOnTexturePicked(byte[] data, String extension);

    @Override
    protected String[] getLibraries() {
        return new String[] { "retrorenderer" };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        nativeSetAssetManager(getAssets());
        copyAllAssetsToInternalStorage("", getFilesDir());
        nativeSetImGuiIniPath(getFilesDir().getAbsolutePath() + "/config_panel.ini");
        nativeSetAssetsPath(getFilesDir().getAbsolutePath());
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (resultCode == RESULT_OK && data != null) {
            Uri uri = data.getData();
            try (InputStream is = getContentResolver().openInputStream(uri);
                 ByteArrayOutputStream buffer = new ByteArrayOutputStream()) {

                byte[] tmp = new byte[4096];
                int read;
                while ((read = is.read(tmp)) != -1) {
                    buffer.write(tmp, 0, read);
                }

                byte[] fileBytes = buffer.toByteArray();

                // Extract extension
                String extension = "";
                String path = uri.getPath();
                if (path != null) {
                    int dotIndex = path.lastIndexOf('.');
                    if (dotIndex >= 0 && dotIndex < path.length() - 1) {
                        extension = path.substring(dotIndex + 1).toLowerCase();
                    }
                }

                if (requestCode == PICK_SCENE_REQUEST) {
                    nativeOnFilePicked(fileBytes, extension);
                } else if (requestCode == PICK_TEXTURE_REQUEST) {
                    nativeOnTexturePicked(fileBytes, extension);
                }

            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }


    private void copyAssetFile(String assetPath, File destFile) {
        // Will overwrite existing!

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

    private void copyAllAssetsToInternalStorage(String assetPath, File rootOutputDir) {
        AssetManager assetManager = getAssets();
        try {
            String[] assets = assetManager.list(assetPath);

            if (assets == null || assets.length == 0) {
                // It's a file
                String fileName = new File(assetPath).getName();
                if ("config_panel.ini".equals(fileName)) {
                    return; // Skip imgui config to preserve window layout
                }
                File outFile = new File(rootOutputDir, assetPath);
                File parentDir = outFile.getParentFile();
                if (parentDir != null && !parentDir.exists()) {
                    parentDir.mkdirs();
                }
                copyAssetFile(assetPath, outFile);
            } else {
                // It's a directory
                for (String asset : assets) {
                    String childPath = assetPath.isEmpty() ? asset : assetPath + "/" + asset;
                    copyAllAssetsToInternalStorage(childPath, rootOutputDir);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void openFilePicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        String[] mimeTypes = {"model/obj", "application/octet-stream", "text/plain"}; // TODO: extract all supported types
        intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
        startActivityForResult(intent, PICK_SCENE_REQUEST);
    }

    public void openTexturePicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        String[] mimeTypes = {"image/png"};
        intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
        startActivityForResult(intent, PICK_TEXTURE_REQUEST);
    }
}
