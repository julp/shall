#include <string.h>

#include "shall.h"

static const Version binary_version = { SHALL_VERSION_MAJOR, SHALL_VERSION_MINOR, SHALL_VERSION_PATCH };

SHALL_API bool shall_version_check(const Version user_version)
{
    return 0 == memcmp(binary_version, user_version, sizeof(binary_version));
}

SHALL_API void shall_version_get(Version version)
{
    memcpy(version, binary_version, sizeof(binary_version));
}
