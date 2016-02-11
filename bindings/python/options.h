#pragma once

typedef int (*set_option_t)(void *, const char *, OptionType, OptionValue, void **);

void python_get_option(int, OptionValue *, PyObject **);
void python_set_options(void *, PyObject *, int, set_option_t);
int formatter_set_option_compat_cb(void *, const char *, OptionType, OptionValue, void **);
int python_set_option(void *, const char *, PyObject *, int, int (*)(void *, const char *, OptionType, OptionValue, void **));
