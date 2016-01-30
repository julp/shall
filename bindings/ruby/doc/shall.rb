module Shall
    module Lexer
        # Lexer for configuration files following the Apache configuration file format (including .htaccess)
        class Apache < Base ; end
        # For C source code with preprocessor directives
        class C < Base ; end
        # Lexer for CMake files
        class CMake < Base ; end
        # For CSS (Cascading Style Sheets)
        class CSS < Base ; end
        # Lexer for unified or context-style diffs or patches
        class Diff < Base ; end
        # A lexer for DTDs (Document Type Definitions)
        class DTD < Base ; end
        # For ERB (Ruby) templates. Use the "secondary" option to delegate tokenization of parts which are outside of ERB tags.
        class ERB < Base ; end
        # XXX
        class HTML < Base ; end
        # TODO
        class Javascript < Base ; end
        # For JSON data structures
        class JSON < Base ; end
        # Lexer for Nginx configuration files
        class Nginx < Base ; end
        # For PHP source code
        class PHP < Base ; end
        # Lexer for the PostgreSQL dialect of SQL
        class PostgreSQL < Base ; end
        # For Ruby source code
        class Ruby < Base ; end
        # A "dummy" lexer that doesn't highlight anything
        class Text < Base ; end
        # A lexer for Varnish configuration language
        class Varnish < Base ; end
        # Generic lexer for XML (eXtensible Markup Language)
        class XML < Base ; end
    end
    module Formatter
        # Format tokens for forums using bbcode syntax to format post
        class BBCode < Base ; end
        # Format tokens as HTML <span> tags within a <pre> tag
        class HTML < Base ; end
        # Format tokens for forums using bbcode syntax to format post
        class RTF < Base ; end
        # Format tokens with ANSI color sequences, for output in a text console
        class Terminal < Base ; end
    end
    module Token
        # end of stream
        EOS = 0
        # ignorable like spaces
        IGNORABLE = 1
        # regular text
        TEXT = 2
        # operator
        OPERATOR = 3
        # syntax element like ';' in C
        PUNCTUATION = 4
        # tag PI
        TAG_PREPROC = 5
        # uncategorized name type
        NAME = 6
        # builtin names; names that are available in the global namespace
        NAME_BUILTIN = 7
        # builtin names that are implicit (`self` in Ruby, `this` in Java)
        NAME_BUILTIN_PSEUDO = 8
        # tag name
        NAME_TAG = 9
        # HTML/XML name entity
        NAME_ENTITY = 10
        # tag attribute's name
        NAME_ATTRIBUTE = 11
        # a function name
        NAME_FUNCTION = 12
        # a class name
        NAME_CLASS = 13
        # a namespace name
        NAME_NAMESPACE = 14
        # variable name
        NAME_VARIABLE = 15
        # name of a class variable
        NAME_VARIABLE_CLASS = 16
        # name of a variable instance
        NAME_VARIABLE_INSTANCE = 17
        # name of a global variable
        NAME_VARIABLE_GLOBAL = 18
        # uncategorized keyword type
        KEYWORD = 19
        # 
        KEYWORD_DEFAULT = 20
        # 
        KEYWORD_BUILTIN = 21
        # a keyword that is constant
        KEYWORD_CONSTANT = 22
        # a keyword used for variable declaration (`var` or `let` in Javascript)
        KEYWORD_DECLARATION = 23
        # a keyword used for namespace declaration (`namespace` in PHP, `package` in Java
        KEYWORD_NAMESPACE = 24
        # a keyword that is't really a keyword
        KEYWORD_PSEUDO = 25
        # a reserved keyword
        KEYWORD_RESERVED = 26
        # a builtin type (`char`, `int`, ... in C)
        KEYWORD_TYPE = 27
        # uncategorized number type
        NUMBER = 28
        # float number
        NUMBER_FLOAT = 29
        # decimal number
        NUMBER_DECIMAL = 30
        # binary number
        NUMBER_BINARY = 31
        # octal number
        NUMBER_OCTAL = 32
        # hexadecimal number
        NUMBER_HEXADECIMAL = 33
        # uncategorized comment type
        COMMENT = 34
        # comment which ends at the end of the line
        COMMENT_SINGLE = 35
        # multiline comment
        COMMENT_MULTILINE = 36
        # comment with documentation value
        COMMENT_DOCUMENTATION = 37
        # uncategorized string type
        STRING = 38
        # single quoted string
        STRING_SINGLE = 39
        # double quoted string
        STRING_DOUBLE = 40
        # string enclosed in backticks
        STRING_BACKTICK = 41
        # regular expression
        STRING_REGEX = 42
        # interned string
        STRING_INTERNED = 43
        # escaped sequence in string like \n, \x32, \u1234, etc
        SEQUENCE_ESCAPED = 44
        # sequence in string for interpolated variables
        SEQUENCE_INTERPOLATED = 45
        # uncategorized literal type
        LITERAL = 46
        # size literals (eg: 3ko)
        LITERAL_SIZE = 47
        # duration literals (eg: 23s)
        LITERAL_DURATION = 48
        # 
        GENERIC = 49
        # the token value as bold
        GENERIC_STRONG = 50
        # the token value is a headline
        GENERIC_HEADING = 51
        # the token value is a subheadline
        GENERIC_SUBHEADING = 52
        # marks the token value as deleted
        GENERIC_DELETED = 53
        # marks the token value as inserted
        GENERIC_INSERTED = 54
    end
end
