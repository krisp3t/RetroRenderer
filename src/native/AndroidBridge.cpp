#include "AndroidBridge.h"
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <vector>

#ifdef __ANDROID__
AAssetManager* g_assetManager = nullptr;
std::string g_imguiIniPath = "";
#endif

extern "C"
JNIEXPORT void JNICALL
Java_com_krisp3t_retrorenderer_MainActivity_nativeSetAssetManager(
    JNIEnv* env,
    jclass /* clazz */,
    jobject jAssetManager) {
    g_assetManager = AAssetManager_fromJava(env, jAssetManager);
}


extern "C" JNIEXPORT void JNICALL
Java_com_krisp3t_retrorenderer_MainActivity_nativeSetImGuiIniPath(JNIEnv* env, jobject, jstring path) {
    const char* nativePath = env->GetStringUTFChars(path, nullptr);
    g_imguiIniPath = nativePath;
    env->ReleaseStringUTFChars(path, nativePath);
}


extern "C" JNIEXPORT void JNICALL
Java_com_krisp3t_retrorenderer_MainActivity_nativeOnFilePicked(JNIEnv* env,
                                                               jobject buffer, jint length, jstring jext) {
    void* data = env->GetDirectBufferAddress(buffer);
    if (!data) {
        // TODO: handle error
        return;
    }

    const char* extCStr = env->GetStringUTFChars(jext, nullptr);
    std::string extension(extCStr ? extCStr : "");
    env->ReleaseStringUTFChars(jext, extCStr);

    std::vector<uint8_t> fileBytes(length);
    std::memcpy(fileBytes.data(), data, length); // single copy from Java direct buffer
}