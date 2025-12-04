/* picoc data type module. This manages a tree of data types and has facilities
 * for parsing data types. */

#include "interpreter.h"


static ValueType *TypeAdd(Picoc *picoc, ParseState *Parser,
    ValueType *ParentType, BaseType Base, int ArraySize,
    const char *Identifier, int Sizeof, int AlignBytes);
static void TypeAddBaseType(Picoc *picoc, ValueType *TypeNode,
    BaseType Base, int Sizeof, int AlignBytes);
static void TypeCleanupNode(Picoc *picoc, ValueType *Typ);
static void TypeParseStruct(ParseState *Parser, ValueType **Typ,
    int IsStruct);
static void TypeParseEnum(ParseState *Parser, ValueType **Typ);
static ValueType *TypeParseBack(ParseState *Parser,
    ValueType *FromType);



/* some basic types */
static int PointerAlignBytes;
static int IntAlignBytes;


/* add a new type to the set of types we know about */
ValueType *TypeAdd(Picoc *picoc, ParseState *Parser,
    ValueType *ParentType, BaseType Base, int ArraySize,
    const char *Identifier, int Sizeof, int AlignBytes)
{
    ValueType *NewType = VariableAlloc(picoc, Parser,
        sizeof(ValueType), true);
    NewType->Base = Base;
    NewType->ArraySize = ArraySize;
    NewType->Sizeof = Sizeof;
    NewType->AlignBytes = AlignBytes;
    NewType->Identifier = Identifier;
    NewType->Members = NULL;
    NewType->FromType = ParentType;
    NewType->DerivedTypeList = NULL;
    NewType->onHeap = true;
    NewType->Next = ParentType->DerivedTypeList;
    ParentType->DerivedTypeList = NewType;

    return NewType;
}

/* given a parent type, get a matching derived type and make one if necessary.
 * Identifier should be registered with the shared string table. */
ValueType *TypeGetMatching(Picoc *picoc, ParseState *Parser,
    ValueType *ParentType, BaseType Base, int ArraySize,
    const char *Identifier, int AllowDuplicates)
{
    int Sizeof;
    int AlignBytes;
    ValueType *ThisType = ParentType->DerivedTypeList;
    while (ThisType != NULL && (ThisType->Base != Base ||
            ThisType->ArraySize != ArraySize || ThisType->Identifier != Identifier))
        ThisType = ThisType->Next;

    if (ThisType != NULL) {
        if (AllowDuplicates)
            return ThisType;
        else
            ProgramFail(Parser, "data type '%s' is already defined", Identifier);
    }

    switch (Base) {
    case TypePointer:
        Sizeof = sizeof(void*);
        AlignBytes = PointerAlignBytes;
        break;
    case TypeArray:
        Sizeof = ArraySize * ParentType->Sizeof;
        AlignBytes = ParentType->AlignBytes;
        break;
    case TypeEnum:
        Sizeof = sizeof(int);
        AlignBytes = IntAlignBytes;
        break;
    default:
        Sizeof = 0; AlignBytes = 0;
        break;  /* structs and unions will get bigger
                    when we add members to them */
    }

    return TypeAdd(picoc, Parser, ParentType, Base, ArraySize, Identifier, Sizeof,
        AlignBytes);
}

/* stack space used by a value */
int TypeStackSizeValue(Value *Val)
{
    if (Val != NULL && Val->ValOnStack)
        return TypeSizeValue(Val, false);
    else
        return 0;
}

/* memory used by a value */
int TypeSizeValue(Value *Val, int Compact)
{
    if (IS_INTEGER_NUMERIC(Val) && !Compact)
        return sizeof(ALIGN_TYPE);  /* allow some extra room for type extension */
    else if (Val->Typ->Base != TypeArray)
        return Val->Typ->Sizeof;
    else
        return Val->Typ->FromType->Sizeof * Val->Typ->ArraySize;
}

/* memory used by a variable given its type and array size */
int TypeSize(ValueType *Typ, int ArraySize, int Compact)
{
    if (IS_INTEGER_NUMERIC_TYPE(Typ) && !Compact)
        return sizeof(ALIGN_TYPE);  /* allow some extra room for type extension */
    else if (Typ->Base != TypeArray)
        return Typ->Sizeof;
    else
        return Typ->FromType->Sizeof * ArraySize;
}

/* add a base type */
void TypeAddBaseType(Picoc *picoc, ValueType *TypeNode, BaseType Base,
            int Sizeof, int AlignBytes)
{
    TypeNode->Base = Base;
    TypeNode->ArraySize = 0;
    TypeNode->Sizeof = Sizeof;
    TypeNode->AlignBytes = AlignBytes;
    TypeNode->Identifier = picoc->StrEmpty;
    TypeNode->Members = NULL;
    TypeNode->FromType = NULL;
    TypeNode->DerivedTypeList = NULL;
    TypeNode->onHeap = false;
    TypeNode->Next = picoc->UberType.DerivedTypeList;
    picoc->UberType.DerivedTypeList = TypeNode;
}

/* initialize the type system */
void TypeInit(Picoc *picoc)
{
    struct IntAlign {char x; int y;} ia;
    struct ShortAlign {char x; short y;} sa;
    struct CharAlign {char x; char y;} ca;
    struct LongAlign {char x; long y;} la;
    struct DoubleAlign {char x; double y;} da;
    struct PointerAlign {char x; void *y;} pa;

    IntAlignBytes = (char*)&ia.y - &ia.x;
    PointerAlignBytes = (char*)&pa.y - &pa.x;

    picoc->UberType.DerivedTypeList = NULL;
    TypeAddBaseType(picoc, &picoc->IntType, TypeInt, sizeof(int), IntAlignBytes);
    TypeAddBaseType(picoc, &picoc->ShortType, TypeShort, sizeof(short),
        (char*)&sa.y - &sa.x);
    TypeAddBaseType(picoc, &picoc->CharType, TypeChar, sizeof(char),
        (char*)&ca.y - &ca.x);
    TypeAddBaseType(picoc, &picoc->LongType, TypeLong, sizeof(long),
        (char*)&la.y - &la.x);
    TypeAddBaseType(picoc, &picoc->UnsignedIntType, TypeUnsignedInt,
        sizeof(unsigned int), IntAlignBytes);
    TypeAddBaseType(picoc, &picoc->UnsignedShortType, TypeUnsignedShort,
        sizeof(unsigned short), (char*)&sa.y - &sa.x);
    TypeAddBaseType(picoc, &picoc->UnsignedLongType, TypeUnsignedLong,
        sizeof(unsigned long), (char*)&la.y - &la.x);
    TypeAddBaseType(picoc, &picoc->UnsignedCharType, TypeUnsignedChar,
        sizeof(unsigned char), (char*)&ca.y - &ca.x);
    TypeAddBaseType(picoc, &picoc->VoidType, TypeVoid, 0, 1);
    TypeAddBaseType(picoc, &picoc->FunctionType, TypeFunction, sizeof(int),
        IntAlignBytes);
    TypeAddBaseType(picoc, &picoc->MacroType, TypeMacro, sizeof(int), IntAlignBytes);
    TypeAddBaseType(picoc, &picoc->GotoLabelType, TypeGotoLabel, 0, 1);
    TypeAddBaseType(picoc, &picoc->FPType, TypeFP, sizeof(double),
        (char*)&da.y - &da.x);
    TypeAddBaseType(picoc, &picoc->TypeType, Type_Type, sizeof(double),
    (char*)&da.y - &da.x);  /* must be large enough to cast to a double */
    picoc->CharArrayType = TypeAdd(picoc, NULL, &picoc->CharType, TypeArray, 0,
        picoc->StrEmpty, sizeof(char), (char*)&ca.y - &ca.x);
    picoc->CharPtrType = TypeAdd(picoc, NULL, &picoc->CharType, TypePointer, 0,
        picoc->StrEmpty, sizeof(void*), PointerAlignBytes);
    picoc->CharPtrPtrType = TypeAdd(picoc, NULL, picoc->CharPtrType, TypePointer, 0,
        picoc->StrEmpty, sizeof(void*), PointerAlignBytes);
    picoc->VoidPtrType = TypeAdd(picoc, NULL, &picoc->VoidType, TypePointer, 0,
        picoc->StrEmpty, sizeof(void*), PointerAlignBytes);
}

/* deallocate heap-allocated types */
void TypeCleanupNode(Picoc *picoc, ValueType *Typ)
{
    ValueType *SubType;
    ValueType *NextSubType;

    /* clean up and free all the sub-nodes */
    for (SubType = Typ->DerivedTypeList; SubType != NULL;
            SubType = NextSubType) {
        NextSubType = SubType->Next;
        TypeCleanupNode(picoc, SubType);
        if (SubType->onHeap) {
            /* if it's a struct or union deallocate all the member values */
            if (SubType->Members != NULL) {
                VariableTableCleanup(picoc, SubType->Members);
                HeapFreeMem(picoc, SubType->Members);
            }

            /* free this node */
            HeapFreeMem(picoc, SubType);
        }
    }
}

void TypeCleanup(Picoc *picoc)
{
    TypeCleanupNode(picoc, &picoc->UberType);
}

/* parse a struct or union declaration */
void TypeParseStruct(ParseState *Parser, ValueType **Typ,
    int IsStruct)
{
    char *MemberIdentifier;
    char *StructIdentifier;
    LexToken Token;
    int AlignBoundary;
    Value *MemberValue;
    Picoc *picoc = Parser->picoc;
    Value *LexValue;
    ValueType *MemberType;

    Token = LexGetToken(Parser, &LexValue, false);
    if (Token == TokenIdentifier) {
        LexGetToken(Parser, &LexValue, true);
        StructIdentifier = LexValue->Val->Identifier;
        Token = LexGetToken(Parser, NULL, false);
    } else {
        static char TempNameBuf[7] = "^s0000";
        StructIdentifier = PlatformMakeTempName(picoc, TempNameBuf);
    }

    *Typ = TypeGetMatching(picoc, Parser, &Parser->picoc->UberType,
        IsStruct ? TypeStruct : TypeUnion, 0, StructIdentifier, true);
    if (Token == TokenLeftBrace && (*Typ)->Members != NULL)
        ProgramFail(Parser, "data type '%t' is already defined", *Typ);

    Token = LexGetToken(Parser, NULL, false);
    if (Token != TokenLeftBrace) {
        /* use the already defined structure */
#if 0
        if ((*Typ)->Members == NULL)
            ProgramFail(Parser, "structure '%s' isn't defined",
                LexValue->Val->Identifier);
#endif
        return;
    }

    if (picoc->TopStackFrame != NULL)
        ProgramFail(Parser, "struct/union definitions can only be globals");

    LexGetToken(Parser, NULL, true);
    (*Typ)->Members = VariableAlloc(picoc, Parser,
        sizeof(HashTable)+STRUCT_TABLE_SIZE*sizeof(HashEntry), true);
    (*Typ)->Members->entries =
        (HashEntry**)((char*)(*Typ)->Members + sizeof(HashTable));
    TableInitTable((*Typ)->Members,
        (HashEntry**)((char*)(*Typ)->Members + sizeof(HashTable)),
        STRUCT_TABLE_SIZE, true);

    do {
        TypeParse(Parser, &MemberType, &MemberIdentifier, NULL);
        if (MemberType == NULL || MemberIdentifier == NULL)
            ProgramFail(Parser, "invalid type in struct");

        MemberValue = VariableAllocValueAndData(picoc, Parser, sizeof(int), false,
            NULL, true);
        MemberValue->Typ = MemberType;
        if (IsStruct) {
            /* allocate this member's location in the struct */
            AlignBoundary = MemberValue->Typ->AlignBytes;
            if (((*Typ)->Sizeof & (AlignBoundary-1)) != 0)
                (*Typ)->Sizeof +=
                    AlignBoundary - ((*Typ)->Sizeof & (AlignBoundary-1));

            MemberValue->Val->Integer = (*Typ)->Sizeof;
            (*Typ)->Sizeof += TypeSizeValue(MemberValue, true);
        } else {
            /* union members always start at 0, make sure it's big enough
                to hold the largest member */
            MemberValue->Val->Integer = 0;
            if (MemberValue->Typ->Sizeof > (*Typ)->Sizeof)
                (*Typ)->Sizeof = TypeSizeValue(MemberValue, true);
        }

        /* make sure to align to the size of the largest member's alignment */
        if ((*Typ)->AlignBytes < MemberValue->Typ->AlignBytes)
            (*Typ)->AlignBytes = MemberValue->Typ->AlignBytes;

        /* define it */
        if (!TableSet(picoc, (*Typ)->Members, MemberIdentifier, MemberValue,
                Parser->FileName, Parser->Line, Parser->CharacterPos))
            ProgramFail(Parser, "member '%s' already defined", &MemberIdentifier);

        if (LexGetToken(Parser, NULL, true) != TokenSemicolon)
            ProgramFail(Parser, "semicolon expected");

    } while (LexGetToken(Parser, NULL, false) != TokenRightBrace);

    /* now align the structure to the size of its largest member's alignment */
    AlignBoundary = (*Typ)->AlignBytes;
    if (((*Typ)->Sizeof & (AlignBoundary-1)) != 0)
        (*Typ)->Sizeof += AlignBoundary - ((*Typ)->Sizeof & (AlignBoundary-1));

    LexGetToken(Parser, NULL, true);
}

/* create a system struct which has no user-visible members */
ValueType *TypeCreateOpaqueStruct(Picoc *picoc, ParseState *Parser,
    const char *StructName, int Size)
{
    ValueType *Typ = TypeGetMatching(picoc, Parser, &picoc->UberType,
        TypeStruct, 0, StructName, false);

    /* create the (empty) table */
    Typ->Members = VariableAlloc(picoc,
        Parser,
        sizeof(HashTable)+STRUCT_TABLE_SIZE*sizeof(HashEntry), true);
    Typ->Members->entries = (HashEntry**)((char*)Typ->Members +
        sizeof(HashTable));
    TableInitTable(Typ->Members,
        (HashEntry**)((char*)Typ->Members+sizeof(HashTable)),
        STRUCT_TABLE_SIZE, true);
    Typ->Sizeof = Size;

    return Typ;
}

/* parse an enum declaration */
void TypeParseEnum(ParseState *Parser, ValueType **Typ)
{
    int EnumValue = 0;
    char *EnumIdentifier;
    LexToken Token;
    Value *LexValue;
    Value InitValue;
    Picoc *picoc = Parser->picoc;

    Token = LexGetToken(Parser, &LexValue, false);
    if (Token == TokenIdentifier) {
        LexGetToken(Parser, &LexValue, true);
        EnumIdentifier = LexValue->Val->Identifier;
        Token = LexGetToken(Parser, NULL, false);
    } else {
        static char TempNameBuf[7] = "^e0000";
        EnumIdentifier = PlatformMakeTempName(picoc, TempNameBuf);
    }

    TypeGetMatching(picoc, Parser, &picoc->UberType, TypeEnum, 0, EnumIdentifier,
        Token != TokenLeftBrace);
    *Typ = &picoc->IntType;
    if (Token != TokenLeftBrace) {
        /* use the already defined enum */
        if ((*Typ)->Members == NULL)
            ProgramFail(Parser, "enum '%s' isn't defined", EnumIdentifier);

        return;
    }

    if (picoc->TopStackFrame != NULL)
        ProgramFail(Parser, "enum definitions can only be globals");

    LexGetToken(Parser, NULL, true);
    (*Typ)->Members = &picoc->GlobalTable;
    memset((void*)&InitValue, '\0', sizeof(Value));
    InitValue.Typ = &picoc->IntType;
    InitValue.Val = (AnyValue*)&EnumValue;
    do {
        if (LexGetToken(Parser, &LexValue, true) != TokenIdentifier)
            ProgramFail(Parser, "identifier expected");

        EnumIdentifier = LexValue->Val->Identifier;
        if (LexGetToken(Parser, NULL, false) == TokenAssign) {
            LexGetToken(Parser, NULL, true);
            EnumValue = ExpressionParseInt(Parser);
        }

        VariableDefine(picoc, Parser, EnumIdentifier, &InitValue, NULL, false);

        Token = LexGetToken(Parser, NULL, true);
        if (Token != TokenComma && Token != TokenRightBrace)
            ProgramFail(Parser, "comma expected");

        EnumValue++;
    } while (Token == TokenComma);
}

/* parse a type - just the basic type */
int TypeParseFront(ParseState *Parser, ValueType **Typ,
    int *IsStatic)
{
    int Unsigned = false;
    int StaticQualifier = false;
    LexToken Token;
    ParseState Before;
    Value *LexerValue;
    Value *VarValue;
    Picoc *picoc = Parser->picoc;
    *Typ = NULL;

    /* ignore leading type qualifiers */
    ParserCopy(&Before, Parser);
    Token = LexGetToken(Parser, &LexerValue, true);
    while (Token == TokenStaticType || Token == TokenAutoType ||
            Token == TokenRegisterType || Token == TokenExternType) {
        if (Token == TokenStaticType)
            StaticQualifier = true;

        Token = LexGetToken(Parser, &LexerValue, true);
    }

    if (IsStatic != NULL)
        *IsStatic = StaticQualifier;

    /* handle signed/unsigned with no trailing type */
    if (Token == TokenSignedType || Token == TokenUnsignedType) {
        LexToken FollowToken = LexGetToken(Parser, &LexerValue, false);
        Unsigned = (Token == TokenUnsignedType);

        if (FollowToken != TokenIntType && FollowToken != TokenLongType &&
                FollowToken != TokenShortType && FollowToken != TokenCharType) {
            if (Token == TokenUnsignedType)
                *Typ = &picoc->UnsignedIntType;
            else
                *Typ = &picoc->IntType;

            return true;
        }

        Token = LexGetToken(Parser, &LexerValue, true);
    }

    switch (Token) {
    case TokenIntType:
        *Typ = Unsigned ? &picoc->UnsignedIntType : &picoc->IntType;
        break;
    case TokenShortType:
        *Typ = Unsigned ? &picoc->UnsignedShortType : &picoc->ShortType;
        break;
    case TokenCharType:
        *Typ = Unsigned ? &picoc->UnsignedCharType : &picoc->CharType;
        break;
    case TokenLongType:
        *Typ = Unsigned ? &picoc->UnsignedLongType : &picoc->LongType;
        break;
    case TokenFloatType:
    case TokenDoubleType:
        *Typ = &picoc->FPType;
        break;
    case TokenVoidType:
        *Typ = &picoc->VoidType;
        break;
    case TokenStructType: case TokenUnionType:
        if (*Typ != NULL)
            ProgramFail(Parser, "bad type declaration");
        TypeParseStruct(Parser, Typ, Token == TokenStructType);
        break;
    case TokenEnumType:
        if (*Typ != NULL)
            ProgramFail(Parser, "bad type declaration");

        TypeParseEnum(Parser, Typ);
        break;
    case TokenIdentifier:
        /* we already know it's a typedef-defined type because we got here */
        VariableGet(picoc, Parser, LexerValue->Val->Identifier, &VarValue);
        *Typ = VarValue->Val->Typ;
        break;

    default:
        ParserCopy(Parser, &Before);
        return false;
    }

    return true;
}

/* parse a type - the part at the end after the identifier. eg.
    array specifications etc. */
ValueType *TypeParseBack(ParseState *Parser,
    ValueType *FromType)
{
    LexToken Token;
    ParseState Before;

    ParserCopy(&Before, Parser);
    Token = LexGetToken(Parser, NULL, true);
    if (Token == TokenLeftSquareBracket) {
        /* add another array bound */
        if (LexGetToken(Parser, NULL, false) == TokenRightSquareBracket) {
            /* an unsized array */
            LexGetToken(Parser, NULL, true);
            return TypeGetMatching(Parser->picoc, Parser,
                TypeParseBack(Parser, FromType), TypeArray, 0,
                    Parser->picoc->StrEmpty, true);
        } else {
            /* get a numeric array size */
            RunMode OldMode = Parser->Mode;
            int ArraySize;
            Parser->Mode = RunModeRun;
            ArraySize = ExpressionParseInt(Parser);
            Parser->Mode = OldMode;

            if (LexGetToken(Parser, NULL, true) != TokenRightSquareBracket)
                ProgramFail(Parser, "']' expected");

            return TypeGetMatching(Parser->picoc, Parser,
                TypeParseBack(Parser, FromType), TypeArray, ArraySize,
                    Parser->picoc->StrEmpty, true);
        }
    } else {
        /* the type specification has finished */
        ParserCopy(Parser, &Before);
        return FromType;
    }
}

/* parse a type - the part which is repeated with each
    identifier in a declaration list */
void TypeParseIdentPart(ParseState *Parser, ValueType *BasicTyp,
    ValueType **Typ, char **Identifier)
{
    int Done = false;
    LexToken Token;
    Value *LexValue;
    ParseState Before;
    *Typ = BasicTyp;
    *Identifier = Parser->picoc->StrEmpty;

    while (!Done) {
        ParserCopy(&Before, Parser);
        Token = LexGetToken(Parser, &LexValue, true);
        switch (Token) {
        case TokenOpenBracket:
            if (*Typ != NULL)
                ProgramFail(Parser, "bad type declaration");

            TypeParse(Parser, Typ, Identifier, NULL);
            if (LexGetToken(Parser, NULL, true) != TokenCloseBracket)
                ProgramFail(Parser, "')' expected");
            break;

        case TokenAsterisk:
            if (*Typ == NULL)
                ProgramFail(Parser, "bad type declaration");

            *Typ = TypeGetMatching(Parser->picoc, Parser, *Typ, TypePointer, 0,
                Parser->picoc->StrEmpty, true);
            break;

        case TokenIdentifier:
            if (*Typ == NULL || *Identifier != Parser->picoc->StrEmpty)
                ProgramFail(Parser, "bad type declaration");

            *Identifier = LexValue->Val->Identifier;
            Done = true;
            break;

        default: ParserCopy(Parser, &Before); Done = true; break;
        }
    }

    if (*Typ == NULL)
        ProgramFail(Parser, "bad type declaration");

    if (*Identifier != Parser->picoc->StrEmpty) {
        /* parse stuff after the identifier */
        *Typ = TypeParseBack(Parser, *Typ);
    }
}

/* parse a type - a complete declaration including identifier */
void TypeParse(ParseState *Parser, ValueType **Typ,
    char **Identifier, int *IsStatic)
{
    ValueType *BasicType;

    TypeParseFront(Parser, &BasicType, IsStatic);
    TypeParseIdentPart(Parser, BasicType, Typ, Identifier);
}

/* check if a type has been fully defined - otherwise it's
    just a forward declaration */
int TypeIsForwardDeclared(ParseState *Parser, ValueType *Typ)
{
    if (Typ->Base == TypeArray)
        return TypeIsForwardDeclared(Parser, Typ->FromType);

    if ((Typ->Base == TypeStruct || Typ->Base == TypeUnion) &&
            Typ->Members == NULL)
        return true;

    return false;
}
