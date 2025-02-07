//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

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


#ifndef _COMPILER_H_
#define _COMPILER_H_

// Note: These are not the "real" T3D headers, type definitions may be very different
#include "platform/platform.h"
#include "codeBlock.h"

namespace Compiler
{
	/// The opcodes for the TorqueScript VM.
	/// Reverse engineered for vSide, DSO version ??
	enum CompiledInstructions
	{
		OP_FUNC_DECL = 0,
		OP_CREATE_OBJECT = 1,
		OP_INEXISTENT_2 = 2, //OP_CREATE_DATABLOCK 1.4.2 | REMOVED 36 | OP_INVALID from Mango
		OP_INEXISTENT_3 = 3, //OP_NAME_OBJECT 1.4.2 | REMOVED 36 | OP_INVALID from Mango
		OP_ADD_OBJECT = 4,
		OP_END_OBJECT = 5,
		OP_JMPIFFNOT = 6,
		OP_JMPIFNOT = 7,
		OP_JMPIFF = 8,
		OP_JMPIF = 9,
		OP_JMPIFNOT_NP = 10,
		OP_JMPIF_NP = 11,
		OP_JMP = 12,
		OP_RETURN = 13,
		OP_CMPEQ = 14,
		OP_CMPGR = 15,
		OP_CMPGE = 16,
		OP_CMPLT = 17,
		OP_CMPLE = 18,
		OP_CMPNE = 19,
		OP_XOR = 20,
		OP_MOD = 21,
		OP_BITAND = 22,
		OP_BITOR = 23,
		OP_NOT = 24,
		OP_NOTF = 25,
		OP_ONESCOMPLEMENT = 26,

		OP_SHR = 27,
		OP_SHL = 28,
		OP_AND = 29,
		OP_OR = 30,

		OP_ADD = 31,
		OP_SUB = 32,
		OP_MUL = 33,
		OP_DIV = 34,
		OP_NEG = 35,

		OP_SETCURVAR = 36,
		OP_SETCURVAR_CREATE = 37,
		OP_SETCURVAR_ARRAY = 38,
		OP_SETCURVAR_ARRAY_CREATE = 39,

		OP_LOADVAR_UINT = 40,
		OP_LOADVAR_FLT = 41,
		OP_LOADVAR_STR = 42,

		OP_SAVEVAR_UINT = 43,
		OP_SAVEVAR_FLT = 44,
		OP_SAVEVAR_STR = 45,

		OP_SETCUROBJECT = 46,
		OP_SETCUROBJECT_NEW = 47,

		OP_SETCURFIELD = 48,
		OP_SETCURFIELD_ARRAY = 49,

		OP_LOADFIELD_UINT = 50,
		OP_LOADFIELD_FLT = 51,
		OP_LOADFIELD_STR = 52,

		OP_SAVEFIELD_UINT = 53,
		OP_SAVEFIELD_FLT = 54,
		OP_SAVEFIELD_STR = 55,

		OP_STR_TO_UINT = 56,
		OP_STR_TO_FLT = 57,
		OP_STR_TO_NONE = 58,
		OP_FLT_TO_UINT = 59,
		OP_FLT_TO_STR = 60,
		OP_FLT_TO_NONE = 61,
		OP_UINT_TO_FLT = 62,
		OP_UINT_TO_STR = 63,
		OP_UINT_TO_NONE = 64,

		OP_LOADIMMED_UINT = 65,
		OP_LOADIMMED_FLT = 66,
		OP_TAG_TO_STR = 67,
		OP_LOADIMMED_STR = 68,
		OP_LOADIMMED_IDENT = 69,

		OP_CALLFUNC_RESOLVE = 70,
		OP_CALLFUNC = 71,

		OP_INEXISTENT_72 = 72, //OP_PROCESS_ARGS,   // NOT REMOVED IN VSIDE
		OP_ADVANCE_STR = 73,
		OP_ADVANCE_STR_APPENDCHAR = 74,
		OP_ADVANCE_STR_COMMA = 75,
		OP_ADVANCE_STR_NUL = 76,
		OP_REWIND_STR = 77,
		OP_TERMINATE_REWIND_STR = 78,
		OP_COMPARE_STR = 79,

		OP_PUSH = 80,
		OP_PUSH_FRAME = 81,

		OP_BREAK = 82,
		OP_INVALID = 83, // from vSide Mango Added?

		// New shit
		OP_SETCUROBJECT_INTERNAL = 1002,
		OP_FINISH_OBJECT = 1003,

		OP_RETURN_VOID = 1004,
		OP_RETURN_UINT = 1005,
		OP_RETURN_FLT = 1006,
		OP_DOCBLOCK_STR = 1007,


		OP_PUSH_UINT = 1008,
		OP_PUSH_FLT = 1009,
		OP_PUSH_VAR = 1010,

		OP_ASSERT = 1011,
		OP_ITER_BEGIN = 1012,
		OP_ITER_BEGIN_STR = 1013,
		OP_ITER = 1014,
		OP_ITER_END = 1015,

		OP_COPYVAR_TO_NONE = 1016,
		OP_SETCURFIELD_TYPE = 1017,

		// OP_INVALID = 1000
	};

	// Call types
	enum callTypes {
		FunctionCall,
		MethodCall,
		ParentCall
	};
};

#endif
