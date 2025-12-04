#include "interpreter.h"

void MsvcSetupFunc(Picoc *picoc)
{
}

void CTest (ParseState *Parser, Value *ReturnValue,
	Value **Param, int NumArgs)
{
    printf("test(%d)\n", Param[0]->Val->Integer);
    Param[0]->Val->Integer = 1234;
}

void CLineNo (ParseState *Parser, Value *ReturnValue,
	Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = Parser->Line;
}

/* list of all library functions and their prototypes */
LibraryFunction MsvcFunctions[] =
{
    {CTest, "void Test(int);"},
    {CLineNo, "int LineNo();"},
    {NULL, NULL}
};

void PlatformLibraryInit(Picoc *picoc)
{
    IncludeRegister(picoc, "picoc_msvc.h", &MsvcSetupFunc, &MsvcFunctions[0], NULL);
}
