#include "AndroidBridge.h"
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <vector>
#include <KrisLogger/Logger.h>
#include "../Engine.h"

#ifdef __ANDROID__
AAssetManager* g_assetManager = nullptr;
std::string g_imguiIniPath = "";
std::string g_assetsPath = "";
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
Java_com_krisp3t_retrorenderer_MainActivity_nativeSetAssetsPath(JNIEnv* env, jobject, jstring path) {
    const char* nativePath = env->GetStringUTFChars(path, nullptr);
    g_assetsPath = nativePath;
    g_assetsPath.append("/");
    env->ReleaseStringUTFChars(path, nativePath);
}


extern "C" JNIEXPORT void JNICALL
Java_com_krisp3t_retrorenderer_MainActivity_nativeOnScenePicked(
JNIEnv* env, jobject /* this */, jbyteArray fileData, jstring extension) {
    size_t length = env->GetArrayLength(fileData);
    std::vector<uint8_t> buffer(length);
    env->GetByteArrayRegion(fileData, 0, length, reinterpret_cast<jbyte*>(buffer.data()));
    //const char* extStr = env->GetStringUTFChars(extension, nullptr);

    auto event = std::make_unique<RetroRenderer::SceneLoadEvent>(
        std::move(buffer),
        length
    );
    RetroRenderer::Engine::Get().EnqueueEvent(std::move(event));

    //env->ReleaseStringUTFChars(extension, extStr);
}

extern "C" JNIEXPORT void JNICALL
Java_com_krisp3t_retrorenderer_MainActivity_nativeOnTexturePicked(
        JNIEnv* env, jobject /* this */, jbyteArray fileData, jstring extension) {
    size_t length = env->GetArrayLength(fileData);
    std::vector<uint8_t> buffer(length);
    env->GetByteArrayRegion(fileData, 0, length, reinterpret_cast<jbyte*>(buffer.data()));
    //const char* extStr = env->GetStringUTFChars(extension, nullptr);

    auto event = std::make_unique<RetroRenderer::TextureLoadEvent>(
            std::move(buffer),
            length
    );
    RetroRenderer::Engine::Get().EnqueueEvent(std::move(event));

    //env->ReleaseStringUTFChars(extension, extStr);
}