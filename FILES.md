* doc.h: main page of doxygen documentation
* all other *.[ch] files at top level: shared sources between shall library and binaries
* lib/: sources that exclusively constitute shall library
    * lexer.c: helper functions for lexers
    * lexers/*.{c,re}: lexers
    * formatters/*.c: formatters
* UT/ unit tests
* bin/*.c source that constitute shall binaries
* shall/*.h public/installed header files
