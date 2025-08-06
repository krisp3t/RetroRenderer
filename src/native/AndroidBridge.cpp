#include "AndroidBridge.h"
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>

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
Java_com_krisp3t_retrorenderer_MainActivity_setImGuiIniPath(JNIEnv* env, jobject, jstring path) {
    const char* nativePath = env->GetStringUTFChars(path, nullptr);
    g_imguiIniPath = nativePath;
    env->ReleaseStringUTFChars(path, nativePath);
}