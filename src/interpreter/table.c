/* picoc hash table module. This hash table code is used for both symbol tables
 * and the shared string table. */

#include "interpreter.h"


static unsigned int TableHash(const char *Key, int Len);
static HashEntry *TableSearch(HashTable *Tbl, const char *Key,
    int *AddAt);
static HashEntry *TableSearchIdentifier(HashTable *Tbl,
    const char *Key, int Len, int *AddAt);

/* initialize the shared string system */
void TableInit(Picoc *pc)
{
    TableInitTable(&pc->StringTable, &pc->StringHashTable[0],
            STRING_TABLE_SIZE, true);
    pc->StrEmpty = TableStrRegister(pc, "");
}

/* hash function for strings */
unsigned int TableHash(const char *Key, int Len)
{
    unsigned int Hash = Len;
    int Offset;
    int Count;

    for (Count = 0, Offset = 8; Count < Len; Count++, Offset+=7) {
        if (Offset > sizeof(unsigned int) * 8 - 7)
            Offset -= sizeof(unsigned int) * 8 - 6;

        Hash ^= *Key++ << Offset;
    }

    return Hash;
}

/* initialize a table */
void TableInitTable(HashTable *Tbl, HashEntry **storage, int Size,
    bool onHeap)
{
    Tbl->Size = Size;
    Tbl->onHeap = onHeap;
    Tbl->entries = storage;
    memset((void*)storage, '\0', sizeof(HashEntry*) * Size);
}

/* check a hash table entry for a key */
HashEntry *TableSearch(HashTable *Tbl, const char *Key,
    int *AddAt)
{
    /* shared strings have unique addresses so we don't need to hash them */
    int HashValue = ((unsigned long)Key) % Tbl->Size;
    HashEntry *Entry;

    for (Entry = Tbl->entries[HashValue]; Entry != NULL; Entry = Entry->Next) {
        if (Entry->p.v.Key == Key)
            return Entry;   /* found */
    }

    *AddAt = HashValue;    /* didn't find it in the chain */
    return NULL;
}

/* set an identifier to a value. returns FALSE if it already exists.
 * Key must be a shared string from TableStrRegister() */
int TableSet(Picoc *pc, HashTable *Tbl, char *Key, Value *Val,
    const char *DeclFileName, int DeclLine, int DeclColumn)
{
    int AddAt;
    HashEntry *FoundEntry = TableSearch(Tbl, Key, &AddAt);

    if (FoundEntry == NULL) {   /* add it to the table */
        HashEntry *NewEntry = VariableAlloc(pc, NULL,
            sizeof(HashEntry), Tbl->onHeap);
        NewEntry->DeclFileName = DeclFileName;
        NewEntry->DeclLine = DeclLine;
        NewEntry->DeclColumn = DeclColumn;
        NewEntry->p.v.Key = Key;
        NewEntry->p.v.Val = Val;
        NewEntry->Next = Tbl->entries[AddAt];
        Tbl->entries[AddAt] = NewEntry;
        return true;
    }

    return false;
}

/* find a value in a table. returns FALSE if not found.
 * Key must be a shared string from TableStrRegister() */
int TableGet(HashTable *Tbl, const char *Key, Value **Val,
    const char **DeclFileName, int *DeclLine, int *DeclColumn)
{
    int AddAt;
    HashEntry *FoundEntry = TableSearch(Tbl, Key, &AddAt);
    if (FoundEntry == NULL)
        return false;

    *Val = FoundEntry->p.v.Val;

    if (DeclFileName != NULL) {
        *DeclFileName = FoundEntry->DeclFileName;
        *DeclLine = FoundEntry->DeclLine;
        *DeclColumn = FoundEntry->DeclColumn;
    }

    return true;
}

/* remove an entry from the table */
Value *TableDelete(Picoc *pc, HashTable *Tbl, const char *Key)
{
    /* shared strings have unique addresses so we don't need to hash them */
    int HashValue = ((unsigned long)Key) % Tbl->Size;
    HashEntry **EntryPtr;

    for (EntryPtr = &Tbl->entries[HashValue];
            *EntryPtr != NULL; EntryPtr = &(*EntryPtr)->Next) {
        if ((*EntryPtr)->p.v.Key == Key) {
            HashEntry *DeleteEntry = *EntryPtr;
            Value *Val = DeleteEntry->p.v.Val;
            *EntryPtr = DeleteEntry->Next;
            HeapFreeMem(pc, DeleteEntry);

            return Val;
        }
    }

    return NULL;
}

/* check a hash table entry for an identifier */
HashEntry *TableSearchIdentifier(HashTable *Tbl,
    const char *Key, int Len, int *AddAt)
{
    int HashValue = TableHash(Key, Len) % Tbl->Size;
    HashEntry *Entry;

    for (Entry = Tbl->entries[HashValue]; Entry != NULL; Entry = Entry->Next) {
        if (strncmp(&Entry->p.Key[0], (char*)Key, Len) == 0 &&
                Entry->p.Key[Len] == '\0')
            return Entry;   /* found */
    }

    *AddAt = HashValue;    /* didn't find it in the chain */
    return NULL;
}

/* set an identifier and return the identifier. share if possible */
char *TableSetIdentifier(Picoc *pc, HashTable *Tbl, const char *Ident,
    int IdentLen)
{
    int AddAt;
    HashEntry *FoundEntry = TableSearchIdentifier(Tbl, Ident, IdentLen,
        &AddAt);

    if (FoundEntry != NULL)
        return &FoundEntry->p.Key[0];
    else {
        /* add it to the table - we economise by not allocating
            the whole structure here */
        HashEntry *NewEntry = HeapAllocMem(pc,
            sizeof(HashEntry) -
            sizeof(union TableEntryPayload) + IdentLen + 1);
        if (NewEntry == NULL)
            ProgramFailNoParser(pc, "(TableSetIdentifier) out of memory");

        strncpy((char *)&NewEntry->p.Key[0], (char *)Ident, IdentLen);
        NewEntry->p.Key[IdentLen] = '\0';
        NewEntry->Next = Tbl->entries[AddAt];
        Tbl->entries[AddAt] = NewEntry;
        return &NewEntry->p.Key[0];
    }
}

/* register a string in the shared string store */
char *TableStrRegister2(Picoc *pc, const char *Str, int Len)
{
    return TableSetIdentifier(pc, &pc->StringTable, Str, Len);
}

char *TableStrRegister(Picoc *pc, const char *Str)
{
    return TableStrRegister2(pc, Str, strlen((char *)Str));
}

/* free all the strings */
void TableStrFree(Picoc *pc)
{
    int Count;
    HashEntry *Entry;
    HashEntry *NextEntry;

    for (Count = 0; Count < pc->StringTable.Size; Count++) {
        for (Entry = pc->StringTable.entries[Count];
                Entry != NULL; Entry = NextEntry) {
            NextEntry = Entry->Next;
            HeapFreeMem(pc, Entry);
        }
    }
}
