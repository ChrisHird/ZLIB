000100000000/* zutil.c -- target dependent utility functions for the compression library
000200000000 * Copyright (C) 1995-2005 Jean-loup Gailly.
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/* @(#) $Id$ */
000700000000
000800000000#include "zutil.h"
000900000000
001000000000#ifndef NO_DUMMY_DECL
001100000000struct internal_state      {int dummy;}; /* for buggy compilers */
001200000000#endif
001300000000
001400000000const char * const z_errmsg[10] = {
001500000000"need dictionary",     /* Z_NEED_DICT       2  */
001600000000"stream end",          /* Z_STREAM_END      1  */
001700000000"",                    /* Z_OK              0  */
001800000000"file error",          /* Z_ERRNO         (-1) */
001900000000"stream error",        /* Z_STREAM_ERROR  (-2) */
002000000000"data error",          /* Z_DATA_ERROR    (-3) */
002100000000"insufficient memory", /* Z_MEM_ERROR     (-4) */
002200000000"buffer error",        /* Z_BUF_ERROR     (-5) */
002300000000"incompatible version",/* Z_VERSION_ERROR (-6) */
002400000000""};
002500000000
002600000000
002700000000const char * ZEXPORT zlibVersion()
002800000000{
002900000000    return ZLIB_VERSION;
003000000000}
003100000000
003200000000uLong ZEXPORT zlibCompileFlags()
003300000000{
003400000000    uLong flags;
003500000000
003600000000    flags = 0;
003700000000    switch (sizeof(uInt)) {
003800000000    case 2:     break;
003900000000    case 4:     flags += 1;     break;
004000000000    case 8:     flags += 2;     break;
004100000000    default:    flags += 3;
004200000000    }
004300000000    switch (sizeof(uLong)) {
004400000000    case 2:     break;
004500000000    case 4:     flags += 1 << 2;        break;
004600000000    case 8:     flags += 2 << 2;        break;
004700000000    default:    flags += 3 << 2;
004800000000    }
004900000000    switch (sizeof(voidpf)) {
005000000000    case 2:     break;
005100000000    case 4:     flags += 1 << 4;        break;
005200000000    case 8:     flags += 2 << 4;        break;
005300000000    default:    flags += 3 << 4;
005400000000    }
005500000000    switch (sizeof(z_off_t)) {
005600000000    case 2:     break;
005700000000    case 4:     flags += 1 << 6;        break;
005800000000    case 8:     flags += 2 << 6;        break;
005900000000    default:    flags += 3 << 6;
006000000000    }
006100000000#ifdef DEBUG
006200000000    flags += 1 << 8;
006300000000#endif
006400000000#if defined(ASMV) || defined(ASMINF)
006500000000    flags += 1 << 9;
006600000000#endif
006700000000#ifdef ZLIB_WINAPI
006800000000    flags += 1 << 10;
006900000000#endif
007000000000#ifdef BUILDFIXED
007100000000    flags += 1 << 12;
007200000000#endif
007300000000#ifdef DYNAMIC_CRC_TABLE
007400000000    flags += 1 << 13;
007500000000#endif
007600000000#ifdef NO_GZCOMPRESS
007700000000    flags += 1L << 16;
007800000000#endif
007900000000#ifdef NO_GZIP
008000000000    flags += 1L << 17;
008100000000#endif
008200000000#ifdef PKZIP_BUG_WORKAROUND
008300000000    flags += 1L << 20;
008400000000#endif
008500000000#ifdef FASTEST
008600000000    flags += 1L << 21;
008700000000#endif
008800000000#ifdef STDC
008900000000#  ifdef NO_vsnprintf
009000000000        flags += 1L << 25;
009100000000#    ifdef HAS_vsprintf_void
009200000000        flags += 1L << 26;
009300000000#    endif
009400000000#  else
009500000000#    ifdef HAS_vsnprintf_void
009600000000        flags += 1L << 26;
009700000000#    endif
009800000000#  endif
009900000000#else
010000000000        flags += 1L << 24;
010100000000#  ifdef NO_snprintf
010200000000        flags += 1L << 25;
010300000000#    ifdef HAS_sprintf_void
010400000000        flags += 1L << 26;
010500000000#    endif
010600000000#  else
010700000000#    ifdef HAS_snprintf_void
010800000000        flags += 1L << 26;
010900000000#    endif
011000000000#  endif
011100000000#endif
011200000000    return flags;
011300000000}
011400000000
011500000000#ifdef DEBUG
011600000000
011700000000#  ifndef verbose
011800000000#    define verbose 0
011900000000#  endif
012000000000int z_verbose = verbose;
012100000000
012200000000void z_error (m)
012300000000    char *m;
012400000000{
012500000000    fprintf(stderr, "%s\n", m);
012600000000    exit(1);
012700000000}
012800000000#endif
012900000000
013000000000/* exported to allow conversion of error code to string for compress() and
013100000000 * uncompress()
013200000000 */
013300000000const char * ZEXPORT zError(err)
013400000000    int err;
013500000000{
013600000000    return ERR_MSG(err);
013700000000}
013800000000
013900000000#if defined(_WIN32_WCE)
014000000000    /* The Microsoft C Run-Time Library for Windows CE doesn't have
014100000000     * errno.  We define it as a global variable to simplify porting.
014200000000     * Its value is always 0 and should not be used.
014300000000     */
014400000000    int errno = 0;
014500000000#endif
014600000000
014700000000#ifndef HAVE_MEMCPY
014800000000
014900000000void zmemcpy(dest, source, len)
015000000000    Bytef* dest;
015100000000    const Bytef* source;
015200000000    uInt  len;
015300000000{
015400000000    if (len == 0) return;
015500000000    do {
015600000000        *dest++ = *source++; /* ??? to be unrolled */
015700000000    } while (--len != 0);
015800000000}
015900000000
016000000000int zmemcmp(s1, s2, len)
016100000000    const Bytef* s1;
016200000000    const Bytef* s2;
016300000000    uInt  len;
016400000000{
016500000000    uInt j;
016600000000
016700000000    for (j = 0; j < len; j++) {
016800000000        if (s1[j] != s2[j]) return 2*(s1[j] > s2[j])-1;
016900000000    }
017000000000    return 0;
017100000000}
017200000000
017300000000void zmemzero(dest, len)
017400000000    Bytef* dest;
017500000000    uInt  len;
017600000000{
017700000000    if (len == 0) return;
017800000000    do {
017900000000        *dest++ = 0;  /* ??? to be unrolled */
018000000000    } while (--len != 0);
018100000000}
018200000000#endif
018300000000
018400000000
018500000000#ifdef SYS16BIT
018600000000
018700000000#ifdef __TURBOC__
018800000000/* Turbo C in 16-bit mode */
018900000000
019000000000#  define MY_ZCALLOC
019100000000
019200000000/* Turbo C malloc() does not allow dynamic allocation of 64K bytes
019300000000 * and farmalloc(64K) returns a pointer with an offset of 8, so we
019400000000 * must fix the pointer. Warning: the pointer must be put back to its
019500000000 * original form in order to free it, use zcfree().
019600000000 */
019700000000
019800000000#define MAX_PTR 10
019900000000/* 10*64K = 640K */
020000000000
020100000000local int next_ptr = 0;
020200000000
020300000000typedef struct ptr_table_s {
020400000000    voidpf org_ptr;
020500000000    voidpf new_ptr;
020600000000} ptr_table;
020700000000
020800000000local ptr_table table[MAX_PTR];
020900000000/* This table is used to remember the original form of pointers
021000000000 * to large buffers (64K). Such pointers are normalized with a zero offset.
021100000000 * Since MSDOS is not a preemptive multitasking OS, this table is not
021200000000 * protected from concurrent access. This hack doesn't work anyway on
021300000000 * a protected system like OS/2. Use Microsoft C instead.
021400000000 */
021500000000
021600000000voidpf zcalloc (voidpf opaque, unsigned items, unsigned size)
021700000000{
021800000000    voidpf buf = opaque; /* just to make some compilers happy */
021900000000    ulg bsize = (ulg)items*size;
022000000000
022100000000    /* If we allocate less than 65520 bytes, we assume that farmalloc
022200000000     * will return a usable pointer which doesn't have to be normalized.
022300000000     */
022400000000    if (bsize < 65520L) {
022500000000        buf = farmalloc(bsize);
022600000000        if (*(ush*)&buf != 0) return buf;
022700000000    } else {
022800000000        buf = farmalloc(bsize + 16L);
022900000000    }
023000000000    if (buf == NULL || next_ptr >= MAX_PTR) return NULL;
023100000000    table[next_ptr].org_ptr = buf;
023200000000
023300000000    /* Normalize the pointer to seg:0 */
023400000000    *((ush*)&buf+1) += ((ush)((uch*)buf-0) + 15) >> 4;
023500000000    *(ush*)&buf = 0;
023600000000    table[next_ptr++].new_ptr = buf;
023700000000    return buf;
023800000000}
023900000000
024000000000void  zcfree (voidpf opaque, voidpf ptr)
024100000000{
024200000000    int n;
024300000000    if (*(ush*)&ptr != 0) { /* object < 64K */
024400000000        farfree(ptr);
024500000000        return;
024600000000    }
024700000000    /* Find the original pointer */
024800000000    for (n = 0; n < next_ptr; n++) {
024900000000        if (ptr != table[n].new_ptr) continue;
025000000000
025100000000        farfree(table[n].org_ptr);
025200000000        while (++n < next_ptr) {
025300000000            table[n-1] = table[n];
025400000000        }
025500000000        next_ptr--;
025600000000        return;
025700000000    }
025800000000    ptr = opaque; /* just to make some compilers happy */
025900000000    Assert(0, "zcfree: ptr not found");
026000000000}
026100000000
026200000000#endif /* __TURBOC__ */
026300000000
026400000000
026500000000#ifdef M_I86
026600000000/* Microsoft C in 16-bit mode */
026700000000
026800000000#  define MY_ZCALLOC
026900000000
027000000000#if (!defined(_MSC_VER) || (_MSC_VER <= 600))
027100000000#  define _halloc  halloc
027200000000#  define _hfree   hfree
027300000000#endif
027400000000
027500000000voidpf zcalloc (voidpf opaque, unsigned items, unsigned size)
027600000000{
027700000000    if (opaque) opaque = 0; /* to make compiler happy */
027800000000    return _halloc((long)items, size);
027900000000}
028000000000
028100000000void  zcfree (voidpf opaque, voidpf ptr)
028200000000{
028300000000    if (opaque) opaque = 0; /* to make compiler happy */
028400000000    _hfree(ptr);
028500000000}
028600000000
028700000000#endif /* M_I86 */
028800000000
028900000000#endif /* SYS16BIT */
029000000000
029100000000
029200000000#ifndef MY_ZCALLOC /* Any system without a special alloc function */
029300000000
029400000000#ifndef STDC
029500000000extern voidp  malloc OF((uInt size));
029600000000extern voidp  calloc OF((uInt items, uInt size));
029700000000extern void   free   OF((voidpf ptr));
029800000000#endif
029900000000
030000000000voidpf zcalloc (opaque, items, size)
030100000000    voidpf opaque;
030200000000    unsigned items;
030300000000    unsigned size;
030400000000{
030500000000    if (opaque) items += size - size; /* make compiler happy */
030600000000    return sizeof(uInt) > 2 ? (voidpf)malloc(items * size) :
030700000000                              (voidpf)calloc(items, size);
030800000000}
030900000000
031000000000void  zcfree (opaque, ptr)
031100000000    voidpf opaque;
031200000000    voidpf ptr;
031300000000{
031400000000    free(ptr);
031500000000    if (opaque) return; /* make compiler happy */
031600000000}
031700000000
031800000000#endif /* MY_ZCALLOC */
