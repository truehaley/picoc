/* picoc include system - can emulate system includes from built-in libraries
 * or it can include and parse files if the system has files */

#include "picoc.h"
#include "interpreter.h"


/* initialize the built-in include libraries */
void IncludeInit(Picoc *picoc)
{
    IncludeRegister(picoc, "ctype.h", NULL, &StdCtypeFunctions[0], NULL);
    IncludeRegister(picoc, "errno.h", &StdErrnoSetupFunc, NULL, NULL);
# ifndef NO_FP
    IncludeRegister(picoc, "math.h", &MathSetupFunc, &MathFunctions[0], NULL);
# endif
    IncludeRegister(picoc, "stdbool.h", &StdboolSetupFunc, NULL, StdboolDefs);
    IncludeRegister(picoc, "stdio.h", &StdioSetupFunc, &StdioFunctions[0], StdioDefs);
    IncludeRegister(picoc, "stdlib.h", &StdlibSetupFunc, &StdlibFunctions[0], NULL);
    IncludeRegister(picoc, "string.h", &StringSetupFunc, &StringFunctions[0], NULL);
    IncludeRegister(picoc, "time.h", &StdTimeSetupFunc, &StdTimeFunctions[0], StdTimeDefs);
# ifndef WIN32
    IncludeRegister(picoc, "unistd.h", &UnistdSetupFunc, &UnistdFunctions[0], UnistdDefs);
# endif
}

/* clean up space used by the include system */
void IncludeCleanup(Picoc *picoc)
{
    IncludeLibrary *ThisInclude = picoc->IncludeLibList;
    IncludeLibrary *NextInclude;

    while (ThisInclude != NULL) {
        NextInclude = ThisInclude->NextLib;
        HeapFreeMem(picoc, ThisInclude);
        ThisInclude = NextInclude;
    }

    picoc->IncludeLibList = NULL;
}

/* register a new build-in include file */
void IncludeRegister(Picoc *picoc, const char *IncludeName,
    void (*SetupFunction)(Picoc *picoc), LibraryFunction *FuncList,
    const char *SetupCSource)
{
    IncludeLibrary *NewLib = HeapAllocMem(picoc, sizeof(IncludeLibrary));
    NewLib->IncludeName = TableStrRegister(picoc, IncludeName);
    NewLib->SetupFunction = SetupFunction;
    NewLib->FuncList = FuncList;
    NewLib->SetupCSource = SetupCSource;
    NewLib->NextLib = picoc->IncludeLibList;
    picoc->IncludeLibList = NewLib;
}

/* include all of the system headers */
void PicocIncludeAllSystemHeaders(Picoc *picoc)
{
    IncludeLibrary *ThisInclude = picoc->IncludeLibList;

    for (; ThisInclude != NULL; ThisInclude = ThisInclude->NextLib)
        IncludeFile(picoc, ThisInclude->IncludeName);
}

/* include one of a number of predefined libraries, or perhaps an actual file */
void IncludeFile(Picoc *picoc, char *FileName)
{
    IncludeLibrary *LInclude;

    /* scan for the include file name to see if it's in our list
        of predefined includes */
    for (LInclude = picoc->IncludeLibList; LInclude != NULL;
            LInclude = LInclude->NextLib) {
        if (strcmp(LInclude->IncludeName, FileName) == 0) {
            /* found it - protect against multiple inclusion */
            if (!VariableDefined(picoc, FileName)) {
                VariableDefine(picoc, NULL, FileName, NULL, &picoc->VoidType, false);

                /* run an extra startup function if there is one */
                if (LInclude->SetupFunction != NULL)
                    (*LInclude->SetupFunction)(picoc);

                /* parse the setup C source code - may define types etc. */
                if (LInclude->SetupCSource != NULL)
                    PicocParse(picoc, FileName, LInclude->SetupCSource,
                        strlen(LInclude->SetupCSource), true, true, false, false);

                /* set up the library functions */
                if (LInclude->FuncList != NULL)
                    LibraryAdd(picoc, LInclude->FuncList);
            }

            return;
        }
    }

    /* not a predefined file, read a real file */
    PicocPlatformScanFile(picoc, FileName);
}
