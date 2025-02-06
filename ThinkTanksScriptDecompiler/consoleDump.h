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

#ifndef _CONSOLEDUMP_H_
#define _CONSOLEDUMP_H_

// Note: These are not the "real" T3D headers, type definitions may be very different
#include "platform/platform.h"
#include "compiler.h"

extern String Decompile(CodeBlock& cb);
extern bool DecompileWrite(String fileName_out, CodeBlock& cb);

#endif