#pragma once

SHALL_API const char *encoding_guess(const char *, size_t, size_t *);
SHALL_API bool encoding_utf8_check(const char *, size_t, const char **);

SHALL_API const char *encoding_stdin_get(void);
SHALL_API const char *encoding_stdout_get(void);

SHALL_API bool encoding_convert_to_utf8(const char *, const char *, size_t, char **, size_t *);
SHALL_API bool encoding_convert_from_utf8(const char *, const char *, size_t, char **, size_t *);
