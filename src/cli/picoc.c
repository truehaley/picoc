/* picoc main program - this varies depending on your operating system and
 * how you're using picoc */
/* platform-dependent code for running programs is in this file */
#if defined(UNIX_HOST) || defined(WIN32)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

/* include only picoc.h here - should be able to use it with only the
    external interfaces, no internals from interpreter.h */
#include "picoc.h"


#if defined(UNIX_HOST) || defined(WIN32)
char __LICENSE[] = {
    #include "LICENSE.h"
};
unsigned int __LICENSE_len = sizeof(__LICENSE);

/* Override via STACKSIZE environment variable */
#define PICOC_STACK_SIZE (128000*4)

int main(int argc, char **argv)
{
    int ParamCount = 1;
    int DontRunMain = false;
    int StackSize = getenv("STACKSIZE") ? atoi(getenv("STACKSIZE")) : PICOC_STACK_SIZE;
    Picoc picoc;

    if (argc < 2 || strcmp(argv[ParamCount], "-h") == 0) {
        printf(PICOC_VERSION "  \n"
               "Format:\n\n"
               "> picoc <file1.c>... [- <arg1>...]    : run a program, calls main() as the entry point\n"
               "> picoc -s <file1.c>... [- <arg1>...] : run a script, runs the program without calling main()\n"
               "> picoc -i                            : interactive mode, Ctrl+d to exit\n"
               "> picoc -c                            : copyright info\n"
               "> picoc -h                            : this help message\n");
        return 0;
    }

    if (strcmp(argv[ParamCount], "-c") == 0) {
        printf("%s\n", (char*)&__LICENSE);
        return 0;
    }

    PicocInitialize(&picoc, StackSize);

    if (strcmp(argv[ParamCount], "-s") == 0) {
        DontRunMain = true;
        PicocIncludeAllSystemHeaders(&picoc);
        ParamCount++;
    }

    if (argc > ParamCount && strcmp(argv[ParamCount], "-i") == 0) {
        PicocIncludeAllSystemHeaders(&picoc);
        PicocParseInteractive(&picoc);
    } else {
        if (PicocPlatformSetExitPoint(&picoc)) {
            PicocCleanup(&picoc);
            return picoc.PicocExitValue;
        }

        for (; ParamCount < argc && strcmp(argv[ParamCount], "-") != 0; ParamCount++)
            PicocPlatformScanFile(&picoc, argv[ParamCount]);

        if (!DontRunMain)
            PicocCallMain(&picoc, argc - ParamCount, &argv[ParamCount]);
    }

    PicocCleanup(&picoc);
    return picoc.PicocExitValue;
}
#endif
