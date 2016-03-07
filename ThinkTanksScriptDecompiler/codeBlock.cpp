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
//-----------------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <assert.h>

#include "codeBlock.h"

using namespace Compiler;
using namespace std;

//-----------------------------------------------------------------------------
// Constructor/Destructor
// Borrowed directly from T3D source
//-----------------------------------------------------------------------------
CodeBlock::CodeBlock()
{
	globalStrings = NULL;
	functionStrings = NULL;
	functionStringsMaxLen = 0;
	globalStringsMaxLen = 0;
	globalFloats = NULL;
	functionFloats = NULL;
	lineBreakPairs = NULL;
	breakList = NULL;
	breakListSize = 0;

	refCount = 0;
	code = NULL;
	name = NULL;
	fullPath = NULL;
	modPath = NULL;
}

CodeBlock::~CodeBlock()
{
	delete[] const_cast<char*>(globalStrings);
	delete[] const_cast<char*>(functionStrings);

	functionStringsMaxLen = 0;
	globalStringsMaxLen = 0;

	delete[] globalFloats;
	delete[] functionFloats;
	delete[] code;
	delete[] breakList;
}

//-----------------------------------------------------------------------------
// Utility functions for reading ints and floats from a C++ stream
//-----------------------------------------------------------------------------
U32 stream_readi(Stream &st) {
	U32 res;

	st.read((char*)&res, sizeof(res));

	// Sanity check
	assert(!st.eof());
	assert(!st.fail());

	return res;
}

F64 stream_readf(Stream &st) {
	F64 res;

	st.read((char*)&res, sizeof(res));

	// Sanity check
	assert(!st.eof());
	assert(!st.fail());

	return res;
}

//-----------------------------------------------------------------------------
// Parse DSO files
// Compatible with ThinkTank's DSO version
//-----------------------------------------------------------------------------
bool CodeBlock::read(String &fileName) {
	std::ifstream st (fileName, std::fstream::binary);
	if (st.fail())
	{
		cerr << "ERROR: Could not open file '" << fileName << "'." << endl;
		return false;
	}

	return read(st);
}

bool CodeBlock::read(Stream &st)
{
	assert(!m_loaded);

	U32 globalSize, size, i;

	version = stream_readi(st);
	assert(version == 0x21);

	size = stream_readi(st);
	fprintf(stderr, "Reading %d bytes of globalStrings, currently at position 0x%X\n", size, (unsigned int)st.tellg());
	if (size)
	{
		globalStrings = new char[size];
		globalStringsMaxLen = size;
		st.read(globalStrings, size);
	}
	globalSize = size;

	size = stream_readi(st);
	fprintf(stderr, "Reading %d bytes of globalFloats, currently at position 0x%X\n", size, (unsigned int)st.tellg());
	if (size)
	{
		globalFloats = new F64[size];
		for (U32 i = 0; i < size; i++)
			globalFloats[i] = stream_readf(st);
	}

	size = stream_readi(st);
	fprintf(stderr, "Reading %d bytes of functionStrings, currently at position 0x%X\n", size, (unsigned int)st.tellg());
	if (size)
	{
		functionStrings = new char[size];
		functionStringsMaxLen = size;
		st.read(functionStrings, size);
	}

	size = stream_readi(st);
	fprintf(stderr, "Reading %d bytes of functionFloats, currently at position 0x%X\n", size, (unsigned int)st.tellg());
	if (size)
	{
		functionFloats = new F64[size];
		for (U32 i = 0; i < size; i++)
			functionFloats[i] = stream_readf(st);
	}

	U32 codeLength;
	codeLength = stream_readi(st);
	codeSize = codeLength;
	lineBreakPairCount = stream_readi(st);

	fprintf(stderr, "Currently at position 0x%X\n", (unsigned int)st.tellg());
	U32 totSize = codeLength + lineBreakPairCount * 2;
	fprintf(stderr, "Code length: %d  Line Break count: %d  totSize: %d\n", codeLength, lineBreakPairCount, totSize);

	code = new U32[totSize];

	for (i = 0; i < codeLength; i++)
	{
		U8 b;
		st.read((char*)&b, 1);
		if (b == 0xFF)
			code[i] = stream_readi(st);
		else
			code[i] = b;
	}

	for (i = codeLength; i < totSize; i++)
		code[i] = stream_readi(st);

	lineBreakPairs = code + codeLength;

	// StringTable-ize our identifiers.
	U32 identCount;
	identCount = stream_readi(st);
	while (identCount--)
	{
		U32 offset;
		offset = stream_readi(st);

		/*StringTableEntry ste;
		if (offset < globalSize)
			ste = StringTable->insert(globalStrings + offset);
		else
			ste = StringTable->insert("");*/

		char *ste;
		if (offset < globalSize)
			ste = globalStrings + offset;
		else
			ste = "";

		U32 count;
		count = stream_readi(st);
		while (count--)
		{
			U32 ip;
			ip = stream_readi(st);

			code[ip] = (U32)ste;
		}
	}

	fprintf(stderr, "Finished reading, currently at position 0x%X\n", (unsigned int)st.tellg());
	int c = st.peek();
	assert(c == EOF && st.eof());

	if (lineBreakPairCount)
		calcBreakList();

	m_loaded = true;

	return true;
}

//-----------------------------------------------------------------------------
// Borrowed directly from T3D source without any changes
// Post-processes the loaded code
//-----------------------------------------------------------------------------
void CodeBlock::calcBreakList()
{
	U32 size = 0;
	S32 line = -1;
	U32 seqCount = 0;
	U32 i;
	for (i = 0; i < lineBreakPairCount; i++)
	{
		U32 lineNumber = lineBreakPairs[i * 2];
		if (lineNumber == U32(line + 1))
			seqCount++;
		else
		{
			if (seqCount)
				size++;
			size++;
			seqCount = 1;
		}
		line = lineNumber;
	}
	if (seqCount)
		size++;

	breakList = new U32[size];
	breakListSize = size;
	line = -1;
	seqCount = 0;
	size = 0;

	for (i = 0; i < lineBreakPairCount; i++)
	{
		U32 lineNumber = lineBreakPairs[i * 2];

		if (lineNumber == U32(line + 1))
			seqCount++;
		else
		{
			if (seqCount)
				breakList[size++] = seqCount;
			breakList[size++] = lineNumber - getMax(0, line) - 1;
			seqCount = 1;
		}

		line = lineNumber;
	}

	if (seqCount)
		breakList[size++] = seqCount;

	for (i = 0; i < lineBreakPairCount; i++)
	{
		U32 *p = lineBreakPairs + i * 2;
		p[0] = (p[0] << 8) | code[p[1]];
	}
}

//-----------------------------------------------------------------------------
// Converts a code pointer and ip into a StringTableEntry pointer
//-----------------------------------------------------------------------------
inline StringTableEntry CodeBlock::CodeToSTE(U32 *code, U32 ip)
{
	StringTableEntry res = (StringTableEntry)(*(code + ip));

	// Sanity check
	if ((res == NULL) ||
		(res >= globalStrings && res < globalStrings + globalStringsMaxLen) ||
		(res >= functionStrings && res < functionStrings + functionStringsMaxLen)) {
		return res;
	}

	abort();
}

//-----------------------------------------------------------------------------
// Checks if a given pointer is actually a string pointer to one of the
// globalStrings or functionStrings string arrays.
//
// Returns 'G#' or 'F#' where 'G' and 'F' denote global or function strings,
// and # is the number of the string in the array
//-----------------------------------------------------------------------------
char* CodeBlock::findStringInfoFromPointer(char * ptr) {
	assert(m_loaded);

	char * stringBuffer = NULL;
	char * stringBufferEnd = NULL;

	// Choose the buffer
	if (ptr >= globalStrings && ptr < globalStrings + globalStringsMaxLen) {
		stringBuffer = globalStrings;
		stringBufferEnd = globalStrings + globalStringsMaxLen;
	}
	else if (ptr >= functionStrings && ptr < functionStrings + functionStringsMaxLen) {
		stringBuffer = functionStrings;
		stringBufferEnd = functionStrings + functionStringsMaxLen;
	}
	else {
		return NULL;
	}

	// We want the beginning of a string
	if (ptr != stringBuffer && *(ptr - 1) != '\0')
		return NULL;

	// Find number of current string, by counting '\0's
	U32 count = 0;
	char *p;
	for (p = stringBuffer; p < ptr && p < stringBufferEnd; p++) {
		if (*p == '\0')
			count++;
	}

	assert(p != stringBufferEnd);

	static char res[15];
	sprintf_s(res, "%c%02u", stringBuffer == globalStrings ? 'G' : 'F', count);

	return res;
}

//-----------------------------------------------------------------------------
// Dumps all strings in stringBuffer
// stringBuffer must point to globalStrings or functionStrings
//-----------------------------------------------------------------------------
void CodeBlock::dumpStrings(char * stringBuffer) {
	assert(m_loaded);

	char * stringBufferEnd = NULL;

	if (stringBuffer == globalStrings) {
		stringBufferEnd = globalStrings + globalStringsMaxLen;
	}
	else if (stringBuffer == functionStrings) {
		stringBufferEnd = functionStrings + functionStringsMaxLen;
	}
	else {
		return;
	}

	char *prev_start, *p;
	p = stringBuffer;
	prev_start = p;

	U32 count = 0;
	for (p = stringBuffer; p < stringBufferEnd; p++) {
		if (*p == '\0') {
			printf("%c%02u (os=0x%08X) = \"%s\"\n", stringBuffer == globalStrings ? 'G' : 'F', count, prev_start-stringBuffer, prev_start);
			prev_start = p + 1;

			count++;
		}
	}

	assert(prev_start == p);
}

//-----------------------------------------------------------------------------
// Dumps code in hex format
//
// If 'strings' is true, it replaces string references with the string
// identifier and then also dumps all string
//-----------------------------------------------------------------------------
void CodeBlock::dumpCode(bool strings) {
	assert(m_loaded);

	U32 ip = 0;

	cerr << "Dumping code in hex-format (Size=" << codeSize << ")..." << endl;
	
	printf("0x%08X : ", ip);
	while (ip < codeSize)
	{
		U32 curCode = code[ip];

		if (ip > 0) {
			if (ip % 4 != 0)
				putc(' ', stdout);
			else
				printf("\n0x%08X : ", ip);
		}

		char *stringInfo;
		if(strings)
			stringInfo = findStringInfoFromPointer((char*)curCode);

		if (stringInfo != NULL) {
			printf("%8s", stringInfo);
		}
		else {
			printf("%08X", curCode);
		}

		ip++;
	}
	putc('\n', stdout);

	if (strings) {
		cerr << "Dumping strings..." << endl;
		dumpStrings(globalStrings);
		dumpStrings(functionStrings);
	}
}

//-----------------------------------------------------------------------------
// Disassembles instructions, and dumps them to stdout
// Based on a T3D function of the same name, heavily modified
//-----------------------------------------------------------------------------
void CodeBlock::dumpInstructions(U32 startIp, U32 number, bool upToReturn) //number=0 means infinite
{
	assert(m_loaded);

	U32 ip = startIp;
	bool smInFunction = false;
	U32 endFuncIp = 0;
	U32 invalidCount = 0;
	U32 totalCount = 0;

	if (number > 0) {
		fprintf(stderr, "Dumping %u instructions starting from IP=%u\n", number, ip);
	}
	else {
		fprintf(stderr, "Dumping all instructions starting from IP=%u\n", ip);
	}

	while (ip < codeSize)
	{
		if (ip > endFuncIp)
		{
			smInFunction = false;
		}

		U32 curInstruction = code[ip++];
		printf("0x%08X : 0x%02X/%02u ", ip-1, curInstruction, curInstruction);
		totalCount++;

		switch (curInstruction)
		{

		case OP_FUNC_DECL:
		{
			StringTableEntry fnName = CodeToSTE(code, ip);
			StringTableEntry fnNamespace = CodeToSTE(code, ip + 1);
			StringTableEntry fnPackage = CodeToSTE(code, ip + 2);
			bool hasBody = bool(code[ip + 3]);
			U32 newIp = code[ip + 4];
			U32 argc = code[ip + 5];
			endFuncIp = newIp;

			printf("OP_FUNC_DECL name=%s nspace=%s package=%s hasbody=%i newip=0x%08X argc=%i\n",
				fnName, fnNamespace, fnPackage, hasBody, newIp, argc);

			if (argc > 0) {
				printf("\tARGS:");

				for (U32 i = 0; i < argc; i++) {
					U32 idx = ip + 6 + i;
					StringTableEntry varName = CodeToSTE(code, idx);

					printf(" \"%s\"", varName);
				}
				putc('\n', stdout);
			}

			// Skip args.

			ip += 6 + argc;
			smInFunction = true;
			break;
		}

		case OP_CREATE_OBJECT:
		{

			StringTableEntry objParent = CodeToSTE(code, ip);
			bool isDataBlock = code[ip + 1];
			U32 failJump = code[ip + 2];

			printf("OP_CREATE_OBJECT objParent=%s isDataBlock=%i failJump=0x%08X\n",
				objParent, isDataBlock, failJump);

			ip += 3;
			break;
		}

		case OP_ADD_OBJECT:
		{
			bool placeAtRoot = code[ip++];
			printf("OP_ADD_OBJECT placeAtRoot=%i\n", placeAtRoot);
			break;
		}

		case OP_END_OBJECT:
		{
			bool placeAtRoot = code[ip++];
			printf("OP_END_OBJECT placeAtRoot=%i\n", placeAtRoot);
			break;
		}

		/*case OP_FINISH_OBJECT:
		{
			printf("OP_FINISH_OBJECT\n");
			break;
		}*/

		case OP_JMPIFFNOT:
		{
			printf("OP_JMPIFFNOT ip=0x%08X\n", code[ip]);
			++ip;
			break;
		}

		case OP_JMPIFNOT:
		{
			printf("OP_JMPIFNOT ip=0x%08X\n", code[ip]);
			++ip;
			break;
		}

		case OP_JMPIFF:
		{
			printf("OP_JMPIFF ip=0x%08X\n", code[ip]);
			++ip;
			break;
		}

		case OP_JMPIF:
		{
			printf("OP_JMPIF ip=0x%08X\n", code[ip]);
			++ip;
			break;
		}

		case OP_JMPIFNOT_NP:
		{
			printf("OP_JMPIFNOT_NP ip=0x%08X\n", code[ip]);
			++ip;
			break;
		}

		case OP_JMPIF_NP:
		{
			printf("OP_JMPIF_NP ip=0x%08X\n", code[ip]);
			++ip;
			break;
		}

		case OP_JMP:
		{
			printf("OP_JMP ip=0x%08X\n", code[ip]);
			++ip;
			break;
		}

		case OP_RETURN:
		{
			printf("OP_RETURN\n");

			if (upToReturn)
				return;

			break;
		}

		/*case OP_RETURN_VOID:
		{
			printf("OP_RETURNVOID\n");

			if (upToReturn)
				return;

			break;
		}

		case OP_RETURN_UINT:
		{
			printf("OP_RETURNUINT\n");

			if (upToReturn)
				return;

			break;
		}

		case OP_RETURN_FLT:
		{
			printf("OP_RETURNFLT\n");

			if (upToReturn)
				return;

			break;
		}*/

		case OP_CMPEQ:
		{
			printf("OP_CMPEQ\n");
			break;
		}

		case OP_CMPGR:
		{
			printf("OP_CMPGR\n");
			break;
		}

		case OP_CMPGE:
		{
			printf("OP_CMPGE\n");
			break;
		}

		case OP_CMPLT:
		{
			printf("OP_CMPLT\n");
			break;
		}

		case OP_CMPLE:
		{
			printf("OP_CMPLE\n");
			break;
		}

		case OP_CMPNE:
		{
			printf("OP_CMPNE\n");
			break;
		}

		case OP_XOR:
		{
			printf("OP_XOR\n");
			break;
		}

		case OP_MOD:
		{
			printf("OP_MOD\n");
			break;
		}

		case OP_BITAND:
		{
			printf("OP_BITAND\n");
			break;
		}

		case OP_BITOR:
		{
			printf("OP_BITOR\n");
			break;
		}

		case OP_NOT:
		{
			printf("OP_NOT\n");
			break;
		}

		case OP_NOTF:
		{
			printf("OP_NOTF\n");
			break;
		}

		case OP_ONESCOMPLEMENT:
		{
			printf("OP_ONESCOMPLEMENT\n");
			break;
		}

		case OP_SHR:
		{
			printf("OP_SHR\n");
			break;
		}

		case OP_SHL:
		{
			printf("OP_SHL\n");
			break;
		}

		case OP_AND:
		{
			printf("OP_AND\n");
			break;
		}

		case OP_OR:
		{
			printf("OP_OR\n");
			break;
		}

		case OP_ADD:
		{
			printf("OP_ADD\n");
			break;
		}

		case OP_SUB:
		{
			printf("OP_SUB\n");
			break;
		}

		case OP_MUL:
		{
			printf("OP_MUL\n");
			break;
		}

		case OP_DIV:
		{
			printf("OP_DIV\n");
			break;
		}

		case OP_NEG:
		{
			printf("OP_NEG\n");
			break;
		}

		case OP_SETCURVAR:
		{
			StringTableEntry var = CodeToSTE(code, ip);

			printf("OP_SETCURVAR var=%s\n", var);
			ip += 1;
			break;
		}

		case OP_SETCURVAR_CREATE:
		{
			StringTableEntry var = CodeToSTE(code, ip);

			printf("OP_SETCURVAR_CREATE var=%s\n", var);
			ip += 1;
			break;
		}

		case OP_SETCURVAR_ARRAY:
		{
			printf("OP_SETCURVAR_ARRAY\n");
			break;
		}

		case OP_SETCURVAR_ARRAY_CREATE:
		{
			printf("OP_SETCURVAR_ARRAY_CREATE\n");
			break;
		}

		case OP_LOADVAR_UINT:
		{
			printf("OP_LOADVAR_UINT\n");
			break;
		}

		case OP_LOADVAR_FLT:
		{
			printf("OP_LOADVAR_FLT\n");
			break;
		}

		case OP_LOADVAR_STR:
		{
			printf("OP_LOADVAR_STR\n");
			break;
		}

		case OP_LOADVAR_VAR:
		{
			printf("OP_LOADVAR_VAR\n");
			break;
		}

		case OP_SAVEVAR_UINT:
		{
			printf("OP_SAVEVAR_UINT\n");
			break;
		}

		case OP_SAVEVAR_FLT:
		{
			printf("OP_SAVEVAR_FLT\n");
			break;
		}

		case OP_SAVEVAR_STR:
		{
			printf("OP_SAVEVAR_STR\n");
			break;
		}

		case OP_SAVEVAR_VAR:
		{
			printf("OP_SAVEVAR_VAR\n");
			break;
		}

		/*case OP_SETCUROBJECT:
		{
			printf("OP_SETCUROBJECT\n");
			break;
		}

		case OP_SETCUROBJECT_NEW:
		{
			printf("OP_SETCUROBJECT_NEW\n");
			break;
		}

		case OP_SETCUROBJECT_INTERNAL:
		{
			printf("OP_SETCUROBJECT_INTERNAL\n");
			++ip;
			break;
		}*/

		case OP_SETCURFIELD:
		{
			StringTableEntry curField = CodeToSTE(code, ip);
			printf("OP_SETCURFIELD field=%s\n", curField);
			ip += 1;
			break;
		}

		/*case OP_SETCURFIELD_ARRAY:
		{
			printf("OP_SETCURFIELD_ARRAY\n");
			break;
		}

		case OP_SETCURFIELD_TYPE:
		{
			U32 type = code[ip];
			printf("OP_SETCURFIELD_TYPE type=%i\n", type);
			++ip;
			break;
		}*/

		case OP_LOADFIELD_UINT:
		{
			printf("OP_LOADFIELD_UINT\n");
			break;
		}

		case OP_LOADFIELD_FLT:
		{
			printf("OP_LOADFIELD_FLT\n");
			break;
		}

		case OP_LOADFIELD_STR:
		{
			printf("OP_LOADFIELD_STR\n");
			break;
		}

		case OP_SAVEFIELD_UINT:
		{
			printf("OP_SAVEFIELD_UINT\n");
			break;
		}

		case OP_SAVEFIELD_FLT:
		{
			printf("OP_SAVEFIELD_FLT\n");
			break;
		}

		case OP_SAVEFIELD_STR:
		{
			printf("OP_SAVEFIELD_STR\n");
			break;
		}

		case OP_STR_TO_UINT:
		{
			printf("OP_STR_TO_UINT\n");
			break;
		}

		case OP_STR_TO_FLT:
		{
			printf("OP_STR_TO_FLT\n");
			break;
		}

		case OP_STR_TO_NONE:
		{
			printf("OP_STR_TO_NONE\n");
			break;
		}

		case OP_FLT_TO_UINT:
		{
			printf("OP_FLT_TO_UINT\n");
			break;
		}

		case OP_FLT_TO_STR:
		{
			printf("OP_FLT_TO_STR\n");
			break;
		}

		case OP_FLT_TO_NONE:
		{
			printf("OP_FLT_TO_NONE\n");
			break;
		}

		case OP_UINT_TO_FLT:
		{
			printf("OP_SAVEFIELD_STR\n");
			break;
		}

		case OP_UINT_TO_STR:
		{
			printf("OP_UINT_TO_STR\n");
			break;
		}

		case OP_UINT_TO_NONE:
		{
			printf("OP_UINT_TO_NONE\n");
			break;
		}

		/*case OP_COPYVAR_TO_NONE:
		{
			printf("OP_COPYVAR_TO_NONE\n");
			break;
		}*/

		case OP_LOADIMMED_UINT:
		{
			U32 val = code[ip];
			printf("OP_LOADIMMED_UINT val=%i\n", val);
			++ip;
			break;
		}

		case OP_LOADIMMED_FLT:
		{
			F64 val = (smInFunction ? functionFloats : globalFloats)[code[ip]];
			printf("OP_LOADIMMED_FLT val=%f\n", val);
			++ip;
			break;
		}

		case OP_TAG_TO_STR:
		{
			const char* str = (smInFunction ? functionStrings : globalStrings) + code[ip];
			printf("OP_TAG_TO_STR str=%s\n", str);
			++ip;
			break;
		}

		case OP_LOADIMMED_STR:
		{
			const char* str = (smInFunction ? functionStrings : globalStrings) + code[ip];
			printf("OP_LOADIMMED_STR str=%s\n", str);
			++ip;
			break;
		}

		/*case OP_DOCBLOCK_STR:
		{
			const char* str = (smInFunction ? functionStrings : globalStrings) + code[ip];
			printf("OP_DOCBLOCK_STR str=%s\n", str);
			++ip;
			break;
		}*/

		case OP_LOADIMMED_IDENT:
		{
			StringTableEntry str = CodeToSTE(code, ip);
			printf("OP_LOADIMMED_IDENT str=%s\n", str);
			ip += 2;
			break;
		}

		case OP_CALLFUNC_RESOLVE:
		{
			StringTableEntry fnNamespace = CodeToSTE(code, ip + 1);
			StringTableEntry fnName = CodeToSTE(code, ip);
			U32 callType = code[ip + 2];

			printf("OP_CALLFUNC_RESOLVE name=%s nspace=%s callType=%s\n", fnName, fnNamespace,
				callType == callTypes::FunctionCall ? "FunctionCall"
				: callType == callTypes::MethodCall ? "MethodCall" : "ParentCall");

			ip += 3;
			break;
		}

		case OP_CALLFUNC:
		{
			StringTableEntry fnNamespace = CodeToSTE(code, ip + 1);
			StringTableEntry fnName = CodeToSTE(code, ip);
			U32 callType = code[ip + 2];

			printf("OP_CALLFUNC name=%s nspace=%s callType=%s\n", fnName, fnNamespace,
				callType == callTypes::FunctionCall ? "FunctionCall"
				: callType == callTypes::MethodCall ? "MethodCall" : "ParentCall");

			ip += 3;
			break;
		}

		case OP_ADVANCE_STR:
		{
			printf("OP_ADVANCE_STR\n");
			break;
		}

		case OP_ADVANCE_STR_APPENDCHAR:
		{
			char ch = code[ip];
			printf("OP_ADVANCE_STR_APPENDCHAR char=%c\n", ch);
			++ip;
			break;
		}

		case OP_ADVANCE_STR_COMMA:
		{
			printf("OP_ADVANCE_STR_COMMA\n");
			break;
		}

		case OP_ADVANCE_STR_NUL:
		{
			printf("OP_ADVANCE_STR_NUL\n");
			break;
		}

		case OP_REWIND_STR:
		{
			printf("OP_REWIND_STR\n");
			break;
		}

		case OP_TERMINATE_REWIND_STR:
		{
			printf("OP_TERMINATE_REWIND_STR\n");
			break;
		}

		case OP_COMPARE_STR:
		{
			printf("OP_COMPARE_STR\n");
			break;
		}

		case OP_PUSH:
		{
			printf("OP_PUSH\n");
			break;
		}

		/*case OP_PUSH_UINT:
		{
			printf("OP_PUSH_UINT\n");
			break;
		}

		case OP_PUSH_FLT:
		{
			printf("OP_PUSH_FLT\n");
			break;
		}

		case OP_PUSH_VAR:
		{
			printf("OP_PUSH_VAR\n");
			break;
		}*/

		case OP_PUSH_FRAME:
		{
			printf("OP_PUSH_FRAME\n");
			break;
		}

		/*case OP_ASSERT:
		{
			const char* message = (smInFunction ? functionStrings : globalStrings) + code[ip];
			printf("OP_ASSERT message=%s\n", message);
			++ip;
			break;
		}

		case OP_BREAK:
		{
			printf("OP_BREAK\n");
			break;
		}

		case OP_ITER_BEGIN:
		{
			StringTableEntry varName = CodeToSTE(code, ip);
			U32 failIp = code[ip + 2];

			printf("OP_ITER_BEGIN varName=%s failIp=%i\n", varName, failIp);

			ip += 3;
			break;
		}

		case OP_ITER_BEGIN_STR:
		{
			StringTableEntry varName = CodeToSTE(code, ip);
			U32 failIp = code[ip + 2];

			printf("OP_ITER_BEGIN varName=%s failIp=%i\n", varName, failIp);

			ip += 3;
			break;
		}

		case OP_ITER:
		{
			U32 breakIp = code[ip];

			printf("OP_ITER breakIp=%i\n", breakIp);

			++ip;
			break;
		}

		case OP_ITER_END:
		{
			printf("OP_ITER_END\n");
			break;
		}*/

		default:
			invalidCount++;
			printf("!!INVALID!! (curInstruction = %d)\n", curInstruction);
			break;
		}

		if (number > 0 && totalCount >= number) {
			break;
		}
	}

	if(invalidCount > 0)
		printf("WARNING: There were %u invalid instructions!\n", invalidCount);

	smInFunction = false;
}
