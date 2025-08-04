#pragma once

#ifdef __ANDROID__
#include <android/asset_manager.h>

extern AAssetManager* g_assetManager;
#endif