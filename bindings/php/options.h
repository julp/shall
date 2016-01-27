#pragma once

typedef int (*set_option_t)(void *, const char *, OptionType, OptionValue, void **);

void php_get_option(int, OptionValue *, zval *);
int php_set_option(void *, const char *, zval *, int, set_option_t TSRMLS_DC);
void php_set_options(void *, zval *, int, int (*)(void *, const char *, OptionType, OptionValue, void **) TSRMLS_DC);
