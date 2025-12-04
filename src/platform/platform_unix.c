#include "picoc.h"
#include "interpreter.h"

#ifdef USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#ifdef DEBUGGER
static int gEnableDebugger = true;
#else
static int gEnableDebugger = false;
#endif

/* mark where to end the program for platforms which require this */
jmp_buf PicocExitBuf;

#ifdef DEBUGGER
#include <signal.h>

Picoc *break_pc = NULL;

static void BreakHandler(int Signal)
{
    break_pc->DebugManualBreak = true;
}

void PlatformInit(Picoc *picoc)
{
    /* capture the break signal and pass it to the debugger */
    break_pc = picoc;
    signal(SIGINT, BreakHandler);
}
#else
void PlatformInit(Picoc *picoc) { }
#endif

void PlatformCleanup(Picoc *picoc) { }

/* get a line of interactive input */
char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt)
{
#ifdef USE_READLINE
    if (Prompt != NULL) {
        /* use GNU readline to read the line */
        char *InLine = readline(Prompt);
        if (InLine == NULL)
            return NULL;

        Buf[MaxLen-1] = '\0';
        strncpy(Buf, InLine, MaxLen-2);
        strncat(Buf, "\n", MaxLen-2);

        if (InLine[0] != '\0')
            add_history(InLine);

        free(InLine);
        return Buf;
    }
#endif

    if (Prompt != NULL)
        printf("%s", Prompt);

    fflush(stdout);
    return fgets(Buf, MaxLen, stdin);
}

/* get a character of interactive input */
int PlatformGetCharacter(void)
{
    fflush(stdout);
    return getchar();
}

/* write a character to the console */
void PlatformPutc(unsigned char OutCh, OutputStreamInfo *Stream)
{
    putchar(OutCh);
}

/* read a file into memory */
char *PlatformReadFile(Picoc *picoc, const char *FileName)
{
    struct stat FileInfo;
    char *ReadText;
    FILE *InFile;
    int BytesRead;
    char *p;

    if (stat(FileName, &FileInfo))
        ProgramFailNoParser(picoc, "can't read file %s\n", FileName);

    ReadText = malloc(FileInfo.st_size + 1);
    if (ReadText == NULL)
        ProgramFailNoParser(picoc, "out of memory\n");

    InFile = fopen(FileName, "r");
    if (InFile == NULL)
        ProgramFailNoParser(picoc, "can't read file %s\n", FileName);

    BytesRead = fread(ReadText, 1, FileInfo.st_size, InFile);
    if (BytesRead == 0)
        ProgramFailNoParser(picoc, "can't read file %s\n", FileName);

    ReadText[BytesRead] = '\0';
    fclose(InFile);

    if ((ReadText[0] == '#') && (ReadText[1] == '!')) {
        for (p = ReadText; (*p != '\0') && (*p != '\r') && (*p != '\n'); ++p) {
            *p = ' ';
        }
    }

    return ReadText;
}

/* read and scan a file for definitions */
void PicocPlatformScanFile(Picoc *picoc, const char *FileName)
{
    char *SourceStr = PlatformReadFile(picoc, FileName);

    /* ignore "#!/path/to/picoc" .. by replacing the "#!" with "//" */
    if (SourceStr != NULL && SourceStr[0] == '#' && SourceStr[1] == '!') {
        SourceStr[0] = '/';
        SourceStr[1] = '/';
    }

    PicocParse(picoc, FileName, SourceStr, strlen(SourceStr), true, false, true,
        gEnableDebugger);
}

/* exit the program */
void PlatformExit(Picoc *picoc, int RetVal)
{
    picoc->PicocExitValue = RetVal;
    longjmp(picoc->PicocExitBuf, 1);
}
