#include "Core.h"

int main(int argc, char**argv)
{
	Core core;
	core.initApp();
	core.getRoot()->startRendering();
	core.destroy();
	return 0;
}

//chirag
//procedural planet generation