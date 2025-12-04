#include "picoc.h"
#include "interpreter.h"

#ifdef DEBUGGER
static int gEnableDebugger = true;
#else
static int gEnableDebugger = false;
#endif

/* mark where to end the program for platforms which require this */
jmp_buf PicocExitBuf;

void PlatformInit(Picoc *picoc)
{
}

void PlatformCleanup(Picoc *picoc)
{
}

/* get a line of interactive input */
char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt)
{
    if (Prompt != NULL)
        printf("%s", Prompt);

    fflush(stdout);
    return fgets(Buf, MaxLen, stdin);
}

/* get a character of interactive input */
int PlatformGetCharacter()
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
        for (p = ReadText; (*p != '\r') && (*p != '\n'); ++p) {
            *p = ' ';
        }
    }

    return ReadText;
}

/* read and scan a file for definitions */
void PicocPlatformScanFile(Picoc *picoc, const char *FileName)
{
    char *SourceStr = PlatformReadFile(picoc, FileName);
    PicocParse(picoc, FileName, SourceStr, strlen(SourceStr), true, false, true,
        gEnableDebugger);
}

/* exit the program */
void PlatformExit(Picoc *picoc, int RetVal)
{
    picoc->PicocExitValue = RetVal;
    longjmp(picoc->PicocExitBuf, 1);
}
