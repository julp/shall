#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "tokens.h"
#include "lexer.h"
#include "utils.h"

#if 0
/*
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
*/
#endif

enum {
    STATE(INITIAL),
    STATE(IN_ENTITY),
    STATE(IN_ELEMENT),
    STATE(IN_ATTLIST),
    STATE(IN_NOTATION),
    STATE(IN_PREPROC),
    STATE(IN_COMMENT),
    STATE(IN_STRING_SINGLE),
    STATE(IN_STRING_DOUBLE),
};

static int default_token_type[] = {
    IGNORABLE, // INITIAL
    IGNORABLE, // IN_ENTITY
    IGNORABLE, // IN_ELEMENT
    IGNORABLE, // IN_ATTLIST
    IGNORABLE, // IN_NOTATION
    IGNORABLE, // IN_PREPROC
    COMMENT_MULTILINE, // IN_COMMENT
    STRING_SINGLE, // IN_STRING_SINGLE
    STRING_SINGLE, // IN_STRING_DOUBLE
};

typedef struct {
    LexerData data;
    int depth;
} DTDLexerData;

static int dtdlex(YYLEX_ARGS)
{
    DTDLexerData *mydata;

    (void) ctxt;
    (void) options;
    mydata = (DTDLexerData *) data;
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

S = [ \n\r\t]+; // [2]
NameStartChar = ':' | [A-Z] | '_' | [a-z] | [\xC0-\xD6] | [\xD8-\xF6] | [\u00F8-\u02FF] | [\u0370-\u037D] | [\u037F-\u1FFF] | [\u200C-\u200D] | [\u2070-\u218F] | [\u2C00-\u2FEF] | [\u3001-\uD7FF] | [\uF900-\uFDCF] | [\uFDF0-\uFFFD] | [\U00010000-\U000EFFFF]; // [4]

NameChar = NameStartChar | '-' | '.' | [0-9] | [\xB7] | [\u0300-\u036F] | [\u203F-\u2040]; // [4a]
Name = NameStartChar NameChar*; // [5]
CharRef = "&#" [0-9]+ ";" | "&#x" [0-9a-fA-F]+ ";"; // [66]
EntityRef = "&" Name ";"; // [68]
Reference = EntityRef | CharRef; // [67]
PEReference = "%" Name ";"; // [69]

EntityValue = '"' ([^%&"] | PEReference | Reference)* '"' | "'" ([^%&'] | PEReference | Reference)* "'"; // [9]
AttValue = '"' ([^<&"] | Reference)* '"' |  "'" ([^<&'] | Reference)* "'"; // [10]

<INITIAL>"<!ELEMENT" S {
    yyless(STR_LEN("<!ELEMENT"));
    BEGIN(IN_ELEMENT);
    TOKEN(NAME_TAG);
}

<INITIAL>"<!ATTLIST" S {
    yyless(STR_LEN("<!ATTLIST"));
    BEGIN(IN_ATTLIST);
    TOKEN(NAME_TAG);
}

<INITIAL>"<!DOCTYPE" S {
    yyless(STR_LEN("<!DOCTYPE"));
    TOKEN(NAME_TAG);
}

<INITIAL>"<!ENTITY" S {
    yyless(STR_LEN("<!ENTITY"));
    BEGIN(IN_ENTITY);
    TOKEN(NAME_TAG);
}

<INITIAL>"<!NOTATION" S {
    yyless(STR_LEN("<!NOTATION"));
    BEGIN(IN_NOTATION);
    TOKEN(NAME_TAG);
}

<IN_NOTATION>"PUBLIC" | "SYSTEM" {
    TOKEN(KEYWORD_CONSTANT);
}

<INITIAL>'<!--' {
    BEGIN(IN_COMMENT);
    TOKEN(COMMENT_MULTILINE);
}

<IN_COMMENT>[^-]'-->' {
    BEGIN(INITIAL);
    TOKEN(COMMENT_MULTILINE);
}

<*> PEReference {
    TOKEN(NAME_ENTITY);
}

<IN_ENTITY>"SYSTEM" | "PUBLIC" | "NDATA" {
    TOKEN(KEYWORD_CONSTANT);
}

<IN_ELEMENT>"EMPTY" | "ANY" | "#PCDATA" {
    TOKEN(KEYWORD_CONSTANT);
}

<IN_ENTITY>Name {
    TOKEN(NAME_ENTITY);
}

<IN_ELEMENT>[(),] {
    TOKEN(PUNCTUATION);
}

<IN_ELEMENT>[|*+?] {
    TOKEN(OPERATOR);
}

<IN_ELEMENT>Name {
    TOKEN(NAME_TAG);
}

<IN_ATTLIST>[()] {
    TOKEN(PUNCTUATION);
}

<IN_ATTLIST>[|] {
    TOKEN(OPERATOR);
}

<IN_ATTLIST>"CDATA" | "ID" | ("IDREF" "S"?) | "ENTITY" | "ENTITIES" | ("NMTOKEN" "S"?) | "NOTATION" {
    TOKEN(KEYWORD_CONSTANT);
}

<IN_ATTLIST>"#IMPLIED" | "#REQUIRED" | "#FIXED" {
    TOKEN(KEYWORD_CONSTANT);
}

<IN_ATTLIST>Name {
    TOKEN(NAME_ATTRIBUTE);
}

<IN_ATTLIST,IN_ENTITY>"'" {
    PUSH_STATE(IN_STRING_SINGLE);
    TOKEN(STRING_SINGLE);
}

<IN_ATTLIST,IN_ENTITY>'"' {
    PUSH_STATE(IN_STRING_DOUBLE);
    TOKEN(STRING_SINGLE);
}

<IN_STRING_SINGLE>"'" {
    POP_STATE();
    TOKEN(STRING_SINGLE);
}

<IN_STRING_DOUBLE>'"' {
    POP_STATE();
    TOKEN(STRING_SINGLE);
}

<IN_ELEMENT,IN_ATTLIST,IN_ENTITY,IN_NOTATION>">" {
    BEGIN(INITIAL);
    TOKEN(NAME_TAG);
}

<INITIAL>"]]>" {
    if (--mydata->depth >= 0) {
        TOKEN(NAME_TAG);
    } else {
        TOKEN(IGNORABLE);
    }
}

<INITIAL>"<![" S? ("INCLUDE" | "IGNORE" | PEReference) S? "[" {
    ++mydata->depth;
    TOKEN(NAME_TAG);
}

<INITIAL>"]" S? ">" {
#if 0
    TOKEN(NAME_TAG);
    DONE(); // TODO
#else
    DONE_AFTER_TOKEN(NAME_TAG);
#endif
}

<*> [^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
    DONE();
}

LexerImplementation dtd_lexer = {
    "DTD",
    "A lexer for DTDs (Document Type Definitions)",
    NULL, // aliases
    (const char * const []) { "*.dtd", NULL },
    (const char * const []) { "application/xml-dtd", NULL },
    NULL, // interpreters
    NULL, // analyse
    NULL, // init
    dtdlex,
    NULL, // finalyze
    sizeof(DTDLexerData),
    NULL, // options
    NULL // dependencies
};
