//-----------------------------------------------------------------------------
// By ruipgpinheiro, March 2016
//
// Based on T3D code by GarageGames
// Many functions written from scratch, or modified.
// Almost all dependencies on T3D were removed, and is now compatible with
// ThinkTanks' DSO format
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include "codeBlock.h"
#include "consoleDump.h"

int main()
{
	printf("Started\n");

	CodeBlock cb;

	std::string fileName = "tank.cs.dso";
	std::string fileName_out = "tank.cs";
	cb.read(fileName);

	cb.dumpCode();
	cb.dumpInstructions(0, 0, false);

	DecompileWrite(fileName_out, cb);

	printf("Done.\n");
	getchar();
	exit(0);
}

