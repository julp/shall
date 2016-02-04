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
* doxygen (optionnal)
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
| -c | chain the following lexer (-l) with the previous one (eg: -l erb -cl php -cl xml to highlight a code mixing ERB, PHP and XML) |

Examples:

* `shall -L`: list lexers (and their options)
* `shall -f html -o secondary=vcl -l erb ~/cindy/varnish.vcl` or `shall -f html -l erb -cl varnish ~/cindy/varnish.vcl`: highlight in HTML the file ~/cindy/varnish.vcl as an ERB template + varnish configuration file

# Credits

* Largely inspired on pygments (themes, terminal formatter: conversion 24-bit color => 256)
* PHP lexer: PHP developpers
* PostgreSQL lexer: PostgreSQL developpers

# Current limitations/Known issues

* re2c: -b option generates broken lexers?
* input/output strings have to be UTF-8 encoded
* formatters: there are leaking (for now) if a same formatter is used to highlight more than one source
* themes: style hashing (recognize that 2 styles are the same) requires a 64-bit system (results may be wrong on 32-bit system)
