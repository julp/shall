# Introduction

Shall is not intented to validate any code.

## Available lexers:

See README in lexers/ subfolder

## Available formatters:

See README in formatters/ subfolder

# Prerequisites

* cmake (>= 2.8.8)
* a C compiler
* re2c (>= 0.13.7)
* doxygen (optionnal)

# Installation

* Download, extract and move to shall sources directory
* Use a temporary directory for building (eg: `mkdir /tmp/shallbuild && cd /tmp/shallbuild`)
* Generate Makefiles: `cmake /path/to/shall/sources -DCMAKE_INSTALL_PREFIX:PATH=/usr/local` (outsource build required)
* Compile: `make`
* Install it: `(sudo) make install`

# Usage

## CLI

shall include a command line tool with the same name to highlight directely documents.

| Option | Description |
| ------ | ----------- |
| -F | list available lexers |
| -l \<name> | force use of *name* lexer (default is to guess the appropriate lexer from filename else to try to guess one from file's content) |
| -o \<name>=\<value> | set lexer option *name* to *value* |
| -f \<name> | switch to formatter *name* (default is: terminal) |
| -O \<name>=\<value> | set formatter option *name* to *value* |

Examples:

* `shall -F`: list lexers (and their options)
* `shall -f html -l erb -o secondary=vcl ~/cindy/varnish.vcl`: highlight in HTML the file ~/cindy/varnish.vcl as an ERB template + varnish configuration file

# Credits

* Largely inspired on: pygments
* PHP lexer : the PHP developpers
* PostgreSQL lexer : the PostgreSQL developpers

# Current limitations

* input/output strings have to be UTF-8 encoded
* for the terminal formatter and/or shall binary, current locale have to be in UTF-8 as well
* -b option of re2c generates broken lexers?
