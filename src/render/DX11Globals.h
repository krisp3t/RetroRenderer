#pragma once
#if WIN32
#include <d3d11.h>
#endif

namespace KrisRenderer
{
	namespace DX11Globals
	{
		ID3D11Device* sDx11Device = nullptr;
		ID3D11DeviceContext* sDx11DeviceContext = nullptr;
	};
}