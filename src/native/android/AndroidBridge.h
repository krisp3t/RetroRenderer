#pragma once

#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <jni.h>
#include <string>

extern AAssetManager *g_assetManager;
extern std::string g_imguiIniPath;
extern std::string g_assetsPath;
#endif