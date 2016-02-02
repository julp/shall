#pragma once

#ifdef __GNUC__
# define GCC_VERSION (__GNUC__ * 1000 + __GNUC_MINOR__)
#else
# define GCC_VERSION 0
#endif /* __GNUC__ */

#ifndef __has_attribute
# define __has_attribute(x) 0
#endif /* !__has_attribute */

#if GCC_VERSION || __has_attribute(aligned)
# define ALIGNED(x) __attribute__((aligned(x)))
#else
# define ALIGNED(x)
#endif /* ALIGNED */

#if GCC_VERSION || __has_attribute(deprecated)
# define DEPRECATED __attribute__((deprecated))
#else
# define DEPRECATED
#endif /* DEPRECATED */

#if GCC_VERSION || __has_attribute(sentinel)
# define SENTINEL __attribute__((sentinel))
#else
# define SENTINEL
#endif /* SENTINEL */

#if GCC_VERSION >= 2003 || __has_attribute(format)
# define FORMAT(archetype, string_index, first_to_check) __attribute__((format(archetype, string_index, first_to_check)))
# define PRINTF(string_index, first_to_check) FORMAT(__printf__, string_index, first_to_check)
#else
# define FORMAT(archetype, string_index, first_to_check)
# define PRINTF(string_index, first_to_check)
#endif /* FORMAT,PRINTF */

#if GCC_VERSION >= 3004 || __has_attribute(warn_unused_result)
# define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
# define WARN_UNUSED_RESULT
#endif /* WARN_UNUSED_RESULT */

#ifndef SHALL_API
# define SHALL_API
#endif /* !SHALL_API */
