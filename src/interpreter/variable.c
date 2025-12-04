/* picoc variable storage. This provides ways of defining and accessing
 * variables */

#include "interpreter.h"

/* maximum size of a value to temporarily copy while we create a variable */
#define MAX_TMP_COPY_BUF (256)


/* initialize the variable system */
void VariableInit(Picoc *picoc)
{
    TableInitTable(&(picoc->GlobalTable), &(picoc->GlobalHashTable)[0],
        GLOBAL_TABLE_SIZE, true);
    TableInitTable(&picoc->StringLiteralTable, &picoc->StringLiteralHashTable[0],
        STRING_LITERAL_TABLE_SIZE, true);
    picoc->TopStackFrame = NULL;
}

/* deallocate the contents of a variable */
void VariableFree(Picoc *picoc, Value *Val)
{
    if (Val->ValOnHeap || Val->AnyValOnHeap) {
        /* free function bodies */
        if (Val->Typ == &picoc->FunctionType &&
                Val->Val->FuncDef.Intrinsic == NULL &&
                Val->Val->FuncDef.Body.Pos != NULL)
            HeapFreeMem(picoc, (void*)Val->Val->FuncDef.Body.Pos);

        /* free macro bodies */
        if (Val->Typ == &picoc->MacroType)
            HeapFreeMem(picoc, (void*)Val->Val->MacroDef.Body.Pos);

        /* free the AnyValue */
        if (Val->AnyValOnHeap)
            HeapFreeMem(picoc, Val->Val);
    }

    /* free the value */
    if (Val->ValOnHeap)
        HeapFreeMem(picoc, Val);
}

/* deallocate the global table and the string literal table */
void VariableTableCleanup(Picoc *picoc, HashTable *table)
{
    int Count;
    HashEntry *Entry;
    HashEntry *NextEntry;

    for (Count = 0; Count < table->Size; Count++) {
        for (Entry = table->entries[Count];
                Entry != NULL;
                Entry = NextEntry) {
            NextEntry = Entry->Next;
            VariableFree(picoc, Entry->p.v.Val);

            /* free the hash table entry */
            HeapFreeMem(picoc, Entry);
        }
    }
}

void VariableCleanup(Picoc *picoc)
{
    VariableTableCleanup(picoc, &picoc->GlobalTable);
    VariableTableCleanup(picoc, &picoc->StringLiteralTable);
}

/* allocate some memory, either on the heap or the stack
    and check if we've run out */
void *VariableAlloc(Picoc *picoc, ParseState *Parser, int Size, bool onHeap)
{
    void *NewValue;

    if (onHeap)
        NewValue = HeapAllocMem(picoc, Size);
    else
        NewValue = HeapAllocStack(picoc, Size);

    if (NewValue == NULL)
        ProgramFail(Parser, "(VariableAlloc) out of memory");

#ifdef DEBUG_HEAP
    if (!onHeap)
        printf("pushing %d at 0x%lx\n", Size, (unsigned long)NewValue);
#endif

    return NewValue;
}

/* allocate a value either on the heap or the stack using space
    dependent on what type we want */
Value *VariableAllocValueAndData(Picoc *picoc, ParseState *Parser,
    int DataSize, bool IsLValue, Value *LValueFrom, bool onHeap)
{
    Value *NewValue = VariableAlloc(picoc, Parser,
        MEM_ALIGN(sizeof(Value)) + DataSize, onHeap);
    NewValue->Val = (AnyValue*)((char*)NewValue +
        MEM_ALIGN(sizeof(Value)));
    NewValue->ValOnHeap = onHeap;
    NewValue->AnyValOnHeap = false;
    NewValue->ValOnStack = !onHeap;
    NewValue->IsLValue = IsLValue;
    NewValue->LValueFrom = LValueFrom;
    if (Parser)
        NewValue->ScopeID = Parser->ScopeID;

    NewValue->OutOfScope = false;

    return NewValue;
}

/* allocate a value given its type */
Value *VariableAllocValueFromType(Picoc *picoc, ParseState *Parser,
    ValueType *Typ, bool IsLValue, Value *LValueFrom, bool onHeap)
{
    int Size = TypeSize(Typ, Typ->ArraySize, false);
    Value *NewValue = VariableAllocValueAndData(picoc, Parser, Size,
        IsLValue, LValueFrom, onHeap);
    assert(Size >= 0 || Typ == &picoc->VoidType);
    NewValue->Typ = Typ;

    return NewValue;
}

/* allocate a value either on the heap or the stack and copy
    its value. handles overlapping data */
Value *VariableAllocValueAndCopy(Picoc *picoc, ParseState *Parser,
    Value *FromValue, bool onHeap)
{
    int CopySize = TypeSizeValue(FromValue, true);
    char TmpBuf[MAX_TMP_COPY_BUF];
    ValueType *DType = FromValue->Typ;
    Value *NewValue;

    assert(CopySize <= MAX_TMP_COPY_BUF);
    memcpy((void*)&TmpBuf[0], (void*)FromValue->Val, CopySize);
    NewValue = VariableAllocValueAndData(picoc, Parser, CopySize,
        FromValue->IsLValue, FromValue->LValueFrom, onHeap);
    NewValue->Typ = DType;
    memcpy((void*)NewValue->Val, (void*)&TmpBuf[0], CopySize);

    return NewValue;
}

/* allocate a value either on the heap or the stack from an
    existing AnyValue and type */
Value *VariableAllocValueFromExistingData(ParseState *Parser,
    ValueType *Typ, AnyValue *FromValue, bool IsLValue,
    Value *LValueFrom)
{
    Value *NewValue = VariableAlloc(Parser->picoc, Parser,
        sizeof(Value), false);
    NewValue->Typ = Typ;
    NewValue->Val = FromValue;
    NewValue->ValOnHeap = false;
    NewValue->AnyValOnHeap = false;
    NewValue->ValOnStack = false;
    NewValue->IsLValue = IsLValue;
    NewValue->LValueFrom = LValueFrom;

    return NewValue;
}

/* allocate a value either on the heap or the stack from an
    existing Value, sharing the value */
Value *VariableAllocValueShared(ParseState *Parser,
    Value *FromValue)
{
    return VariableAllocValueFromExistingData(Parser, FromValue->Typ,
        FromValue->Val, FromValue->IsLValue,
        FromValue->IsLValue ? FromValue : NULL);
}

/* reallocate a variable so its data has a new size */
void VariableRealloc(ParseState *Parser, Value *FromValue,
    int NewSize)
{
    if (FromValue->AnyValOnHeap)
        HeapFreeMem(Parser->picoc, FromValue->Val);

    FromValue->Val = VariableAlloc(Parser->picoc, Parser, NewSize, true);
    FromValue->AnyValOnHeap = true;
}

int VariableScopeBegin(ParseState *Parser, int* OldScopeID)
{
    int Count;
    HashEntry *Entry;
    HashEntry *NextEntry;
#ifdef DEBUG_VAR_SCOPE
    int FirstPrint = 0;
#endif

    if (Parser->ScopeID == -1)
        return -1;

    HashTable *table = (Parser->picoc->TopStackFrame == NULL) ?
        &(Parser->picoc->GlobalTable) : &(Parser->picoc->TopStackFrame)->LocalTable;

    /* XXX dumb hash, let's hope for no collisions... */
    *OldScopeID = Parser->ScopeID;
    Parser->ScopeID = (int)(intptr_t)(Parser->SourceText) *
        ((int)(intptr_t)(Parser->Pos) / sizeof(char*));
    /* or maybe a more human-readable hash for debugging? */
    /* Parser->ScopeID = Parser->Line * 0x10000 + Parser->CharacterPos; */

    for (Count = 0; Count < table->Size; Count++) {
        for (Entry = table->entries[Count];
                Entry != NULL; Entry = NextEntry) {
            NextEntry = Entry->Next;
            if (Entry->p.v.Val->ScopeID == Parser->ScopeID &&
                    Entry->p.v.Val->OutOfScope == true) {
                Entry->p.v.Val->OutOfScope = false;
                Entry->p.v.Key = (char*)((intptr_t)Entry->p.v.Key & ~1);
#ifdef DEBUG_VAR_SCOPE
                if (!FirstPrint) PRINT_SOURCE_POS();
                FirstPrint = 1;
                printf(">>> back into scope: %s %x %d\n", Entry->p.v.Key,
                    Entry->p.v.Val->ScopeID, Entry->p.v.Val->Val->Integer);
#endif
            }
        }
    }

    return Parser->ScopeID;
}

void VariableScopeEnd(ParseState *Parser, int ScopeID, int PrevScopeID)
{
    int Count;
    HashEntry *Entry;
    HashEntry *NextEntry = NULL;
#ifdef DEBUG_VAR_SCOPE
    int FirstPrint = 0;
#endif

    if (ScopeID == -1)
        return;

    HashTable *table = (Parser->picoc->TopStackFrame == NULL) ?
        &(Parser->picoc->GlobalTable) : &(Parser->picoc->TopStackFrame)->LocalTable;

    for (Count = 0; Count < table->Size; Count++) {
        for (Entry = table->entries[Count]; Entry != NULL;
            Entry = NextEntry) {
            NextEntry = Entry->Next;
            if ((Entry->p.v.Val->ScopeID == ScopeID) &&
                    (Entry->p.v.Val->OutOfScope == false)) {
#ifdef DEBUG_VAR_SCOPE
                if (!FirstPrint) PRINT_SOURCE_POS();
                FirstPrint = 1;
                printf(">>> out of scope: %s %x %d\n", Entry->p.v.Key,
                    Entry->p.v.Val->ScopeID, Entry->p.v.Val->Val->Integer);
#endif
                Entry->p.v.Val->OutOfScope = true;
                Entry->p.v.Key = (char*)((intptr_t)Entry->p.v.Key | 1); /* alter the key so it won't be found by normal searches */
            }
        }
    }

    Parser->ScopeID = PrevScopeID;
}

int VariableDefinedAndOutOfScope(Picoc *picoc, const char* Ident)
{
    int Count;
    HashEntry *Entry;

    HashTable * table = (picoc->TopStackFrame == NULL) ?
        &(picoc->GlobalTable) : &(picoc->TopStackFrame)->LocalTable;

    for (Count = 0; Count < table->Size; Count++) {
        for (Entry = table->entries[Count]; Entry != NULL;
            Entry = Entry->Next) {
            if (Entry->p.v.Val->OutOfScope == true &&
                    (char*)((intptr_t)Entry->p.v.Key & ~1) == Ident)
                return true;
        }
    }
    return false;
}

/* define a variable. Ident must be registered */
Value *VariableDefine(Picoc *picoc, ParseState *Parser, char *Ident,
    Value *InitValue, ValueType *Typ, int MakeWritable)
{
    int ScopeID = Parser ? Parser->ScopeID : -1;
    Value * AssignValue;
    HashTable * currentTable = (picoc->TopStackFrame == NULL) ?
        &(picoc->GlobalTable) : &(picoc->TopStackFrame)->LocalTable;

#ifdef DEBUG_VAR_SCOPE
    if (Parser) fprintf(stderr, "def %s %x (%s:%d:%d)\n", Ident, ScopeID,
        Parser->FileName, Parser->Line, Parser->CharacterPos);
#endif

    if (InitValue != NULL)
        AssignValue = VariableAllocValueAndCopy(picoc, Parser, InitValue,
            picoc->TopStackFrame == NULL);
    else
        AssignValue = VariableAllocValueFromType(picoc, Parser, Typ, MakeWritable,
            NULL, picoc->TopStackFrame == NULL);

    AssignValue->IsLValue = MakeWritable;
    AssignValue->ScopeID = ScopeID;
    AssignValue->OutOfScope = false;

    if (!TableSet(picoc, currentTable, Ident, AssignValue, Parser ?
            ((char*)Parser->FileName) : NULL, Parser ? Parser->Line : 0,
            Parser ? Parser->CharacterPos : 0))
        ProgramFail(Parser, "'%s' is already defined", Ident);

    return AssignValue;
}

/* define a variable. Ident must be registered. If it's a redefinition
    from the same declaration don't throw an error */
Value *VariableDefineButIgnoreIdentical(ParseState *Parser,
    char *Ident, ValueType *Typ, int IsStatic, int *FirstVisit)
{
    int DeclLine;
    int DeclColumn;
    const char *DeclFileName;
    Picoc *picoc = Parser->picoc;
    Value *ExistingValue;

    /* is the type a forward declaration? */
    if (TypeIsForwardDeclared(Parser, Typ))
        ProgramFail(Parser, "type '%t' isn't defined", Typ);

    if (IsStatic) {
        char MangledName[LINEBUFFER_MAX];
        char *MNPos = &MangledName[0];
        char *MNEnd = &MangledName[LINEBUFFER_MAX-1];
        const char *RegisteredMangledName;

        /* make the mangled static name (avoiding using sprintf()
            to minimise library impact) */
        memset((void*)&MangledName, '\0', sizeof(MangledName));
        *MNPos++ = '/';
        strncpy(MNPos, (char*)Parser->FileName, MNEnd - MNPos);
        MNPos += strlen(MNPos);

        if (picoc->TopStackFrame != NULL) {
            /* we're inside a function */
            if (MNEnd - MNPos > 0)
                *MNPos++ = '/';
            strncpy(MNPos, (char*)picoc->TopStackFrame->FuncName, MNEnd - MNPos);
            MNPos += strlen(MNPos);
        }

        if (MNEnd - MNPos > 0) *MNPos++ = '/';
        strncpy(MNPos, Ident, MNEnd - MNPos);
        RegisteredMangledName = TableStrRegister(picoc, MangledName);

        /* is this static already defined? */
        if (!TableGet(&picoc->GlobalTable, RegisteredMangledName, &ExistingValue,
                &DeclFileName, &DeclLine, &DeclColumn)) {
            /* define the mangled-named static variable store in the global scope */
            ExistingValue = VariableAllocValueFromType(Parser->picoc, Parser, Typ,
                true, NULL, true);
            TableSet(picoc, &picoc->GlobalTable, (char*)RegisteredMangledName,
                ExistingValue, (char *)Parser->FileName, Parser->Line,
                Parser->CharacterPos);
            *FirstVisit = true;
        }

        /* static variable exists in the global scope - now make a
            mirroring variable in our own scope with the short name */
        VariableDefinePlatformVar(Parser->picoc, Parser, Ident, ExistingValue->Typ,
            ExistingValue->Val, true);
        return ExistingValue;
    } else {
        if (Parser->Line != 0 && TableGet((picoc->TopStackFrame == NULL) ?
                    &picoc->GlobalTable : &picoc->TopStackFrame->LocalTable, Ident,
                    &ExistingValue, &DeclFileName, &DeclLine, &DeclColumn)
                && DeclFileName == Parser->FileName && DeclLine == Parser->Line &&
                DeclColumn == Parser->CharacterPos)
            return ExistingValue;
        else
            return VariableDefine(Parser->picoc, Parser, Ident, NULL, Typ, true);
    }
}

/* check if a variable with a given name is defined. Ident must be registered */
int VariableDefined(Picoc *picoc, const char *Ident)
{
    Value *FoundValue;

    if (picoc->TopStackFrame == NULL || !TableGet(&picoc->TopStackFrame->LocalTable,
            Ident, &FoundValue, NULL, NULL, NULL)) {
        if (!TableGet(&picoc->GlobalTable, Ident, &FoundValue, NULL, NULL, NULL))
            return false;
    }

    return true;
}

/* get the value of a variable. must be defined. Ident must be registered */
void VariableGet(Picoc *picoc, ParseState *Parser, const char *Ident,
    Value **LVal)
{
    if (picoc->TopStackFrame == NULL || !TableGet(&picoc->TopStackFrame->LocalTable,
            Ident, LVal, NULL, NULL, NULL)) {
        if (!TableGet(&picoc->GlobalTable, Ident, LVal, NULL, NULL, NULL)) {
            if (VariableDefinedAndOutOfScope(picoc, Ident))
                ProgramFail(Parser, "'%s' is out of scope", Ident);
            else
                ProgramFail(Parser, "VariableGet Ident: '%s' is undefined", Ident);
        }
    }
}

/* define a global variable shared with a platform global. Ident will be registered */
void VariableDefinePlatformVar(Picoc *picoc, ParseState *Parser, char *Ident,
    ValueType *Typ, AnyValue *FromValue, int IsWritable)
{
    Value *SomeValue = VariableAllocValueAndData(picoc, NULL, 0, IsWritable,
        NULL, true);
    SomeValue->Typ = Typ;
    SomeValue->Val = FromValue;

    if (!TableSet(picoc,
            (picoc->TopStackFrame == NULL) ? &picoc->GlobalTable : &picoc->TopStackFrame->LocalTable,
            TableStrRegister(picoc, Ident), SomeValue,
            Parser ? Parser->FileName : NULL,
            Parser ? Parser->Line : 0, Parser ? Parser->CharacterPos : 0))
        ProgramFail(Parser, "'%s' is already defined", Ident);
}

/* free and/or pop the top value off the stack. Var must be
    the top value on the stack! */
void VariableStackPop(ParseState *Parser, Value *Var)
{
    int Success;

#ifdef DEBUG_HEAP
    if (Var->ValOnStack)
        printf("popping %ld at 0x%lx\n",
            (unsigned long)(sizeof(Value) + TypeSizeValue(Var, false)),
            (unsigned long)Var);
#endif

    if (Var->ValOnHeap) {
        if (Var->Val != NULL)
            HeapFreeMem(Parser->picoc, Var->Val);
        Success = HeapPopStack(Parser->picoc, Var, sizeof(Value));  /* free from heap */
    } else if (Var->ValOnStack)
        Success = HeapPopStack(Parser->picoc, Var,
        sizeof(Value)+TypeSizeValue(Var, false));  /* free from stack */
    else
        Success = HeapPopStack(Parser->picoc, Var, sizeof(Value));  /* value isn't our problem */

    if (!Success)
        ProgramFail(Parser, "stack underrun");
}

/* add a stack frame when doing a function call */
void VariableStackFrameAdd(ParseState *Parser, const char *FuncName,
    int NumParams)
{
    StackFrame *NewFrame;

    HeapPushStackFrame(Parser->picoc);
    NewFrame = HeapAllocStack(Parser->picoc,
        sizeof(StackFrame)+sizeof(Value*)*NumParams);
    if (NewFrame == NULL)
        ProgramFail(Parser, "(VariableStackFrameAdd) out of memory");

    ParserCopy(&NewFrame->ReturnParser, Parser);
    NewFrame->FuncName = FuncName;
    NewFrame->Parameter = (NumParams > 0) ?
        ((void*)((char*)NewFrame+sizeof(StackFrame))) : NULL;
    TableInitTable(&NewFrame->LocalTable, &NewFrame->LocalHashTable[0],
        LOCAL_TABLE_SIZE, false);
    NewFrame->PreviousStackFrame = Parser->picoc->TopStackFrame;
    Parser->picoc->TopStackFrame = NewFrame;
}

/* remove a stack frame */
void VariableStackFramePop(ParseState *Parser)
{
    if (Parser->picoc->TopStackFrame == NULL)
        ProgramFail(Parser, "stack is empty - can't go back");

    ParserCopy(Parser, &Parser->picoc->TopStackFrame->ReturnParser);
    Parser->picoc->TopStackFrame = Parser->picoc->TopStackFrame->PreviousStackFrame;
    HeapPopStackFrame(Parser->picoc);
}

/* get a string literal. assumes that Ident is already
    registered. NULL if not found */
Value *VariableStringLiteralGet(Picoc *picoc, char *Ident)
{
    Value *LVal = NULL;

    if (TableGet(&picoc->StringLiteralTable, Ident, &LVal, NULL, NULL, NULL))
        return LVal;
    else
        return NULL;
}

/* define a string literal. assumes that Ident is already registered */
void VariableStringLiteralDefine(Picoc *picoc, char *Ident, Value *Val)
{
    TableSet(picoc, &picoc->StringLiteralTable, Ident, Val, NULL, 0, 0);
}

/* check a pointer for validity and dereference it for use */
void *VariableDereferencePointer(Value *PointerValue, Value **DerefVal,
    int *DerefOffset, ValueType **DerefType, int *DerefIsLValue)
{
    if (DerefVal != NULL)
        *DerefVal = NULL;

    if (DerefType != NULL)
        *DerefType = PointerValue->Typ->FromType;

    if (DerefOffset != NULL)
        *DerefOffset = 0;

    if (DerefIsLValue != NULL)
        *DerefIsLValue = true;

    return PointerValue->Val->Pointer;
}
