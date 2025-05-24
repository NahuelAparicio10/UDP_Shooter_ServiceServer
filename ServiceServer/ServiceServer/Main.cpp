#include "ServiceManager.h"


int main()
{
	ServiceManager serviceManager;

	serviceManager.InitializeServices();

	std::cout << "[SERVICE SERVER] All services running. Press ENTER to stop." << std::endl;
	std::cin.get();

	serviceManager.StopServices();
	std::cout << "[SERVICE SERVER] Services stopped. Exiting." << std::endl;
	return 0;
}