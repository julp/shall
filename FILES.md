* doc.h: main page of doxygen documentation
* common/*.[ch] files at top level: shared sources between shall library and binaries
* lib/*.c: sources that exclusively constitute the core of shall library
    * lexers/helpers.c: helper functions for lexers
    * lexers/*.{c,re}: lexers
    * formatters/*.c: formatters
* UT/ unit tests
* bin/*.c source that constitute shall binaries
* include/*.h public/installed header files
