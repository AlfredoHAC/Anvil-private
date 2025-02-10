#include "anvlpch.h"

#include "Core/application.h"


// TODO: Add prefix 'ANVL_' to header guard definitions
// TODO: Verify all failure returns on memory allocations
// TODO: Remove unecessary 'null' redefinition
int main()
{

	Application app =
	{
		.name = "AnvilFramework",
		.hints = 
		{
			.window_width = 1280,
			.window_height = 720
		}
	};

	if (!anvlAppInit(&app))
	{
		return 1;
	}

	anvlAppRun(&app);

	if (!anvlAppShutdown(&app))
	{
		return 1;
	}

	return 0;
}
