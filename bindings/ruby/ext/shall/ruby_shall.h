#pragma once

#include <ruby.h>
#include <shall/shall.h>
#include <shall/formatter.h>
#include <shall/tokens.h>

#define DEBUG
#define WITH_TYPED_DATA 1

#ifdef DEBUG
# include <stdio.h>
# define debug(format, ...) \
    fprintf(stderr, format "\n", ## __VA_ARGS__)
#else
# define debug(format, ...) \
    /* NOP */
#endif /* DEBUG */

VALUE mShall;

// ruby_helpers.c
void rb_set_options(VALUE, VALUE);
int rb_set_option(VALUE, VALUE, VALUE);
VALUE rb_get_option(int, OptionValue *);
void ary_push_string_cb(const char *, void *);
