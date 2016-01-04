#if 0
http://pygments.org/docs/tokens/

#define TOKEN(constant, description, cssclass) \
    // your job here
#include "keywords.h"
#undef TOKEN

#endif

// internal use
TOKEN(EOS, "end of stream", " ")
TOKEN(IGNORABLE, "ignorable like spaces", " ")
TOKEN(TEXT, "regular text", " ")
// public
TOKEN(TAG_PREPROC, "tag PI", "tp")
TOKEN(NAME_BUILTIN, "builtin names; names that are available in the global namespace", "nb")
TOKEN(NAME_BUILTIN_PSEUDO, "builtin names that are implicit", "bp")
TOKEN(NAME_TAG, "tag name", "nt")
TOKEN(NAME_ENTITY, "HTML/XML name entity", "ne")
TOKEN(NAME_ATTRIBUTE, "tag attribute's name", "na")
TOKEN(NAME_VARIABLE, "variable name", "v")
TOKEN(NAME_VARIABLE_CLASS, "name of a class variable", "vc")
TOKEN(NAME_VARIABLE_INSTANCE, "name of a variable instance", "vi")
TOKEN(NAME_VARIABLE_GLOBAL, "name of a global variable", "vg")
TOKEN(NAME_FUNCTION, "a function name", "nf")
TOKEN(NAME_CLASS, "a class name", "nc")
TOKEN(NAME_NAMESPACE, "a namespace name", "nn")
TOKEN(PUNCTUATION, "syntax element like ';' in C", "p")
TOKEN(KEYWORD, "", "k")
TOKEN(KEYWORD_DEFAULT, "", "kd") // KD
TOKEN(KEYWORD_BUILTIN, "", "kb")
TOKEN(KEYWORD_CONSTANT, "keyword for constants", "kc")
TOKEN(KEYWORD_DECLARATION, "", "kd") // KD
TOKEN(KEYWORD_NAMESPACE, "", "kn")
TOKEN(KEYWORD_PSEUDO, "", "kp")
TOKEN(KEYWORD_RESERVED, "reserved keyword", "kr")
TOKEN(KEYWORD_TYPE, "builtin type", "kt")
TOKEN(OPERATOR, "operator", "o")
TOKEN(NUMBER_FLOAT, "float number", "mf")
TOKEN(NUMBER_DECIMAL, "decimal number", "md")
TOKEN(NUMBER_BINARY, "binary number", "mb")
TOKEN(NUMBER_OCTAL, "octal number", "mo")
TOKEN(NUMBER_HEXADECIMAL, "hexadecimal number", "mh")
TOKEN(COMMENT_SINGLE, "comment which ends at the end of the line", "cs")
TOKEN(COMMENT_MULTILINE, "multiline comment", "cm")
TOKEN(COMMENT_DOCUMENTATION, "comment with documentation value", "cd")
TOKEN(STRING_SINGLE, "single quoted string", "ss") // TODO: rename into "strict string" (no interpolation, no escaped char)?
TOKEN(STRING_DOUBLE, "double quoted string", "sd") // TODO: rename into "interpolated string"?
TOKEN(STRING_BACKTICK, "string enclosed in backticks", "sb") // TODO: rename into "execution string" (strings run into a subshell)?
TOKEN(STRING_REGEX, "regular expression", "sr")
TOKEN(STRING_INTERNED, "interned string", "si")
#define ESCAPED_CHAR SEQUENCE_ESCAPED
TOKEN(SEQUENCE_ESCAPED, "escaped sequence in string like \\n, \\x32, \\u1234, etc", "es")
TOKEN(SEQUENCE_INTERPOLATED, "sequence in string for interpolated variables", "is")
TOKEN(LITERAL_SIZE, "size literals (eg: 3ko)", "ls")
TOKEN(LITERAL_DURATION, "duration literals (eg: 23s)", "ld")

TOKEN(GENERIC_STRONG, "the token value as bold", "gb")
TOKEN(GENERIC_HEADING, "the token value is a headline", "gh")
TOKEN(GENERIC_SUBHEADING, "the token value is a subheadline", "gs")
TOKEN(GENERIC_DELETED, "marks the token value as deleted", "gd")
TOKEN(GENERIC_INSERTED, "marks the token value as inserted", "gi")

#if 0
/* Coloration syntaxique */
.hll { background-color: #ffffcc }
.c { color: #60a0b0; font-style: italic } /* Comment */
.k { color: #007020; font-weight: bold } /* Keyword */
.o { color: #666666 } /* Operator */
.cm { color: #60a0b0; font-style: italic } /* Comment.Multiline */
.cp { color: #007020 } /* Comment.Preproc */
.c1 { color: #60a0b0; font-style: italic } /* Comment.Single */
.cs { color: #60a0b0; background-color: #fff0f0 } /* Comment.Special */
.gd { color: #A00000 } /* Generic.Deleted */
.ge { font-style: italic } /* Generic.Emph */
.gr { color: #FF0000 } /* Generic.Error */
.gh { color: #000080; font-weight: bold } /* Generic.Heading */
.gi { color: #00A000 } /* Generic.Inserted */
.go { color: #808080 } /* Generic.Output */
.gp { color: #c65d09; font-weight: bold } /* Generic.Prompt */
.gs { font-weight: bold } /* Generic.Strong */
.gu { color: #800080; font-weight: bold } /* Generic.Subheading */
.gt { color: #0040D0 } /* Generic.Traceback */
.kc { color: #007020; font-weight: bold } /* Keyword.Constant */
.kd { color: #007020; font-weight: bold } /* Keyword.Declaration */
.kn { color: #007020; font-weight: bold } /* Keyword.Namespace */
.kp { color: #007020 } /* Keyword.Pseudo */
.kr { color: #007020; font-weight: bold } /* Keyword.Reserved */
.kt { color: #902000 } /* Keyword.Type */
.m { color: #40a070 } /* Literal.Number */
.s { color: #4070a0 } /* Literal.String */
.na { color: #4070a0 } /* Name.Attribute */
.nb { color: #007020 } /* Name.Builtin */
.nc { color: #0e84b5; font-weight: bold } /* Name.Class */
.no { color: #60add5 } /* Name.Constant */
.nd { color: #555555; font-weight: bold } /* Name.Decorator */
.ni { color: #d55537; font-weight: bold } /* Name.Entity */
.ne { color: #007020 } /* Name.Exception */
.nf { color: #06287e } /* Name.Function */
.nl { color: #002070; font-weight: bold } /* Name.Label */
.nn { color: #0e84b5; font-weight: bold } /* Name.Namespace */
.nt { color: #062873; font-weight: bold } /* Name.Tag */
.nv { color: #bb60d5 } /* Name.Variable */
.ow { color: #007020; font-weight: bold } /* Operator.Word */
.w { color: #bbbbbb } /* Text.Whitespace */
.mf { color: #40a070 } /* Literal.Number.Float */
.mh { color: #40a070 } /* Literal.Number.Hex */
.mi { color: #40a070 } /* Literal.Number.Integer */
.mo { color: #40a070 } /* Literal.Number.Oct */
.sb { color: #4070a0 } /* Literal.String.Backtick */
.sc { color: #4070a0 } /* Literal.String.Char */
.sd { color: #4070a0; font-style: italic } /* Literal.String.Doc */
.s2 { color: #4070a0 } /* Literal.String.Double */
.se { color: #4070a0; font-weight: bold } /* Literal.String.Escape */
.sh { color: #4070a0 } /* Literal.String.Heredoc */
.si { color: #70a0d0; font-style: italic } /* Literal.String.Interpol */
.sx { color: #c65d09 } /* Literal.String.Other */
.sr { color: #235388 } /* Literal.String.Regex */
.s1 { color: #4070a0 } /* Literal.String.Single */
.ss { color: #517918 } /* Literal.String.Symbol */
.bp { color: #007020 } /* Name.Builtin.Pseudo */
.vc { color: #bb60d5 } /* Name.Variable.Class */
.vg { color: #bb60d5 } /* Name.Variable.Global */
.vi { color: #bb60d5 } /* Name.Variable.Instance */
.il { color: #40a070 } /* Literal.Number.Integer.Long */
#endif
