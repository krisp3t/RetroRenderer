#pragma once
#include <memory>
#if WIN32
#include <d3d11.h>
#endif

// Meyers Singleton
namespace KrisRenderer
{
	class Window;

	class Application
	{
	public:
		static Application& Get();
		Application(const Application&) = delete;
		void operator=(const Application&) = delete;
		void Loop();
		#if WIN32
			static ID3D11Device* sDx11Device;
			static ID3D11DeviceContext* sDx11DeviceContext;
		#endif
	private:
		Application();
		static std::unique_ptr<Window> sWindow;
		// std::unique_ptr<Renderer> mRenderer;
	};
}
