#if 0
http://pygments.org/docs/tokens/

#define TOKEN(constant, parent, description, cssclass) \
    // your job here
#include "keywords.h"
#undef TOKEN

#endif

// internal use
TOKEN(EOS, -1, "end of stream", " ")
TOKEN(IGNORABLE, -1, "ignorable like spaces", " ")
TOKEN(TEXT, -1, "regular text", " ")
// public
TOKEN(OPERATOR, -1, "operator", "o")
TOKEN(PUNCTUATION, -1, "syntax element like ';' in C", "p")

TOKEN(TAG_PREPROC, -1, "tag PI", "tp")

TOKEN(NAME, -1, "uncategorized name type", "n")
    TOKEN(NAME_BUILTIN, NAME, "builtin names; names that are available in the global namespace", "nb")
    TOKEN(NAME_BUILTIN_PSEUDO, NAME, "builtin names that are implicit (`self` in Ruby, `this` in Java)", "bp")
    TOKEN(NAME_TAG, NAME, "tag name", "nt")
    TOKEN(NAME_ENTITY, NAME, "HTML/XML name entity", "ne")
    TOKEN(NAME_ATTRIBUTE, NAME, "tag attribute's name", "na")
    TOKEN(NAME_FUNCTION, NAME, "a function name", "nf")
    TOKEN(NAME_CLASS, NAME, "a class name", "nc")
    TOKEN(NAME_NAMESPACE, NAME, "a namespace name", "nn")
    TOKEN(NAME_VARIABLE, NAME, "variable name", "v")
        TOKEN(NAME_VARIABLE_CLASS, NAME_VARIABLE, "name of a class variable", "vc")
        TOKEN(NAME_VARIABLE_INSTANCE, NAME_VARIABLE, "name of a variable instance", "vi")
        TOKEN(NAME_VARIABLE_GLOBAL, NAME_VARIABLE, "name of a global variable", "vg")

TOKEN(KEYWORD, -1, "uncategorized keyword type", "k")
    TOKEN(KEYWORD_DEFAULT, KEYWORD, "", "kd") // KD
    TOKEN(KEYWORD_BUILTIN, KEYWORD, "", "kb")
    TOKEN(KEYWORD_CONSTANT, KEYWORD, "a keyword that is constant", "kc") // TODO: is a constant (TRUE/FALSE/NULL/nil) or define a constant? (`const` in C or PHP)
    TOKEN(KEYWORD_DECLARATION, KEYWORD, "a keyword used for variable declaration (`var` or `let` in Javascript)", "kd") // KD
    TOKEN(KEYWORD_NAMESPACE, KEYWORD, "a keyword used for namespace declaration (`namespace` in PHP, `package` in Java", "kn")
    TOKEN(KEYWORD_PSEUDO, KEYWORD, "a keyword that is't really a keyword", "kp")
    TOKEN(KEYWORD_RESERVED, KEYWORD, "a reserved keyword", "kr")
    TOKEN(KEYWORD_TYPE, KEYWORD, "a builtin type (`char`, `int`, ... in C)", "kt")

TOKEN(NUMBER, -1, "uncategorized number type", "m")
    TOKEN(NUMBER_FLOAT, NUMBER, "float number", "mf")
    TOKEN(NUMBER_DECIMAL, NUMBER, "decimal number", "md")
    TOKEN(NUMBER_BINARY, NUMBER, "binary number", "mb")
    TOKEN(NUMBER_OCTAL, NUMBER, "octal number", "mo")
    TOKEN(NUMBER_HEXADECIMAL, NUMBER, "hexadecimal number", "mh")

TOKEN(COMMENT, -1, "uncategorized comment type", "c")
    TOKEN(COMMENT_SINGLE, COMMENT, "comment which ends at the end of the line", "cs")
    TOKEN(COMMENT_MULTILINE, COMMENT, "multiline comment", "cm")
    TOKEN(COMMENT_DOCUMENTATION, COMMENT, "comment with documentation value", "cd")

TOKEN(STRING, -1, "uncategorized string type", "s")
    // TODO: rename into STRING_STRICT (no interpolation, no escaped char)?
    TOKEN(STRING_SINGLE, -1, "single quoted string", "ss")
    // TODO: rename into "interpolated string"?
    TOKEN(STRING_DOUBLE, -1, "double quoted string", "sd")
    // TODO: rename into "execution string" (strings run into a subshell)? (but for consistency we should have to distinguish with and without interpolation)
    TOKEN(STRING_BACKTICK, -1, "string enclosed in backticks", "sb")
    TOKEN(STRING_REGEX, -1, "regular expression", "sr")
    TOKEN(STRING_INTERNED, -1, "interned string", "si")

#define ESCAPED_CHAR SEQUENCE_ESCAPED
TOKEN(SEQUENCE_ESCAPED, -1, "escaped sequence in string like \\n, \\x32, \\u1234, etc", "es")
TOKEN(SEQUENCE_INTERPOLATED, -1, "sequence in string for interpolated variables", "is")

TOKEN(LITERAL, -1, "uncategorized literal type", "l")
    TOKEN(LITERAL_SIZE, LITERAL, "size literals (eg: 3ko)", "ls")
    TOKEN(LITERAL_DURATION, LITERAL, "duration literals (eg: 23s)", "ld")

TOKEN(GENERIC, -1, "", "g")
    TOKEN(GENERIC_STRONG, GENERIC, "the token value as bold", "gb")
    TOKEN(GENERIC_HEADING, GENERIC, "the token value is a headline", "gh")
    TOKEN(GENERIC_SUBHEADING, GENERIC, "the token value is a subheadline", "gs")
    TOKEN(GENERIC_DELETED, GENERIC, "marks the token value as deleted", "gd")
    TOKEN(GENERIC_INSERTED, GENERIC, "marks the token value as inserted", "gi")
