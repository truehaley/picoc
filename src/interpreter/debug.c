/* picoc interactive debugger */
#include "interpreter.h"

#define BREAKPOINT_HASH(p) (((unsigned long)(p)->FileName) ^ (((p)->Line << 16) | ((p)->CharacterPos << 16)))

#ifdef DEBUGGER
/* initialize the debugger by clearing the breakpoint table */
void DebugInit(Picoc *picoc)
{
    TableInitTable(&picoc->BreakpointTable, &picoc->BreakpointHashTable[0],
        BREAKPOINT_TABLE_SIZE, true);
    picoc->BreakpointCount = 0;
}

/* free the contents of the breakpoint table */
void DebugCleanup(Picoc *picoc)
{
    HashEntry *Entry;
    HashEntry *NextEntry;
    int Count;

    for (Count = 0; Count < picoc->BreakpointTable.Size; Count++) {
        for (Entry = picoc->BreakpointHashTable[Count]; Entry != NULL;
                Entry = NextEntry) {
            NextEntry = Entry->Next;
            HeapFreeMem(picoc, Entry);
        }
    }
}

/* search the table for a breakpoint */
static HashEntry *DebugTableSearchBreakpoint(ParseState *Parser,
    int *AddAt)
{
    HashEntry *Entry;
    Picoc *picoc = Parser->picoc;
    int HashValue = BREAKPOINT_HASH(Parser) % picoc->BreakpointTable.Size;

    for (Entry = picoc->BreakpointHashTable[HashValue];
            Entry != NULL; Entry = Entry->Next) {
        if (Entry->p.b.FileName == Parser->FileName &&
                Entry->p.b.Line == Parser->Line &&
                Entry->p.b.CharacterPos == Parser->CharacterPos)
            return Entry;   /* found */
    }

    *AddAt = HashValue;    /* didn't find it in the chain */
    return NULL;
}

/* set a breakpoint in the table */
void DebugSetBreakpoint(ParseState *Parser)
{
    int AddAt;
    HashEntry *FoundEntry = DebugTableSearchBreakpoint(Parser, &AddAt);
    Picoc *picoc = Parser->picoc;

    if (FoundEntry == NULL) {
        /* add it to the table */
        HashEntry *NewEntry = HeapAllocMem(picoc, sizeof(*NewEntry));
        if (NewEntry == NULL)
            ProgramFailNoParser(picoc, "(DebugSetBreakpoint) out of memory");

        NewEntry->p.b.FileName = Parser->FileName;
        NewEntry->p.b.Line = Parser->Line;
        NewEntry->p.b.CharacterPos = Parser->CharacterPos;
        NewEntry->Next = picoc->BreakpointHashTable[AddAt];
        picoc->BreakpointHashTable[AddAt] = NewEntry;
        picoc->BreakpointCount++;
    }
}

/* delete a breakpoint from the hash table */
int DebugClearBreakpoint(ParseState *Parser)
{
    HashEntry **EntryPtr;
    Picoc *picoc = Parser->picoc;
    int HashValue = BREAKPOINT_HASH(Parser) % picoc->BreakpointTable.Size;

    for (EntryPtr = &picoc->BreakpointHashTable[HashValue];
            *EntryPtr != NULL; EntryPtr = &(*EntryPtr)->Next) {
        HashEntry *DeleteEntry = *EntryPtr;
        if (DeleteEntry->p.b.FileName == Parser->FileName &&
                DeleteEntry->p.b.Line == Parser->Line &&
                DeleteEntry->p.b.CharacterPos == Parser->CharacterPos) {
            *EntryPtr = DeleteEntry->Next;
            HeapFreeMem(picoc, DeleteEntry);
            picoc->BreakpointCount--;

            return true;
        }
    }

    return false;
}

/* before we run a statement, check if there's anything we have to
    do with the debugger here */
void DebugCheckStatement(ParseState *Parser)
{
    int DoBreak = false;
    int AddAt;
    Picoc *picoc = Parser->picoc;

    /* has the user manually pressed break? */
    if (picoc->DebugManualBreak) {
        PlatformPrintf(picoc->CStdOut, "break\n");
        DoBreak = true;
        picoc->DebugManualBreak = false;
    }

    /* is this a breakpoint location? */
    if (Parser->picoc->BreakpointCount != 0 &&
            DebugTableSearchBreakpoint(Parser, &AddAt) != NULL)
        DoBreak = true;

    /* handle a break */
    if (DoBreak) {
        PlatformPrintf(picoc->CStdOut, "Handling a break\n");
        PicocParseInteractiveNoStartPrompt(picoc, false);
    }
}

void DebugStep(void)
{
}
#endif /* DEBUGGER */
