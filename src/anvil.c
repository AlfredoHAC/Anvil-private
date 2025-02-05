#include "anvlpch.h"

#include "Core/application.h"


// TODO: Change block starting to a new line
int main() {

	Application app = {
		.name = "AnvilFramework",
		.hints = {
			.window_width = 1280,
			.window_height = 720
		}
	};

	if (!anvlAppInit(&app)) {
		return 1;
	}

	anvlAppRun(&app);

	if (!anvlAppShutdown(&app)) {
		return 1;
	}

	return 0;
}
