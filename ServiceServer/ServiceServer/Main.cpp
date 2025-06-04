#include "ServiceManager.h"
#include "ConsoleUtils.h"

int main()
{
	ServiceManager serviceManager;

	serviceManager.InitializeServices();

	WriteConsole("[SERVICE SERVER] All services running. Close Window to Exit.");
	std::cin.get();

	serviceManager.StopServices();
	WriteConsole("[SERVICE SERVER] Services stopped. Exiting.");
	return 0;
}