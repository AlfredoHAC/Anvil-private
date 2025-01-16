#include "Core/application.h"

int main() {
	const char* app_name = "AnvilFramework";
	uint16 window_width = 1280;
	uint16 window_height = 720;

	bool initialized = anvlAppInit(app_name, window_width, window_height);
	if (!initialized) {
		return 1;
	}

	return 0;
}
