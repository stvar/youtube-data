// Copyright (C) 2016, 2017, 2018  Stefan Vargyas
// 
// This file is part of Json-Type.
// 
// Json-Type is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Json-Type is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Json-Type.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __INT_TRAITS_H
#define __INT_TRAITS_H

#include "config.h"

#include <stdint.h>
#include <limits.h>

#define TYPEOF(p) \
    typeof(p)
#define TYPES_COMPATIBLE(t, u) \
    (__builtin_types_compatible_p(t, u))
#define TYPEOF_IS(p, t) \
    TYPES_COMPATIBLE(TYPEOF(p), t)

#define IS_CONSTANT(x) \
    (__builtin_constant_p(x))

#define CONST_CAST(p, t)                \
    (                                   \
        STATIC(TYPEOF_IS(p, t const*)), \
        (t*) (p)                        \
    )

// stev: TODO: review all TYPE_IS_xxx macros below
// and make use of __builtin_types_compatible_p

#define TYPE_IS_INTEGER(t) ((t) 1.5 == 1)

#define TYPE_IS_UNSIGNED(t) ((t) 0 < (t) -1)

#define TYPE_IS_SIGNED(t) (!TYPE_IS_UNSIGNED(t))

#define TYPE_IS_UNSIGNED_INT(t) \
    (TYPE_IS_INTEGER(t) && TYPE_IS_UNSIGNED(t))

#define TYPE_IS_SIGNED_INT(t) \
    (TYPE_IS_INTEGER(t) && TYPE_IS_SIGNED(t))

#define TYPE_UNSIGNED_INT_MAX_(t) ((t) -1)

#define TYPE_UNSIGNED_INT_MAX(t)         \
    (                                    \
        STATIC(TYPE_IS_UNSIGNED_INT(t)), \
        TYPE_UNSIGNED_INT_MAX_(t)        \
    )

#define TYPE_SIGNED_INT_MAX_(t) \
    ((((t) 1 << (sizeof(t) * CHAR_BIT - 2)) - 1) * 2 + 1)

#define TYPE_SIGNED_INT_MAX(t)         \
    (                                  \
        STATIC(TYPE_IS_SIGNED_INT(t)), \
        TYPE_SIGNED_INT_MAX_(t)        \
    )

#define TYPE_INTEGER_MAX_(t)            \
    (                                   \
        TYPE_IS_UNSIGNED(t)             \
            ? TYPE_UNSIGNED_INT_MAX_(t) \
            : TYPE_SIGNED_INT_MAX_(t)   \
    )

#define TYPE_INTEGER_MAX(t)         \
    (                               \
        STATIC(TYPE_IS_INTEGER(t)), \
        TYPE_INTEGER_MAX_(t)        \
    )

#define TYPE_DIGITS(t)                           \
    (                                            \
        sizeof(t) * CHAR_BIT - TYPE_IS_SIGNED(t) \
    )

// 643/2136 is log10(2) for the most 7 significant digits -- see, e.g.:
//
// Terence Jackson and Keith Matthews:
// On Shanks’ algorithm for computing the continued fraction of logb a
// Journal of Integer Sequences 5 (2002), Article 02.2.7
// http://www.numbertheory.org/pdfs/log.pdf

#define TYPE_DIGITS10(t) (TYPE_DIGITS(t) * 643 / 2136)

#define TYPE_IS_SIZET(t)                        \
    (                                           \
        (TYPE_IS_UNSIGNED_INT(t)) &&            \
        (TYPE_UNSIGNED_INT_MAX_(t) == SIZE_MAX) \
    )

#define TYPEOF_IS_(x, m) \
    TYPE_IS_ ## m(TYPEOF(x))
#define TYPEOF_IS_SIZET(x) \
    TYPEOF_IS_(x, SIZET)
#define TYPEOF_IS_INTEGER(x) \
    TYPEOF_IS_(x, INTEGER)
#define TYPEOF_IS_UNSIGNED_INT(x) \
    TYPEOF_IS_(x, UNSIGNED_INT)

#define SIZE_NOT_NULL_(m, x)        \
    ({                              \
        STATIC(TYPEOF_IS_SIZET(x)); \
        m(x > 0);                   \
        x;                          \
    })
#define ASSERT_SIZE_NOT_NULL(x) \
    SIZE_NOT_NULL_(ASSERT, x)
#define VERIFY_SIZE_NOT_NULL(x) \
    SIZE_NOT_NULL_(VERIFY, x)

#define UINT_TO_SIZE(x)                                       \
    ({                                                        \
        STATIC(TYPE_UNSIGNED_INT_MAX(TYPEOF(x)) <= SIZE_MAX); \
        (size_t) (x);                                         \
    })

#define INT_AS_SIZE(x)                                      \
    ({                                                      \
        STATIC(TYPE_SIGNED_INT_MAX(TYPEOF(x)) <= SIZE_MAX); \
        ASSERT((x) >= 0);                                   \
        (size_t) (x);                                       \
    })

#define SIZE_AS_INT(x)               \
    ({                               \
        STATIC(TYPEOF_IS_SIZET(x));  \
        STATIC(INT_MAX <= SIZE_MAX); \
        ASSERT((x) <= INT_MAX);      \
        (int) (x);                   \
    })

#define SIZE_DEC_NO_OVERFLOW(x)     \
    (                               \
        STATIC(TYPEOF_IS_SIZET(x)), \
        (x) > 0                     \
    )

#define SIZE_INC_NO_OVERFLOW(x)     \
    (                               \
        STATIC(TYPEOF_IS_SIZET(x)), \
        (x) < SIZE_MAX              \
    )

#define SIZE_SUB_NO_OVERFLOW(x, y)  \
    (                               \
        STATIC(TYPEOF_IS_SIZET(x)), \
        STATIC(TYPEOF_IS_SIZET(y)), \
        (x) >= (y)                  \
    )

#define SIZE_ADD_NO_OVERFLOW(x, y)  \
    (                               \
        STATIC(TYPEOF_IS_SIZET(x)), \
        STATIC(TYPEOF_IS_SIZET(y)), \
        (x) <= SIZE_MAX - (y)       \
    )

#define SIZE_MUL_NO_OVERFLOW(x, y)  \
    (                               \
        STATIC(TYPEOF_IS_SIZET(x)), \
        STATIC(TYPEOF_IS_SIZET(y)), \
        (y) == 0 ||                 \
        (x) <= SIZE_MAX / (y)       \
    )

#define SIZE_NO_OVERFLOW(m, n, ...) \
    m(SIZE_ ## n ## _NO_OVERFLOW(__VA_ARGS__))

#define ASSERT_SIZE_INC_NO_OVERFLOW(x)    SIZE_NO_OVERFLOW(ASSERT, INC, x)
#define ASSERT_SIZE_DEC_NO_OVERFLOW(x)    SIZE_NO_OVERFLOW(ASSERT, DEC, x)

#define ASSERT_SIZE_SUB_NO_OVERFLOW(x, y) SIZE_NO_OVERFLOW(ASSERT, SUB, x, y)
#define ASSERT_SIZE_ADD_NO_OVERFLOW(x, y) SIZE_NO_OVERFLOW(ASSERT, ADD, x, y)
#define ASSERT_SIZE_MUL_NO_OVERFLOW(x, y) SIZE_NO_OVERFLOW(ASSERT, MUL, x, y)

#define VERIFY_SIZE_INC_NO_OVERFLOW(x)    SIZE_NO_OVERFLOW(VERIFY, INC, x)
#define VERIFY_SIZE_DEC_NO_OVERFLOW(x)    SIZE_NO_OVERFLOW(VERIFY, DEC, x)

#define VERIFY_SIZE_SUB_NO_OVERFLOW(x, y) SIZE_NO_OVERFLOW(VERIFY, SUB, x, y)
#define VERIFY_SIZE_ADD_NO_OVERFLOW(x, y) SIZE_NO_OVERFLOW(VERIFY, ADD, x, y)
#define VERIFY_SIZE_MUL_NO_OVERFLOW(x, y) SIZE_NO_OVERFLOW(VERIFY, MUL, x, y)

#define SIZE_UN_OP(m, n, op, x)    \
    ({                             \
        SIZE_NO_OVERFLOW(m, n, x); \
        (x) op;                    \
    })

#define SIZE_INC(x) SIZE_UN_OP(ASSERT, INC, +1, x)
#define SIZE_DEC(x) SIZE_UN_OP(ASSERT, DEC, -1, x)

#define SIZE_BIN_OP(m, n, op, x, y)   \
    ({                                \
        SIZE_NO_OVERFLOW(m, n, x, y); \
        (x) op (y);                   \
    })

#define SIZE_SUB(x, y) SIZE_BIN_OP(ASSERT, SUB, -, x, y)
#define SIZE_ADD(x, y) SIZE_BIN_OP(ASSERT, ADD, +, x, y)
#define SIZE_MUL(x, y) SIZE_BIN_OP(ASSERT, MUL, *, x, y)

#define SIZE_SUB_EQ(x, y) SIZE_BIN_OP(ASSERT, SUB, -=, x, y)
#define SIZE_ADD_EQ(x, y) SIZE_BIN_OP(ASSERT, ADD, +=, x, y)
#define SIZE_MUL_EQ(x, y) SIZE_BIN_OP(ASSERT, MUL, *=, x, y)

#define SIZE_PRE_OP(m, n, op, x)   \
    ({                             \
        SIZE_NO_OVERFLOW(m, n, x); \
        op (x);                    \
    })
#define SIZE_POST_OP(m, n, op, x)  \
    ({                             \
        SIZE_NO_OVERFLOW(m, n, x); \
        (x) op;                    \
    })

#define SIZE_PRE_INC(x) SIZE_PRE_OP(ASSERT, INC, ++, x)
#define SIZE_PRE_DEC(x) SIZE_PRE_OP(ASSERT, DEC, --, x)

#define SIZE_POST_INC(x) SIZE_POST_OP(ASSERT, INC, ++, x)
#define SIZE_POST_DEC(x) SIZE_POST_OP(ASSERT, DEC, --, x)

#define SIZE_MIN2(x, y)             \
    (                               \
        STATIC(TYPEOF_IS_SIZET(x)), \
        STATIC(TYPEOF_IS_SIZET(y)), \
        (x) < (y) ? (x) : (y)       \
    )

#define SIZE_MAX2(x, y)             \
    (                               \
        STATIC(TYPEOF_IS_SIZET(x)), \
        STATIC(TYPEOF_IS_SIZET(y)), \
        (x) > (y) ? (x) : (y)       \
    )

#define SIZE_BIT (sizeof(size_t) * CHAR_BIT)

#if SIZE_MAX == UINT_MAX
#define SZ(x) x ## U
#elif SIZE_MAX == ULONG_MAX
#define SZ(x) x ## UL
#else
#error size_t is neither unsigned int nor unsigned long
#endif

#define SIZE_IS_BITS(x, n)                  \
    ({                                      \
        STATIC(TYPEOF_IS_SIZET(x));         \
        STATIC(TYPEOF_IS_INTEGER(n));       \
        STATIC(IS_CONSTANT(n));             \
        STATIC((n) >= 0 && (n) < SIZE_BIT); \
        (x) < (SZ(1) << (n));               \
    })

#define INT_IS_BITS(x, n)            \
    ({                               \
        size_t __v = INT_AS_SIZE(x); \
        SIZE_IS_BITS(__v, n);        \
    })

typedef unsigned int bits_t;

#define BITS_MAX UINT_MAX

#define TYPE_IS_BITS(t) \
    (TYPE_UNSIGNED_INT_MAX(t) == BITS_MAX)
#define TYPEOF_IS_BITS(x) \
    TYPEOF_IS_(x, BITS)

#define BITS_AS_SIZE(x)                       \
    (                                         \
        STATIC(TYPEOF_IS_BITS(0 * (x) + 1U)), \
        STATIC(BITS_MAX <= SIZE_MAX),         \
        (size_t) (x)                          \
    )

#define BITS_IS_BITS(x, n)            \
    ({                                \
        size_t __v = BITS_AS_SIZE(x); \
        SIZE_IS_BITS(__v, n);         \
    })

#define SIZE_TRUNC_BITS(x, n)                       \
    ({                                              \
        size_t __x = (x);                           \
        STATIC(TYPEOF_IS_SIZET(x));                 \
        STATIC(TYPEOF_IS_INTEGER(n));               \
        STATIC(IS_CONSTANT(n));                     \
        STATIC((n) >= 0 && (n) <= SIZE_BIT);        \
        ((n) < SIZE_BIT) && (__x >= (SZ(1) << (n))) \
            ? ((SZ(1) << (n)) - SZ(1))              \
            : __x;                                  \
    })

#define INT_TRUNC_BITS(x, n)         \
    ({                               \
        size_t __v = INT_AS_SIZE(x); \
        SIZE_TRUNC_BITS(__v, n);     \
    })

#define BITS_TRUNC_BITS(x, n)         \
    ({                                \
        size_t __v = BITS_AS_SIZE(x); \
        SIZE_TRUNC_BITS(__v, n);      \
    })

#define BITS_INC_MAX(x, n)                    \
    ({                                        \
        size_t __v = BITS_AS_SIZE(x);         \
        STATIC(TYPEOF_IS_INTEGER(n));         \
        STATIC(IS_CONSTANT(n));               \
        STATIC((n) >= 0 && (n) <= SIZE_BIT);  \
        (x) = __v >= ((SZ(1) << (n)) - SZ(1)) \
                   ? ((SZ(1) << (n)) - SZ(1)) \
                   : __v + 1;                 \
    })

#define SIZE_IS_POW2(n)             \
    (                               \
        STATIC(TYPEOF_IS_SIZET(n)), \
        (n) && !((n) & ((n) - 1))   \
    )

// int __builtin_clz(unsigned int),
// int __builtin_clzl(unsigned long):
// Returns the number of leading 0-bits in x, starting at the most
// significant bit position. If x is 0, the result is undefined.  
// https://gcc.gnu.org/onlinedocs/gcc-4.3.4/gcc/Other-Builtins.html

#if SIZE_MAX == UINT_MAX
#define SIZE_CLZ(x)                  \
    (                                \
        STATIC(INT_MAX <= SIZE_MAX), \
        (size_t) __builtin_clz(x)    \
    )
#elif SIZE_MAX == ULONG_MAX
#define SIZE_CLZ(x)                  \
    (                                \
        STATIC(INT_MAX <= SIZE_MAX), \
        (size_t) __builtin_clzl(x)   \
    )
#else
#error size_t is neither unsigned int nor unsigned long
#endif

// stev: compute floor(log2(n)) for n > 0:

#define SIZE_LOG2(n)                \
    ({                              \
        STATIC(TYPEOF_IS_SIZET(n)); \
        ASSERT(n > 0);              \
        SIZE_BIT - SIZE_CLZ(n) - 1; \
    })

#define TYPE_WIDTH(t) (sizeof(t) * CHAR_BIT)

#endif/*__INT_TRAITS_H*/


