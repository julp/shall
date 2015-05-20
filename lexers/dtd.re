#include <stdlib.h>
#include <string.h>

#include "cpp.h"
#include "tokens.h"
#include "lexer.h"
#include "lexer-private.h"

#if 0
"<!ELEMENT" <espace> <nom> <espace> <"EMPTY" ou "ANY" ou une liste qui commence par "(" et finit par ")"> <espace optionnel> ">"

"<!ATTLIST" <espace> <nom> (<espace> <nom> <type : "CDATA" ou "ID" | "IDREF" | "IDREFS" | "ENTITY" | "ENTITIES" | "NMTOKEN" | "NMTOKENS" ou ("NOTATION" S)? suivi d'une liste de noms entre "(" et ")" séparée par des "|"> <espace> <"#REQUIRED" | "#IMPLIED">)* <espace optionnel> ">"

"<!NOTATION" <espace> <nom> <espace> <"SYSTEM" ou "PUBLIC"> <espace> <nom> (si "PUBLIC": <espace> <nom>)? <espace optionnel> ">"

"<!ENTITY"
    1)    <espace> <nom> <un nom entre quotes ou ("SYSTEM" + espace + nom ou "PUBLIC" + espace + nom + espace + nom) suivi éventuellement de espace + "NDATA" + espace + nom>
    ou 2) <espace> "%" <espace> <nom> <espace> <un nom entre quotes ou "SYSTEM" + espace + nom ou "PUBLIC" + espace + nom + espace + nom>
 <espace optionnel> ">"

"%" Name ";"

"<![" ("INCLUDE" | "IGNORE") <espace optionnel> "[" <+/- n'importe quoi ?> "]]>"
=> INCLUDE peut être composé de n'importe quoi (d'autres ELEMENT/ATTLIST/... - les mêmes que DOCTYPE)
=> IGNORE ne peut pas contenir "]]>" (ni "<![")
#endif

enum {
    STATE(INITIAL),
    STATE(IN_TAG),
    STATE(IN_ELEMENT),
    STATE(IN_PREPROC),
    STATE(IN_COMMENT),
};

typedef struct {
    LexerData data;
    int in_dtd;
    int saved_state;
} XMLLexerData;

static int dtdlex(YYLEX_ARGS) {
    XMLLexerData *mydata;

    mydata = (XMLLexerData *) data;
    YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;

S = [ \n\r\t]+; // [2]

<INITIAL>"<!ELEMENT" {
    BEGIN(IN_ELEMENT);
    return NAME_TAG;
}

<INITIAL>"<!" ("DOCTYPE" | "ENTITY" | "NOTATION" | "ATTLIST") {
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

<IN_TAG,IN_ELEMENT>'>' {
    BEGIN(INITIAL);
    return NAME_TAG;
}

<IN_ELEMENT> [(),] {
    return PUNCTUATION;
}

<IN_ELEMENT> [|?*+] {
    return OPERATOR;
}

<IN_ELEMENT> "#PCDATA" {
    return KEYWORD_TYPE;
}

// TEMPORARY/TODO:
// seulement <!DOCTYPE avec un intSubset peut être terminé avec un espace facultatif entre les caractères ']' et '>' (gérer le cas où il n'y a pas d'espace)
<INITIAL> "]" S ">" {
    mydata->in_dtd = 0;
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
    sizeof(XMLLexerData),
    NULL
};
