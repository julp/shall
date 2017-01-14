module Shall
    module Lexer
        # Lexer for configuration files following the Apache configuration file format (including .htaccess)
        class Apache < Base ; end
        # Lexer for (ba|k|)sh shell scripts.
        class Bash < Base ; end
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
        # For EEX (Elixir) templates. Use the "secondary" option to delegate tokenization of parts which are outside of EEX tags.
        class EEX < Base ; end
        # For the Elixir programming language (elixir-lang.org)
        class Elixir < Base ; end
        # For ERB (Ruby) templates. Use the "secondary" option to delegate tokenization of parts which are outside of ERB tags.
        class ERB < Base ; end
        # For the Google Go programming language (golang.org)
        class Go < Base ; end
        # XXX
        class HTML < Base ; end
        # TODO
        class Javascript < Base ; end
        # For JSON data structures
        class JSON < Base ; end
        # For the Lua programming language (lua.org)
        class Lua < Base ; end
        # Lexer for the MySQL dialect of SQL
        class MySQL < Base ; end
        # Lexer for Nginx configuration files
        class Nginx < Base ; end
        # For PHP source code
        class PHP < Base ; end
        # Lexer for the PostgreSQL dialect of SQL
        class PostgreSQL < Base ; end
        # For the Python programming language (python.org)
        class Python < Base ; end
        # For Ruby source code
        class Ruby < Base ; end
        # A "dummy" lexer that doesn't highlight anything
        class Text < Base ; end
        # Twig template engine
        class Twig < Base ; end
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
        # an invalid part
        ERROR = 3
        # operator
        OPERATOR = 4
        # syntax element like ';' in C
        PUNCTUATION = 5
        # tag PI
        TAG_PREPROC = 6
        # uncategorized name type
        NAME = 7
        # builtin names; names that are available in the global namespace
        NAME_BUILTIN = 8
        # builtin names that are implicit (`self` in Ruby, `this` in Java)
        NAME_BUILTIN_PSEUDO = 9
        # tag name
        NAME_TAG = 10
        # HTML/XML name entity
        NAME_ENTITY = 11
        # tag attribute's name
        NAME_ATTRIBUTE = 12
        # a function name
        NAME_FUNCTION = 13
        # a class name
        NAME_CLASS = 14
        # a namespace name
        NAME_NAMESPACE = 15
        # variable name
        NAME_VARIABLE = 16
        # name of a class variable
        NAME_VARIABLE_CLASS = 17
        # name of a global variable
        NAME_VARIABLE_GLOBAL = 18
        # name of a variable instance
        NAME_VARIABLE_INSTANCE = 19
        # uncategorized keyword type
        KEYWORD = 20
        # 
        KEYWORD_DEFAULT = 21
        # 
        KEYWORD_BUILTIN = 22
        # a keyword that is constant
        KEYWORD_CONSTANT = 23
        # a keyword used for variable declaration (`var` or `let` in Javascript)
        KEYWORD_DECLARATION = 24
        # a keyword used for namespace declaration (`namespace` in PHP, `package` in Java
        KEYWORD_NAMESPACE = 25
        # a keyword that is't really a keyword
        KEYWORD_PSEUDO = 26
        # a reserved keyword
        KEYWORD_RESERVED = 27
        # a builtin type (`char`, `int`, ... in C)
        KEYWORD_TYPE = 28
        # uncategorized number type
        NUMBER = 29
        # float number
        NUMBER_FLOAT = 30
        # decimal number
        NUMBER_DECIMAL = 31
        # imaginary number
        NUMBER_IMAGINARY = 32
        # binary number
        NUMBER_BINARY = 33
        # octal number
        NUMBER_OCTAL = 34
        # hexadecimal number
        NUMBER_HEXADECIMAL = 35
        # uncategorized comment type
        COMMENT = 36
        # comment which ends at the end of the line
        COMMENT_SINGLE = 37
        # multiline comment
        COMMENT_MULTILINE = 38
        # comment with documentation value
        COMMENT_DOCUMENTATION = 39
        # uncategorized string type
        STRING = 40
        # single quoted string
        STRING_SINGLE = 41
        # double quoted string
        STRING_DOUBLE = 42
        # string enclosed in backticks
        STRING_BACKTICK = 43
        # regular expression
        STRING_REGEX = 44
        # interned string
        STRING_INTERNED = 45
        # escaped sequence in string like \n, \x32, \u1234, etc
        SEQUENCE_ESCAPED = 46
        # sequence in string for interpolated variables
        SEQUENCE_INTERPOLATED = 47
        # uncategorized literal type
        LITERAL = 48
        # size literals (eg: 3ko)
        LITERAL_SIZE = 49
        # duration literals (eg: 23s)
        LITERAL_DURATION = 50
        # 
        GENERIC = 51
        # the token value as bold
        GENERIC_STRONG = 52
        # the token value is a headline
        GENERIC_HEADING = 53
        # the token value is a subheadline
        GENERIC_SUBHEADING = 54
        # marks the token value as deleted
        GENERIC_DELETED = 55
        # marks the token value as inserted
        GENERIC_INSERTED = 56
    end
end
