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

#ifndef __PTR_TRAITS_H
#define __PTR_TRAITS_H

#include "config.h"

#include "int-traits.h"

#if CONFIG_MEM_ALIGNOF != 1
#error we need the __alignof__ unary operator
#endif

#define MEM_ALIGNOF(t) __alignof__(t)

#define PTR_TO_SIZE(p)                              \
    (                                               \
        STATIC(CONFIG_PTR_TO_INT_IDENTOP),          \
        STATIC(TYPE_IS_SIZET(uintptr_t)),           \
        STATIC(sizeof(void*) == sizeof(uintptr_t)), \
        (uintptr_t) (p)                             \
    )

#define PTR_ALIGN_SIZE(p, a)         \
    ({                               \
        size_t __r;                  \
        ASSERT(SIZE_IS_POW2(a));     \
        (__r = PTR_TO_SIZE(p) % (a)) \
            ? (a) - __r              \
            : 0;                     \
    })

#define PTR_ALIGN(p, a)                \
    ({                                 \
        char* __q = (p);               \
        __q += PTR_ALIGN_SIZE(__q, a); \
    })

#define PTR_DIFF(p, b)                  \
    ({                                  \
        ptrdiff_t __d = (p) - (b);      \
        ASSERT(__d >= 0);               \
        STATIC(PTRDIFF_MAX < SIZE_MAX); \
        (size_t) __d;                   \
    })

#define PTR_OFFSET(p, b, l)             \
    ({                                  \
        ptrdiff_t __d = (p) - (b);      \
        ASSERT(__d >= 0);               \
        STATIC(TYPEOF_IS_SIZET(l));     \
        STATIC(PTRDIFF_MAX < SIZE_MAX); \
        ASSERT((size_t) __d <= (l));    \
        (size_t) __d;                   \
    })

#define PTR_IS_IN_BUF(p, o, b, l)          \
    ({                                     \
        STATIC(TYPEOF_IS_SIZET(o));        \
        STATIC(TYPEOF_IS_SIZET(l));        \
        STATIC(PTRDIFF_MAX <= SIZE_MAX);   \
        ASSERT((p) >= (b));                \
        ASSERT((o) <= (l));                \
        (size_t) ((p) - (b)) == (l) - (o); \
    })

#define TYPEOF_IS_CHAR_PTR(x)         \
    (                                 \
        TYPEOF_IS(x, const char*) ||  \
        TYPEOF_IS(x, char*) ||        \
        TYPEOF_IS(x, const char[]) || \
        TYPEOF_IS(x, char[])          \
    )

#define TYPEOF_IS_UCHAR_PTR(x)           \
    (                                    \
        TYPEOF_IS(x, const uchar_t*) ||  \
        TYPEOF_IS(x, uchar_t*) ||        \
        TYPEOF_IS(x, const uchar_t[]) || \
        TYPEOF_IS(x, uchar_t[])          \
    )

#define TYPEOF_IS_CHAR_UCHAR_PTR(x) \
    (                               \
        TYPEOF_IS_CHAR_PTR(x) ||    \
        TYPEOF_IS_UCHAR_PTR(x)      \
    )

#define TYPEOF_IS_CHAR_UCHAR(x) \
    (                           \
        TYPEOF_IS(x, char) ||   \
        TYPEOF_IS(x, uchar_t)   \
    )

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define ARRAY_INDEX(a, i)                  \
    (                                      \
        STATIC(TYPEOF_IS_UNSIGNED_INT(i)), \
        (i) < ARRAY_SIZE(a)                \
    )

#define ARRAY_NULL_ELEM(a, i)             \
    (                                     \
        ARRAY_INDEX(a, i) ? (a)[i] : NULL \
    )

#define ARRAY_CHAR_NULL_ELEM(a, i)          \
    (                                       \
        STATIC(TYPEOF_IS_CHAR_PTR((a)[0])), \
        ARRAY_NULL_ELEM(a, i)               \
    )

#define ARRAY_NULL_ELEM_REF(a, i)            \
    (                                        \
        ARRAY_INDEX(a, i) ? (a) + (i) : NULL \
    )

#define ARRAY_NON_NULL_ELEM(a, i)           \
    (                                       \
        STATIC(TYPEOF_IS_CHAR_PTR((a)[0])), \
        ARRAY_INDEX(a, i) && (a)[i]         \
            ? (a)[i] : "???"                \
    )

#endif/*__PTR_TRAITS_H*/


