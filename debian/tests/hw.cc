#include <iostream>
#include <ola/client/OlaClient.h>

int main(void) {
	ola::io::LoopbackDescriptor d;
	ola::client::OlaClient c(&d);

	bool success = c.Setup();

	std::cout << "Hello World!" << std::endl << "success: " << (success ? "t" : "f") << std::endl;

	return success ? 0 : 1;
}
