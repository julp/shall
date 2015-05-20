# Installation as shared extension

## Prerequisites

* a C compiler
* having phpize tool
* having shall shared library installed

## Installation

```
cd /path/to/shall/sources/bindings/php
phpize
./configure --with-shall=/path/to/shall
make
(sudo) make install
```

Add a line `extension=shall.so` in your php.ini and restart Apache/PHP-FPM.

# Usage

## Definition

```php
<?php
namespace Shall {
    // Token types
    namespace Token {
        const EOS;
        const IGNORABLE;
        const TEXT;
        const TAG_PREPROC;
        const NAME_BUILTIN;
        const NAME_BUILTIN_PSEUDO;
        const NAME_TAG;
        const NAME_ENTITY;
        const NAME_ATTRIBUTE;
        const NAME_VARIABLE;
        const NAME_VARIABLE_CLASS;
        const NAME_VARIABLE_INSTANCE;
        const NAME_VARIABLE_GLOBAL;
        const NAME_FUNCTION;
        const NAME_CLASS;
        const NAME_NAMESPACE;
        const PUNCTUATION;
        const KEYWORD;
        const KEYWORD_DEFAULT;
        const KEYWORD_BUILTIN;
        const KEYWORD_CONSTANT;
        const KEYWORD_DECLARATION;
        const KEYWORD_NAMESPACE;
        const KEYWORD_PSEUDO;
        const KEYWORD_RESERVED;
        const KEYWORD_TYPE;
        const OPERATOR;
        const NUMBER_FLOAT;
        const NUMBER_DECIMAL;
        const NUMBER_BINARY;
        const NUMBER_OCTAL;
        const NUMBER_HEXADECIMAL;
        const COMMENT_SINGLE;
        const COMMENT_MULTILINE;
        const COMMENT_DOCUMENTATION;
        const STRING_SINGLE;
        const STRING_DOUBLE;
        const STRING_BACKTICK;
        const STRING_REGEX;
        const STRING_INTERNED;
        const ESCAPED_CHAR;
        const LITERAL_SIZE;
        const LITERAL_DURATION;
        const GENERIC_STRONG;
        const GENERIC_HEADING;
        const GENERIC_SUBHEADING;
        const GENERIC_DELETED;
        const GENERIC_INSERTED;
    }

    /**
     * Highlight the given code
     *
     * @param code the code to highlight
     * @param lexer the lexer to use for highlighting
     *
     * @return the result of syntax highlighting as a string
     **/
    string function highlight(string $code, Shall\Lexer\Base $lexer);

    /**
     * Try to find out a lexer for the given code source
     *
     * @param code the source to analyse
     * @param options an array of options to pass to the lexer
     *
     * @return the lexer (Shall\Lexer\Base object) which gives the better match (NULL if any)
     **/
    mixed function lexer_guess(string $code [, array $options ]);

    /**
     * Get a lexer from its name or one of its aliases
     *
     * @param name the name of the lexer
     * @param options an array of options to pass to the lexer
     *
     * @return the lexer (Shall\Lexer\Base object) which corresponds to the given name or NULL if none
     **/
    mixed function lexer_by_name(string $name [, array $options ]);

    /**
     * Get a lexer from a filename
     *
     * @param filename the name of the file
     * @param options an array of options to pass to the lexer
     *
     * @return the lexer (Shall\Lexer\Base object) which corresponds to the given filename or NULL if none
     **/
    mixed function lexer_for_filename(string $filename [, array $options ]);

    namespace Lexer {
        abstract class Base {
            /**
             * TODO
             **/
            public function __construct([ array $options ]);

            /**
             * TODO
             **/
            mixed public function getOption(string $name);

            /**
             * TODO
             **/
            mixed? public function setOption(string $name, mixed $value);

            string public function getName();
            array public function getAliases();
            array public function getMimeTypes();
        }

        class Apache extends Base {}
        class PHP extends Base {}
        // ...
    }

    namespace Formatter {
        abstract class Base {
            string public function startDocument();
            string public function endDocument();
            string public function startToken(int $type);
            string public function endToken(int $type);
            string public function writeToken(string $token);
        }

        class HTML extends Base {}
        class Terminal extends Base {}
        // ...
    }
}
```

## Example

```php
<?php
$code = <<<'EOS'
echo 'Hello world!';
EOS;

echo Shall\highlight(
    $code,
    new Shall\Lexer\PHP( # or Shall\lexer_by_name('php',
        array(
            'start_inline' => TRUE,
        )
    ),
    new Shall\Formatter\HTML/*( array of options )*/
);
```
