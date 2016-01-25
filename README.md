# Introduction

Shall is a syntax highlighter written in C.

Intended goals:
* self-descriptive (buid documentation and other helps from shall itself)
* lexer stacking (switch from one language to another, eg: postgresql => python (plpython) or php => html => css/js)
* realist and strict lexing (but shall is not intented to validate any code)

## Available lexers:

See README.md in lib/lexers/

## Available formatters:

See README.md in lib/formatters/

# Prerequisites

* cmake (>= 2.8.8)
* a C(99) compiler
* re2c (>= 0.13.7)
* doxygen (optionnal) (currently disabled)
* ICU or iconv (optionnal, for conversions in CLI)

# Installation

* Download sources or clone the repository
* Use a temporary directory for building (eg: `mkdir /tmp/shallbuild && cd /tmp/shallbuild`)
* Generate Makefiles: `cmake /path/to/shall/sources -DCMAKE_INSTALL_PREFIX:PATH=/usr/local` (outsource build required)
* Compile: `make`
* Install it (optionnal): `(sudo) make install`

# Usage

## CLI

shall include a command line tool with the same name to highlight directely documents.

| Option | Description |
| ------ | ----------- |
| -L | list available lexers/formatters/themes and their options |
| -l \<name> | force use of *name* lexer (default is to guess the appropriate lexer from filename else to try to guess one from file's content) |
| -o \<name>=\<value> | set lexer option *name* to *value* |
| -f \<name> | switch to formatter *name* (default is: terminal) |
| -O \<name>=\<value> | set formatter option *name* to *value* |
| -t \<name> | dump CSS to use *name* theme with the html formatter |
| -v | prints processed filename before highlighting it (usefull when you highlight few files at once - glob) |

Examples:

* `shall -L`: list lexers (and their options)
* `shall -f html -o secondary=vcl -l erb ~/cindy/varnish.vcl`: highlight in HTML the file ~/cindy/varnish.vcl as an ERB template + varnish configuration file

# Credits

* Largely inspired on pygments
* PHP lexer: PHP developpers
* PostgreSQL lexer: PostgreSQL developpers

# Current limitations

* input/output strings have to be UTF-8 encoded
* for the terminal formatter and/or shall binary, current locale have to be in UTF-8 as well
* -b option of re2c generates broken lexers?
