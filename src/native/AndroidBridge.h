#pragma once

#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <string>

extern AAssetManager* g_assetManager;
extern std::string g_imguiIniPath;
#endif