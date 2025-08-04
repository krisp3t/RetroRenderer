#include "AndroidBridge.h"
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#ifdef __ANDROID__
AAssetManager* g_assetManager = nullptr;
#endif

extern "C"
JNIEXPORT void JNICALL
Java_com_krisp3t_retrorenderer_MainActivity_nativeSetAssetManager(
    JNIEnv* env,
    jclass /* clazz */,
    jobject jAssetManager) {
    g_assetManager = AAssetManager_fromJava(env, jAssetManager);
}