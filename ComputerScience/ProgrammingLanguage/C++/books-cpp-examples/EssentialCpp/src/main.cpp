#include "Example.hpp"
#ifndef EXAMPLE_NAME
#define EXAMPLE_NAME empty
#endif

#define RUN_EXAMPLE(name) esscpp::##name()

int main() {
	std::cout << "ready to work " << std::endl;
	RUN_EXAMPLE(EXAMPLE_NAME);
	return 0;
}