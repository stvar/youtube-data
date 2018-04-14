// Copyright (C) 2018  Stefan Vargyas
// 
// This file is part of Cache-DB.
// 
// Cache-DB is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Cache-DB is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Cache-DB.  If not, see <http://www.gnu.org/licenses/>.

#include "config.h"

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <locale.h>
#include <string.h>
#include <alloca.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <getopt.h>
#include <dlfcn.h>
#include <time.h>
#include <link.h>
#include <gdbm.h>

#include "int-traits.h"
#include "ptr-traits.h"
#include "char-traits.h"

#define UNUSED    __attribute__((unused))
#define PRINTF(F) __attribute__((format(printf, F, F + 1)))
#define NORETURN  __attribute__((noreturn))

#define ISASCII CHAR_IS_ASCII
#define ISDIGIT CHAR_IS_DIGIT
#define ISSPACE CHAR_IS_SPACE
#define ISPRINT CHAR_IS_PRINT

#define STRINGIFY_(s) #s
#define STRINGIFY(s)  STRINGIFY_(s)

// stev: important requirement: VERIFY evaluates E only once!

#define VERIFY(E)             \
    do {                      \
        if (!(E))             \
            UNEXPECT_ERR(#E); \
    }                         \
    while (0)

#define UNEXPECT_ERR(M, ...)               \
    do {                                   \
        unexpect_error(__FILE__, __LINE__, \
            __func__, M, ## __VA_ARGS__);  \
    }                                      \
    while (0)

#define UNEXPECT_VAR(F, N) UNEXPECT_ERR(#N "=" F, N)

#define INVALID_ARG(F, N)                    \
    do {                                     \
        invalid_argument(__FILE__, __LINE__, \
            __func__, #N, F, N);             \
    }                                        \
    while (0)

#ifdef DEBUG
#define ASSERT(E)                             \
    do {                                      \
        if (!(E))                             \
            assert_failed(__FILE__, __LINE__, \
                __func__, #E);                \
    }                                         \
    while (0)
#else
#define ASSERT(E) \
    do {} while (0)
#endif

#if !CONFIG_ERROR_FUNCTION_ATTRIBUTE
#error we need GCC to support the 'error' function attribute
#else
#define STATIC(E)                                   \
    ({                                              \
        extern int __attribute__                    \
            ((error("assertion failed: '" #E "'"))) \
            static_assert();                        \
        (void) ((E) ? 0 : static_assert());         \
    })
#endif

#define CHAR_CAST__(p, t, q)           \
    (                                  \
        STATIC(TYPEOF_IS_CHAR_PTR(p)), \
        (q t*) (p)                     \
    )
#define CHAR_CAST_(p, q) \
    CHAR_CAST__(p, char, q)
#define CHAR_CAST(p) \
    CHAR_CAST_(p, )
#define CONST_CHAR_CAST(p) \
    CHAR_CAST_(p, const)
#define UCHAR_CAST_(p, q) \
    CHAR_CAST__(p, uchar_t, q)
#define UCHAR_CAST(p) \
    UCHAR_CAST_(p, )
#define CONST_UCHAR_CAST(p) \
    UCHAR_CAST_(p, const)

static const char program[] = STRINGIFY(PROGRAM);
static const char library[] = STRINGIFY(LIBRARY);
static const char verdate[] = "0.2 -- 2018-01-14 13:34"; // $ date +'%F %R'

static const char license[] =
"Copyright (C) 2018  Stefan Vargyas.\n"
"License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
"This is free software: you are free to change and redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n";

const char stdin_name[] = "<stdin>";

typedef unsigned char uchar_t;
typedef char fmt_buf_t[512];

enum error_type_t { FATAL, WARNING, ERROR };

static void verror(
    enum error_type_t err, const char* fmt, bool nl,
    va_list arg)
{
    static const char* const errors[] = {
        [FATAL]   = "fatal error",
        [WARNING] = "warning",
        [ERROR]   = "error",
    };
    static const char* const fmts[] = {
        "%s: %s: %s\n",
        "%s: %s: %s"
    };
    fmt_buf_t b;

    vsnprintf(b, sizeof b - 1, fmt, arg);
    b[sizeof b - 1] = 0;

    fprintf(stderr, fmts[!nl], program, errors[err], b);
}

static void fatal_error(const char* fmt, ...)
    PRINTF(1)
    NORETURN;

static void fatal_error(const char* fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    verror(FATAL, fmt, true, arg);
    va_end(arg);

    exit(127);
}

static void assert_failed(
    const char* file, int line,
    const char* func, const char* expr)
    NORETURN;

static void assert_failed(
    const char* file, int line, const char* func,
    const char* expr)
{
    fatal_error(
        "assertion failed: %s:%d:%s: %s",
        file, line, func, expr);
}

static void unexpect_error(
    const char* file, int line,
    const char* func, const char* msg, ...)
    PRINTF(4)
    NORETURN;

static void unexpect_error(
    const char* file, int line, const char* func,
    const char* msg, ...)
{
    va_list arg;
    fmt_buf_t b;

    va_start(arg, msg);
    vsnprintf(b, sizeof b - 1, msg, arg);
    va_end(arg);

    b[sizeof b - 1] = 0;

    fatal_error(
        "unexpected error: %s:%d:%s: %s",
        file, line, func, b);
}

static void invalid_argument(
    const char* file, int line, const char* func,
    const char* name, const char* msg, ...)
{
    va_list arg;
    fmt_buf_t b;

    va_start(arg, msg);
    vsnprintf(b, sizeof b - 1, msg, arg);
    va_end(arg);

    b[sizeof b - 1] = 0;

    fatal_error(
        "invalid argument: %s:%d:%s: %s='%s'",
        file, line, func, name, b);
}

#if 0
static void warning(const char*, ...)
    PRINTF(1);

static void warning(const char* fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    verror(WARNING, fmt, true, arg);
    va_end(arg);
}
#endif

static void error(const char*, ...)
    PRINTF(1)
    NORETURN;

static void error(const char* fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    verror(ERROR, fmt, true, arg);
    va_end(arg);

    exit(1);
}

static void error_fmt(const char* fmt, ...)
    PRINTF(1);

static void error_fmt(const char* fmt, ...)
{
    va_list a;

    va_start(a, fmt);
    verror(ERROR, fmt, true, a);
    va_end(a);
}

static void sys_error(const char*, ...)
    PRINTF(1)
    NORETURN;

static void sys_error(const char* fmt, ...)
{
    int sys = errno;
    va_list arg;
    fmt_buf_t b;

    va_start(arg, fmt);
    vsnprintf(b, sizeof b - 1, fmt, arg);
    va_end(arg);

    b[sizeof b - 1] = 0;

    error("%s: %s", b, strerror(sys));
}

static size_t current_timestamp(void)
{
    time_t t;

    t = time(NULL);
    return INT_AS_SIZE(t);
}

#define TIME_BITS SIZE_BIT

#ifndef HASH_BITS
#define HASH_BITS 32
#endif

#if HASH_BITS == 32
#define hash_t uint32_t
#define HASH_PREC 8
#define HASH_ZFMT "%08" PRIx32
#define HASH_MAX UINT32_MAX
#elif HASH_BITS == 64
#define hash_t uint64_t
#define HASH_PREC 16
#define HASH_ZFMT "%016" PRIx64
#define HASH_MAX UINT64_MAX
#else
#error HASH_BITS is neither 32 nor 64
#endif

#if HASH_MAX == UINT_MAX
#define HS(x) x ## U
#elif HASH_MAX == ULONG_MAX
#define HS(x) x ## UL
#elif HASH_MAX == ULLONG_MAX
#define HS(x) x ## ULL
#else
#error hash_t is neither unsigned int nor unsigned long nor unsigned long long
#endif

#define HASH_KEY_FMT HASH_ZFMT " %s"

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
// FNV-1 hash

#if HASH_BITS == 32
#define FNV_PRIME        HS(16777619)
#define FNV_OFFSET_BASIS HS(2166136261)
#elif HASH_BITS == 64
#define FNV_PRIME        HS(1099511628211)
#define FNV_OFFSET_BASIS HS(14695981039346656037)
#endif

static hash_t hash_key(datum key)
{
    const uchar_t *p, *e;
    hash_t r = FNV_OFFSET_BASIS;

    ASSERT(key.dsize >= 0);

    for (p = CONST_UCHAR_CAST(key.dptr),
         e = CONST_UCHAR_CAST(key.dptr) + key.dsize;
         p < e;
         p ++) {
         r *= FNV_PRIME;
         r ^= *p;
    }

    return r ? r : 1;
}

enum options_action_t
{
    options_action_dump_meta,
    options_action_dump_raw,
    options_action_dump_pretty,
    options_action_lookup_key,
    options_action_update_key,
    options_action_remove_key,
    options_action_restore_key,
    options_action_import_db,
};

struct options_t
{
    enum options_action_t
                 action;
    const char*  act_arg;
    const char*  home;

    const char*  hash_file;
    size_t       age_base;
    size_t       threshold;

    bits_t       dry_run: 1;
    bits_t       hash_age: 1;
    bits_t       keep_prev: 1;
    bits_t       null: 1;
    bits_t       log: 1;
    bits_t       prev_key: 1;
    bits_t       save_key: 1;
    bits_t       no_error: 1;
    bits_t       sync: 1;
    bits_t       verbose: 2;

    size_t       argc;
    char* const *argv;
};

struct database_metadata_t
{
    uint8_t timestamp_size;
    uint8_t file_hash_size;
};

struct database_entry_t
{
    size_t timestamp;
    hash_t file_hash;
};

struct database_t
{
    struct database_metadata_t
                    meta;
    bits_t          null: 1;
    const char*     home;
    GDBM_FILE       file;
    FILE*           log;
};

static void database_fatal_error(const char* msg)
{
    fatal_error("database error: %s", msg);
}

static const char database_db_name[]  = "cache.db";
static const char database_log_name[] = "cache.log";

enum database_error_type_t
{
    database_generic_error,
    database_init_error,
    database_meta_error,
    database_log_error,
    database_sys_error,
};

static void database_error(
    const struct database_t* db,
    enum database_error_type_t type,
    const char* fmt, ...)
    PRINTF(3);

static void database_error(
    const struct database_t* db UNUSED,
    enum database_error_type_t type,
    const char* fmt, ...)
{
    static const char head[] =
        "database error";

    int e = errno, g = gdbm_errno;
    const char* d = NULL;
    const char* s = NULL;
    fmt_buf_t b;
    va_list a;

    if (type == database_init_error && g) {
        d = gdbm_strerror(g);
        ASSERT(d != NULL);
    }

    if ((type == database_init_error ||
         type == database_log_error ||
         type == database_sys_error) && e) {
        s = strerror(e);
        ASSERT(s != NULL);
    }

    va_start(a, fmt);
    vsnprintf(b, sizeof b, fmt, a);
    va_end(a);

    if (type == database_meta_error ||
        type == database_log_error) {
        error_fmt(s
            ? "%s: %s/%s: %s: %s"
            : "%s: %s/%s: %s",
            head, db->home,
            type == database_log_error
            ? database_log_name
            : database_db_name,
            b, s);
    }
    else {
        error_fmt(   // d  s
              d && s // 1  1
            ? "%s: %s: %s: %s"
            : d || s // 1  0
                     // 0  1
            ? "%s: %s: %s"
                     // 0  0  <=> !d && !s
            : "%s: %s",
            head, b, d ? d : s, s);
    }

    if (type == database_init_error ||
        type == database_meta_error)
        exit(1);
}

#define DATABASE_ERROR__(r, t, m, ...) \
    ({                                 \
        database_error(db,             \
            database_ ## t ## _error,  \
            m, ## __VA_ARGS__);        \
        r false;                       \
    })
#define DATABASE_ERROR_(m, ...) \
    (!opts->no_error && DATABASE_ERROR__(, generic, m, ## __VA_ARGS__))
#define DATABASE_ERROR(m, ...) \
    DATABASE_ERROR_(m, ## __VA_ARGS__)
#define DATABASE_INIT_ERROR(m, ...) \
    DATABASE_ERROR__((void), init, m, ## __VA_ARGS__)
#define DATABASE_META_ERROR(m, ...) \
    DATABASE_ERROR__(, meta, m, ## __VA_ARGS__)
#define DATABASE_LOG_ERROR(m, ...) \
    DATABASE_ERROR__((void), log, m, ## __VA_ARGS__)
#define DATABASE_SYS_ERROR(m, ...) \
    DATABASE_ERROR__(, sys, m, ## __VA_ARGS__)

static char* norm_func_name(
    char* func, size_t sz)
{
    char *p, *e; 

    if (sz <= 9 || memcmp(func, "database_", 9))
        return NULL;

    func += 9;
    sz -= 9;

    for (p = func,
         e = func + sz;
         p < e;
         p ++) {
         if (*p == '_')
             *p = '-';
    }

    return func;
}

typedef char iso_date_buf_t[20];

static const char iso_date_fmt[] =
    "%Y-%m-%d %H:%M:%S";

static void database_log(
    struct database_t* db, const char* func,
    const char* fmt, ...)
    PRINTF(3);

static void database_log(
    struct database_t* db, const char* func,
    const char* fmt, ...)
{
    struct tm t;
    iso_date_buf_t b;
    va_list a;
    time_t c;
    size_t n;
    char* f;

    if (db->log == NULL)
        return;

    c = time(NULL);
    localtime_r(&c, &t);
    n = strftime(b, sizeof b, iso_date_fmt, &t);
    VERIFY(n);

    fputs(b, db->log);

    n = strlen(func);
    f = alloca(n + 1);
    strcpy(f, func);

    if ((f = norm_func_name(f, n)))
        fprintf(db->log, " %s: ", f);
    else
        fputc(' ', db->log);

    va_start(a, fmt);
    vfprintf(db->log, fmt, a);
    va_end(a);

    fputc('\n', db->log);
}

#define DATABASE_LOG(m, ...)       \
    do {                           \
        database_log(db, __func__, \
            m, ## __VA_ARGS__);    \
    } while (0)

#define DATABASE_DB_ERROR_LOG(e, m, ...) \
    DATABASE_LOG("db-error: " m ": %s",  \
        ## __VA_ARGS__, gdbm_strerror(e))

#define DATABASE_SYS_ERROR_LOG(e, m, ...) \
    DATABASE_LOG("sys-error: " m ": %s",  \
        ## __VA_ARGS__, strerror(e))

#define DATABASE_ERROR_LOG(m, ...)   \
    DATABASE_LOG("error: " m, ## __VA_ARGS__)

#define DATABASE_ENTRY(n) \
    (((struct database_entry_t){0,0}).n)

static struct database_metadata_t database_metadata = {
    .timestamp_size = sizeof(DATABASE_ENTRY(timestamp)),
    .file_hash_size = sizeof(DATABASE_ENTRY(file_hash))
};

static bool database_get_metadata(
    struct database_t* db,
    struct database_metadata_t* meta)
{
    const size_t n =
        sizeof(struct database_metadata_t);
    datum k, e;

    k.dptr = "";
    k.dsize = 0;

    e = gdbm_fetch(db->file, k);

    if (e.dptr == NULL) {
        memset(meta, 0, n);
        return false;
    }
    else {
        ASSERT(INT_AS_SIZE(e.dsize) == n);

        memcpy(meta, e.dptr, n);
        free(e.dptr);

        return true;
    }
}

static bool database_is_empty(
    struct database_t* db)
{
    datum k;
    bool r;

    k = gdbm_firstkey(db->file);
    r = k.dptr == NULL;
    free(k.dptr);

    return r;
}

static bool database_check_metadata(
    struct database_t* db,
    bool read_only)
{
    datum k, c;

    if (database_get_metadata(db, &db->meta)) {
        if (!read_only &&
            memcmp(&db->meta, &database_metadata,
                sizeof db->meta)) {
            gdbm_close(db->file);
            return DATABASE_META_ERROR(
                "wrong metadata");
        }
    }
    else
    if (database_is_empty(db)) {
        k.dptr = "";
        k.dsize = 0;

        c.dptr = (char*) &database_metadata;
        c.dsize = sizeof  database_metadata;

        if (gdbm_store(db->file, k, c, GDBM_INSERT)) {
            gdbm_close(db->file);
            return DATABASE_META_ERROR(
                "gdbm_store: failed writing metadata");
        }

        db->meta = database_metadata;
    }
    else {
        if (true) {
            gdbm_close(db->file);
            return DATABASE_META_ERROR(
                "missing metadata");
        }
    }

    return true;
}

static void database_init(
    struct database_t* db, 
    const struct options_t* opts)
{
    static const size_t flags[] = {
        [options_action_dump_meta]   = GDBM_READER,
        [options_action_dump_raw]    = GDBM_READER,
        [options_action_dump_pretty] = GDBM_READER,
        [options_action_lookup_key]  = GDBM_READER,
        [options_action_update_key]  = GDBM_WRCREAT,
        [options_action_remove_key]  = GDBM_WRITER,
        [options_action_restore_key] = GDBM_WRITER,
        [options_action_import_db]   = GDBM_WRCREAT,
    };

    const size_t *p;
    size_t f;
    bool m;

    memset(db, 0, sizeof(*db));

    db->null = opts->null;

    if (opts->home == NULL)
        INVALID_ARG("%s", opts->home);
    db->home = opts->home;

    if (!(p = ARRAY_NULL_ELEM_REF(
                flags, opts->action)))
        INVALID_ARG("%d", opts->action);

    f = *p;
    if (opts->sync)
        f |= GDBM_SYNC;

    db->file = gdbm_open(
        CHAR_CAST(database_db_name),
        0, f, S_IRUSR|S_IWUSR|S_IRGRP,
        database_fatal_error);

    if (db->file == NULL)
        return DATABASE_INIT_ERROR(
            "gdbm_open");

    m = opts->action == options_action_dump_meta;

    if (!database_check_metadata(db, m))
        return;

    if (m || !opts->log)
        return;

    if (!(db->log = fopen(database_log_name, "a")))
        return DATABASE_LOG_ERROR(
            "fopen");

    if (setvbuf(db->log, NULL, _IOLBF, 0))
        return DATABASE_LOG_ERROR(
            "setvbuf");
}

static void database_done(
    struct database_t* db)
{
    if (db->log)
        fclose(db->log);
    if (db->file)
        gdbm_close(db->file);
}

static bool database_get_entry(
    struct database_t* db, datum key,
    struct database_entry_t* entry)
{
    const size_t n =
        sizeof(struct database_entry_t);
    datum e;

    ASSERT(key.dptr != NULL);

    e = gdbm_fetch(db->file, key);

    if (e.dptr == NULL || e.dsize == 0) {
        memset(entry, 0, n);
        return false;
    }
    else {
        ASSERT(INT_AS_SIZE(e.dsize) == n);

        memcpy(entry, e.dptr, n);
        free(e.dptr);

        return true;
    }
}

static void database_set_entry(
    struct database_t* db, datum key,
    struct database_entry_t entry)
{
    datum e;
    int r;

    ASSERT(key.dptr != NULL);

    e.dptr = (char*) &entry;
    e.dsize = sizeof  entry;

    r = gdbm_store(db->file, key, e, GDBM_REPLACE);
    VERIFY(r == 0);
}

static datum make_previous_key(datum key)
{
    static const char suffix[] = "$previous";
    // stev: 'sz' accounts for the terminating NUL char too
    const size_t sz = ARRAY_SIZE(suffix);

    size_t l;
    char* n;
    datum r;

    ASSERT(key.dsize > 1);
    l = INT_AS_SIZE(key.dsize - 1);

    ASSERT_SIZE_ADD_NO_OVERFLOW(l, sz);
    n = malloc(l + sz);
    VERIFY(n != NULL);

    memcpy(n, key.dptr, l);
    memcpy(n + l, suffix, sz);

    // stev: '$previous' key
    r.dptr = n;
    r.dsize = l + sz;

    return r;
}

static bool database_get_prev_entry(
    struct database_t* db, datum key,
    struct database_entry_t* entry)
{
    datum k;
    bool r;

    k = make_previous_key(key);
    r = database_get_entry(db, k, entry);
    free(k.dptr);

    return r;
}

#define SIZE_AS_TIME(x)                 \
    ({                                  \
        STATIC(                         \
            TYPEOF_IS_SIZET(x));        \
        STATIC(                         \
            TYPE_INTEGER_MAX(time_t) <= \
            SIZE_MAX);                  \
        ASSERT(                         \
            TYPE_INTEGER_MAX(time_t) >= \
            (x));                       \
        (time_t) (x);                   \
    })

static void print_abs_timestamp(
    size_t timestamp,
    FILE* file)
{
    struct tm t;
    iso_date_buf_t b;
    size_t n;
    time_t s;

    s = SIZE_AS_TIME(timestamp);

    localtime_r(&s, &t);
    n = strftime(b, sizeof b, iso_date_fmt, &t);
    VERIFY(n);

    fputs(b, file);
}

static void buf_printf(
    char** buf, size_t* size,
    const char* fmt, ...)
    PRINTF(3);

static void buf_printf(
    char** buf, size_t* size,
    const char* fmt, ...)
{
    va_list a;
    size_t n;
    int r;

    va_start(a, fmt);
    r = vsnprintf(*buf, *size, fmt, a);
    va_end(a);

    VERIFY(r >= 0);
    n = INT_AS_SIZE(r);
    VERIFY(n < *size);

    *buf += n;
    *size -= n;
}

static void print_rel_timestamp(
    size_t timestamp,
    size_t base,
    bool ralign,
    FILE* file)
{
    enum date_unit_t {
        sec, min, hour, day, week, month, year,
    };
    static const char* names[] = {
        [sec] = "sec", [min] = "min", [hour] = "hour",
        [day] = "day", [week] = "week", [month] = "month",
        [year] = "year"
    };
    static const size_t divs[] = {
        [sec] = 60, [min] = 60, [hour] = 24,
        [day] = 7, [week] = 4, [month] = 12
    };

    const size_t m =
        2 + 1 + 6 + // max of prec + space + max of len + 1
        1 +         // space
        2 + 1 + 6 + // max of prec + space + max of len + 1
        1 + 5 +     // space + max suffix len
        1;          // NUL char
    const size_t n = ARRAY_SIZE(names);

    size_t i, j, t, l = m;
    char b[m], *p = b;
    size_t u[n];
    bool s;

    t = (s = timestamp > base)
        ? timestamp - base
        : base - timestamp;

    if (t == 0) {
        buf_printf(&p, &l, "now");
        goto print_out;
    }

    memset(u, 0, sizeof(u));

    STATIC(
        ARRAY_SIZE(names) - 1 ==
        ARRAY_SIZE(divs));

    for (i = 0; t && i < n - 1; i ++) {
        size_t d = divs[i];

        u[i] = t % d;
        t /= d;
    }
    u[i] = t;

    if (u[year] >= 100) {
        buf_printf(&p, &l, "+100 years");
        goto suffix;
    }

    for (i = n, j = 0; i > 0; i --) {
        size_t v = u[i - 1];

        if (!v)
            continue;

        if (ralign) {
            buf_printf(&p, &l,
                "%s%2zu %*s%s",
                j == 0 ? "" : " ", v,
                6 - (v != 1), names[i - 1],
                v != 1 ? "s" : "");
        }
        else {
            buf_printf(&p, &l,
                "%s%zu %s%s",
                j == 0 ? "" : " ",
                v, names[i - 1],
                v != 1 ? "s" : "");
        }

        if (++ j >= 2)
            break;
    }
    ASSERT(j >= 1 && j <= 2);

suffix:
    buf_printf(&p, &l,
        ralign ? " %-5s" : " %s",
        s ? "ahead" : "ago");

print_out:
    if (ralign)
        fprintf(file, "%*s", SIZE_AS_INT(m - 1), b); 
    else
        fputs(b, file);
}

enum database_print_opts_t
{
    database_print_opts_verbose  = SZ(1) << 0,
    database_print_opts_pretty   = SZ(1) << 1,
    database_print_opts_ralign   = SZ(1) << 2,
    database_print_opts_hash_age = SZ(1) << 3,
};

static void database_print_entry(
    struct database_t* db, datum key,
    const struct database_entry_t* entry,
    enum database_print_opts_t opts,
    size_t age_base)
{
    struct database_entry_t e;
    char s;
    bool r;

    ASSERT(key.dptr != NULL);

    if (entry != NULL)
        e = *entry;
    else {
        r = database_get_entry(db, key, &e);
        VERIFY(r);
    }

    if (!(opts & database_print_opts_pretty)) {
        size_t v;
        bool s;

        if (!(opts & database_print_opts_hash_age))
            age_base = 0;

        v = (s = e.timestamp > age_base)
            ? e.timestamp - age_base
            : age_base - e.timestamp;
        fprintf(stdout, "%s%zu",
            age_base && s ? "-" : "", v);
    }
    else
    if (opts & database_print_opts_hash_age)
        print_rel_timestamp(
            e.timestamp, age_base,
            opts & database_print_opts_ralign,
            stdout);
    else
        print_abs_timestamp(
            e.timestamp, stdout);

    s = opts & database_print_opts_pretty
        ? ' ' : '\t';

    fprintf(stdout,
        "%c" HASH_ZFMT, s, e.file_hash);

    if (!(opts & database_print_opts_verbose))
        fputc('\n', stdout);
    else {
        fprintf(stdout, "%c%s%c",
            s, key.dptr, db->null ? '\0' : '\n');
    }
}

static bool database_dump(
    struct database_t* db,
    size_t age_base,
    bool hash_age,
    bool pretty)
{
    enum database_print_opts_t o;
    size_t i = 0;
    datum k, k2;

    o = database_print_opts_verbose |
        database_print_opts_ralign;
    if (pretty)
        o |= database_print_opts_pretty;
    if (hash_age)
        o |= database_print_opts_hash_age;

    k = gdbm_firstkey(db->file);
    while (k.dptr != NULL) {
        i += k.dsize == 0;
        ASSERT(i <= 1);

        k2 = gdbm_nextkey(db->file, k);

        if (k.dsize != 0)
            database_print_entry(
                db, k, NULL, o, age_base);
        free(k.dptr);

        k = k2;
    }

    return true;
}

#ifdef DEBUG
#define ASSERT_ACTION_IS(w) \
    ASSERT(opts->action == options_action_ ## w)
#else
#define ASSERT_ACTION_IS(w) \
    do { (void) opts->action; } while (0)
#endif

static bool database_dump_metadata(
    struct database_t* db,
    const struct options_t* opts)
{
    ASSERT_ACTION_IS(dump_meta);

    fprintf(stdout,
        "timestamp-size: %d\n"
        "file-hash-size: %d\n",
        db->meta.timestamp_size,
        db->meta.file_hash_size);

    return true;
}

static bool database_dump_raw(
    struct database_t* db,
    const struct options_t* opts)
{
    ASSERT_ACTION_IS(dump_raw);

    return database_dump(
        db, opts->age_base, opts->hash_age, false);
}

static bool database_dump_pretty(
    struct database_t* db,
    const struct options_t* opts)
{
    ASSERT_ACTION_IS(dump_pretty);

    return database_dump(
        db, opts->age_base, opts->hash_age, true);
}

#define DATABASE_CHECK_KEY()    \
    ({                          \
        ASSERT(opts->act_arg);  \
        *opts->act_arg != 0 ||  \
        DATABASE_ERROR(         \
            "empty key given"); \
    })

static bool database_lookup_key(
    struct database_t* db,
    const struct options_t* opts)
{
    enum database_print_opts_t o = 0;
    struct database_entry_t e;
    datum k;

    ASSERT_ACTION_IS(lookup_key);

    if (!DATABASE_CHECK_KEY())
        return false;

    k.dptr = CONST_CAST(opts->act_arg, char);
    k.dsize = strlen(opts->act_arg) + 1;

    if (!database_get_entry(db, k, &e))
        return DATABASE_ERROR(
            "key not found: %s", k.dptr);

    if (opts->verbose)
        o |= database_print_opts_verbose;
    if (opts->hash_age)
        o |= database_print_opts_hash_age |
             database_print_opts_pretty;

    database_print_entry(
        db, k, &e, o, opts->age_base);

    if (!opts->prev_key ||
        !database_get_prev_entry(db, k, &e))
        return true;

    database_print_entry(
        db, k, &e, o, opts->age_base);

    return true;
}

typedef char hash_file_name_buf_t[HASH_PREC + 1];
typedef char hash_file_name2_buf_t[HASH_PREC + 2];

#define HASH_FILE_NAME_SIZE_(s) \
    (HASH_PREC + ARRAY_SIZE(#s))
#define HASH_FILE_MAKE_NAME_(b, h, s)    \
    do {                                 \
        int __r;                         \
        const size_t __s =               \
            HASH_FILE_NAME_SIZE_(s) - 1; \
        STATIC(                          \
            TYPEOF_IS(b, char[]));       \
        STATIC(                          \
            ARRAY_SIZE(b) ==             \
            HASH_FILE_NAME_SIZE_(s));    \
        __r = snprintf((b), sizeof(b),   \
                HASH_ZFMT #s, (h));      \
        VERIFY(__r > 0);                 \
        VERIFY(INT_AS_SIZE(__r) == __s); \
    } while (0)
#define HASH_FILE_MAKE_NAME2(b, h) \
    HASH_FILE_MAKE_NAME_(b, h, ~)
#define HASH_FILE_MAKE_NAME(b, h) \
    HASH_FILE_MAKE_NAME_(b, h, )

enum hash_file_create_result_t
{
    hash_file_create_ok,
    hash_file_create_error,
    hash_file_create_exists,
};

static enum hash_file_create_result_t
    hash_file_create(
        struct database_t* db, hash_t hash)
{
    enum hash_file_create_result_t r;
    hash_file_name_buf_t b;
    int f, n, e;

    HASH_FILE_MAKE_NAME(b, hash);

    f = open(b, O_WRONLY|O_CREAT|O_EXCL|O_NONBLOCK|O_NOCTTY,
                S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    e = errno;

    r =   f < 0 && e == EEXIST
        ? hash_file_create_exists
        : f < 0 
        ? hash_file_create_error
        : hash_file_create_ok;

    if (r == hash_file_create_error)
        DATABASE_SYS_ERROR_LOG(e, "open %s", b);
    else
    if (r == hash_file_create_exists)
        DATABASE_ERROR_LOG("%s already exists", b);

    if (r != hash_file_create_ok)
        return r;

    n = close(f);
    e = errno;

    r = n != 0
        ? hash_file_create_error
        : hash_file_create_ok;

    if (r == hash_file_create_error)
        DATABASE_SYS_ERROR_LOG(e, "close %s", b);

    return r;
}

enum hash_file_exists_result_t
{
    hash_file_exists_ok,
    hash_file_exists_not,
    hash_file_exists_error,
    hash_file_exists_non_regular,
};

static enum hash_file_exists_result_t
    hash_file_exists(
        struct database_t* db, hash_t hash,
        size_t* size)
{
    enum hash_file_exists_result_t r;
    hash_file_name_buf_t b;
    struct stat s;
    int n, e;

    HASH_FILE_MAKE_NAME(b, hash);

    n = stat(b, &s);
    e = errno;

    r =   n && e == ENOENT
        ? hash_file_exists_not
        : n && e != ENOENT
        ? hash_file_exists_error
        : !n && !S_ISREG(s.st_mode)
        ? hash_file_exists_non_regular
        : hash_file_exists_ok;

    if (r == hash_file_exists_error)
        DATABASE_SYS_ERROR_LOG(e, "stat %s", b);
    else
    if (r == hash_file_exists_non_regular)
        DATABASE_ERROR_LOG("%s not a regular file", b);

    if (size != NULL)
        *size = INT_AS_SIZE(s.st_size);

    return r;
}

static enum hash_file_exists_result_t
    hash_file_new_hash(
        struct database_t* db, datum key,
        struct database_entry_t* entry,
        const char* func)
{
    enum hash_file_exists_result_t r;
    size_t i = 0;

    entry->file_hash = hash_key(key);

    while ((r = hash_file_exists(db, entry->file_hash, NULL)) ==
                hash_file_exists_ok) {
        entry->file_hash ++;
        i ++;
    }

    if (i && func) {
#define __func__ func
        DATABASE_LOG("%zu collisions on " HASH_KEY_FMT,
            i, entry->file_hash, key.dptr);
#undef  __func__
    }

    return r;
}

static bool hash_file_copy(
    struct database_t* db, 
    const char* source,
    const char* target,
    const char* func)
{
#define __func__ func
    const size_t sz = 64 * 1024;

    struct stat s0, s1;
    bool r = false;
    int f0, f1;
    char* b;

    if ((f0 = open(source, O_RDONLY|O_NOFOLLOW)) < 0) {
        DATABASE_SYS_ERROR_LOG(errno,
            "failed opening source %s", source);
        return false;
    }
    if (fstat(f0, &s0)) {
        DATABASE_SYS_ERROR_LOG(errno,
            "failed stating source %s", source);
        goto close0;
    }
    if (!S_ISREG(s0.st_mode)) {
        DATABASE_SYS_ERROR_LOG(errno,
            "source %s is not regular", source);
        goto close0;
    }

    if ((f1 = open(target, O_WRONLY|O_CREAT|O_TRUNC,
                S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) < 0) {
        DATABASE_SYS_ERROR_LOG(errno,
            "failed opening target %s", target);
        goto close0;
    }
    if (fstat(f1, &s1)) {
        DATABASE_SYS_ERROR_LOG(errno,
            "failed stating target %s", target);
        goto close1;
    }
    if (!S_ISREG(s1.st_mode)) {
        DATABASE_LOG("target %s is not regular", target);
        goto close1;
    }

    if (s0.st_ino == s1.st_ino &&
        s0.st_dev == s1.st_dev) {
        DATABASE_LOG("source %s and target %s are the same file",
            source, target);
        goto close1;
    }

    b = malloc(sz);

    while (true) {
        ssize_t r, w;

        r = read(f0, b, sz);
        if (r < 0) {
            DATABASE_SYS_ERROR_LOG(errno,
                "failed reading source %s", source);
            goto free_buf;
        }
        if (r == 0)
            break;

        w = write(f1, b, INT_AS_SIZE(r));
        if (w < 0 || w != r) {
            DATABASE_SYS_ERROR_LOG(errno,
                "failed writing target %s", target);
            goto free_buf;
        }
    }
    r = true;

free_buf:
    free(b);

close1:
    if (close(f1))
        DATABASE_SYS_ERROR_LOG(errno,
            "failed closing target %s", target);

close0:
    if (close(f0))
        DATABASE_SYS_ERROR_LOG(errno,
            "failed closing source %s", source);

    return r;
#undef __func__
}

static bool database_preserve_key(
    struct database_t* db, datum key,
    struct database_entry_t entry,
    const char* hash_file,
    bool keep_backup,
    hash_t* result)
{
    struct database_entry_t e1, e2 = entry;
    enum hash_file_exists_result_t s;
    hash_file_name_buf_t n1, n2;
    hash_file_name2_buf_t n0;
    datum k, e;

    // stev: '$previous' key
    k = make_previous_key(key);

    e.dptr = (char*) &entry;
    e.dsize = sizeof  entry;

    HASH_FILE_MAKE_NAME(n2, e2.file_hash);

    if (database_get_entry(db, k, &e1)) {
        ASSERT(e1.file_hash);

        entry.file_hash = e1.file_hash;
        HASH_FILE_MAKE_NAME2(n0, e1.file_hash);
        HASH_FILE_MAKE_NAME(n1, entry.file_hash);

        // stev: (0) copy '$previous' hash file to a backup file
        if (!hash_file_copy(db, n1, n0, __func__)) {
            DATABASE_ERROR_LOG(
                "failed copying hash file %s to %s", n1, n0);
            *result = 0;
            goto out;
        } 
    }
    else {
        ASSERT(!e1.file_hash);

        // stev: (1) create a new '$previous' file hash;
        // its associated hash file hasn't yet been created
        if ((s = hash_file_new_hash(db, k, &entry, __func__)) !=
                 hash_file_exists_not) {
            DATABASE_ERROR_LOG(
                "failed creating new file hash " HASH_ZFMT
                " on %s", entry.file_hash, k.dptr);
            *result = 0;
            goto out;
        }
        HASH_FILE_MAKE_NAME(n1, entry.file_hash);
    }
    *result = entry.file_hash;

    // stev: (2) copy the initiating hash file to '$previous'
    if (!hash_file_copy(db, n2, n1, __func__)) {
        DATABASE_ERROR_LOG(
            "failed copying hash file %s to %s", n2, n1);
        goto undo_backup;
    } 

    // stev: (3.a) empty the initiating hash file
    if (!hash_file && truncate(n2, 0)) {
        DATABASE_SYS_ERROR_LOG(errno,
            "failed emptying hash file %s", n2);
        goto undo_copy;
    } 
    // stev: (3.b) move content to the initiating hash file
    if (hash_file && rename(hash_file, n2)) {
        DATABASE_SYS_ERROR_LOG(errno,
            "failed moving %s to hash file %s", hash_file, n2);
        goto undo_copy;
    } 

    // stev: (4) store '$previous' entry in database
    if (gdbm_store(db->file, k, e, GDBM_REPLACE)) {
        DATABASE_DB_ERROR_LOG(gdbm_errno,
            "failed storing entry on %s", n1);
        goto undo_copy;
    }

    entry.file_hash = e2.file_hash;
    entry.timestamp = current_timestamp();

    // stev: (5) store an updated $key entry in database
    if (gdbm_store(db->file, key, e, GDBM_REPLACE)) {
        DATABASE_DB_ERROR_LOG(gdbm_errno,
            "failed updating entry on %s", n2);
        goto undo_store;
    }

    // stev: (6) remove backup hash file if created
    if (!keep_backup && e1.file_hash && unlink(n0)) {
        DATABASE_SYS_ERROR_LOG(errno,
            "failed removing backup hash file %s", n0);
        goto undo_store2;
    }

    DATABASE_LOG(
        "saved to %s key %s %s", n1, n2, key.dptr);

    free(k.dptr);
    return true;

undo_store2:
    // stev: (7) rollback the '$key' entry
    if (gdbm_store(db->file, key,
            (e.dptr = (char*) &e2, e), GDBM_REPLACE)) {
        DATABASE_DB_ERROR_LOG(gdbm_errno,
            "failed rolling back entry on %s", n2);
    }
    else {
        DATABASE_LOG(
            "rolled back %s entry on %s",
            e1.file_hash ? "existing" : "new", n2);
    }

undo_store:
    // stev: (8) rollback the '$previous' entry
    if (! e1.file_hash
        ? gdbm_delete(db->file, k)
        : gdbm_store(db->file, k,
            (e.dptr = (char*) &e1, e), GDBM_REPLACE)) {
        DATABASE_DB_ERROR_LOG(gdbm_errno,
            "failed rolling back %s entry on %s",
            e1.file_hash ? "existing" : "new", n1);
    }
    else {
        DATABASE_LOG(
            "rolled back %s entry on %s",
            e1.file_hash ? "existing" : "new", n1);
    }

undo_copy:
    // stev: (9) restore the initiating file hash content
    if (!rename(n1, n2))
        DATABASE_LOG("renamed hash file %s to %s", n1, n2);
    else 
        DATABASE_SYS_ERROR_LOG(errno,
            "failed renaming hash file %s to %s", n1, n2);

undo_backup:
    if (e1.file_hash) {
        // stev: (10.a) restore original '$previous' hash file
        if (!rename(n0, n1))
            DATABASE_LOG("renamed hash file %s to %s", n0, n1);
        else
            DATABASE_SYS_ERROR_LOG(errno,
                "failed renaming hash file %s to %s", n0, n1);
    }
    else {
        // stev: (10.b) remove newly created '$previous' hash file
        if (!unlink(n1))
            DATABASE_LOG("removed hash file %s", n1);
        else
            DATABASE_SYS_ERROR_LOG(errno,
                "failed removing hash file %s", n1);
    }

out:
    free(k.dptr);
    return false;
}

static bool database_update_key(
    struct database_t* db,
    const struct options_t* opts)
{
    enum hash_file_create_result_t c;
    enum hash_file_exists_result_t s;
    struct database_entry_t e;
    hash_t p = 0;
    size_t t;
    datum k;
    char m;
    bool u;

    ASSERT_ACTION_IS(update_key);

    if (!DATABASE_CHECK_KEY())
        return false;

    t = current_timestamp();

    k.dptr = CONST_CAST(opts->act_arg, char);
    k.dsize = strlen(opts->act_arg) + 1;

    if (!database_get_entry(db, k, &e)) {
        const char* f = opts->verbose ? __func__ : NULL;

        if ((s = hash_file_new_hash(db, k, &e, f)) ==
                 hash_file_exists_not)
             c = hash_file_create(db, e.file_hash);

        if (c == hash_file_create_ok && opts->verbose > 1)
            DATABASE_LOG("hash file created on " HASH_KEY_FMT,
                e.file_hash, k.dptr);

        u = s == hash_file_exists_not &&
            c == hash_file_create_ok;
        m = u ? '-' : '!';
    }
    else {
        bool r, b = true;
        size_t n = 0;

        if ((s = hash_file_exists(db, e.file_hash, &n)) ==
                 hash_file_exists_not)
            DATABASE_LOG("hash file missing on " HASH_KEY_FMT,
                e.file_hash, k.dptr);

        // stev: a threshold of value 0 means refresh
        // anyway; entries having timestamps in the
        // future (t < e.timestamp) aren't refreshed
        r = t >= e.timestamp &&
            t - e.timestamp >= opts->threshold;

        if (r && !opts->dry_run && opts->verbose > 1)
            DATABASE_LOG(
                "threshold %zu elapsed on " HASH_KEY_FMT,
                opts->threshold, e.file_hash, k.dptr);

        // stev: if hash file was given, update
        // the key irrespective of its age 
        if (!r) r = opts->hash_file != NULL;

        if (opts->save_key &&
            r && !opts->dry_run &&
            s == hash_file_exists_ok) {
            b = database_preserve_key(
                    db, k, e, opts->hash_file,
                    opts->keep_prev, &p);
        }
        else
        if (opts->prev_key) {
            struct database_entry_t e;

            if (database_get_prev_entry(db, k, &e))
                p = e.file_hash;
        }

        m = s == hash_file_exists_ok && b ?  (n == 0 || r) ? '-' : '+' : '!';
        u = s == hash_file_exists_ok && b && (n == 0 || r) && !opts->dry_run;
            //!!!??? update if b is true ???
    }

    if (u) {
        e.timestamp = t;
        database_set_entry(db, k, e);
    }

    DATABASE_LOG(
        "%c" HASH_KEY_FMT, m, e.file_hash, k.dptr);
    fprintf(stdout,
        "%c" HASH_ZFMT, m, e.file_hash);
    if (p)
        fprintf(stdout, ":" HASH_ZFMT, p);
    fputc('\n', stdout);

    return m != '!';
}

static bool database_remove_key(
    struct database_t* db,
    const struct options_t* opts)
{
    struct database_entry_t e;
    datum k;
    int r;

    ASSERT_ACTION_IS(remove_key);

    if (!DATABASE_CHECK_KEY())
        return false;

    k.dptr = CONST_CAST(opts->act_arg, char);
    k.dsize = strlen(opts->act_arg) + 1;

    if (!database_get_entry(db, k, &e))
        return DATABASE_ERROR(
            "key not found: %s", k.dptr);
    else {
        DATABASE_LOG(" " HASH_KEY_FMT,
            e.file_hash, k.dptr);

        r = gdbm_delete(db->file, k);
        VERIFY(r == 0); //!!!???

        return true;
    }
}

static bool database_restore_key(
    struct database_t* db,
    const struct options_t* opts)
{
    ASSERT_ACTION_IS(restore_key);

    (void) db;   // use db
    (void) opts; // use opts

    return DATABASE_ERROR("restore-key: "
        "operation not yet implemented");
}

#if SIZE_MAX < ULONG_MAX
static size_t strtosz(
    const char* ptr, char** end, int base)
{
    unsigned long r;

    errno = 0;
    r = strtoul(ptr, end, base);

    if (errno == 0 && r > SIZE_MAX)
        errno = ERANGE;

    return r;
}
#define STR_TO_SIZE strtosz
#elif SIZE_MAX == ULONG_MAX
#define STR_TO_SIZE strtoul
#elif SIZE_MAX == UULONG_MAX
#define STR_TO_SIZE strtoull
#else
#error unexpected SIZE_MAX > UULONG_MAX
#endif

static bool parse_size(
    const char* ptr, const char** end,
    int base, size_t* result)
{
    if (ISSPACE(*ptr) ||
        *ptr == '-' ||
        *ptr == '+')
        return false;

    errno = 0;
    *result = STR_TO_SIZE(
        ptr, (char**) end, base);
    ASSERT(*end >= ptr);
    return errno == 0;
}

#if HASH_MAX < SIZE_MAX
static bool parse_hash(
    const char* ptr, const char** end,
    int base, hash_t* result)
{
    size_t r;

    STATIC(HASH_MAX < SIZE_MAX);

    if (!parse_size(ptr, end, base, &r) ||
        r > HASH_MAX)
        return false;

    *result = r;
    return true;
}
#elif HASH_MAX == SIZE_MAX
#define parse_hash parse_size
#else
#error HASH_MAX > SIZE_MAX
#endif

static const char* normalize_for_print(
    char* buf, size_t sz)
{
    char *p = buf, *e = buf + sz;

    for (; p < e; p ++) {
        if (ISSPACE(*p))
            *p = ' ';
        else
        if (!ISPRINT(*p))
            *p = '?';
    }
    return buf;
}

static bool parse_input_line(
    const char* buf, size_t size,
    struct database_entry_t* entry,
    char** key, size_t* len,
    size_t* result)
{
    const char* p = buf;
    size_t d;

#define parse_tab(p)             \
    ({                           \
        bool __t = *(p) == '\t'; \
        if (__t) ++ (p);         \
        __t;                     \
    })

    // stev: format '^[0-9]+\t[0-9a-fA-F]+\t.+$'

    if (!parse_size(p, &p, 10, &entry->timestamp) ||
        !parse_tab(p) ||
        !parse_hash(p, &p, 16, &entry->file_hash) ||
        !parse_tab(p)) {
        *result = PTR_DIFF(p, buf) + 1;
        return false;
    }
    if ((d = PTR_DIFF(p, buf)) >= size) {
        *result = d + 1;
        return false;
    }
    d = size - d;

    ASSERT_SIZE_INC_NO_OVERFLOW(d);
    d ++;

    if (*len < d) {
        if (*len == 0)
            ASSERT(*key == NULL);

        *len = d;
        *key = realloc(*key, d);
        VERIFY(*key != NULL);
    }
    memcpy(*key, p, d);

    *result = d;
    return true;
}

#undef  DATABASE_ERROR
#define DATABASE_ERROR(m, ...) \
    ((void) DATABASE_ERROR_(m, ## __VA_ARGS__))

static bool database_import_db(
    struct database_t* db,
    const struct options_t* opts)
{
    size_t n = 0, m = 0, i = 0, j = 0;
    char *b = NULL, *c = NULL;
    ssize_t r;
    FILE* f;
    char d;
    int s;

    ASSERT_ACTION_IS(import_db);

    if (!(s = strcmp(opts->act_arg, "-")))
        f = stdin;
    else
    if (!(f = fopen(opts->act_arg, "r")))
        return DATABASE_SYS_ERROR(
            "open '%s' failed",
            opts->act_arg);

    d = opts->null ? '\0' : '\n';

    while ((r = getdelim(&b, &n, d, f)) >= 0) {
        size_t l = INT_AS_SIZE(r);
        struct database_entry_t e;
        size_t p;
        datum k;

        ASSERT(b != NULL);
        ASSERT(n > 0);

        ASSERT(l > 0);
        ASSERT(l < n);

        if (b[l - 1] == d)
            b[-- l] = 0;

        i ++;

        if (l == 0) {
            DATABASE_ERROR(
                "%s:%zu: empty input line",
                s ? opts->act_arg : stdin_name, i);
            j ++;
            continue;
        }
        if (!parse_input_line(b, l, &e, &c, &m, &p)) {
            DATABASE_ERROR(
                "%s:%zu:%zu: invalid input line\n  %s\n  %*c",
                s ? opts->act_arg : stdin_name,
                i, p, normalize_for_print(b, l),
                SIZE_AS_INT(p), '^');
            j ++;
            continue;
        }

        ASSERT(c != NULL);
        ASSERT(p > 1);

        k.dptr = c;
        k.dsize = p;
        database_set_entry(db, k, e);

        if (opts->verbose > 1)
            DATABASE_LOG(
                "timestamp %zu on " HASH_KEY_FMT,
                e.timestamp, e.file_hash, c);
    }

    if (c != NULL) {
        ASSERT(m > 0);
        free(c);
    }
    if (b != NULL) {
        ASSERT(n > 0);
        free(b);
    }

    if (s) fclose(f);

    return j == 0;
}

static int dl_callback(
    struct dl_phdr_info* info,
    size_t size UNUSED, void* result)
{
    const char **r = result, *p;

    if (info->dlpi_name &&
        (p = strrchr(info->dlpi_name, '/')) &&
        !strncmp(p + 1, library, strlen(library))) {
        *r = info->dlpi_name;
        return 1;
    }
    return 0;
}

static char* lib_runtime_path(void)
{
    const char* r = NULL;

    if (!dl_iterate_phdr(dl_callback, &r))
        return NULL;
    ASSERT(r != NULL);

    return realpath(r, NULL);
}

#if !defined(GDBM_VERSION_MAJOR) || \
    !defined(GDBM_VERSION_MINOR) || \
    !defined(GDBM_VERSION_PATCH)
#if !defined(GDBM_VERSION)
#error GDBM_VERSION not defined
#elif GDBM_VERSION <= 0 || GDBM_VERSION >= 100000
#error GDBM_VERSION has an invalid value
#endif
#define GDBM_VERSION_MAJOR ((GDBM_VERSION / 10000))
#define GDBM_VERSION_MINOR ((GDBM_VERSION % 10000) / 100)
#define GDBM_VERSION_PATCH ((GDBM_VERSION % 10000) % 100)
#endif

struct lib_version_t
{
    size_t major;
    size_t minor;
    size_t patch;
    char*  vdate;
    char*  build;
    char*  rpath;
};

static void lib_version_init(
    struct lib_version_t* ver,
    bool extra_info)
{
    char y[5], m[3], d[3];
    const char *c, *v;
    struct tm t;
    int p[3];
    int n, l;

    memset(ver, 0, sizeof(*ver));

    // GDBM version 1.8.3. 10/15/2002 (built Feb 21 2009 06:52:31)
    v = gdbm_version;

    // stev: note that 'patch' could be missing; also 'vdate' could
    // be missing, e.g. when building GDBM out of a git source tree

    memset(p, 0, sizeof(p));
    n = sscanf(v, "GDBM version %zu.%zu.%n%zu.%n",
            &ver->major,
            &ver->minor,
            &p[0],
            &ver->patch,
            &p[1]);

    ASSERT(p[0] >= 0);
    ASSERT(p[1] >= 0);

    if (n < 2)
        error("failed parsing gdbm library version numbers");

    if (!p[1])
        ver->patch = 0;

    if (!extra_info)
        return;

    v += p[1] ? p[1] : p[0];

    if (sscanf(v, " %2[0-9]/%2[0-9]/%4[0-9]%n", m, d, y, &p[2]) != 3)
        goto get_build;

    ver->vdate = malloc(11);
    VERIFY(ver->vdate != NULL);
    n = snprintf(ver->vdate, 11, "%s-%s-%s", y, m, d);
    VERIFY(n == 10);

    ASSERT(p[2] >= 0);
    v += p[2];

get_build:
    if (sscanf(v, " (built %m[^)])", &ver->build) != 1)
        goto get_rpath;

    c = setlocale(LC_TIME, "en_US");
    VERIFY(c != NULL);

    memset(&t, 0, sizeof(t));
    v = strptime(ver->build, "%b %d %Y %H:%M:%S", &t);

    c = setlocale(LC_TIME, c);
    VERIFY(c != NULL);

    if (v == NULL) {
        free(ver->build);
        ver->build = NULL;
        goto get_rpath;
    }

    l = strlen(ver->build);
    n = strftime(ver->build, l + 1, iso_date_fmt, &t);
    VERIFY(n);

get_rpath:
    ver->rpath = lib_runtime_path();
}

static void lib_version_done(
    struct lib_version_t* ver)
{
    free(ver->vdate);
    free(ver->build);
    free(ver->rpath);
}

static void lib_version_check(void)
{
    struct lib_version_t v;

    lib_version_init(&v, false);

    if (v.major != GDBM_VERSION_MAJOR ||
        v.minor != GDBM_VERSION_MINOR)
        fatal_error(
            "gdbm library is not of expected "
            "version %d.%d but %zu.%zu.%zu",
            GDBM_VERSION_MAJOR,
            GDBM_VERSION_MINOR,
            v.major, v.minor,
            v.patch);
}

static void version(void)
{
#ifdef  DEBUG
#define DEBUG_NAME "yes"
#else
#define DEBUG_NAME "no"
#endif
#define NSTR(s) ((s) ? (s) : null)

    static const char null[] = "???";
    struct lib_version_t v;

    lib_version_init(&v, true);

    fprintf(stdout, 
        "%s: version %s\n"
        "%s: %s: version: %zu.%zu.%zu -- %s\n"
        "%s: %s: build: %s\n"
        "%s: %s: path: %s\n"
        "%s: TIME_BITS: %zu\n"
        "%s: HASH_BITS: %d\n"
        "%s: DEBUG: " DEBUG_NAME "\n\n%s",
        program, verdate,
        program, library, v.major, v.minor,
            v.patch, NSTR(v.vdate),
        program, library, NSTR(v.build),
        program, library, NSTR(v.rpath),
        program, TIME_BITS,
        program, HASH_BITS,
        program, license);

    lib_version_done(&v);
}

static size_t size_to_string(
    char* buf, size_t len, size_t val)
{
    size_t r;
    int n;

    n = snprintf(buf, len, "%zu", val);
    r = INT_AS_SIZE(n);

    ASSERT(n >= 0);
    ASSERT(r < len);

    return r;
}

static const char* threshold_to_string(
    char* buf, size_t len,
    size_t val)
{
    if (val == SIZE_MAX)
        return "inf";
    else {
        size_to_string(buf, len, val);
        return buf;
    }
}

static void usage(void)
{
    fprintf(stdout,
        "usage: %s [ACTION|OPTION]...\n"
        "where actions are specified as:\n"
        "  -M|--dump-meta        print out database's meta-data information\n"
        "  -D|--dump-raw         print out the content of the database on\n"
        "  -P|--dump-pretty        stdout in raw or pretty format; each\n"
        "                          line of output consists of three fields\n"
        "                          -- timestamp, hash and key strings --\n"
        "                          separated by TAB chars in case of action\n"
        "                          `-D|--dump-raw' or SPACE chars otherwise;\n"
        "                          the default action is `-D|--dump-raw'\n"
        "  -L|--lookup-key=STR   apply the named operation on the given key;\n"
        "  -U|--update-key=STR     note that to each key is associated a pair\n"
        "  -R|--remove-key=STR     <TIMESTAMP, HASH>; HASH is a %d-bit value:\n"
        "                          a hash digest of the STR itself; HASH does\n"
        "                          not change during the entire existence of\n"
        "                          STR within the database; TIMESTAMP records\n"
        "                          time of occurrence of update operations\n"
        "                          applied to the associated STR; not every\n"
        "                          key update operation changes timestamps,\n"
        "                          but only those which happen at a moment of\n"
        "                          time following that of the previous update\n"
        "                          operation by at least the amount specified\n"
        "                          by `-t|--threshold' or, otherwise, when the\n"
        "                          hash file exists and is empty\n"
        "  -E|--restore-key=STR  restore key STR to previosly saved entry under\n"
        "                          key 'STR$previous' (also remove the latter)\n"
        "  -I|--import-db=FILE   import data from given FILE ('-' means stdin);\n"
        "                          this file is expected to contain a series\n"
        "                          of records, terminated by NL or NUL chars,\n"
        "                          depending on options `-0|--[no-]null'; each\n"
        "                          record is form 'TIMESTAMP \\t HASH \\t KEY';\n"
        "                          TIMESTAMP is a %zu-bit decimal number, HASH\n"
        "                          is a %d-bit hexadecimal number and KEY is a\n"
        "                          non-empty string\n"
        "the options are:\n"
        "  -0|--[no-]null        end records with NUL or otherwise with NL char\n"
        "                          in output produced by actions `-D|--dump-raw'\n"
        "                          or `-P|--dump-pretty'; in case the action is\n"
        "                          `-I|--import-db', expect that the records of\n"
        "                          the input file be separated by NUL chars or\n"
        "                          else by NL chars; in both of the two cases,\n"
        "                          NL is the default record terminator char\n"
        "  -a|--hash-age[=NUM]   when the action specified implies printing out\n"
        "     --no-hash-age        timestamps -- `-D|-P|--dump-{raw,pretty}'\n"
        "                          and `-L|--lookup-key' --, do produce the age\n"
        "                          of hash file instead; for `-P|--dump-pretty'\n"
        "                          and `-L|--lookup-key' age values are also\n"
        "                          printed prettily; if NUM is given, compute\n"
        "                          hash age values relative to it interpreted\n"
        "                          as number of seconds since the Epoch\n"
        "  -d|--[no-]dry-run     when action is `-U|--update-key', if there is\n"
        "                          already a database entry associated to the\n"
        "                          given key, do not update its aged timestamp\n"
        "                          or otherwise do (default)\n"
        "  -e|--[no-]save-key    when action is `-U|--update-key', if there is\n"
        "                          already a database entry associated to the\n"
        "                          given key 'STR', preserve it under the key\n"
        "                          'STR$previous' or otherwise do not (default\n"
        "                          do not); `--save-key' implies `--prev-key'\n"
        "  -f|--hash-file=FILE   when action is `-U|--update-key', if option\n"
        "     --no-hash-file       `-e|--save-key' was given, move the given\n"
        "                          FILE over the cached hash file and update\n"
        "                          the key regardless of given threshold value\n"
        "  -h|--home=DIR         database, log and hash files home directory\n"
        "  -k|--[no-]keep-prev   when action is `-U|--update-key', backup the\n"
        "                          hash file attached to the key 'STR$previous'\n"
        "                          when given `-e|--save-key' too, or otherwise\n"
        "                          do not (default not)\n"
        "  -l|--[no-]log         do log out the operations applied on the\n"
        "                          '$home/cache.db' database in the file\n"
        "                          '$home/cache.log' or otherwise do not\n"
        "                          (default not)\n"
        "  -p|--[no-]prev-key    when action is `-U|--update-key', if there is\n"
        "                          already a database entry associated to the\n"
        "                          given key 'STR', output the hash file name\n"
        "                          of key 'STR$previous' too, if there is one,\n"
        "                          or otherwise do not (default not); likewise,\n"
        "                          if action is `-L|--lookup-key', along with\n"
        "                          the info printed out for given key 'STR',\n"
        "                          print the info of key 'STR$previous' too\n"
        "  -r|--[no-]error       suppress error reports on stderr or otherwise\n"
        "                          do not (default do not); note that reporting\n"
        "                          fatal errors on stderr is never suppressed\n"
        "  -s|--[no-]sync        make each database library operation be\n"
        "                          synchronized to disk prior to returning\n"
        "                          to application (i.e. open the database\n"
        "                          with GDBM_SYNC) or, otherwise, do not\n"
        "                          (default do not)\n"
        "  -t|--threshold=VAL    the threshold value that, when exceeded by\n"
        "                          the amount of time elapsed since the last\n"
        "                          update operation on the given key, makes\n"
        "                          the key's associated hash file be updated;\n"
        "                          VAL is 'inf' or of form '[0-9]+[dhms]?'\n"
        "     --dump-opts        print options and exit\n"
        "  -v|--[no-]verbose     be verbose (cumulative) or not\n"
        "  -V|--version          print version numbers and exit\n"
        "  -?|--help             display this help info and exit\n",
        program, HASH_BITS, TIME_BITS, HASH_BITS);
}

static void dump_options(const struct options_t* opts)
{
    static const char* const actions[] = {
        [options_action_dump_meta]   = "dump-meta",
        [options_action_dump_raw]    = "dump-raw",
        [options_action_dump_pretty] = "dump-pretty",
        [options_action_lookup_key]  = "lookup-key",
        [options_action_update_key]  = "update-key",
        [options_action_remove_key]  = "remove-key",
        [options_action_restore_key] = "restore-key",
        [options_action_import_db]   = "import-db",
    };
    static const char* const noyes[] = {
        [0] = "no",
        [1] = "yes"
    };

    const size_t m = TYPE_DIGITS10(size_t) + 1;
    char* const* p;
    char b[m];
    size_t i;

    fprintf(stdout,
        "action:    %s\n"
        "actarg:    %s\n"
        "home:      %s\n"
        "dry-run:   %s\n"
        "age-base:  %zu\n"
        "hash-age:  %s\n"
        "hash-file: %s\n"
        "keep-prev: %s\n"
        "null:      %s\n"
        "log:       %s\n"
        "prev-key:  %s\n"
        "save-key:  %s\n"
        "no-error:  %s\n"
        "sync:      %s\n"
        "threshold: %s\n"
        "verbose:   %zu\n"
        "argc:      %zu\n",
#define NAME0(X, T)                  \
    ({                               \
        size_t __v = opts->X;        \
        ARRAY_NON_NULL_ELEM(T, __v); \
    })
#define NAME(X)  (NAME0(X, X ## s))
#define NNUL(X)  (opts->X ? opts->X : "-")
#define NOYES(X) (NAME0(X, noyes))
        NAME(action),
        NNUL(act_arg),
        NNUL(home),
        NOYES(dry_run),
        opts->age_base,
        NOYES(hash_age),
        NNUL(hash_file),
        NOYES(keep_prev),
        NOYES(null),
        NOYES(log),
        NOYES(prev_key),
        NOYES(save_key),
        NOYES(no_error),
        NOYES(sync),
#undef  NOYES
#undef  NNUL
#undef  NAME
#undef  NAME0
        threshold_to_string(b, m, opts->threshold),
        BITS_AS_SIZE(opts->verbose),
        opts->argc);

    for (i = 0,
         p = opts->argv;
         i < opts->argc;
         i ++,
         p ++) {
        const size_t w = 4;
        size_t n;

        n = size_to_string(b, m, i);
        ASSERT(n < m);

        fprintf(stdout,
            "argv[%s]:%*s%s\n",
            b, w > n ? SIZE_AS_INT(w - n) : 0, "", *p);
    }
}

static void invalid_opt_arg(const char* opt_name, const char* opt_arg)
{
    error("invalid argument for '%s' option: '%s'", opt_name, opt_arg);
}

static void invalid_opt_arg2(const char* opt_name, const char* opt_arg,
    const char* reason)
{
    error("invalid argument for '%s' option: '%s': %s", opt_name, opt_arg,
        reason);
}

static void illegal_opt_arg(const char* opt_name, const char* opt_arg)
{
    error("illegal argument for '%s' option: '%s'", opt_name, opt_arg);
}

static void missing_opt_arg_str(const char* opt_name)
{
    error("argument for option '%s' not found", opt_name);
}

static void missing_opt_arg_ch(char opt_name)
{
    error("argument for option '-%c' not found", opt_name);
}

static void not_allowed_opt_arg(const char* opt_name)
{
    error("option '%s' does not allow an argument", opt_name);
}

static void invalid_opt_str(const char* opt_name)
{
    error("invalid command line option '%s'", opt_name);
}

static void invalid_opt_ch(char opt_name)
{
    error("invalid command line option '-%c'", opt_name);
}

static const char* parse_dir_optarg(
    const char* opt_name, const char* opt_arg)
{
    struct stat s;
    int r, e;

    r = stat(opt_arg, &s);
    e = errno;

    if (r && e != ENOENT && e != ENOTDIR)
        sys_error("%s: stat failed", opt_arg);
    if (r || !S_ISDIR(s.st_mode))
        invalid_opt_arg2(opt_name, opt_arg,
            r ? "directory not found"
              : "not a directory");

    return opt_arg;
}

static size_t parse_size_optarg(
    const char* opt_name, const char* opt_arg,
    size_t min, size_t max, const char** end)
{
    const char *p, *q = NULL;
    size_t n, v = 0;

    if (!(n = strlen(opt_arg)))
        invalid_opt_arg(opt_name, opt_arg);

    if (!parse_size(p = opt_arg, &q, 10, &v) ||
        (!end && PTR_OFFSET(q, p, n) != n))
        invalid_opt_arg(opt_name, opt_arg);

    if ((min > 0 && v < min) ||
        (max > 0 && v > max))
        illegal_opt_arg(opt_name, opt_arg);

    if (end)
        *end = q;

    return v;
}

static size_t parse_timestamp_optarg(
    const char* opt_name, const char* opt_arg)
{
    const char *p = NULL;
    size_t v;

    if (opt_arg == NULL)
        return current_timestamp();

    v = parse_size_optarg(
            opt_name, opt_arg, 0, 0, &p);
    ASSERT(p != NULL);

    if (*p)
        invalid_opt_arg(opt_name, opt_arg);

    return v;
}

static size_t parse_threshold_optarg(
    const char* opt_name, const char* opt_arg)
{
    static const size_t mul[] = {
        1, 60, 60 * 60, 24 * 60 * 60
    };
    static const char suf[] = "smhd";

    const size_t hy = 100 * 365;
    const char *p = NULL;
    size_t v, i;
    char c;

    if (!strcmp(opt_arg, "inf"))
        return SIZE_MAX;

    v = parse_size_optarg(
            opt_name, opt_arg, 0, 0, &p);
    ASSERT(p != NULL);

    if (*p == 0)
        return v;

    c = *p;
    if (c == 'd' ||
        c == 'h' ||
        c == 'm' ||
        c == 's')
        p ++;

    if (*p != 0)
        invalid_opt_arg(opt_name, opt_arg);

    p = strchr(suf, c);
    ASSERT(p != NULL);

    i = PTR_DIFF(p, suf);
    ASSERT(i < ARRAY_SIZE(mul));

    if (!SIZE_MUL_NO_OVERFLOW(v, mul[i]))
        illegal_opt_arg(opt_name, opt_arg);

    v *= mul[i];

    // stev: arbitrary upper bound: 100 years
    if (v > SIZE_MUL(hy, mul[3]))
        illegal_opt_arg(opt_name, opt_arg);

    return v;
}

const struct options_t* options(int argc, char* argv[])
{
    static struct options_t opts = {
        .action    = options_action_dump_raw,
        .act_arg   = NULL,
        .home      = NULL,
        .hash_file = NULL,
        .threshold = 1800,
        .dry_run   = false,
        .age_base  = 0,
        .hash_age  = false,
        .keep_prev = false,
        .null      = false,
        .log       = false,
        .prev_key  = false,
        .save_key  = false,
        .no_error  = false,
        .sync      = false,
        .verbose   = 0,
        .argc      = 0,
        .argv      = NULL
    };

    enum {
        // stev: actions:
        dump_meta_act   = 'M',
        dump_raw_act    = 'D',
        dump_pretty_act = 'P',
        lookup_key_act  = 'L',
        update_key_act  = 'U',
        remove_key_act  = 'R',
        restore_key_act = 'E',
        import_db_act   = 'I',

        // stev: options:
        null_opt        = '0',
        hash_age_opt    = 'a',
        dry_run_opt     = 'd',
        save_key_opt    = 'e',
        hash_file_opt   = 'f',
        home_opt        = 'h',
        keep_prev_opt   = 'k',
        log_opt         = 'l',
        prev_key_opt    = 'p',
        no_error_opt    = 'r',
        sync_opt        = 's',
        threshold_opt   = 't',
        verbose_opt     = 'v',
        version_opt     = 'V',
        help_opt        = '?',

        dump_opts_opt   = 128,

        no_dry_run_opt,
        no_hash_age_opt,
        no_hash_file_opt,
        no_keep_prev_opt,
        no_null_opt,
        no_log_opt,
        no_prev_key_opt,
        no_save_key_opt,
        no_no_error_opt,
        no_sync_opt,
        no_verbose_opt,
    };

    static struct option long_opts[] = {
        { "dump-meta",    0,       0, dump_meta_act },
        { "dump-raw",     0,       0, dump_raw_act },
        { "dump-pretty",  0,       0, dump_pretty_act },
        { "lookup-key",   1,       0, lookup_key_act },
        { "update-key",   1,       0, update_key_act },
        { "remove-key",   1,       0, remove_key_act },
        { "restore-key",  1,       0, restore_key_act },
        { "import-db",    1,       0, import_db_act },
        { "home",         1,       0, home_opt },
        { "dry-run",      0,       0, dry_run_opt },
        { "no-dry-run",   0,       0, no_dry_run_opt },
        { "hash-age",     2,       0, hash_age_opt },
        { "no-hash-age",  0,       0, no_hash_age_opt },
        { "hash-file",    1,       0, hash_file_opt },
        { "no-hash-file", 0,       0, no_hash_file_opt },
        { "keep-prev",    0,       0, keep_prev_opt },
        { "no-keep-prev", 0,       0, no_keep_prev_opt },
        { "null",         0,       0, null_opt },
        { "no-null",      0,       0, no_null_opt },
        { "log",          0,       0, log_opt },
        { "no-log",       0,       0, no_log_opt },
        { "prev-key",     0,       0, prev_key_opt },
        { "no-prev-key",  0,       0, no_prev_key_opt },
        { "save-key",     0,       0, save_key_opt },
        { "no-save-key",  0,       0, no_save_key_opt },
        { "no-error",     0,       0, no_error_opt },
        { "error",        0,       0, no_no_error_opt },
        { "sync",         0,       0, sync_opt },
        { "no-sync",      0,       0, no_sync_opt },
        { "threshold",    1,       0, threshold_opt },
        { "dump-opts",    0,       0, dump_opts_opt },
        { "version",      0,       0, version_opt },
        { "verbose",      0,       0, verbose_opt },
        { "no-verbose",   0,       0, no_verbose_opt },
        { "help",         0, &optopt, help_opt },
        { 0,              0,       0, 0 }
    };
    static const char short_opts[] =
        ":"
        "DE:I:L:MPR:U:"
        "0a::def:h:klprst:vV";

    struct bits_opts_t {
        bits_t dump: 1;
        bits_t usage: 1;
        bits_t version: 1;
    };
    struct bits_opts_t bits = {
        .dump    = false,
        .usage   = false,
        .version = false,
    };

    int opt;

#define argv_optind()                      \
    ({                                     \
        size_t i = INT_AS_SIZE(optind);    \
        ASSERT_SIZE_DEC_NO_OVERFLOW(i);    \
        ASSERT(i - 1 < INT_AS_SIZE(argc)); \
        argv[i - 1];                       \
    })

#define optopt_char()                   \
    ({                                  \
        ASSERT(ISASCII((char) optopt)); \
        (char) optopt;                  \
    })

    opterr = 0;
    optind = 1;
    while ((opt = getopt_long(argc, argv, short_opts,
        long_opts, 0)) != EOF) {
        switch (opt) {
        case dump_meta_act:
            opts.action = options_action_dump_meta;
            opts.act_arg = NULL;
            break;
        case dump_raw_act:
            opts.action = options_action_dump_raw;
            opts.act_arg = NULL;
            break;
        case dump_pretty_act:
            opts.action = options_action_dump_pretty;
            opts.act_arg = NULL;
            break;
        case lookup_key_act:
            opts.action = options_action_lookup_key;
            opts.act_arg = optarg;
            break;
        case update_key_act:
            opts.action = options_action_update_key;
            opts.act_arg = optarg;
            break;
        case remove_key_act:
            opts.action = options_action_remove_key;
            opts.act_arg = optarg;
            break;
        case restore_key_act:
            opts.action = options_action_restore_key;
            opts.act_arg = optarg;
            break;
        case import_db_act:
            opts.action = options_action_import_db;
            opts.act_arg = optarg;
            break;
        case home_opt:
            opts.home = parse_dir_optarg(
                "home", optarg);
            break;
        case dry_run_opt:
            opts.dry_run = true;
            break;
        case no_dry_run_opt:
            opts.dry_run = false;
            break;
        case hash_age_opt:
            opts.hash_age = true;
            opts.age_base = parse_timestamp_optarg(
                "hash-age", optarg);
            break;
        case no_hash_age_opt:
            opts.hash_age = false;
            opts.age_base = 0;
            break;
        case hash_file_opt:
            opts.hash_file = optarg;
            break;
        case no_hash_file_opt:
            opts.hash_file = NULL;
            break;
        case keep_prev_opt:
            opts.keep_prev = true;
            break;
        case no_keep_prev_opt:
            opts.keep_prev = false;
            break;
        case null_opt:
            opts.null = true;
            break;
        case no_null_opt:
            opts.null = false;
            break;
        case log_opt:
            opts.log = true;
            break;
        case no_log_opt:
            opts.log = false;
            break;
        case prev_key_opt:
            opts.prev_key = true;
            break;
        case no_prev_key_opt:
            opts.prev_key = false;
            break;
        case save_key_opt:
            opts.save_key = true;
            break;
        case no_save_key_opt:
            opts.save_key = false;
            break;
        case no_error_opt:
            opts.no_error = true;
            break;
        case no_no_error_opt:
            opts.no_error = false;
            break;
        case sync_opt:
            opts.sync = true;
            break;
        case no_sync_opt:
            opts.sync = false;
            break;
        case threshold_opt:
            opts.threshold = parse_threshold_optarg(
                "threshold", optarg);
            break;
        case dump_opts_opt:
            bits.dump = true;
            break;
        case version_opt:
            bits.version = true;
            break;
        case verbose_opt: {
            size_t v = opts.verbose + 1;
            opts.verbose = SIZE_TRUNC_BITS(v, 2);
            break;
        }
        case no_verbose_opt:
            opts.verbose = 0;
            break;
        case 0:
            bits.usage = true;
            break;
        case ':': {
            const char* opt = argv_optind();
            if (opt[0] == '-' && opt[1] == '-')
                missing_opt_arg_str(opt);
            else
                missing_opt_arg_ch(optopt_char());
            break;
        }
        case '?':
        default:
            if (optopt == 0)
                invalid_opt_str(argv_optind());
            else
            if (optopt != '?') {
                char* opt = argv_optind();
                if (opt[0] == '-' && opt[1] == '-') {
                    char* end = strchr(opt, '=');
                    if (end) *end = '\0';
                    not_allowed_opt_arg(opt);
                }
                else
                    invalid_opt_ch(optopt_char());
            }
            else
                bits.usage = true;
            break;
        }
    }

    ASSERT(argc >= optind);

    argc -= optind;
    argv += optind;

    if (argc > 0) {
        argc --;
        argv ++;
    }

    opts.argc = argc;
    opts.argv = argv;

    if (bits.version)
        version();
    if (bits.dump)
        dump_options(&opts);
    if (bits.usage)
        usage();

    if (bits.dump ||
        bits.version ||
        bits.usage)
        exit(0);

    if (opts.home == NULL)
        error("home directory was not given");
    if (chdir(opts.home))
        sys_error("chdir failed on home dir");

    lib_version_check();

    return &opts;
}

int main(int argc, char* argv[])
{
    typedef bool (*database_func_t)(
        struct database_t*, const struct options_t*);
    static const database_func_t funcs[] = {
        [options_action_dump_meta]   = database_dump_metadata,
        [options_action_dump_raw]    = database_dump_raw,
        [options_action_dump_pretty] = database_dump_pretty,
        [options_action_lookup_key]  = database_lookup_key,
        [options_action_update_key]  = database_update_key,
        [options_action_remove_key]  = database_remove_key,
        [options_action_restore_key] = database_restore_key,
        [options_action_import_db]   = database_import_db,
    };

    const struct options_t* opts = options(argc, argv);

    database_func_t func;
    struct database_t db;
    bool r;

    if (!(func = ARRAY_NULL_ELEM(funcs, opts->action)))
        UNEXPECT_VAR("%d", opts->action);

    database_init(&db, opts);
    r = func(&db, opts);
    database_done(&db);

    return !r;
}


