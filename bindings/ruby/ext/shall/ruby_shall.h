#pragma once

#include <ruby.h>
#include <shall/cpp.h>
#include <shall/formatter.h>
#include <shall/shall.h>
#include <shall/tokens.h>
#include <shall/types.h>

#define DEBUG
#define WITH_TYPED_DATA 1

VALUE mShall;

// ruby_helpers.c
void rb_set_options(VALUE, VALUE);
int rb_set_option(VALUE, VALUE, VALUE);
VALUE rb_get_option(int, OptionValue *);
void ary_push_string_cb(const char *, void *);
