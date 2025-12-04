/* picoc interactive debugger */
#include "interpreter.h"

#define BREAKPOINT_HASH(p) (((unsigned long)(p)->FileName) ^ (((p)->Line << 16) | ((p)->CharacterPos << 16)))

#ifdef DEBUGGER
/* initialize the debugger by clearing the breakpoint table */
void DebugInit(Picoc *pc)
{
    TableInitTable(&pc->BreakpointTable, &pc->BreakpointHashTable[0],
        BREAKPOINT_TABLE_SIZE, true);
    pc->BreakpointCount = 0;
}

/* free the contents of the breakpoint table */
void DebugCleanup(Picoc *pc)
{
    HashEntry *Entry;
    HashEntry *NextEntry;
    int Count;

    for (Count = 0; Count < pc->BreakpointTable.Size; Count++) {
        for (Entry = pc->BreakpointHashTable[Count]; Entry != NULL;
                Entry = NextEntry) {
            NextEntry = Entry->Next;
            HeapFreeMem(pc, Entry);
        }
    }
}

/* search the table for a breakpoint */
static HashEntry *DebugTableSearchBreakpoint(ParseState *Parser,
    int *AddAt)
{
    HashEntry *Entry;
    Picoc *pc = Parser->pc;
    int HashValue = BREAKPOINT_HASH(Parser) % pc->BreakpointTable.Size;

    for (Entry = pc->BreakpointHashTable[HashValue];
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
    Picoc *pc = Parser->pc;

    if (FoundEntry == NULL) {
        /* add it to the table */
        HashEntry *NewEntry = HeapAllocMem(pc, sizeof(*NewEntry));
        if (NewEntry == NULL)
            ProgramFailNoParser(pc, "(DebugSetBreakpoint) out of memory");

        NewEntry->p.b.FileName = Parser->FileName;
        NewEntry->p.b.Line = Parser->Line;
        NewEntry->p.b.CharacterPos = Parser->CharacterPos;
        NewEntry->Next = pc->BreakpointHashTable[AddAt];
        pc->BreakpointHashTable[AddAt] = NewEntry;
        pc->BreakpointCount++;
    }
}

/* delete a breakpoint from the hash table */
int DebugClearBreakpoint(ParseState *Parser)
{
    HashEntry **EntryPtr;
    Picoc *pc = Parser->pc;
    int HashValue = BREAKPOINT_HASH(Parser) % pc->BreakpointTable.Size;

    for (EntryPtr = &pc->BreakpointHashTable[HashValue];
            *EntryPtr != NULL; EntryPtr = &(*EntryPtr)->Next) {
        HashEntry *DeleteEntry = *EntryPtr;
        if (DeleteEntry->p.b.FileName == Parser->FileName &&
                DeleteEntry->p.b.Line == Parser->Line &&
                DeleteEntry->p.b.CharacterPos == Parser->CharacterPos) {
            *EntryPtr = DeleteEntry->Next;
            HeapFreeMem(pc, DeleteEntry);
            pc->BreakpointCount--;

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
    Picoc *pc = Parser->pc;

    /* has the user manually pressed break? */
    if (pc->DebugManualBreak) {
        PlatformPrintf(pc->CStdOut, "break\n");
        DoBreak = true;
        pc->DebugManualBreak = false;
    }

    /* is this a breakpoint location? */
    if (Parser->pc->BreakpointCount != 0 &&
            DebugTableSearchBreakpoint(Parser, &AddAt) != NULL)
        DoBreak = true;

    /* handle a break */
    if (DoBreak) {
        PlatformPrintf(pc->CStdOut, "Handling a break\n");
        PicocParseInteractiveNoStartPrompt(pc, false);
    }
}

void DebugStep(void)
{
}
#endif /* DEBUGGER */
