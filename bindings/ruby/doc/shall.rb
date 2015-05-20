module Shall
    module Lexer
        # For PHP source code
        class PHP < Base ; end
        # For ERB (Ruby) templates. Use the "secondary" option to delegate tokenization of parts which are outside of ERB tags.
        class ERB < Base ; end
        # For Ruby source code
        class Ruby < Base ; end
        # Generic lexer for XML (eXtensible Markup Language)
        class XML < Base ; end
        # A lexer for DTDs (Document Type Definitions)
        class DTD < Base ; end
        # For C source code with preprocessor directives
        class C < Base ; end
        # Lexer for CMake files
        class CMake < Base ; end
        # Lexer for unified or context-style diffs or patches
        class Diff < Base ; end
        # For JSON data structures
        class JSON < Base ; end
        # Lexer for Nginx configuration files
        class Nginx < Base ; end
        # Lexer for configuration files following the Apache configuration file format (including .htaccess)
        class Apache < Base ; end
        # A lexer for Varnish configuration language
        class Varnish < Base ; end
        # Lexer for the PostgreSQL dialect of SQL
        class PostgreSQL < Base ; end
        # A "dummy" lexer that doesn't highlight anything
        class Text < Base ; end
    end
    module Formatter
        # Format tokens as a XML tree
        class XML < Base ; end
        # Format tokens as HTML 4 <span> tags within a <pre> tag
        class HTML < Base ; end
        # Format tokens with ANSI color sequences, for output in a text console
        class Terminal < Base ; end
        # Format tokens in plain text, mostly instended for tests. Each token is written on a new line with the form: <token name>: <token value>
        class Plain < Base ; end
    end
    module Token
        # end of stream
        EOS = 0
        # ignorable like spaces
        IGNORABLE = 1
        # regular text
        TEXT = 2
        # tag PI
        TAG_PREPROC = 3
        # builtin names; names that are available in the global namespace
        NAME_BUILTIN = 4
        # builtin names that are implicit
        NAME_BUILTIN_PSEUDO = 5
        # tag name
        NAME_TAG = 6
        # HTML/XML name entity
        NAME_ENTITY = 7
        # tag attribute's name
        NAME_ATTRIBUTE = 8
        # variable name
        NAME_VARIABLE = 9
        # name of a class variable
        NAME_VARIABLE_CLASS = 10
        # name of a variable instance
        NAME_VARIABLE_INSTANCE = 11
        # name of a global variable
        NAME_VARIABLE_GLOBAL = 12
        # a function name
        NAME_FUNCTION = 13
        # a class name
        NAME_CLASS = 14
        # a namespace name
        NAME_NAMESPACE = 15
        # syntax element like ';' in C
        PUNCTUATION = 16
        # 
        KEYWORD = 17
        # 
        KEYWORD_DEFAULT = 18
        # 
        KEYWORD_BUILTIN = 19
        # keyword for constants
        KEYWORD_CONSTANT = 20
        # 
        KEYWORD_DECLARATION = 21
        # 
        KEYWORD_NAMESPACE = 22
        # 
        KEYWORD_PSEUDO = 23
        # reserved keyword
        KEYWORD_RESERVED = 24
        # builtin type
        KEYWORD_TYPE = 25
        # operator
        OPERATOR = 26
        # float number
        NUMBER_FLOAT = 27
        # decimal number
        NUMBER_DECIMAL = 28
        # binary number
        NUMBER_BINARY = 29
        # octal number
        NUMBER_OCTAL = 30
        # hexadecimal number
        NUMBER_HEXADECIMAL = 31
        # comment which ends at the end of the line
        COMMENT_SINGLE = 32
        # multiline comment
        COMMENT_MULTILINE = 33
        # comment with documentation value
        COMMENT_DOCUMENTATION = 34
        # single quoted string
        STRING_SINGLE = 35
        # double quoted string
        STRING_DOUBLE = 36
        # string enclosed in backticks
        STRING_BACKTICK = 37
        # regular expression
        STRING_REGEX = 38
        # interned string
        STRING_INTERNED = 39
        # escaped sequence in string like \n, \x32, \u1234, etc
        SEQUENCE_ESCAPED = 40
        # sequence in string for interpolated variables
        SEQUENCE_INTERPOLATED = 41
        # size literals (eg: 3ko)
        LITERAL_SIZE = 42
        # duration literals (eg: 23s)
        LITERAL_DURATION = 43
        # the token value as bold
        GENERIC_STRONG = 44
        # the token value is a headline
        GENERIC_HEADING = 45
        # the token value is a subheadline
        GENERIC_SUBHEADING = 46
        # marks the token value as deleted
        GENERIC_DELETED = 47
        # marks the token value as inserted
        GENERIC_INSERTED = 48
    end
end
