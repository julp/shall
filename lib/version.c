/**
 * @file lib/version.c
 * @brief version utility functions
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "vernum.h"
#include "version.h"

static const Version binary_version = { SHALL_VERSION_MAJOR, SHALL_VERSION_MINOR, SHALL_VERSION_PATCH };

SHALL_API bool version_check(const Version user_version)
{
    return 0 == memcmp(binary_version, user_version, sizeof(binary_version));
}

SHALL_API void version_get(Version version)
{
    memcpy(version, binary_version, sizeof(binary_version));
}

SHALL_API bool version_to_string(const Version version, char *buffer, size_t buffer_size)
{
    int written;

    written = snprintf(buffer, buffer_size, "%" PRIu8 "%c%" PRIu8 "%c%" PRIu8, version[0], VERSION_DELIMITER, version[1], VERSION_DELIMITER, version[2]);

    return (-1 != written && ((size_t) written) < buffer_size);
}
