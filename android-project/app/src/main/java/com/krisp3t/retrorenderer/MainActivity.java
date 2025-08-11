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

public class MainActivity extends SDLActivity {
    // Permissions
    private static final int PICK_FILE_REQUEST = 1;
    // Native
    private static native void nativeSetAssetManager(AssetManager assetManager);
    public native void nativeSetImGuiIniPath(String path);
    private static native void nativeOnFilePicked(ByteBuffer buffer, int length, String extension);
    private ByteBuffer currentFileBuffer; // keep reference to prevent GC

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
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == PICK_FILE_REQUEST && resultCode == RESULT_OK) {
            Uri uri = data.getData();
            try {
                assert uri != null;
                InputStream is = getContentResolver().openInputStream(uri);
                ByteArrayOutputStream bos = new ByteArrayOutputStream();

                byte[] tmp = new byte[8192];
                int read;
                while ((read = is.read(tmp)) != -1) {
                    bos.write(tmp, 0, read);
                }
                is.close();

                byte[] fileBytes = bos.toByteArray();

                // Try to get extension from URI
                String name = uri.getLastPathSegment();
                String ext = "";
                if (name != null) {
                    int dot = name.lastIndexOf('.');
                    if (dot != -1) {
                        ext = name.substring(dot + 1);
                    }
                }

                // Create direct buffer and keep reference
                currentFileBuffer = ByteBuffer.allocateDirect(fileBytes.length);
                currentFileBuffer.put(fileBytes);
                currentFileBuffer.flip();

                nativeOnFilePicked(currentFileBuffer, fileBytes.length, ext);
                releaseFileBuffer();

            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void releaseFileBuffer() {
        currentFileBuffer = null; // Allow GC
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

    public void openFilePicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        String[] mimeTypes = {"model/obj", "application/octet-stream", "text/plain"}; // TODO: extract all supported types
        intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
        startActivityForResult(intent, PICK_FILE_REQUEST);
    }
}
