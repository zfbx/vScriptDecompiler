//-----------------------------------------------------------------------------
// By ruipgpinheiro, March 2016
//
// Based on T3D code by GarageGames
// Many functions written from scratch, or modified.
// Almost all dependencies on T3D were removed, and is now compatible with
// ThinkTanks' DSO format
//
// jamesu 2019 - added changes to optionally work with onverse scripts.
// zfbx 2025 - removed onverse to focus on vside debugging
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "codeBlock.h"
#include "consoleDump.h"

using namespace std;

void usage(int argc, char *argv[]) {
	cerr << "Usage: " << argv[0] << " <DSO filename> [options]" << endl;
	cerr << "Valid options:\n";
	cerr << "\t--strings : Dump strings\n";
	cerr << "\t--hexdump : Dump code block in hexadecimal format\n";
	cerr << "\t\tNote: --strings --hexdump replaces string references in the hexdump with identifiers\n";
	cerr << "\t--disassemble : Dump disassembled instructions\n";
	cerr << "\t--nodecompile : Don't decompile script\n";
	cerr << "\t--wait : Wait for key press after conclusion\n";
	cerr << "All commands output to stdout. To write to a file, redirect it to a file, i.e.\n";
	cerr << argv[0] << " <DSO filename> [options] > [output_filename]\n";
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		usage(argc, argv);
		exit(EXIT_FAILURE);
	}

	//std::string fileName = "tests/append_char.cs.dso";

	std::string fileName = argv[1];
	bool strings = false;
	bool hexdump = false;
	bool disassemble = false;
	bool decompile = true;
	bool wait = false;
	bool pipeonly = false;

	for (int i = 2; i < argc; i++) {
		char *curArg = argv[i];

		if (strcmp(curArg, "--strings") == 0)
			strings = true;
		else if (strcmp(curArg, "--hexdump") == 0)
			hexdump = true;
		else if (strcmp(curArg, "--disassemble") == 0)
			disassemble = true;
		else if (strcmp(curArg, "--nodecompile") == 0) // why wouldn't you want it to decompile? :P
			decompile = false;
		else if (strcmp(curArg, "--wait") == 0)
			wait = true;
		else if (strcmp(curArg, "--pipeonly") == 0)
			pipeonly = true;
		else {
			cerr << "Invalid argument " << curArg << endl;
			usage(argc, argv);
			exit(EXIT_FAILURE);
		}
	}
	

	cerr << "Started" << endl;

	CodeBlock cb;

	if (!cb.readVside(fileName)) {
		exit(EXIT_FAILURE);
	}
	
	if (hexdump)
		cb.dumpCode(strings);

	if (strings && !hexdump)
		cb.dumpAllStrings();

	if (disassemble)
		cb.dumpInstructions(0, 0, false);

	if (decompile) {
		cerr << "Decompiling..." << endl;
		cout << Decompile(cb);
	}

	if (!pipeonly) {
		std::string fileName_out = fileName;
		if (fileName_out.length() >= 4) {
			fileName_out.erase(fileName_out.length() - 4); 
		}
		DecompileWrite(fileName_out, cb);
	}

	cerr << "Done.";

	if (wait) {
		cerr << " Press any key to exit.";
		getchar();
	}
	else {
		cerr << endl;
	}

	exit(EXIT_SUCCESS);
}