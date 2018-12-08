//
// Created by yuwenyong on 17-9-12.
//

#ifndef NET4CXX_COMMON_DEFINE_H
#define NET4CXX_COMMON_DEFINE_H

#include "net4cxx/common/compilerdefs.h"

#include <cstddef>
#include <cinttypes>
#include <climits>

#define NET4CXX_LITTLEENDIAN 0
#define NET4CXX_BIGENDIAN    1

#if PLATFORM == PLATFORM_WINDOWS
#  define NET4CXX_PATH_MAX MAX_PATH
#  define _USE_MATH_DEFINES
#  ifndef DECLSPEC_NORETURN
#    define DECLSPEC_NORETURN __declspec(noreturn)
#  endif //DECLSPEC_NORETURN
#  ifndef DECLSPEC_DEPRECATED
#    define DECLSPEC_DEPRECATED __declspec(deprecated)
#  endif //DECLSPEC_DEPRECATED
#else //PLATFORM != PLATFORM_WINDOWS
#  define NET4CXX_PATH_MAX PATH_MAX
#  define DECLSPEC_NORETURN
#  define DECLSPEC_DEPRECATED
#endif //PLATFORM

#if !defined(COREDEBUG)
#  define NET4CXX_INLINE inline
#else //COREDEBUG
#  if !defined(NET4CXX_DEBUG)
#    define NET4CXX_DEBUG
#  endif //NET4CXX_DEBUG
#  define NET4CXX_INLINE
#endif //!COREDEBUG

#if COMPILER == COMPILER_GNU
#  define ATTR_NORETURN __attribute__((__noreturn__))
#  define ATTR_PRINTF(F, V) __attribute__ ((__format__ (__printf__, F, V)))
#  define ATTR_DEPRECATED __attribute__((__deprecated__))
#else //COMPILER != COMPILER_GNU
#  define ATTR_NORETURN
#  define ATTR_PRINTF(F, V)
#  define ATTR_DEPRECATED
#endif //COMPILER == COMPILER_GNU

#ifdef NET4CXX_API_USE_DYNAMIC_LINKING
#  if COMPILER == COMPILER_MICROSOFT
#    define NET4CXX_API_EXPORT __declspec(dllexport)
#    define NET4CXX_API_IMPORT __declspec(dllimport)
#  elif COMPILER == COMPILER_GNU
#    define NET4CXX_API_EXPORT __attribute__((visibility("default")))
#    define NET4CXX_API_IMPORT
#  else
#    error compiler not supported!
#  endif
#else
#  define NET4CXX_API_EXPORT
#  define NET4CXX_API_IMPORT
#endif

#ifdef NET4CXX_API_EXPORT_COMMON
#  define NET4CXX_COMMON_API NET4CXX_API_EXPORT
#else
#  define NET4CXX_COMMON_API NET4CXX_API_IMPORT
#endif

#define NS_BEGIN    namespace net4cxx {
#define NS_END      }

NS_BEGIN

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

NS_END

#ifndef NDEBUG
#	ifndef _DEBUG
#		define _DEBUG
#	endif
#	ifndef DEBUG
#		define DEBUG
#	endif
#   ifndef _GLIBCXX_DEBUG
#       define _GLIBCXX_DEBUG
#   endif
#   define NET4CXX_DEBUG
#endif


#define NET4CXX_VERSION      "2.1.0"
#define NET4CXX_VER          "net4cxx/" NET4CXX_VERSION


#endif //NET4CXX_COMMON_DEFINE_H
