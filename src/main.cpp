#if WIN32
#pragma comment(lib, "d3d11.lib")
#endif
#include "application.h"

namespace KrisRenderer
{
	int Main()
	{
		Application& app = Application::Get();
		app.Loop();
		return 0;
	}
}

int main(int argc, char* argv[])
{
	return KrisRenderer::Main();
}