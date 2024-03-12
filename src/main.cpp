
#include <iostream>
#include "application.h"

using namespace MiniRenderer;

int main(int, char**) {
	auto app = std::make_unique<Application>();
	app->loop();
	return 0;
}