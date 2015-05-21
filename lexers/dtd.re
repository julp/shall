#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "tokens.h"
#include "lexer.h"
#include "utils.h"
#include "lexer-private.h"

#if 0
"<!ELEMENT" <espace> <nom> <espace> <"EMPTY" ou "ANY" ou une liste qui commence par "(" et finit par ")"> <espace optionnel> ">"

"<!ATTLIST" <espace> <nom> (<espace> <nom> <espace> <type : "CDATA" ou "ID" | "IDREF" | "IDREFS" | "ENTITY" | "ENTITIES" | "NMTOKEN" | "NMTOKENS" ou ("NOTATION" S)? suivi d'une liste de noms entre "(" et ")" séparée par des "|"> <espace> <"#REQUIRED" | "#IMPLIED">)* <espace optionnel> ">"

"<!NOTATION" <espace> <nom> <espace> <"SYSTEM" ou "PUBLIC"> <espace> <nom> (si "PUBLIC": <espace> <nom>)? <espace optionnel> ">"

"<!ENTITY"
    1)    <espace> <nom> <un nom entre quotes ou ("SYSTEM" + espace + nom ou "PUBLIC" + espace + nom + espace + nom) suivi éventuellement de espace + "NDATA" + espace + nom>
    ou 2) <espace> "%" <espace> <nom> <espace> <un nom entre quotes ou "SYSTEM" + espace + nom ou "PUBLIC" + espace + nom + espace + nom>
 <espace optionnel> ">"

"%" Name ";"

"<![" ("INCLUDE" | "IGNORE") <espace optionnel> "[" <+/- n'importe quoi ?> "]]>"
=> INCLUDE peut être composé de n'importe quoi (d'autres ELEMENT/ATTLIST/... - les mêmes que DOCTYPE)
=> IGNORE ne peut pas contenir "]]>" (ni "<![")

http://www.ibm.com/developerworks/library/x-tiparam/
http://www.xmlgrrl.com/publications/DSDTD/ch10.html
#endif

enum {
    ELEMENT,
    ATTLIST,
    NOTATION,
    ENTITY
};

enum {
    STATE(INITIAL),
    STATE(IN_TAG),
    STATE(IN_ELEMENT),
    STATE(IN_PREPROC),
    STATE(IN_COMMENT),
};

typedef struct {
    LexerData data;
    int *in_dtd; // if not NULL, this is &XMLLexerData.in_dtd of parent lexer
    int depth;
    int argno;
    int tag;
} DTDLexerData;

#define YYSTRNCMP(x) \
    strncmp_l(x, STR_LEN(x), (char *) YYTEXT, YYLENG, STR_LEN(x))

static int dtdlex(YYLEX_ARGS) {
    DTDLexerData *mydata;

    mydata = (DTDLexerData *) data;
    YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;

S = [ \n\r\t]+; // [2]
NameStartChar = ':' | [A-Z] | '_' | [a-z] | [\xC0-\xD6] | [\xD8-\xF6] | [\u00F8-\u02FF] | [\u0370-\u037D] | [\u037F-\u1FFF] | [\u200C-\u200D] | [\u2070-\u218F] | [\u2C00-\u2FEF] | [\u3001-\uD7FF] | [\uF900-\uFDCF] | [\uFDF0-\uFFFD] | [\U00010000-\U000EFFFF]; // [4]

NameChar = NameStartChar | '-' | '.' | [0-9] | [\xB7] | [\u0300-\u036F] | [\u203F-\u2040]; // [4a]
Name = NameStartChar NameChar*; // [5]
PEReference = "%" Name ";"; // [69]

<INITIAL>"<!ELEMENT" S {
    yyless(STR_LEN("<!ELEMENT"));
    BEGIN(IN_TAG);
    mydata->argno = 0;
    mydata->tag = ELEMENT;
    return NAME_TAG;
}

<INITIAL>"<!ATTLIST" S {
    yyless(STR_LEN("<!ATTLIST"));
    BEGIN(IN_TAG);
    mydata->argno = 0;
    mydata->tag = ATTLIST;
    return NAME_TAG;
}

<INITIAL>"<!DOCTYPE" S {
    yyless(STR_LEN("<!DOCTYPE"));
    return NAME_TAG;
}

<INITIAL>"<!" ("ENTITY" | "NOTATION") S {
    mydata->argno = 0;
    mydata->tag = ENTITY;
    BEGIN(IN_TAG);
    return NAME_TAG;
}

<INITIAL>'<!--' {
    BEGIN(IN_COMMENT);
    return COMMENT_MULTILINE;
}

<IN_COMMENT>[^-]'-->' {
    BEGIN(INITIAL);
    return COMMENT_MULTILINE;
}

<IN_COMMENT>[^] {
    return COMMENT_MULTILINE;
}

<*> PEReference {
    return NAME_ENTITY;
}

<IN_TAG>[^ \n\r\t>]+ {
    // NOTE: positionnal argument is a bad idea as entity can break it?
    ++mydata->argno;
    switch (mydata->tag) {
        case ELEMENT:
            if (2 == mydata->argno) {
                if (0 == YYSTRNCMP("EMPTY") || 0 == YYSTRNCMP("ANY")) {
                    return KEYWORD;
                }
            }
            break;
        case ATTLIST:
            if (mydata->argno >= 2) {
                switch ((mydata->argno - 2) % 3) {
                    case 1:
                        if (0 == YYSTRNCMP("CDATA") || 0 == YYSTRNCMP("ID") || 0 == YYSTRNCMP("IDREF") /* ... */) {
                            return KEYWORD;
                        }
                        break;
                    case 2:
                        if (0 == YYSTRNCMP("#IMPLIED") || 0 == YYSTRNCMP("#REQUIRED")) {
                            return KEYWORD;
                        }
                        break;
                }
            }
            break;
    }

    return IGNORABLE;
}

<IN_TAG>'>' {
    BEGIN(INITIAL);
    return NAME_TAG;
}

/*
<IN_ELEMENT>[(),] {
    return PUNCTUATION;
}

<IN_ELEMENT>[|?*+] {
    return OPERATOR;
}

<IN_ELEMENT>"#PCDATA" {
    return KEYWORD_TYPE;
}
*/

<INITIAL>"]]>" {
    if (--mydata->depth >= 0) {
        return NAME_TAG;
    } else {
        return IGNORABLE;
    }
}

<INITIAL>"<![" S? ("INCLUDE" | "IGNORE" | PEReference) S? "[" {
    ++mydata->depth;
    return NAME_TAG;
}

<INITIAL>"]" S? ">" {
    if (NULL != mydata->in_dtd) {
        *mydata->in_dtd = 0;
    }
    return NAME_TAG;
}

<*> [^] {
    return IGNORABLE;
}
*/
}

LexerImplementation dtd_lexer = {
    "DTD",
    "A lexer for DTDs (Document Type Definitions)",
    NULL,
    (const char * const []) { "*.dtd", NULL },
    (const char * const []) { "application/xml-dtd", NULL },
    NULL,
    NULL,
    NULL,
    dtdlex,
    sizeof(DTDLexerData),
    NULL
};
