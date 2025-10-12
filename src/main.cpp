#include "ctrl.h"

int main(int argc, const char* argv[]) {
	std::string procName;
	std::string dllName;
	std::string dllDir;

	if (argc == 4) {
		procName = argv[1];
		dllName = argv[2];
		dllDir = argv[3];
	}

	ctrl::run(procName, dllName, dllDir);

	return 0;
}