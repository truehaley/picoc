/* picoc main header file - this has all the main data structures and
 * function prototypes. If you're just calling picoc you should look at the
 * external interface instead, in picoc.h */
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "platform.h"

#ifndef NULL
#define NULL 0
#endif

/*
#ifndef min
#define min(x,y) (((x)<(y))?(x):(y))
#endif
#ifndef min
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
*/
/* Get the name of a type */
// #define typename(x) _Generic((x),   \
//     _Bool: "_Bool", \
//     unsigned char: "unsigned char", \
//     char: "char", \
//     signed char: "signed char", \
//     short int: "short int", \
//     unsigned short int: "unsigned short int",   \
//     int: "int", \
//     unsigned int: "unsigned int", \
//     long int: "long int", \
//     unsigned long int: "unsigned long int", \
//     long long int: "long long int", \
//     unsigned long long int: "unsigned long long int", \
//     float: "float", \
//     double: "double", \
//     long double: "long double", \
//     char *: "pointer to char", \
//     void *: "pointer to void", \
//     int *: "pointer to int", \
//     default: "other") (x)


#define MEM_ALIGN(x) (((x) + sizeof(ALIGN_TYPE)-1) & ~(sizeof(ALIGN_TYPE)-1))

/* for debugging */
#define PRINT_SOURCE_POS() { \
                                PrintSourceTextErrorLine(Parser->pc->CStdOut, \
                                                         Parser->FileName, \
                                                         Parser->SourceText, \
                                                         Parser->Line, \
                                                         Parser->CharacterPos); \
                                PlatformPrintf(Parser->pc->CStdOut, "\n"); \
                            }

#define PRINT_TYPE(typ) PlatformPrintf(Parser->pc->CStdOut, "%t\n", typ);

typedef FILE IOFILE;

/* coercion of numeric types to other numeric types */
#define IS_FP(v) ((v)->Typ->Base == TypeFP)
#define FP_VAL(v) ((v)->Val->FP)

/* ap -> AllowPointerCoercion = true | false */
#define IS_POINTER_COERCIBLE(v, ap) ((ap) ? ((v)->Typ->Base == TypePointer) : 0)
#define POINTER_COERCE(v) ((int)(v)->Val->Pointer)

#define IS_INTEGER_NUMERIC_TYPE(t) ((t)->Base >= TypeInt && (t)->Base <= TypeUnsignedLong)
#define IS_INTEGER_NUMERIC(v) IS_INTEGER_NUMERIC_TYPE((v)->Typ)
#define IS_NUMERIC_COERCIBLE(v) (IS_INTEGER_NUMERIC(v) || IS_FP(v))
#define IS_NUMERIC_COERCIBLE_PLUS_POINTERS(v,ap) (IS_NUMERIC_COERCIBLE(v) || IS_POINTER_COERCIBLE(v,ap))


typedef struct HashTableStruct HashTable;
struct PicocStruct;

typedef struct PicocStruct Picoc;

/* lexical tokens */
#define TOK_DEF(name, prefix, postfix, infix, repr, string)   name,
typedef enum  {
    #include "token-defs.h"
} LexToken;

/* used in dynamic memory allocation */
typedef struct AllocNodeStruct AllocNode;
struct AllocNodeStruct {
    unsigned int Size;
    AllocNode *NextFree;
};

/* whether we're running or skipping code */
typedef enum {
    RunModeRun,                 /* we're running code as we parse it */
    RunModeSkip,                /* skipping code, not running */
    RunModeReturn,              /* returning from a function */
    RunModeCaseSearch,          /* searching for a case label */
    RunModeBreak,               /* breaking out of a switch/while/do */
    RunModeContinue,            /* as above but repeat the loop */
    RunModeGoto                 /* searching for a goto label */
} RunMode;

/* parser state - has all this detail so we can parse nested files */
typedef struct {
    Picoc *pc;                  /* the picoc instance this parser is a part of */
    const unsigned char *Pos;   /* the character position in the source text */
    char *FileName;             /* what file we're executing (registered string) */
    short int Line;             /* line number we're executing */
    short int CharacterPos;     /* character/column in the line we're executing */
    RunMode Mode;          /* whether to skip or run code */
    int SearchLabel;            /* what case label we're searching for */
    const char *SearchGotoLabel;/* what goto label we're searching for */
    const char *SourceText;     /* the entire source text */
    short int HashIfLevel;      /* how many "if"s we're nested down */
    short int HashIfEvaluateToLevel;    /* if we're not evaluating an if branch,
                                          what the last evaluated level was */
    char DebugMode;             /* debugging mode */
    int ScopeID;   /* for keeping track of local variables (free them after t
                      hey go out of scope) */
} ParseState;

/* values */
typedef enum {
    TypeVoid,                   /* no type */
    TypeInt,                    /* integer */
    TypeShort,                  /* short integer */
    TypeChar,                   /* a single character (signed) */
    TypeLong,                   /* long integer */
    TypeUnsignedInt,            /* unsigned integer */
    TypeUnsignedShort,          /* unsigned short integer */
    TypeUnsignedChar,           /* unsigned 8-bit number */ /* must be before unsigned long */
    TypeUnsignedLong,           /* unsigned long integer */
    TypeFP,                     /* floating point */
    TypeFunction,               /* a function */
    TypeMacro,                  /* a macro */
    TypePointer,                /* a pointer */
    TypeArray,                  /* an array of a sub-type */
    TypeStruct,                 /* aggregate type */
    TypeUnion,                  /* merged type */
    TypeEnum,                   /* enumerated integer type */
    TypeGotoLabel,              /* a label we can "goto" */
    Type_Type                   /* a type for storing types */
} BaseType;

/* data type */
typedef struct ValueTypeStruct ValueType;
struct ValueTypeStruct {
    BaseType Base;             /* what kind of type this is */
    int ArraySize;                  /* the size of an array type */
    int Sizeof;                     /* the storage required */
    int AlignBytes;                 /* the alignment boundary of this type */
    const char *Identifier;         /* the name of a struct or union */
    ValueType *FromType;     /* the type we're derived from (or NULL) */
    ValueType *DerivedTypeList;  /* first in a list of types derived from this one */
    ValueType *Next;         /* next item in the derived type list */
    HashTable *Members;          /* members of a struct or union */
    bool onHeap;                     /* true if allocated on the heap */
    int StaticQualifier;            /* true if it's a static */
};

typedef struct ValueStruct Value;                       /* Forward declaration */

/* Function Pointer type for Intrinsics */
typedef void (*IntrinsicFunc)(ParseState *Parser,
			 Value *ReturnValue,
			 Value **Param,
			 int NumArgs);

/* function definition */
typedef struct {
    ValueType *ReturnType;   /* the return value type */
    int NumParams;                  /* the number of parameters */
    int VarArgs;                    /* has a variable number of arguments after
                                        the explicitly specified ones */
    ValueType **ParamType;   /* array of parameter types */
    char **ParamName;               /* array of parameter names */
    IntrinsicFunc Intrinsic;        /* intrinsic call address or NULL */
    ParseState Body;         /* lexical tokens of the function body if
                                        not intrinsic */
} FuncDef;

/* macro definition */
typedef struct {
    int NumParams;              /* the number of parameters */
    char **ParamName;           /* array of parameter names */
    ParseState Body;     /* lexical tokens of the function body
                                        if not intrinsic */
} MacroDef;

/* values */
typedef union {
    char Character;
    short ShortInteger;
    int Integer;
    long LongInteger;
    unsigned short UnsignedShortInteger;
    unsigned int UnsignedInteger;
    unsigned long UnsignedLongInteger;
    unsigned char UnsignedCharacter;
    char *Identifier;
    char ArrayMem[2];       /* placeholder for where the data starts,
                                doesn't point to it */
    ValueType *Typ;
    FuncDef FuncDef;
    MacroDef MacroDef;
    double FP;
    void *Pointer;      /* unsafe native pointers */
} AnyValue;

// typedef above
struct ValueStruct {
    ValueType *Typ;      /* the type of this value */
    AnyValue *Val;        /* pointer to the AnyValue which holds the actual content */
    Value *LValueFrom;   /* if an LValue, this is a Value our LValue is contained within (or NULL) */
    char ValOnHeap;             /* this Value is on the heap */
    char ValOnStack;            /* the AnyValue is on the stack along with this Value */
    char AnyValOnHeap;          /* the AnyValue is separately allocated from the Value on the heap */
    char IsLValue;              /* is modifiable and is allocated somewhere we can usefully modify it */
    int ScopeID;                /* to know when it goes out of scope */
    char OutOfScope;
};

/* hash table data structure */
typedef struct HashEntryStruct HashEntry;
struct HashEntryStruct {
    HashEntry *Next;        /* next item in this hash chain */
    const char *DeclFileName;       /* where the variable was declared */
    unsigned short DeclLine;
    unsigned short DeclColumn;

    union TableEntryPayload {
        struct ValueEntry {
            char *Key;              /* points to the shared string table */
            Value *Val;      /* the value we're storing */
        } v;                        /* used for tables of values */

        char Key[1];                /* dummy size - used for the shared string table */

        /* defines a breakpoint */
        struct BreakpointEntry {
            const char *FileName;
            short int Line;
            short int CharacterPos;
        } b;

    } p;
};

// typedef above
struct HashTableStruct {
    short Size;
    bool onHeap;
    HashEntry **entries;
};

/* stack frame for function calls */
typedef struct StackFrameStruct StackFrame;
struct StackFrameStruct {
    ParseState ReturnParser;         /* how we got here */
    const char *FuncName;                   /* the name of the function we're in */
    Value *ReturnValue;              /* copy the return value here */
    Value **Parameter;               /* array of parameter values */
    int NumParams;                          /* the number of parameters */
    HashTable LocalTable;                /* the local variables and parameters */
    HashEntry *LocalHashTable[LOCAL_TABLE_SIZE];
    StackFrame *PreviousStackFrame;  /* the next lower stack frame */
};

/* lexer state */
typedef enum {
    LexModeNormal,
    LexModeHashInclude,
    LexModeHashDefine,
    LexModeHashDefineSpace,
    LexModeHashDefineSpaceIdent
} LexMode;

typedef struct {
    const char *Pos;
    const char *End;
    const char *FileName;
    int Line;
    int CharacterPos;
    const char *SourceText;
    LexMode Mode;
    int EmitExtraNewlines;
} LexState;

/* library function definition */
typedef struct {
    void (*Func)(ParseState *Parser, Value *, Value **, int);
    const char *Prototype;
} LibraryFunction;

/* output stream-type specific state information */
typedef union {
    struct StringOutputStream {
        ParseState *Parser;
        char *WritePos;
    } Str;
} OutputStreamInfo;


#if 0 // TODO: unused?
/* stream-specific method for writing characters to the console */
typedef void CharWriter(unsigned char, OutputStreamInfo *);
/* used when writing output to a string - eg. sprintf() */
typedef struct {
    CharWriter *Putch;
    OutputStreamInfo i;
} OutputStream;
#endif

/* possible results of parsing a statement */
typedef enum { ParseResultEOF, ParseResultError, ParseResultOk } ParseResult;

/* a chunk of heap-allocated tokens we'll cleanup when we're done */
typedef struct CleanupTokenNodeStruct CleanupTokenNode;
struct CleanupTokenNodeStruct {
    void *Tokens;
    const char *SourceText;
    CleanupTokenNode *Next;
};

/* linked list of lexical tokens used in interactive mode */
typedef struct TokenLineStruct TokenLine;
struct TokenLineStruct {
    TokenLine *Next;
    unsigned char *Tokens;
    int NumBytes;
};


/* a list of libraries we can include */
typedef struct IncludeLibraryStruct IncludeLibrary;
struct IncludeLibraryStruct {
    char *IncludeName;
    void (*SetupFunction)(Picoc *pc);
    LibraryFunction *FuncList;
    const char *SetupCSource;
    IncludeLibrary *NextLib;
};

#define FREELIST_BUCKETS (8)        /* freelists for 4, 8, 12 ... 32 byte allocs */
#define SPLIT_MEM_THRESHOLD (16)    /* don't split memory which is close in size */
#define BREAKPOINT_TABLE_SIZE (21)


/* the entire state of the picoc system */
// typedef aboove
struct PicocStruct {
    /* parser global data */
    HashTable GlobalTable;
    CleanupTokenNode *CleanupTokenList;
    HashEntry *GlobalHashTable[GLOBAL_TABLE_SIZE];

    /* lexer global data */
    TokenLine *InteractiveHead;
    TokenLine *InteractiveTail;
    TokenLine *InteractiveCurrentLine;
    int LexUseStatementPrompt;
    AnyValue LexAnyValue;
    Value LexValue;
    HashTable ReservedWordTable;
    HashEntry *ReservedWordHashTable[RESERVED_WORD_TABLE_SIZE];

    /* the table of string literal values */
    HashTable StringLiteralTable;
    HashEntry *StringLiteralHashTable[STRING_LITERAL_TABLE_SIZE];

    /* the stack */
    StackFrame *TopStackFrame;

    /* the value passed to exit() */
    int PicocExitValue;

    /* a list of libraries we can include */
    IncludeLibrary *IncludeLibList;

    /* heap memory */
    unsigned char *HeapMemory;  /* stack memory since our heap is malloc()ed */
    void *HeapBottom;           /* the bottom of the (downward-growing) heap */
    void *StackFrame;           /* the current stack frame */
    void *HeapStackTop;         /* the top of the stack */

    AllocNode *FreeListBucket[FREELIST_BUCKETS]; /* we keep a pool of freelist buckets to reduce fragmentation */
    AllocNode *FreeListBig;    /* free memory which doesn't fit in a bucket */

    /* types */
    ValueType UberType;
    ValueType IntType;
    ValueType ShortType;
    ValueType CharType;
    ValueType LongType;
    ValueType UnsignedIntType;
    ValueType UnsignedShortType;
    ValueType UnsignedLongType;
    ValueType UnsignedCharType;
    ValueType FPType;
    ValueType VoidType;
    ValueType TypeType;
    ValueType FunctionType;
    ValueType MacroType;
    ValueType EnumType;
    ValueType GotoLabelType;
    ValueType *CharPtrType;
    ValueType *CharPtrPtrType;
    ValueType *CharArrayType;
    ValueType *VoidPtrType;

    /* debugger */
    HashTable BreakpointTable;
    HashEntry *BreakpointHashTable[BREAKPOINT_TABLE_SIZE];
    int BreakpointCount;
    int DebugManualBreak;

    /* C library */
    int BigEndian;
    int LittleEndian;

    IOFILE *CStdOut;
    IOFILE CStdOutBase;

    /* the picoc version string */
    const char *VersionString;

    /* exit longjump buffer */
#if defined(UNIX_HOST) || defined(WIN32)
    jmp_buf PicocExitBuf;
#endif

    /* string table */
    HashTable StringTable;
    HashEntry *StringHashTable[STRING_TABLE_SIZE];
    char *StrEmpty;
};

/* table.c */
extern void TableInit(Picoc *pc);
extern char *TableStrRegister(Picoc *pc, const char *Str);
extern char *TableStrRegister2(Picoc *pc, const char *Str, int Len);
extern void TableInitTable(HashTable *Tbl, HashEntry **storage,
    int Size, bool onHeap);
extern int TableSet(Picoc *pc, HashTable *Tbl, char *Key, Value *Val,
    const char *DeclFileName, int DeclLine, int DeclColumn);
extern int TableGet(HashTable *Tbl, const char *Key, Value **Val,
    const char **DeclFileName, int *DeclLine, int *DeclColumn);
extern Value *TableDelete(Picoc *pc, HashTable *Tbl, const char *Key);
extern char *TableSetIdentifier(Picoc *pc, HashTable *Tbl, const char *Ident,
    int IdentLen);
extern void TableStrFree(Picoc *pc);

/* lex.c */
extern void LexInit(Picoc *pc);
extern void LexCleanup(Picoc *pc);
extern void *LexAnalyse(Picoc *pc, const char *FileName, const char *Source,
    int SourceLen, int *TokenLen);
extern void LexInitParser(ParseState *Parser, Picoc *pc,
    const char *SourceText, void *TokenSource, char *FileName, int RunIt, int SetDebugMode);
extern LexToken LexGetToken(ParseState *Parser, Value **Value,
    int IncPos);
extern LexToken LexRawPeekToken(ParseState *Parser);
extern void LexToEndOfMacro(ParseState *Parser);
extern void *LexCopyTokens(ParseState *StartParser, ParseState *EndParser);
extern void LexInteractiveClear(Picoc *pc, ParseState *Parser);
extern void LexInteractiveCompleted(Picoc *pc, ParseState *Parser);
extern void LexInteractiveStatementPrompt(Picoc *pc);

/* parse.c */
/* the following are defined in picoc.h:
 * void PicocParse(const char *FileName, const char *Source, int SourceLen, int RunIt, int CleanupNow, int CleanupSource);
 * void PicocParseInteractive(); */
extern void PicocParseInteractiveNoStartPrompt(Picoc *pc, int EnableDebugger);
extern ParseResult ParseStatement(ParseState *Parser,
    int CheckTrailingSemicolon);
extern Value *ParseFunctionDefinition(ParseState *Parser,
    ValueType *ReturnType, char *Identifier);
extern void ParseCleanup(Picoc *pc);
extern void ParserCopyPos(ParseState *To, ParseState *From);
extern void ParserCopy(ParseState *To, ParseState *From);

/* expression.c */
extern int ExpressionParse(ParseState *Parser, Value **Result);
extern long ExpressionParseInt(ParseState *Parser);
extern void ExpressionAssign(ParseState *Parser, Value *DestValue,
    Value *SourceValue, int Force, const char *FuncName, int ParamNo, int AllowPointerCoercion);
extern long ExpressionCoerceInteger(Value *Val);
extern unsigned long ExpressionCoerceUnsignedInteger(Value *Val);
extern double ExpressionCoerceFP(Value *Val);

/* type.c */
extern void TypeInit(Picoc *pc);
extern void TypeCleanup(Picoc *pc);
extern int TypeSize(ValueType *Typ, int ArraySize, int Compact);
extern int TypeSizeValue(Value *Val, int Compact);
extern int TypeStackSizeValue(Value *Val);
extern int TypeLastAccessibleOffset(Picoc *pc, Value *Val);
extern int TypeParseFront(ParseState *Parser, ValueType **Typ,
    int *IsStatic);
extern void TypeParseIdentPart(ParseState *Parser,
    ValueType *BasicTyp, ValueType **Typ, char **Identifier);
extern void TypeParse(ParseState *Parser, ValueType **Typ,
    char **Identifier, int *IsStatic);
extern ValueType *TypeGetMatching(Picoc *pc, ParseState *Parser,
    ValueType *ParentType, BaseType Base, int ArraySize, const char *Identifier, int AllowDuplicates);
extern ValueType *TypeCreateOpaqueStruct(Picoc *pc, ParseState *Parser,
    const char *StructName, int Size);
extern int TypeIsForwardDeclared(ParseState *Parser, ValueType *Typ);

/* heap.c */
#ifdef DEBUG_HEAP
extern void ShowBigList(Picoc *pc);
#endif
extern void HeapInit(Picoc *pc, int StackSize);
extern void HeapCleanup(Picoc *pc);
extern void *HeapAllocStack(Picoc *pc, int Size);
extern int HeapPopStack(Picoc *pc, void *Addr, int Size);
extern void HeapUnpopStack(Picoc *pc, int Size);
extern void HeapPushStackFrame(Picoc *pc);
extern int HeapPopStackFrame(Picoc *pc);
extern void *HeapAllocMem(Picoc *pc, int Size);
extern void HeapFreeMem(Picoc *pc, void *Mem);

/* variable.c */
extern void VariableInit(Picoc *pc);
extern void VariableCleanup(Picoc *pc);
extern void VariableFree(Picoc *pc, Value *Val);
extern void VariableTableCleanup(Picoc *pc, HashTable *entries);
extern void *VariableAlloc(Picoc *pc, ParseState *Parser, int Size, bool onHeap);
extern void VariableStackPop(ParseState *Parser, Value *Var);
extern Value *VariableAllocValueAndData(Picoc *pc, ParseState *Parser,
    int DataSize, int IsLValue, Value *LValueFrom, bool onHeap);
extern Value *VariableAllocValueAndCopy(Picoc *pc, ParseState *Parser,
    Value *FromValue, bool onHeap);
extern Value *VariableAllocValueFromType(Picoc *pc, ParseState *Parser,
    ValueType *Typ, int IsLValue, Value *LValueFrom, bool onHeap);
extern Value *VariableAllocValueFromExistingData(ParseState *Parser,
    ValueType *Typ, AnyValue *FromValue, int IsLValue,
    Value *LValueFrom);
extern Value *VariableAllocValueShared(ParseState *Parser,
    Value *FromValue);
extern Value *VariableDefine(Picoc *pc, ParseState *Parser,
    char *Ident, Value *InitValue, ValueType *Typ, int MakeWritable);
extern Value *VariableDefineButIgnoreIdentical(ParseState *Parser,
    char *Ident, ValueType *Typ, int IsStatic, int *FirstVisit);
extern int VariableDefined(Picoc *pc, const char *Ident);
extern int VariableDefinedAndOutOfScope(Picoc *pc, const char *Ident);
extern void VariableRealloc(ParseState *Parser, Value *FromValue,
    int NewSize);
extern void VariableGet(Picoc *pc, ParseState *Parser, const char *Ident,
    Value **LVal);
extern void VariableDefinePlatformVar(Picoc *pc, ParseState *Parser,
    char *Ident, ValueType *Typ, AnyValue *FromValue, int IsWritable);
extern void VariableStackFrameAdd(ParseState *Parser, const char *FuncName,
    int NumParams);
extern void VariableStackFramePop(ParseState *Parser);
extern Value *VariableStringLiteralGet(Picoc *pc, char *Ident);
extern void VariableStringLiteralDefine(Picoc *pc, char *Ident, Value *Val);
extern void *VariableDereferencePointer(Value *PointerValue,
    Value **DerefVal, int *DerefOffset, ValueType **DerefType,
    int *DerefIsLValue);
extern int VariableScopeBegin(ParseState *Parser, int *PrevScopeID);
extern void VariableScopeEnd(ParseState *Parser, int ScopeID, int PrevScopeID);

/* clibrary.c */
extern void BasicIOInit(Picoc *pc);
extern void LibraryInit(Picoc *pc);
extern void LibraryAdd(Picoc *pc, LibraryFunction *FuncList);
extern void CLibraryInit(Picoc *pc);
extern void PrintCh(char OutCh, IOFILE *Stream);
extern void PrintSimpleInt(long Num, IOFILE *Stream);
extern void PrintInt(long Num, int FieldWidth, int ZeroPad, int LeftJustify,
  IOFILE *Stream);
extern void PrintStr(const char *Str, IOFILE *Stream);
extern void PrintFP(double Num, IOFILE *Stream);
extern void PrintType(ValueType *Typ, IOFILE *Stream);
extern void LibPrintf(ParseState *Parser, Value *ReturnValue,
  Value **Param, int NumArgs);

/* platform.c */
/* the following are defined in picoc.h:
 * void PicocCallMain(int argc, char **argv);
 * int PicocPlatformSetExitPoint();
 * void PicocInitialize(int StackSize);
 * void PicocCleanup();
 * void PicocPlatformScanFile(const char *FileName);
 * extern int PicocExitValue; */
extern void ProgramFail(ParseState *Parser, const char *Message, ...);
extern void ProgramFailNoParser(Picoc *pc, const char *Message, ...);
extern void AssignFail(ParseState *Parser, const char *Format,
    ValueType *Type1, ValueType *Type2, int Num1, int Num2,
    const char *FuncName, int ParamNo);
extern void LexFail(Picoc *pc, LexState *Lexer, const char *Message, ...);
extern void PlatformInit(Picoc *pc);
extern void PlatformCleanup(Picoc *pc);
extern char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt);
extern int PlatformGetCharacter(void);
extern void PlatformPutc(unsigned char OutCh, OutputStreamInfo *);
extern void PlatformPrintf(IOFILE *Stream, const char *Format, ...);
extern void PlatformVPrintf(IOFILE *Stream, const char *Format, va_list Args);
extern void PlatformExit(Picoc *pc, int ExitVal);
extern char *PlatformMakeTempName(Picoc *pc, char *TempNameBuffer);
extern void PlatformLibraryInit(Picoc *pc);

/* include.c */
extern void IncludeInit(Picoc *pc);
extern void IncludeCleanup(Picoc *pc);
extern void IncludeRegister(Picoc *pc, const char *IncludeName,
    void (*SetupFunction)(Picoc *pc), LibraryFunction *FuncList,
    const char *SetupCSource);
extern void IncludeFile(Picoc *pc, char *Filename);
/* the following is defined in picoc.h:
 * void PicocIncludeAllSystemHeaders(); */

#ifdef DEBUGGER
/* debug.c */
extern void DebugInit(Picoc *pc);
extern void DebugCleanup(Picoc *pc);
extern void DebugCheckStatement(ParseState *Parser);
extern void DebugSetBreakpoint(ParseState *Parser);
extern int DebugClearBreakpoint(ParseState *Parser);
extern void DebugStep(void)
#endif

/* stdio.c */
extern const char StdioDefs[];
extern LibraryFunction StdioFunctions[];
extern void StdioSetupFunc(Picoc *pc);

/* math.c */
extern LibraryFunction MathFunctions[];
extern void MathSetupFunc(Picoc *pc);

/* string.c */
extern LibraryFunction StringFunctions[];
extern void StringSetupFunc(Picoc *pc);

/* stdlib.c */
extern LibraryFunction StdlibFunctions[];
extern void StdlibSetupFunc(Picoc *pc);

/* time.c */
extern const char StdTimeDefs[];
extern LibraryFunction StdTimeFunctions[];
extern void StdTimeSetupFunc(Picoc *pc);

/* errno.c */
extern void StdErrnoSetupFunc(Picoc *pc);

/* ctype.c */
extern LibraryFunction StdCtypeFunctions[];

/* stdbool.c */
extern const char StdboolDefs[];
extern void StdboolSetupFunc(Picoc *pc);

/* unistd.c */
extern const char UnistdDefs[];
extern LibraryFunction UnistdFunctions[];
extern void UnistdSetupFunc(Picoc *pc);

#endif /* INTERPRETER_H */
