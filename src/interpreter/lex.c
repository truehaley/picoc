/* picoc lexer - converts source text into a tokenised form */

#include "interpreter.h"


#define isCidstart(c) (isalpha(c) || (c)=='_' || (c)=='#')
#define isCident(c) (isalnum(c) || (c)=='_')

#define IS_HEX_ALPHA_DIGIT(c) (((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#define IS_BASE_DIGIT(c,b) (((c) >= '0' && (c) < '0' + (((b)<10)?(b):10)) || (((b) > 10) ? IS_HEX_ALPHA_DIGIT(c) : false))
#define GET_BASE_DIGIT(c) (((c) <= '9') ? ((c) - '0') : (((c) <= 'F') ? ((c) - 'A' + 10) : ((c) - 'a' + 10)))

#define NEXTIS(c,x,y) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else GotToken = (y); }
#define NEXTIS3(c,x,d,y,z) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else NEXTIS(d,y,z) }
#define NEXTIS4(c,x,d,y,e,z,a) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else NEXTIS3(d,y,e,z,a) }
#define NEXTIS3PLUS(c,x,d,y,e,z,a) { if (NextChar == (c)) { LEXER_INC(Lexer); GotToken = (x); } else if (NextChar == (d)) { if (Lexer->Pos[1] == (e)) { LEXER_INCN(Lexer, 2); GotToken = (z); } else { LEXER_INC(Lexer); GotToken = (y); } } else GotToken = (a); }
#define NEXTISEXACTLY3(c,d,y,z) { if (NextChar == (c) && Lexer->Pos[1] == (d)) { LEXER_INCN(Lexer, 2); GotToken = (y); } else GotToken = (z); }

#define LEXER_INC(l) ( (l)->Pos++, (l)->CharacterPos++ )
#define LEXER_INCN(l, n) ( (l)->Pos+=(n), (l)->CharacterPos+=(n) )
#define TOKEN_DATA_OFFSET (2)

/* maximum value which can be represented by a "char" data type */
#define MAX_CHAR_VALUE (255)

static LexToken LexCheckReservedWord(Picoc *picoc, const char *Word);
static LexToken LexGetNumber(Picoc *picoc, LexState *Lexer, Value *value);
static LexToken LexGetWord(Picoc *picoc, LexState *Lexer, Value *value);
static unsigned char LexUnEscapeCharacterConstant(const char **From,
    unsigned char FirstChar, int Base);
static unsigned char LexUnEscapeCharacter(const char **From, const char *End);
static LexToken LexGetStringConstant(Picoc *picoc, LexState *Lexer,
    Value *value, char EndChar);
static LexToken LexGetCharacterConstant(Picoc *picoc, LexState *Lexer,
    Value *value);
static void LexSkipComment(LexState *Lexer, char NextChar);
static void LexSkipLineCont(LexState *Lexer, char NextChar);
static LexToken LexScanGetToken(Picoc *picoc, LexState *Lexer,
    Value **value);
static int LexTokenSize(LexToken Token);
static void *LexTokenize(Picoc *picoc, LexState *Lexer, int *TokenLen);
static LexToken LexGetRawToken(ParseState *Parser, Value **value,
    int IncPos);
static void LexHashIncPos(ParseState *Parser, int IncPos);
static void LexHashIfdef(ParseState *Parser, int IfNot);
static void LexHashIf(ParseState *Parser);
static void LexHashElse(ParseState *Parser);
static void LexHashEndif(ParseState *Parser);


struct ReservedWord {
    const char *Word;
    LexToken Token;
};

static struct ReservedWord ReservedWords[] = {
    /* wtf, when optimizations are set escaping certain chars is required or they disappear */
    {"#define", TokenHashDefine},
    {"#else", TokenHashElse},
    {"#endif", TokenHashEndif},
    {"#if", TokenHashIf},
    {"#ifdef", TokenHashIfdef},
    {"#ifndef", TokenHashIfndef},
    {"#include", TokenHashInclude},
    {"auto", TokenAutoType},
    {"break", TokenBreak},
    {"case", TokenCase},
    {"char", TokenCharType},
    {"continue", TokenContinue},
    {"default", TokenDefault},
    {"delete", TokenDelete},
    {"do", TokenDo},
    {"double", TokenDoubleType},
    {"else", TokenElse},
    {"enum", TokenEnumType},
    {"extern", TokenExternType},
    {"float", TokenFloatType},
    {"for", TokenFor},
    {"goto", TokenGoto},
    {"if", TokenIf},
    {"int", TokenIntType},
    {"long", TokenLongType},
    {"new", TokenNew},
    {"register", TokenRegisterType},
    {"return", TokenReturn},
    {"short", TokenShortType},
    {"signed", TokenSignedType},
    {"sizeof", TokenSizeof},
    {"static", TokenStaticType},
    {"struct", TokenStructType},
    {"switch", TokenSwitch},
    {"typedef", TokenTypedef},
    {"union", TokenUnionType},
    {"unsigned", TokenUnsignedType},
    {"void", TokenVoidType},
    {"while", TokenWhile}
};



/* initialize the lexer */
void LexInit(Picoc *picoc)
{
    int Count;

    TableInitTable(&picoc->ReservedWordTable, &picoc->ReservedWordHashTable[0],
        sizeof(ReservedWords) / sizeof(struct ReservedWord)*2, true);

    for (Count = 0; Count < sizeof(ReservedWords) / sizeof(struct ReservedWord);
            Count++) {
        TableSet(picoc, &picoc->ReservedWordTable,
            TableStrRegister(picoc, ReservedWords[Count].Word),
            (Value*)&ReservedWords[Count], NULL, 0, 0);
    }

    picoc->LexValue.Typ = NULL;
    picoc->LexValue.Val = &picoc->LexAnyValue;
    picoc->LexValue.LValueFrom = false;
    picoc->LexValue.ValOnHeap = false;
    picoc->LexValue.ValOnStack = false;
    picoc->LexValue.AnyValOnHeap = false;
    picoc->LexValue.IsLValue = false;
}

/* deallocate */
void LexCleanup(Picoc *picoc)
{
    int Count;

    LexInteractiveClear(picoc, NULL);

    for (Count = 0; Count < sizeof(ReservedWords) / sizeof(struct ReservedWord);
            Count++)
        TableDelete(picoc, &picoc->ReservedWordTable,
            TableStrRegister(picoc, ReservedWords[Count].Word));
}

/* check if a word is a reserved word - used while scanning */
LexToken LexCheckReservedWord(Picoc *picoc, const char *Word)
{
    Value *val;

    if (TableGet(&picoc->ReservedWordTable, Word, &val, NULL, NULL, NULL))
        return ((struct ReservedWord*)val)->Token;
    else
        return TokenNone;
}

/* get a numeric literal - used while scanning */
LexToken LexGetNumber(Picoc *picoc, LexState *Lexer, Value *value)
{
    long Result = 0;
    long Base = 10;
    LexToken ResultToken;
    double FPResult;
    double FPDiv;
    /* long/unsigned flags */
#if 0 /* unused for now */
    char IsLong = 0;
    char IsUnsigned = 0;
#endif

    if (*Lexer->Pos == '0') {
        /* a binary, octal or hex literal */
        LEXER_INC(Lexer);
        if (Lexer->Pos != Lexer->End) {
            if (*Lexer->Pos == 'x' || *Lexer->Pos == 'X') {
                Base = 16; LEXER_INC(Lexer);
            } else if (*Lexer->Pos == 'b' || *Lexer->Pos == 'B') {
                Base = 2; LEXER_INC(Lexer);
            } else if (*Lexer->Pos != '.')
                Base = 8;
        }
    }

    /* get the value */
    for (; Lexer->Pos != Lexer->End && IS_BASE_DIGIT(*Lexer->Pos, Base);
            LEXER_INC(Lexer))
        Result = Result * Base + GET_BASE_DIGIT(*Lexer->Pos);

    if (*Lexer->Pos == 'u' || *Lexer->Pos == 'U') {
        LEXER_INC(Lexer);
        /* IsUnsigned = 1; */
    }
    if (*Lexer->Pos == 'l' || *Lexer->Pos == 'L') {
        LEXER_INC(Lexer);
        /* IsLong = 1; */
    }

    value->Typ = &picoc->LongType; /* ignored? */
    value->Val->LongInteger = Result;

    ResultToken = TokenIntegerConstant;

    if (Lexer->Pos == Lexer->End)
        return ResultToken;

    if (Lexer->Pos == Lexer->End) {
        return ResultToken;
    }

    if (*Lexer->Pos != '.' && *Lexer->Pos != 'e' && *Lexer->Pos != 'E') {
        return ResultToken;
    }

    value->Typ = &picoc->FPType;
    FPResult = (double)Result;

    if (*Lexer->Pos == '.') {
        LEXER_INC(Lexer);
        for (FPDiv = 1.0/Base; Lexer->Pos != Lexer->End && IS_BASE_DIGIT(*Lexer->Pos, Base);
                LEXER_INC(Lexer), FPDiv /= (double)Base) {
            FPResult += GET_BASE_DIGIT(*Lexer->Pos) * FPDiv;
        }
    }

    if (Lexer->Pos != Lexer->End && (*Lexer->Pos == 'e' || *Lexer->Pos == 'E')) {
        int ExponentSign = 1;

        LEXER_INC(Lexer);
        if (Lexer->Pos != Lexer->End && *Lexer->Pos == '-') {
            ExponentSign = -1;
            LEXER_INC(Lexer);
        }

        Result = 0;
        while (Lexer->Pos != Lexer->End && IS_BASE_DIGIT(*Lexer->Pos, Base)) {
            Result = Result * Base + GET_BASE_DIGIT(*Lexer->Pos);
            LEXER_INC(Lexer);
        }

        FPResult *= pow((double)Base, (double)Result * ExponentSign);
    }

    value->Val->FP = FPResult;

    if (*Lexer->Pos == 'f' || *Lexer->Pos == 'F')
        LEXER_INC(Lexer);

    return TokenFPConstant;
}

/* get a reserved word or identifier - used while scanning */
LexToken LexGetWord(Picoc *picoc, LexState *Lexer, Value *value)
{
    const char *StartPos = Lexer->Pos;
    LexToken Token;

    do {
        LEXER_INC(Lexer);
    } while (Lexer->Pos != Lexer->End && isCident((int)*Lexer->Pos));

    value->Typ = NULL;
    value->Val->Identifier = TableStrRegister2(picoc, StartPos, Lexer->Pos - StartPos);

    Token = LexCheckReservedWord(picoc, value->Val->Identifier);
    switch (Token) {
    case TokenHashInclude:
        Lexer->Mode = LexModeHashInclude;
        break;
    case TokenHashDefine:
        Lexer->Mode = LexModeHashDefine;
        break;
    default:
        break;
    }

    if (Token != TokenNone)
        return Token;

    if (Lexer->Mode == LexModeHashDefineSpace)
        Lexer->Mode = LexModeHashDefineSpaceIdent;

    return TokenIdentifier;
}

/* unescape a character from an octal character constant */
unsigned char LexUnEscapeCharacterConstant(const char **From,
    unsigned char FirstChar, int Base)
{
    int CCount;
    unsigned char Total = GET_BASE_DIGIT(FirstChar);
    for (CCount = 0; IS_BASE_DIGIT(**From, Base) && CCount < 2; CCount++, (*From)++)
        Total = Total * Base + GET_BASE_DIGIT(**From);

    return Total;
}

/* unescape a character from a string or character constant */
unsigned char LexUnEscapeCharacter(const char **From, const char *End)
{
    unsigned char ThisChar;

    while (*From != End && **From == '\\' &&
            &(*From)[1] != End && (*From)[1] == '\n' )
        (*From) += 2;  /* skip escaped end of lines with LF line termination */

    while (*From != End && **From == '\\' &&
            &(*From)[1] != End &&
            &(*From)[2] != End && (*From)[1] == '\r' && (*From)[2] == '\n')
        (*From) += 3;  /* skip escaped end of lines with CR/LF line termination */

    if (*From == End)
        return '\\';

    if (**From == '\\') {
        /* it's escaped */
        (*From)++;
        if (*From == End)
            return '\\';

        ThisChar = *(*From)++;
        switch (ThisChar) {
        case '\\':
            return '\\';
        case '\'':
            return '\'';
        case '"':
            return '"';
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        case '0':
        case '1':
        case '2':
        case '3':
            return LexUnEscapeCharacterConstant(From, ThisChar, 8);
        case 'x':
            return LexUnEscapeCharacterConstant(From, '0', 16);
        default:
            return ThisChar;
        }
    }
    else
        return *(*From)++;
}

/* get a string constant - used while scanning */
LexToken LexGetStringConstant(Picoc *picoc, LexState *Lexer,
    Value *value, char EndChar)
{
    int Escape = false;
    const char *StartPos = Lexer->Pos;
    const char *EndPos;
    char *EscBuf;
    char *EscBufPos;
    char *RegString;
    Value *ArrayValue;

    while (Lexer->Pos != Lexer->End && (*Lexer->Pos != EndChar || Escape)) {
        /* find the end */
        if (Escape) {
            if (*Lexer->Pos == '\r' && Lexer->Pos+1 != Lexer->End)
                Lexer->Pos++;

            if (*Lexer->Pos == '\n' && Lexer->Pos+1 != Lexer->End) {
                Lexer->Line++;
                Lexer->Pos++;
                Lexer->CharacterPos = 0;
                Lexer->EmitExtraNewlines++;
            }

            Escape = false;
        } else if (*Lexer->Pos == '\\')
            Escape = true;

        LEXER_INC(Lexer);
    }
    EndPos = Lexer->Pos;

    EscBuf = HeapAllocStack(picoc, EndPos - StartPos);
    if (EscBuf == NULL)
        LexFail(picoc, Lexer, "(LexGetStringConstant) out of memory");

    for (EscBufPos = EscBuf, Lexer->Pos = StartPos; Lexer->Pos != EndPos;)
        *EscBufPos++ = LexUnEscapeCharacter(&Lexer->Pos, EndPos);

    /* try to find an existing copy of this string literal */
    RegString = TableStrRegister2(picoc, EscBuf, EscBufPos - EscBuf);
    HeapPopStack(picoc, EscBuf, EndPos - StartPos);
    ArrayValue = VariableStringLiteralGet(picoc, RegString);
    if (ArrayValue == NULL) {
        /* create and store this string literal */
        ArrayValue = VariableAllocValueAndData(picoc, NULL, 0, false, NULL, true);
        ArrayValue->Typ = picoc->CharArrayType;
        ArrayValue->Val = (AnyValue *)RegString;
        VariableStringLiteralDefine(picoc, RegString, ArrayValue);
    }

    /* create the the pointer for this char* */
    value->Typ = picoc->CharPtrType;
    value->Val->Pointer = RegString;
    if (*Lexer->Pos == EndChar)
        LEXER_INC(Lexer);

    return TokenStringConstant;
}

/* get a character constant - used while scanning */
LexToken LexGetCharacterConstant(Picoc *picoc, LexState *Lexer,
    Value *value)
{
    value->Typ = &picoc->CharType;
    value->Val->Character = LexUnEscapeCharacter(&Lexer->Pos, Lexer->End);
    if (Lexer->Pos != Lexer->End && *Lexer->Pos != '\'')
        LexFail(picoc, Lexer, "expected \"'\"");

    LEXER_INC(Lexer);
    return TokenCharacterConstant;
}

/* skip a comment - used while scanning */
void LexSkipComment(LexState *Lexer, char NextChar)
{
    if (NextChar == '*') {
        /* conventional C comment */
        while (Lexer->Pos != Lexer->End &&
                (*(Lexer->Pos-1) != '*' || *Lexer->Pos != '/')) {
            if (*Lexer->Pos == '\n')
                Lexer->EmitExtraNewlines++;
            LEXER_INC(Lexer);
        }

        if (Lexer->Pos != Lexer->End)
            LEXER_INC(Lexer);

        Lexer->Mode = LexModeNormal;
    } else {
        /* C++ style comment */
        while (Lexer->Pos != Lexer->End && *Lexer->Pos != '\n')
            LEXER_INC(Lexer);
    }
}

/* skip a line continuation - used while scanning */
void LexSkipLineCont(LexState *Lexer, char NextChar)
{
    while (Lexer->Pos != Lexer->End && *Lexer->Pos != '\n') {
        LEXER_INC(Lexer);
    }
}

/* get a single token from the source - used while scanning */
LexToken LexScanGetToken(Picoc *picoc, LexState *Lexer,
    Value **value)
{
    char ThisChar;
    char NextChar;
    LexToken GotToken = TokenNone;

    /* handle cases line multi-line comments or string constants
        which mess up the line count */
    if (Lexer->EmitExtraNewlines > 0) {
        Lexer->EmitExtraNewlines--;
        return TokenEndOfLine;
    }

    /* scan for a token */
    do {
        *value = &picoc->LexValue;
        while (Lexer->Pos != Lexer->End && isspace((int)*Lexer->Pos)) {
            if (*Lexer->Pos == '\n') {
                Lexer->Line++;
                Lexer->Pos++;
                Lexer->Mode = LexModeNormal;
                Lexer->CharacterPos = 0;
                return TokenEndOfLine;
            } else if (Lexer->Mode == LexModeHashDefine ||
                                    Lexer->Mode == LexModeHashDefineSpace)
                Lexer->Mode = LexModeHashDefineSpace;
            else if (Lexer->Mode == LexModeHashDefineSpaceIdent)
                Lexer->Mode = LexModeNormal;

            LEXER_INC(Lexer);
        }

        if (Lexer->Pos == Lexer->End || *Lexer->Pos == '\0')
            return TokenEOF;

        ThisChar = *Lexer->Pos;
        if (isCidstart((int)ThisChar))
            return LexGetWord(picoc, Lexer, *value);

        if (isdigit((int)ThisChar))
            return LexGetNumber(picoc, Lexer, *value);

        NextChar = (Lexer->Pos+1 != Lexer->End) ? *(Lexer->Pos+1) : 0;
        LEXER_INC(Lexer);
        switch (ThisChar) {
        case '"':
            GotToken = LexGetStringConstant(picoc, Lexer, *value, '"');
            break;
        case '\'':
            GotToken = LexGetCharacterConstant(picoc, Lexer, *value);
            break;
        case '(':
            if (Lexer->Mode == LexModeHashDefineSpaceIdent)
                GotToken = TokenOpenMacroBracket;
            else
                GotToken = TokenOpenBracket;
            Lexer->Mode = LexModeNormal;
            break;
        case ')':
            GotToken = TokenCloseBracket;
            break;
        case '=':
            NEXTIS('=', TokenEqual, TokenAssign);
            break;
        case '+':
            NEXTIS3('=', TokenAddAssign, '+', TokenIncrement, TokenPlus);
            break;
        case '-':
            NEXTIS4('=', TokenSubtractAssign, '>', TokenArrow, '-',
                TokenDecrement, TokenMinus);
            break;
        case '*':
            NEXTIS('=', TokenMultiplyAssign, TokenAsterisk); break;
        case '/':
            if (NextChar == '/' || NextChar == '*') {
                LEXER_INC(Lexer);
                LexSkipComment(Lexer, NextChar);
            } else
                NEXTIS('=', TokenDivideAssign, TokenSlash);
            break;
        case '%':
            NEXTIS('=', TokenModulusAssign, TokenModulus); break;
        case '<':
            if (Lexer->Mode == LexModeHashInclude)
                GotToken = LexGetStringConstant(picoc, Lexer, *value, '>');
            else {
                NEXTIS3PLUS('=', TokenLessEqual, '<', TokenShiftLeft, '=',
                    TokenShiftLeftAssign, TokenLessThan);
            }
            break;
        case '>':
            NEXTIS3PLUS('=', TokenGreaterEqual, '>', TokenShiftRight, '=',
                TokenShiftRightAssign, TokenGreaterThan);
            break;
        case ';':
            GotToken = TokenSemicolon;
            break;
        case '&':
            NEXTIS3('=', TokenArithmeticAndAssign, '&', TokenLogicalAnd,
                TokenAmpersand);
            break;
        case '|':
            NEXTIS3('=', TokenArithmeticOrAssign, '|', TokenLogicalOr,
                TokenArithmeticOr);
            break;
        case '{':
            GotToken = TokenLeftBrace;
            break;
        case '}':
            GotToken = TokenRightBrace;
            break;
        case '[':
            GotToken = TokenLeftSquareBracket;
            break;
        case ']':
            GotToken = TokenRightSquareBracket;
            break;
        case '!':
            NEXTIS('=', TokenNotEqual, TokenUnaryNot);
            break;
        case '^':
            NEXTIS('=', TokenArithmeticExorAssign, TokenArithmeticExor);
            break;
        case '~':
            GotToken = TokenUnaryExor;
            break;
        case ',':
            GotToken = TokenComma;
            break;
        case '.':
            NEXTISEXACTLY3('.', '.', TokenEllipsis, TokenDot);
            break;
        case '?':
            GotToken = TokenQuestionMark;
            break;
        case ':':
            GotToken = TokenColon;
            break;
// XXX: line continuation feature
        case '\\':
            if (NextChar == ' ' || NextChar == '\n') {
                LEXER_INC(Lexer);
                LexSkipLineCont(Lexer, NextChar);
            } else
                LexFail(picoc, Lexer, "illegal character '%c'", ThisChar);
            break;
        default:
            LexFail(picoc, Lexer, "illegal character '%c'", ThisChar);
            break;
        }
    } while (GotToken == TokenNone);

    return GotToken;
}

/* what size value goes with each token */
int LexTokenSize(LexToken Token)
{
    switch (Token) {
    case TokenIdentifier: case TokenStringConstant: return sizeof(char*);
    case TokenIntegerConstant: return sizeof(long);
    case TokenCharacterConstant: return sizeof(unsigned char);
    case TokenFPConstant: return sizeof(double);
    default: return 0;
    }
}

/* produce tokens from the lexer and return a heap buffer with
    the result - used for scanning */
void *LexTokenize(Picoc *picoc, LexState *Lexer, int *TokenLen)
{
    int MemUsed = 0;
    int ValueSize;
    int LastCharacterPos = 0;
    int ReserveSpace = (Lexer->End - Lexer->Pos) * 4 + 16;
    void *HeapMem;
    void *TokenSpace = HeapAllocStack(picoc, ReserveSpace);
    LexToken Token;
    Value *GotValue;
    char *TokenPos = (char*)TokenSpace;

    if (TokenSpace == NULL)
        LexFail(picoc, Lexer, "(LexTokenize TokenSpace == NULL) out of memory");

    do {
        /* store the token at the end of the stack area */
        Token = LexScanGetToken(picoc, Lexer, &GotValue);

#ifdef DEBUG_LEXER
        printf("Token: %02x\n", Token);
#endif
        *(unsigned char*)TokenPos = Token;
        TokenPos++;
        MemUsed++;

        *(unsigned char*)TokenPos = (unsigned char)LastCharacterPos;
        TokenPos++;
        MemUsed++;

        ValueSize = LexTokenSize(Token);
        if (ValueSize > 0) {
            /* store a value as well */
            memcpy((void*)TokenPos, (void*)GotValue->Val, ValueSize);
            TokenPos += ValueSize;
            MemUsed += ValueSize;
        }

        LastCharacterPos = Lexer->CharacterPos;

    } while (Token != TokenEOF);

    HeapMem = HeapAllocMem(picoc, MemUsed);
    if (HeapMem == NULL)
        LexFail(picoc, Lexer, "(LexTokenize HeapMem == NULL) out of memory");

    assert(ReserveSpace >= MemUsed);
    memcpy(HeapMem, TokenSpace, MemUsed);
    HeapPopStack(picoc, TokenSpace, ReserveSpace);
#ifdef DEBUG_LEXER
    {
        int Count;
        printf("Tokens: ");
        for (Count = 0; Count < MemUsed; Count++)
            printf("%02x ", *((unsigned char*)HeapMem+Count));
        printf("\n");
    }
#endif
    if (TokenLen)
        *TokenLen = MemUsed;

    return HeapMem;
}

/* lexically analyse some source text */
void *LexAnalyse(Picoc *picoc, const char *FileName, const char *Source,
    int SourceLen, int *TokenLen)
{
    LexState Lexer;

    Lexer.Pos = Source;
    Lexer.End = Source + SourceLen;
    Lexer.Line = 1;
    Lexer.FileName = FileName;
    Lexer.Mode = LexModeNormal;
    Lexer.EmitExtraNewlines = 0;
    Lexer.CharacterPos = 1;
    Lexer.SourceText = Source;

    return LexTokenize(picoc, &Lexer, TokenLen);
}

/* prepare to parse a pre-tokenised buffer */
void LexInitParser(ParseState *Parser, Picoc *picoc, const char *SourceText,
    void *TokenSource, char *FileName, int RunIt, int EnableDebugger)
{
    Parser->picoc = picoc;
    Parser->Pos = TokenSource;
    Parser->Line = 1;
    Parser->FileName = FileName;
    Parser->Mode = RunIt ? RunModeRun : RunModeSkip;
    Parser->SearchLabel = 0;
    Parser->HashIfLevel = 0;
    Parser->HashIfEvaluateToLevel = 0;
    Parser->CharacterPos = 0;
    Parser->SourceText = SourceText;
    Parser->DebugMode = EnableDebugger;
}

/* get the next token, without pre-processing */
LexToken LexGetRawToken(ParseState *Parser, Value **value,
    int IncPos)
{
    int ValueSize;
    char *Prompt = NULL;
    LexToken Token = TokenNone;
    Picoc *picoc = Parser->picoc;

    do {
        /* get the next token */
        if (Parser->Pos == NULL && picoc->InteractiveHead != NULL)
            Parser->Pos = picoc->InteractiveHead->Tokens;

        if (Parser->FileName != picoc->StrEmpty || picoc->InteractiveHead != NULL) {
            /* skip leading newlines */
            while ((Token = (LexToken)*(unsigned char*)Parser->Pos) == TokenEndOfLine) {
                Parser->Line++;
                Parser->Pos += TOKEN_DATA_OFFSET;
            }
        }

        if (Parser->FileName == picoc->StrEmpty &&
                (picoc->InteractiveHead == NULL || Token == TokenEOF)) {
            /* we're at the end of an interactive input token list */
            char LineBuffer[LINEBUFFER_MAX];
            void *LineTokens;
            int LineBytes;
            TokenLine *LineNode;

            if (picoc->InteractiveHead == NULL ||
                    (unsigned char*)Parser->Pos ==
                    &picoc->InteractiveTail->Tokens[picoc->InteractiveTail->NumBytes-TOKEN_DATA_OFFSET]) {
                /* get interactive input */
                if (picoc->LexUseStatementPrompt) {
                    Prompt = INTERACTIVE_PROMPT_STATEMENT;
                    picoc->LexUseStatementPrompt = false;
                } else
                    Prompt = INTERACTIVE_PROMPT_LINE;

                if (PlatformGetLine(&LineBuffer[0], LINEBUFFER_MAX, Prompt) == NULL)
                    return TokenEOF;

                /* put the new line at the end of the linked list of interactive lines */
                LineTokens = LexAnalyse(picoc, picoc->StrEmpty, &LineBuffer[0],
                    strlen(LineBuffer), &LineBytes);
                LineNode = VariableAlloc(picoc, Parser,
                    sizeof(TokenLine), true);
                LineNode->Tokens = LineTokens;
                LineNode->NumBytes = LineBytes;
                if (picoc->InteractiveHead == NULL) {
                    /* start a new list */
                    picoc->InteractiveHead = LineNode;
                    Parser->Line = 1;
                    Parser->CharacterPos = 0;
                } else
                    picoc->InteractiveTail->Next = LineNode;

                picoc->InteractiveTail = LineNode;
                picoc->InteractiveCurrentLine = LineNode;
                Parser->Pos = LineTokens;
            } else {
                /* go to the next token line */
                if (Parser->Pos != &picoc->InteractiveCurrentLine->Tokens[picoc->InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET]) {
                    /* scan for the line */
                    for (picoc->InteractiveCurrentLine = picoc->InteractiveHead;
                            Parser->Pos != &picoc->InteractiveCurrentLine->Tokens[picoc->InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET];
                            picoc->InteractiveCurrentLine = picoc->InteractiveCurrentLine->Next) {
                        assert(picoc->InteractiveCurrentLine->Next != NULL);
                    }
                }

                assert(picoc->InteractiveCurrentLine != NULL);
                picoc->InteractiveCurrentLine = picoc->InteractiveCurrentLine->Next;
                assert(picoc->InteractiveCurrentLine != NULL);
                Parser->Pos = picoc->InteractiveCurrentLine->Tokens;
            }

            Token = (LexToken)*(unsigned char*)Parser->Pos;
        }
    } while ((Parser->FileName == picoc->StrEmpty && Token == TokenEOF) ||
        Token == TokenEndOfLine);

    Parser->CharacterPos = *((unsigned char*)Parser->Pos + 1);
    ValueSize = LexTokenSize(Token);
    if (ValueSize > 0) {
        /* this token requires a value - unpack it */
        if (value != NULL) {
            switch (Token) {
            case TokenStringConstant:
                picoc->LexValue.Typ = picoc->CharPtrType;
                break;
            case TokenIdentifier:
                picoc->LexValue.Typ = NULL;
                break;
            case TokenIntegerConstant:
                picoc->LexValue.Typ = &picoc->LongType;
                break;
            case TokenCharacterConstant:
                picoc->LexValue.Typ = &picoc->CharType;
                break;
            case TokenFPConstant:
                picoc->LexValue.Typ = &picoc->FPType;
                break;
            default:
                break;
            }

            memcpy((void*)picoc->LexValue.Val,
                (void*)((char*)Parser->Pos+TOKEN_DATA_OFFSET), ValueSize);
            picoc->LexValue.ValOnHeap = false;
            picoc->LexValue.ValOnStack = false;
            picoc->LexValue.IsLValue = false;
            picoc->LexValue.LValueFrom = NULL;
            *value = &picoc->LexValue;
        }

        if (IncPos)
            Parser->Pos += ValueSize + TOKEN_DATA_OFFSET;
    } else {
        if (IncPos && Token != TokenEOF)
            Parser->Pos += TOKEN_DATA_OFFSET;
    }

#ifdef DEBUG_LEXER
    printf("Got token=%02x inc=%d pos=%d\n", Token, IncPos, Parser->CharacterPos);
#endif
    assert(Token >= TokenNone && Token <= TokenEndOfFunction);
    return Token;
}

/* correct the token position depending if we already incremented the position */
void LexHashIncPos(ParseState *Parser, int IncPos)
{
    if (!IncPos)
        LexGetRawToken(Parser, NULL, true);
}

/* handle a #ifdef directive */
void LexHashIfdef(ParseState *Parser, int IfNot)
{
    /* get symbol to check */
    int IsDefined;
    Value *IdentValue;
    Value *SavedValue;
    LexToken Token = LexGetRawToken(Parser, &IdentValue, true);

    if (Token != TokenIdentifier)
        ProgramFail(Parser, "identifier expected");

    /* is the identifier defined? */
    IsDefined = TableGet(&Parser->picoc->GlobalTable, IdentValue->Val->Identifier,
        &SavedValue, NULL, NULL, NULL);
    if (Parser->HashIfEvaluateToLevel == Parser->HashIfLevel &&
            ((IsDefined && !IfNot) || (!IsDefined && IfNot))) {
        /* #if is active, evaluate to this new level */
        Parser->HashIfEvaluateToLevel++;
    }

    Parser->HashIfLevel++;
}

/* handle a #if directive */
void LexHashIf(ParseState *Parser)
{
    /* get symbol to check */
    Value *IdentValue;
    Value *SavedValue = NULL;
    ParseState MacroParser;
    LexToken Token = LexGetRawToken(Parser, &IdentValue, true);

    if (Token == TokenIdentifier) {
        /* look up a value from a macro definition */
        if (!TableGet(&Parser->picoc->GlobalTable, IdentValue->Val->Identifier,
                &SavedValue, NULL, NULL, NULL))
            ProgramFail(Parser, "'%s' is undefined", IdentValue->Val->Identifier);

        if (SavedValue->Typ->Base != TypeMacro)
            ProgramFail(Parser, "value expected");

        ParserCopy(&MacroParser, &SavedValue->Val->MacroDef.Body);
        Token = LexGetRawToken(&MacroParser, &IdentValue, true);
    }

    if (Token != TokenCharacterConstant && Token != TokenIntegerConstant)
        ProgramFail(Parser, "value expected");

    /* is the identifier defined? */
    if (Parser->HashIfEvaluateToLevel == Parser->HashIfLevel &&
            IdentValue->Val->Character) {
        /* #if is active, evaluate to this new level */
        Parser->HashIfEvaluateToLevel++;
    }

    Parser->HashIfLevel++;
}

/* handle a #else directive */
void LexHashElse(ParseState *Parser)
{
    if (Parser->HashIfEvaluateToLevel == Parser->HashIfLevel - 1)
        Parser->HashIfEvaluateToLevel++;  /* #if was not active, make
                                            this next section active */
    else if (Parser->HashIfEvaluateToLevel == Parser->HashIfLevel) {
        /* #if was active, now go inactive */
        if (Parser->HashIfLevel == 0)
            ProgramFail(Parser, "#else without #if");

        Parser->HashIfEvaluateToLevel--;
    }
}

/* handle a #endif directive */
void LexHashEndif(ParseState *Parser)
{
    if (Parser->HashIfLevel == 0)
        ProgramFail(Parser, "#endif without #if");

    Parser->HashIfLevel--;
    if (Parser->HashIfEvaluateToLevel > Parser->HashIfLevel)
        Parser->HashIfEvaluateToLevel = Parser->HashIfLevel;
}

#if 0 /* useful for debug */
#define TOK_DEF(name, prefix, postfix, infix, repr, string)   string,
void LexPrintToken(LexToken Token)
{
    const char* const TokenNames[] = {
        #include "token-defs.h"
    };
    printf("{%s}", TokenNames[Token]);
}
#endif

/* get the next token given a parser state, pre-processing as we go */
LexToken LexGetToken(ParseState *Parser, Value **value,
    int IncPos)
{
    int TryNextToken;
    LexToken Token;

    /* implements the pre-processor #if commands */
    do {
        int WasPreProcToken = true;

        Token = LexGetRawToken(Parser, value, IncPos);
        switch (Token) {
        case TokenHashIfdef:
            LexHashIncPos(Parser, IncPos); LexHashIfdef(Parser, false);
            break;
        case TokenHashIfndef:
            LexHashIncPos(Parser, IncPos); LexHashIfdef(Parser, true);
            break;
        case TokenHashIf:
            LexHashIncPos(Parser, IncPos); LexHashIf(Parser);
            break;
        case TokenHashElse:
            LexHashIncPos(Parser, IncPos); LexHashElse(Parser);
            break;
        case TokenHashEndif:
            LexHashIncPos(Parser, IncPos); LexHashEndif(Parser);
            break;
        default:
            WasPreProcToken = false;
            break;
        }

        /* if we're going to reject this token, increment the token
            pointer to the next one */
        TryNextToken = (Parser->HashIfEvaluateToLevel < Parser->HashIfLevel &&
                Token != TokenEOF) || WasPreProcToken;
        if (!IncPos && TryNextToken)
            LexGetRawToken(Parser, NULL, true);

    } while (TryNextToken);

    return Token;
}

/* take a quick peek at the next token, skipping any pre-processing */
LexToken LexRawPeekToken(ParseState *Parser)
{
    return (LexToken)*(unsigned char*)Parser->Pos;
}

/* find the end of the line */
void LexToEndOfMacro(ParseState *Parser)
{
// XXX: line continuation feature
    bool isContinued = false;
    while (true) {
        LexToken Token = (LexToken)*(unsigned char*)Parser->Pos;
        if (Token == TokenEOF)
            return;
        else if (Token == TokenEndOfLine) {
            if (!isContinued)
                return;
            isContinued = false;
        }
        if (Token == TokenBackSlash)
            isContinued = true;
        LexGetRawToken(Parser, NULL, true);
    }
}

/* copy the tokens from StartParser to EndParser into new memory, removing
    TokenEOFs and terminate with a TokenEndOfFunction */
void *LexCopyTokens(ParseState *StartParser, ParseState *EndParser)
{
    int MemSize = 0;
    int CopySize;
    unsigned char *Pos = (unsigned char*)StartParser->Pos;
    unsigned char *NewTokens;
    unsigned char *NewTokenPos;
    TokenLine *ILine;
    Picoc *picoc = StartParser->picoc;

    if (picoc->InteractiveHead == NULL) {
        /* non-interactive mode - copy the tokens */
        MemSize = EndParser->Pos - StartParser->Pos;
        NewTokens = VariableAlloc(picoc, StartParser, MemSize + TOKEN_DATA_OFFSET, true);
        memcpy(NewTokens, (void*)StartParser->Pos, MemSize);
    } else {
        /* we're in interactive mode - add up line by line */
        for (picoc->InteractiveCurrentLine = picoc->InteractiveHead;
                picoc->InteractiveCurrentLine != NULL &&
                (Pos < &picoc->InteractiveCurrentLine->Tokens[0] ||
                    Pos >= &picoc->InteractiveCurrentLine->Tokens[picoc->InteractiveCurrentLine->NumBytes]);
                picoc->InteractiveCurrentLine = picoc->InteractiveCurrentLine->Next) {
        } /* find the line we just counted */

        if (EndParser->Pos >= StartParser->Pos &&
                EndParser->Pos < &picoc->InteractiveCurrentLine->Tokens[picoc->InteractiveCurrentLine->NumBytes]) {
            /* all on a single line */
            MemSize = EndParser->Pos - StartParser->Pos;
            NewTokens = VariableAlloc(picoc, StartParser, MemSize + TOKEN_DATA_OFFSET, true);
            memcpy(NewTokens, (void*)StartParser->Pos, MemSize);
        } else {
            /* it's spread across multiple lines */
            MemSize = &picoc->InteractiveCurrentLine->Tokens[picoc->InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET] - Pos;

            for (ILine = picoc->InteractiveCurrentLine->Next;
                    ILine != NULL &&
                    (EndParser->Pos < &ILine->Tokens[0] || EndParser->Pos >= &ILine->Tokens[ILine->NumBytes]);
                    ILine = ILine->Next)
                MemSize += ILine->NumBytes - TOKEN_DATA_OFFSET;

            assert(ILine != NULL);
            MemSize += EndParser->Pos - &ILine->Tokens[0];
            NewTokens = VariableAlloc(picoc, StartParser, MemSize + TOKEN_DATA_OFFSET, true);

            CopySize = &picoc->InteractiveCurrentLine->Tokens[picoc->InteractiveCurrentLine->NumBytes-TOKEN_DATA_OFFSET] - Pos;
            memcpy(NewTokens, Pos, CopySize);
            NewTokenPos = NewTokens + CopySize;
            for (ILine = picoc->InteractiveCurrentLine->Next; ILine != NULL &&
                    (EndParser->Pos < &ILine->Tokens[0] || EndParser->Pos >= &ILine->Tokens[ILine->NumBytes]);
                    ILine = ILine->Next) {
                memcpy(NewTokenPos, &ILine->Tokens[0], ILine->NumBytes - TOKEN_DATA_OFFSET);
                NewTokenPos += ILine->NumBytes-TOKEN_DATA_OFFSET;
            }
            assert(ILine != NULL);
            memcpy(NewTokenPos, &ILine->Tokens[0], EndParser->Pos - &ILine->Tokens[0]);
        }
    }

    NewTokens[MemSize] = (unsigned char)TokenEndOfFunction;

    return NewTokens;
}

/* indicate that we've completed up to this point in the interactive input
    and free expired tokens */
void LexInteractiveClear(Picoc *picoc, ParseState *Parser)
{
    while (picoc->InteractiveHead != NULL) {
        TokenLine *NextLine = picoc->InteractiveHead->Next;

        HeapFreeMem(picoc, picoc->InteractiveHead->Tokens);
        HeapFreeMem(picoc, picoc->InteractiveHead);
        picoc->InteractiveHead = NextLine;
    }

    if (Parser != NULL)
        Parser->Pos = NULL;

    picoc->InteractiveTail = NULL;
}

/* indicate that we've completed up to this point in the interactive
    input and free expired tokens */
void LexInteractiveCompleted(Picoc *picoc, ParseState *Parser)
{
    while (picoc->InteractiveHead != NULL &&
            !(Parser->Pos >= &picoc->InteractiveHead->Tokens[0] &&
                Parser->Pos < &picoc->InteractiveHead->Tokens[picoc->InteractiveHead->NumBytes])) {
        /* this token line is no longer needed - free it */
        TokenLine *NextLine = picoc->InteractiveHead->Next;

        HeapFreeMem(picoc, picoc->InteractiveHead->Tokens);
        HeapFreeMem(picoc, picoc->InteractiveHead);
        picoc->InteractiveHead = NextLine;

        if (picoc->InteractiveHead == NULL) {
            /* we've emptied the list */
            Parser->Pos = NULL;
            picoc->InteractiveTail = NULL;
        }
    }
}

/* the next time we prompt, make it the full statement prompt */
void LexInteractiveStatementPrompt(Picoc *picoc)
{
    picoc->LexUseStatementPrompt = true;
}
