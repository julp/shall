#pragma once

#include <stdbool.h>

// TODO
#define SHALL_API

#define VERSION_LENGTH 3
#define VERSION_DELIMITER '.'
#define VERSION_STRING_MAX_LENGTH 12 /* STR_SIZE("XXX.XXX.XXX") */

typedef uint8_t Version[VERSION_LENGTH];

SHALL_API void version_get(Version);
SHALL_API bool version_check(const Version);
SHALL_API bool version_to_string(const Version, char *, size_t);
