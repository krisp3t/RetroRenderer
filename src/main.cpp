#if WIN32
#endif
#include "Application.h"

namespace KrisRenderer
{
	int Main()
	{
		Application &app = Application::Get();
		app.Loop();
		return 0;
	}
}

int main()
{
	return KrisRenderer::Main();
}