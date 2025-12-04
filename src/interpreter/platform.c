/* picoc's interface to the underlying platform. most platform-specific code
 * is in platform/platform_XX.c and platform/library_XX.c */

#include "picoc.h"
#include "interpreter.h"


static void PrintSourceTextErrorLine(IOFILE *Stream, const char *FileName,
        const char *SourceText, int Line, int CharacterPos);

#ifdef DEBUGGER
static int gEnableDebugger = true;
#else
static int gEnableDebugger = false;
#endif


/* initialize everything */
void PicocInitialize(Picoc *picoc, int StackSize)
{
    memset(picoc, '\0', sizeof(*picoc));
    PlatformInit(picoc);
    BasicIOInit(picoc);
    HeapInit(picoc, StackSize);
    TableInit(picoc);
    VariableInit(picoc);
    LexInit(picoc);
    TypeInit(picoc);
    IncludeInit(picoc);
    LibraryInit(picoc);
    PlatformLibraryInit(picoc);
#ifdef DEBUGGER
    DebugInit(picoc);
#endif
}

/* free memory */
void PicocCleanup(Picoc *picoc)
{
#ifdef DEBUGGER
    DebugCleanup(picoc);
#endif
    IncludeCleanup(picoc);
    ParseCleanup(picoc);
    LexCleanup(picoc);
    VariableCleanup(picoc);
    TypeCleanup(picoc);
    TableStrFree(picoc);
    HeapCleanup(picoc);
    PlatformCleanup(picoc);
}

/* platform-dependent code for running programs */
#if defined(UNIX_HOST) || defined(WIN32)

#define CALL_MAIN_NO_ARGS_RETURN_VOID "main();"
#define CALL_MAIN_WITH_ARGS_RETURN_VOID "main(__argc,__argv);"
#define CALL_MAIN_NO_ARGS_RETURN_INT "__exit_value = main();"
#define CALL_MAIN_WITH_ARGS_RETURN_INT "__exit_value = main(__argc,__argv);"

void PicocCallMain(Picoc *picoc, int argc, char **argv)
{
    /* check if the program wants arguments */
    Value *FuncValue = NULL;

    if (!VariableDefined(picoc, TableStrRegister(picoc, "main")))
        ProgramFailNoParser(picoc, "main() is not defined");

    VariableGet(picoc, NULL, TableStrRegister(picoc, "main"), &FuncValue);
    if (FuncValue->Typ->Base != TypeFunction)
        ProgramFailNoParser(picoc, "main is not a function - can't call it");

    if (FuncValue->Val->FuncDef.NumParams != 0) {
        /* define the arguments */
        VariableDefinePlatformVar(picoc, NULL, "__argc", &picoc->IntType,
            (AnyValue*)&argc, false);
        VariableDefinePlatformVar(picoc, NULL, "__argv", picoc->CharPtrPtrType,
            (AnyValue*)&argv, false);
    }

    if (FuncValue->Val->FuncDef.ReturnType == &picoc->VoidType) {
        if (FuncValue->Val->FuncDef.NumParams == 0)
            PicocParse(picoc, "startup", CALL_MAIN_NO_ARGS_RETURN_VOID,
                strlen(CALL_MAIN_NO_ARGS_RETURN_VOID), true, true, false,
                gEnableDebugger);
        else
            PicocParse(picoc, "startup", CALL_MAIN_WITH_ARGS_RETURN_VOID,
                strlen(CALL_MAIN_WITH_ARGS_RETURN_VOID), true, true, false,
                gEnableDebugger);
    } else {
        VariableDefinePlatformVar(picoc, NULL, "__exit_value", &picoc->IntType,
            (AnyValue *)&picoc->PicocExitValue, true);

        if (FuncValue->Val->FuncDef.NumParams == 0)
            PicocParse(picoc, "startup", CALL_MAIN_NO_ARGS_RETURN_INT,
                strlen(CALL_MAIN_NO_ARGS_RETURN_INT), true, true, false,
                gEnableDebugger);
        else
            PicocParse(picoc, "startup", CALL_MAIN_WITH_ARGS_RETURN_INT,
                strlen(CALL_MAIN_WITH_ARGS_RETURN_INT), true, true, false,
                gEnableDebugger);
    }
}
#endif

void PrintSourceTextErrorLine(IOFILE *Stream, const char *FileName,
    const char *SourceText, int Line, int CharacterPos)
{
    int LineCount;
    int CCount;
    const char *LinePos;
    const char *CPos;

    if (SourceText != NULL) {
        /* find the source line */
        for (LinePos = SourceText, LineCount = 1; *LinePos != '\0' &&
                LineCount < Line; LinePos++) {
            if (*LinePos == '\n')
                LineCount++;
        }

        /* display the line */
        for (CPos = LinePos; *CPos != '\n' && *CPos != '\0'; CPos++)
            PrintCh(*CPos, Stream);
        PrintCh('\n', Stream);

        /* display the error position */
        for (CPos = LinePos, CCount = 0; *CPos != '\n' && *CPos != '\0' &&
                (CCount < CharacterPos || *CPos == ' '); CPos++, CCount++) {
            if (*CPos == '\t')
                PrintCh('\t', Stream);
            else
                PrintCh(' ', Stream);
        }
    } else {
        /* assume we're in interactive mode - try to make the arrow match
            up with the input text */
        for (CCount = 0;
                CCount < CharacterPos+(int)strlen(INTERACTIVE_PROMPT_STATEMENT);
                CCount++)
            PrintCh(' ', Stream);
    }
    PlatformPrintf(Stream, "^\n%s:%d:%d ", FileName, Line, CharacterPos);
}

/* exit with a message */
void ProgramFail(ParseState *Parser, const char *Message, ...)
{
    va_list Args;

    PrintSourceTextErrorLine(Parser->picoc->CStdOut, Parser->FileName,
        Parser->SourceText, Parser->Line, Parser->CharacterPos);
    va_start(Args, Message);
    PlatformVPrintf(Parser->picoc->CStdOut, Message, Args);
    va_end(Args);
    PlatformPrintf(Parser->picoc->CStdOut, "\n");
    PlatformExit(Parser->picoc, 1);
}

/* exit with a message, when we're not parsing a program */
void ProgramFailNoParser(Picoc *picoc, const char *Message, ...)
{
    va_list Args;

    va_start(Args, Message);
    PlatformVPrintf(picoc->CStdOut, Message, Args);
    va_end(Args);
    PlatformPrintf(picoc->CStdOut, "\n");
    PlatformExit(picoc, 1);
}

/* like ProgramFail() but gives descriptive error messages for assignment */
void AssignFail(ParseState *Parser, const char *Format,
    ValueType *Type1, ValueType *Type2, int Num1, int Num2,
    const char *FuncName, int ParamNo)
{
    IOFILE *Stream = Parser->picoc->CStdOut;

    PrintSourceTextErrorLine(Parser->picoc->CStdOut, Parser->FileName,
        Parser->SourceText, Parser->Line, Parser->CharacterPos);
    PlatformPrintf(Stream, "can't %s ", (FuncName == NULL) ? "assign" : "set");

    if (Type1 != NULL)
        PlatformPrintf(Stream, Format, Type1, Type2);
    else
        PlatformPrintf(Stream, Format, Num1, Num2);

    if (FuncName != NULL)
        PlatformPrintf(Stream, " in argument %d of call to %s()", ParamNo,
            FuncName);

    PlatformPrintf(Stream, "\n");
    PlatformExit(Parser->picoc, 1);
}

/* exit lexing with a message */
void LexFail(Picoc *picoc, LexState *Lexer, const char *Message, ...)
{
    va_list Args;

    PrintSourceTextErrorLine(picoc->CStdOut, Lexer->FileName, Lexer->SourceText,
        Lexer->Line, Lexer->CharacterPos);
    va_start(Args, Message);
    PlatformVPrintf(picoc->CStdOut, Message, Args);
    va_end(Args);
    PlatformPrintf(picoc->CStdOut, "\n");
    PlatformExit(picoc, 1);
}

/* printf for compiler error reporting */
void PlatformPrintf(IOFILE *Stream, const char *Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    PlatformVPrintf(Stream, Format, Args);
    va_end(Args);
}

void PlatformVPrintf(IOFILE *Stream, const char *Format, va_list Args)
{
    const char *FPos;

    for (FPos = Format; *FPos != '\0'; FPos++) {
        if (*FPos == '%') {
            FPos++;
            switch (*FPos) {
            case 's':
                PrintStr(va_arg(Args, char*), Stream);
                break;
            case 'd':
                PrintSimpleInt(va_arg(Args, int), Stream);
                break;
            case 'c':
                PrintCh(va_arg(Args, int), Stream);
                break;
            case 't':
                PrintType(va_arg(Args, ValueType*), Stream);
                break;
            case 'f':
                PrintFP(va_arg(Args, double), Stream);
                break;
            case '%':
                PrintCh('%', Stream);
                break;
            case '\0':
                FPos--;
                break;
            default:
                break;
            }
        } else
            PrintCh(*FPos, Stream);
    }
}

/* make a new temporary name. takes a static buffer of char [7] as a parameter.
 * should be initialized to "XX0000"
 * where XX can be any characters */
char *PlatformMakeTempName(Picoc *picoc, char *TempNameBuffer)
{
    int CPos = 5;

    while (CPos > 1) {
        if (TempNameBuffer[CPos] < '9') {
            TempNameBuffer[CPos]++;
            return TableStrRegister(picoc, TempNameBuffer);
        } else {
            TempNameBuffer[CPos] = '0';
            CPos--;
        }
    }

    return TableStrRegister(picoc, TempNameBuffer);
}
