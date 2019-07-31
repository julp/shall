/**
 * Spec/language reference:
 * - https://www.gnu.org/software/bash/manual/bashref.html
 */

#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "tokens.h"
#include "lexer.h"

enum {
    STATE(INITIAL),
    STATE(IN_ANSI_C_QUOTED_STRING),
    STATE(IN_DOUBLE_QUOTED_STRING),
    // $((
    STATE(IN_COMMAND_SUBSTITUTION),
    // [[ ?
};

static int default_token_type[] = {
    [ STATE(INITIAL) ] = IGNORABLE,
    [ STATE(IN_ANSI_C_QUOTED_STRING) ] = STRING_DOUBLE,
    [ STATE(IN_DOUBLE_QUOTED_STRING) ] = STRING_DOUBLE,
    [ STATE(IN_COMMAND_SUBSTITUTION) ] = STRING_DOUBLE,
};

static int yylex(YYLEX_ARGS)
{
    (void) ctxt;
    (void) data;
    (void) options;

    while (YYCURSOR < YYLIMIT) {
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

newline = [\n];
default_ifs = [ \t\n];
name = [a-zA-Z_][a-zA-Z_0-9]*;
blank = [ \t];
octdigit = [0-7];
hexdigit = [0-9a-fA-F];
metacharacter = [ \t|&;()<>]; // blank | [|&;()<>]
//token = word | operator;
control_operator = "|&" | [;|&]{1,2} | [()];
redirection_operator = [0-9]* "&"? ("<" ">"? | ">" [>|]?) "&"? [0-9]* "-"?;
operator = control_operator | redirection_operator;

<INITIAL> "$'" {
    BEGIN(IN_ANSI_C_QUOTED_STRING);
    TOKEN(STRING_DOUBLE);
}

<INITIAL> "'" {
    YYCTYPE *end;

    //++YYCURSOR;
    if (NULL == (end = memchr(YYCURSOR, '\'', YYLIMIT - YYCURSOR))) {
        YYCURSOR = YYLIMIT;
        DONE();
    } else {
        YYCURSOR = end;
        TOKEN(STRING_SINGLE);
    }
}

<INITIAL> "#" [^\n]* {
    TOKEN(COMMENT_SINGLE);
}

<INITIAL> "$" ([*@#?\-$!0_] | [1-9] | name) {
    TOKEN(NAME_VARIABLE);
}

<IN_ANSI_C_QUOTED_STRING> "\\" ([abeEfnrtv\\'"] | octdigit {1,3} | "x" hexdigit{1,2} | "u" hexdigit{1,4} | "U" hexdigit{1,8} | "c" .) {
    TOKEN(ESCAPED_CHAR);
}

<IN_ANSI_C_QUOTED_STRING> "'" {
    BEGIN(INITIAL);
    TOKEN(STRING_DOUBLE);
}

<INITIAL> [!{}[\]] {
    TOKEN(PUNCTUATION);
}

<INITIAL> "." | ":" | "[" | "alias" | "bg" | "break" | "builtin" | "caller" | "cd" | "command" | "compgen" | "complete" | "compopt" | "continue" | "declare" | "dirs" | "disown" | "echo" | "enable" | "eval" | "exec" | "exit" | "export" | "fc" | "fg" | "getopts" | "hash" | "help" | "history" | "jobs" | "kill" | "let" | "local" | "logout" | "mapfile" | "popd" | "printf" | "pushd" | "pwd" | "read" | "readarray" | "readonly" | "return" | "set" | "shift" | "shopt" | "source" | "suspend" | "test" | "times" | "trap" | "type" | "typeset" | "ulimit" | "umask" | "unalias" | "unset" | "wait" {
    TOKEN(NAME_BUILTIN);
}

<INITIAL> "case" | "do" | "done" | "elif" | "else" | "esac" | "fi" | "for" | "function" | "if" | "in" | "select" | "then" | "time" | "until" | "while" {
    TOKEN(KEYWORD);
}

<INITIAL> '"' {
    BEGIN(IN_DOUBLE_QUOTED_STRING);
    TOKEN(STRING_DOUBLE);
}

// TODO: newline?
// TODO: '!' only when history expansion is enabled
<IN_DOUBLE_QUOTED_STRING> '\\' [$`"\\!] {
    TOKEN(ESCAPED_CHAR);
}

<INITIAL,IN_DOUBLE_QUOTED_STRING,IN_COMMAND_SUBSTITUTION> "`" {
    PUSH_STATE(IN_COMMAND_SUBSTITUTION);
    TOKEN(STRING_DOUBLE);
}

<INITIAL,IN_DOUBLE_QUOTED_STRING,IN_COMMAND_SUBSTITUTION> "$(" {
    PUSH_STATE(IN_COMMAND_SUBSTITUTION);
    TOKEN(STRING_DOUBLE);
}

<IN_COMMAND_SUBSTITUTION>"`" {
    POP_STATE();
    TOKEN(STRING_DOUBLE);
}

<IN_COMMAND_SUBSTITUTION>")" {
    POP_STATE();
    TOKEN(STRING_DOUBLE);
}

<IN_DOUBLE_QUOTED_STRING> '"' {
    BEGIN(INITIAL);
    TOKEN(STRING_DOUBLE);
}

<INITIAL> ([^ \n\t|&;()<>$#"'`] | "\\" metacharacter)+ {
    TOKEN(STRING_SINGLE);
}

<INITIAL> (metacharacter | newline) + {
    TOKEN(IGNORABLE);
}

<*> [^] {
    TOKEN(default_token_type[YYSTATE]);
}
*/
    }
    DONE();
}

LexerImplementation bash_lexer = {
    "Bash",
    "Lexer for (ba|k|)sh shell scripts.",
    (const char * const []) { "shell", "sh", /*"ksh",*/ NULL },
    (const char * const []) { "*.sh", /*"*.ksh", */"*.bash", "*.ebuild", "*.eclass", ".bashrc", "bashrc", ".bash_*", "bash_*", "PKGBUILD", NULL },
    (const char * const []) { "application/x-sh", "application/x-shellscript", NULL },
    (const char * const []) { "bash", "sh", /*"ksh", */NULL },
    NULL, // analyze
    NULL, // init
    yylex,
    NULL, // finalize
    sizeof(LexerData),
    NULL, // options
    NULL, // dependencies
    NULL, // yypush_parse
    NULL, // yypstate_new
    NULL, // yypstate_delete
};
