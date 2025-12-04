#ifndef TOK_DEF
    #define TOK_DEF(name, prefix, postfix, infix, repr, string)
#endif

TOK_DEF( TokenNone,                  0,  0,  0, "none",     "None"                  )   /* 0x00 */
TOK_DEF( TokenComma,                 0,  0,  0, ",",        "Comma"                 )   /* 0x01 */
TOK_DEF( TokenAssign,                0,  0,  2, "=",        "Assign"                )   /* 0x02 */
TOK_DEF( TokenAddAssign,             0,  0,  2, "+=",       "AddAssign"             )
TOK_DEF( TokenSubtractAssign,        0,  0,  2, "-=",       "SubtractAssign"        )
TOK_DEF( TokenMultiplyAssign,        0,  0,  2, "*=",       "MultiplyAssign"        )
TOK_DEF( TokenDivideAssign,          0,  0,  2, "/=",       "DivideAssign"          )
TOK_DEF( TokenModulusAssign,         0,  0,  2, "%=",       "ModulusAssign"         )
TOK_DEF( TokenShiftLeftAssign,       0,  0,  2, "<<=",      "ShiftLeftAssign"       )   /* 0x08 */
TOK_DEF( TokenShiftRightAssign,      0,  0,  2, ">>=",      "ShiftRightAssign"      )
TOK_DEF( TokenArithmeticAndAssign,   0,  0,  2, "&=",       "ArithmeticAndAssign"   )
TOK_DEF( TokenArithmeticOrAssign,    0,  0,  2, "|=",       "ArithmeticOrAssign"    )
TOK_DEF( TokenArithmeticExorAssign,  0,  0,  2, "^=",       "ArithmeticExorAssign"  )
TOK_DEF( TokenQuestionMark,          0,  0,  3, "?",        "QuestionMark"          )   /* 0x0d */
TOK_DEF( TokenColon,                 0,  0,  3, ":",        "Colon"                 )
TOK_DEF( TokenLogicalOr,             0,  0,  4, "||",       "LogicalOr"             )   /* 0x0f */
TOK_DEF( TokenLogicalAnd,            0,  0,  5, "&&",       "LogicalAnd"            )   /* 0x10 */
TOK_DEF( TokenArithmeticOr,          0,  0,  6, "|",        "ArithmeticOr"          )   /* 0x11 */
TOK_DEF( TokenArithmeticExor,        0,  0,  7, "^",        "ArithmeticExor"        )   /* 0x12 */
TOK_DEF( TokenAmpersand,            14,  0,  8, "&",        "Ampersand"             )   /* 0x13 */
TOK_DEF( TokenEqual,                 0,  0,  9, "==",       "Equal"                 )   /* 0x14 */
TOK_DEF( TokenNotEqual,              0,  0,  9, "!=",       "NotEqual"              )
TOK_DEF( TokenLessThan,              0,  0, 10, "<",        "LessThan"              )   /* 0x16 */
TOK_DEF( TokenGreaterThan,           0,  0, 10, ">",        "GreaterThan"           )
TOK_DEF( TokenLessEqual,             0,  0, 10, "<=",       "LessEqual"             )
TOK_DEF( TokenGreaterEqual,          0,  0, 10, ">=",       "GreaterEqual"          )
TOK_DEF( TokenShiftLeft,             0,  0, 11, "<<",       "ShiftLeft"             )   /* 0x1a */
TOK_DEF( TokenShiftRight,            0,  0, 11, ">>",       "ShiftRight"            )
TOK_DEF( TokenPlus,                 14,  0, 12, "+",        "Plus"                  )   /* 0x1c */
TOK_DEF( TokenMinus,                14,  0, 12, "-",        "Minus"                 )
TOK_DEF( TokenAsterisk,             14,  0, 13, "*",        "Asterisk"              )   /* 0x1e */
TOK_DEF( TokenSlash,                 0,  0, 13, "/",        "Slash"                 )
TOK_DEF( TokenModulus,               0,  0, 13, "%",        "Modulus"               )
TOK_DEF( TokenIncrement,            14, 15,  0, "++",       "Increment"             )   /* 0x21 */
TOK_DEF( TokenDecrement,            14, 15,  0, "--",       "Decrement"             )
TOK_DEF( TokenUnaryNot,             14,  0,  0, "!",        "UnaryNot"              )
TOK_DEF( TokenUnaryExor,            14,  0,  0, "~",        "UnaryExor"             )
TOK_DEF( TokenSizeof,               14,  0,  0, "sizeof",   "Sizeof"                )
TOK_DEF( TokenCast,                 14,  0,  0, "cast",     "Cast"                  )
TOK_DEF( TokenLeftSquareBracket,     0,  0, 15, "[",        "LeftSquareBracket"     )   /* 0x27 */
TOK_DEF( TokenRightSquareBracket,    0, 15,  0, "]",        "RightSquareBracket"    )
TOK_DEF( TokenDot,                   0,  0, 15, ".",        "Dot"                   )
TOK_DEF( TokenArrow,                 0,  0, 15, "->",       "Arrow"                 )
TOK_DEF( TokenOpenBracket,          15,  0,  0, "(",        "OpenBracket"           )   /* 0x2b */
TOK_DEF( TokenCloseBracket,          0, 15,  0, ")",        "CloseBracket"          )
TOK_DEF( TokenIdentifier,            0,  0,  0, "",         "Identifier"            )   /* 0x2d */
TOK_DEF( TokenIntegerConstant,       0,  0,  0, "",         "IntegerConstant"       )
TOK_DEF( TokenFPConstant,            0,  0,  0, "",         "FPConstant"            )
TOK_DEF( TokenStringConstant,        0,  0,  0, "",         "StringConstant"        )
TOK_DEF( TokenCharacterConstant,     0,  0,  0, "",         "CharacterConstant"     )
TOK_DEF( TokenSemicolon,             0,  0,  0, "",         "Semicolon"             )   /* 0x32 */
TOK_DEF( TokenEllipsis,              0,  0,  0, "",         "Ellipsis"              )
TOK_DEF( TokenLeftBrace,             0,  0,  0, "",         "LeftBrace"             )   /* 0x34 */
TOK_DEF( TokenRightBrace,            0,  0,  0, "",         "RightBrace"            )
TOK_DEF( TokenIntType,               0,  0,  0, "",         "IntType"               )   /* 0x36 */
TOK_DEF( TokenCharType,              0,  0,  0, "",         "CharType"              )
TOK_DEF( TokenFloatType,             0,  0,  0, "",         "FloatType"             )
TOK_DEF( TokenDoubleType,            0,  0,  0, "",         "DoubleType"            )
TOK_DEF( TokenVoidType,              0,  0,  0, "",         "VoidType"              )
TOK_DEF( TokenEnumType,              0,  0,  0, "",         "EnumType"              )
TOK_DEF( TokenLongType,              0,  0,  0, "",         "LongType"              )   /* 0x3c */
TOK_DEF( TokenSignedType,            0,  0,  0, "",         "SignedType"            )
TOK_DEF( TokenShortType,             0,  0,  0, "",         "ShortType"             )
TOK_DEF( TokenStaticType,            0,  0,  0, "",         "StaticType"            )
TOK_DEF( TokenAutoType,              0,  0,  0, "",         "AutoType"              )
TOK_DEF( TokenRegisterType,          0,  0,  0, "",         "RegisterType"          )
TOK_DEF( TokenExternType,            0,  0,  0, "",         "ExternType"            )
TOK_DEF( TokenStructType,            0,  0,  0, "",         "StructType"            )
TOK_DEF( TokenUnionType,             0,  0,  0, "",         "UnionType"             )
TOK_DEF( TokenUnsignedType,          0,  0,  0, "",         "UnsignedType"          )
TOK_DEF( TokenTypedef,               0,  0,  0, "",         "Typedef"               )
TOK_DEF( TokenContinue,              0,  0,  0, "",         "Continue"              )   /* 0x46 */
TOK_DEF( TokenDo,                    0,  0,  0, "",         "Do"                    )
TOK_DEF( TokenElse,                  0,  0,  0, "",         "Else"                  )
TOK_DEF( TokenFor,                   0,  0,  0, "",         "For"                   )
TOK_DEF( TokenGoto,                  0,  0,  0, "",         "Goto"                  )
TOK_DEF( TokenIf,                    0,  0,  0, "",         "If"                    )
TOK_DEF( TokenWhile,                 0,  0,  0, "",         "While"                 )
TOK_DEF( TokenBreak,                 0,  0,  0, "",         "Break"                 )
TOK_DEF( TokenSwitch,                0,  0,  0, "",         "Switch"                )
TOK_DEF( TokenCase,                  0,  0,  0, "",         "Case"                  )
TOK_DEF( TokenDefault,               0,  0,  0, "",         "Default"               )
TOK_DEF( TokenReturn,                0,  0,  0, "",         "Return"                )
TOK_DEF( TokenHashDefine,            0,  0,  0, "",         "HashDefine"            )   /* 0x52 */
TOK_DEF( TokenHashInclude,           0,  0,  0, "",         "HashInclude"           )
TOK_DEF( TokenHashIf,                0,  0,  0, "",         "HashIf"                )
TOK_DEF( TokenHashIfdef,             0,  0,  0, "",         "HashIfdef"             )
TOK_DEF( TokenHashIfndef,            0,  0,  0, "",         "HashIfndef"            )
TOK_DEF( TokenHashElse,              0,  0,  0, "",         "HashElse"              )
TOK_DEF( TokenHashEndif,             0,  0,  0, "",         "HashEndif"             )
TOK_DEF( TokenNew,                   0,  0,  0, "",         "New"                   )   /* 0x59 */
TOK_DEF( TokenDelete,                0,  0,  0, "",         "Delete"                )
TOK_DEF( TokenOpenMacroBracket,      0,  0,  0, "",         "OpenMacroBracket"      )   /* 0x5b */
TOK_DEF( TokenEOF,                   0,  0,  0, "",         "EOF"                   )   /* 0x5c */
TOK_DEF( TokenEndOfLine,             0,  0,  0, "",         "EndOfLine"             )
TOK_DEF( TokenEndOfFunction,         0,  0,  0, "",         "EndOfFunction"         )
TOK_DEF( TokenBackSlash,             0,  0,  0, "",         "BackSlash"             )

#undef TOK_DEF
