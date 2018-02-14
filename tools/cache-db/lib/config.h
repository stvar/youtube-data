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

#ifndef CONFIG_H
#define CONFIG_H

#ifndef __GNUC__
#error we need a GCC compiler
#endif

#ifndef __GNUC_MINOR__
#error __GNUC_MINOR__ is not defined
#endif

#ifndef __GNUC_PATCHLEVEL__
#error __GNUC_PATCHLEVEL__ is not defined
#endif

#define GCC_VERSION \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

//
// CONFIG_ERROR_FUNCTION_ATTRIBUTE
//

// stev: CONFIG_ERROR_FUNCTION_ATTRIBUTE establishes whether GCC
// supports the function attribute of the kind:
//   `__attribute__ ((error(MESSAGE)))'.
// This attribute is used for defining a powerful static (that
// is compile-time) assertion checking macro.

// The GCC's documentation of version 4.3.0 mentions for the
// first time the presence of the 'error' function attribute:
// http://gcc.gnu.org/onlinedocs/gcc-4.3.0/gcc/Function-Attributes.html.
//
// Note that I did not tested the attribute to work with GCC
// of versions prior to the particular 4.3.4.
#if GCC_VERSION < 40300
#define CONFIG_ERROR_FUNCTION_ATTRIBUTE 0
#else
#define CONFIG_ERROR_FUNCTION_ATTRIBUTE 1
#endif

//
// CONFIG_MEM_ALIGNOF
//

// stev: CONFIG_MEM_ALIGNOF establishes whether the compiler is
// providing an '__alignof__' builtin function which allows to
// inquire about the minimum alignment required by a type or a
// variable. GCC does have such a builtin: I made only a couple
// of investigations for versions upward from v4.0.0 (look into
// http://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/Alignment.html).

// Note that as of GCC v4.8.0 the builtin function is no longer
// needed, since ISO/IEC 9899:2011 specifies the existence of
// the '_Alignof()' unary operator which applied to a 'type-id'
// gets back the alignment requirement of the type in question.
#if GCC_VERSION < 40000
#define CONFIG_MEM_ALIGNOF 0
#else
#define CONFIG_MEM_ALIGNOF 1
#endif

//
// CONFIG_PTR_TO_INT_IDENTOP
//

// stev: CONFIG_PTR_TO_INT_IDENTOP establishes whether the compiler
// implements the conversion from pointer types to integer types of
// equal size as an identity operation. That is that the bits which
// result upon casting a pointer to an integer of the same size are
// identical with those of the pointer representation itself.

// stev: the issues implied by CONFIG_PTR_TO_INT_IDENTOP parameter
// are pertaining to specifications of C language: ISO/IEC 9899:TC3,
// 6.3.2.3 Pointers; and to specifications of C++ language: ISO/IEC
// 14882:2003(E), 5.2.10 Reinterpret cast.

// stev: the implementation of GCC at least upward from v4.0.0 does
// precisely this (see http://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/
// Arrays-and-pointers-implementation.html):
//
//   The result of converting a pointer to an integer or vice versa
//   (C90 6.3.4, C99 6.3.2.3):
//
//   A cast from pointer to integer discards most-significant bits
//   if the pointer representation is larger than the integer type,
//   sign-extends [1] if the pointer representation is smaller than
//   the integer type, otherwise the bits are unchanged.
//
//   A cast from integer to pointer discards most-significant bits
//   if the pointer representation is smaller than the integer type,
//   extends according to the signedness of the integer type if the
//   pointer representation is larger than the integer type,
//   otherwise the bits are unchanged.
//
//   Footnotes
//
//   [1] Future versions of GCC may zero-extend, or use a target-
//       defined ptr_extend pattern. Do not rely on sign extension.
//
// Note that I only looked into the GCC documentation of versions
// between v4.0.0 and v7.2.0.
#if GCC_VERSION < 40000 || GCC_VERSION > 70200
#define CONFIG_PTR_TO_INT_IDENTOP 0
#else
#define CONFIG_PTR_TO_INT_IDENTOP 1
#endif

//
// CONFIG_PTR_NULL_ZERO_REPRESENTATION
//

// stev: this configuration param subsumes to the issues described,
// for example, by the work of Kayvan Memarian and Peter Sewell, of
// University of Cambridge: 'Clarifying the C memory object model'
// (see http://www.open-std.org/jtc1/sc22/wg14/www/docs/n2012.htm
// for the section on null pointers).
//
// Prior to enabling CONFIG_PTR_NULL_ZERO_REPRESENTATION, make sure
// that the things implied by it work on the target platform and
// compiler.

// Joseph Myers, https://gcc.gnu.org/ml/gcc/2015-04/msg00325.html:
//   > Can null pointers be assumed to be represented with 0?
//   For all targets supported by GCC, yes.
#define CONFIG_PTR_NULL_ZERO_REPRESENTATION 1

// stev: instead of spreading STATIC(...) assertions throughout
// the source files, in places where the config parameter below
// ensures the well-functioning of the code, we choose to check
// for it being enabled at this global level:

#if !CONFIG_PTR_NULL_ZERO_REPRESENTATION
#error we need the NULL pointer to be represented as 0
#endif

//
// CONFIG_DYNAMIC_LINKER
//

// stev: CONFIG_DYNAMIC_LINKER specifies the complete path to the
// platform's dynamic linker; it is needed by the shared library
// 'json.so' and by every compiled type library for to be able to 
// to produce 'json.so' version numbers when issued by themselves;
// the following shell function prints out dynamic linkers' full
// path used by GCC on the target platform:
//
//   $ dynamic-linker() { local e=''; [[ "$1" == -+(32|64) ]] && { e="${1:1}"; shift; }; "${1:-gcc}" -Xlinker --verbose ${e:+-m$e} 2>/dev/null|sed -nr '/^found\s+ld.*\sat\s+([^ \t]+)\s*$/{s//\1/;p;q}'; }
//
//   $ dynamic-linker -64
//   /lib64/ld-linux-x86-64.so.2
//
//   $ dynamic-linker -32
//   /lib/ld-linux.so.2
//
// If CONFIG_DYNAMIC_LINKER is not defined, the library 'json.so'
// and any compiled type library would build just fine, but with
// no ability to produce 'json.so' version numbers by themselves.
#ifdef __LP64__
#define CONFIG_DYNAMIC_LINKER "/lib64/ld-linux-x86-64.so.2"
#else
#define CONFIG_DYNAMIC_LINKER "/lib/ld-linux.so.2"
#endif

#endif /* CONFIG_H */

