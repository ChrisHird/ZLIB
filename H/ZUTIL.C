000100000000/* zutil.h -- internal interface and configuration of the compression library
000200000000 * Copyright (C) 1995-2005 Jean-loup Gailly.
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/* WARNING: this file should *not* be used by applications. It is
000700000000   part of the implementation of the compression library and is
000800000000   subject to change. Applications should only use zlib.h.
000900000000 */
001000000000
001100000000/* @(#) $Id$ */
001200000000
001300000000#ifndef ZUTIL_H
001400000000#define ZUTIL_H
001500000000
001600000000#define ZLIB_INTERNAL
001700000000#include "zlib.h"
001800000000
001900000000#ifdef STDC
002000000000#  ifndef _WIN32_WCE
002100000000#    include <stddef.h>
002200000000#  endif
002300000000#  include <string.h>
002400000000#  include <stdlib.h>
002500000000#endif
002600000000#ifdef NO_ERRNO_H
002700000000#   ifdef _WIN32_WCE
002800000000      /* The Microsoft C Run-Time Library for Windows CE doesn't have
002900000000       * errno.  We define it as a global variable to simplify porting.
003000000000       * Its value is always 0 and should not be used.  We rename it to
003100000000       * avoid conflict with other libraries that use the same workaround.
003200000000       */
003300000000#     define errno z_errno
003400000000#   endif
003500000000    extern int errno;
003600000000#else
003700000000#  ifndef _WIN32_WCE
003800000000#    include <errno.h>
003900000000#  endif
004000000000#endif
004100000000
004200000000#ifndef local
004300000000#  define local static
004400000000#endif
004500000000/* compile with -Dlocal if your debugger can't find static symbols */
004600000000
004700000000typedef unsigned char  uch;
004800000000typedef uch FAR uchf;
004900000000typedef unsigned short ush;
005000000000typedef ush FAR ushf;
005100000000typedef unsigned long  ulg;
005200000000
005300000000extern const char * const z_errmsg[10]; /* indexed by 2-zlib_error */
005400000000/* (size given to avoid silly warnings with Visual C++) */
005500000000
005600000000#define ERR_MSG(err) z_errmsg[Z_NEED_DICT-(err)]
005700000000
005800000000#define ERR_RETURN(strm,err) \
005900000000  return (strm->msg = (char*)ERR_MSG(err), (err))
006000000000/* To be used only when the state is known to be valid */
006100000000
006200000000        /* common constants */
006300000000
006400000000#ifndef DEF_WBITS
006500000000#  define DEF_WBITS MAX_WBITS
006600000000#endif
006700000000/* default windowBits for decompression. MAX_WBITS is for compression only */
006800000000
006900000000#if MAX_MEM_LEVEL >= 8
007000000000#  define DEF_MEM_LEVEL 8
007100000000#else
007200000000#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
007300000000#endif
007400000000/* default memLevel */
007500000000
007600000000#define STORED_BLOCK 0
007700000000#define STATIC_TREES 1
007800000000#define DYN_TREES    2
007900000000/* The three kinds of block type */
008000000000
008100000000#define MIN_MATCH  3
008200000000#define MAX_MATCH  258
008300000000/* The minimum and maximum match lengths */
008400000000
008500000000#define PRESET_DICT 0x20 /* preset dictionary flag in zlib header */
008600000000
008700000000        /* target dependencies */
008800000000
008900000000#if defined(MSDOS) || (defined(WINDOWS) && !defined(WIN32))
009000000000#  define OS_CODE  0x00
009100000000#  if defined(__TURBOC__) || defined(__BORLANDC__)
009200000000#    if(__STDC__ == 1) && (defined(__LARGE__) || defined(__COMPACT__))
009300000000       /* Allow compilation with ANSI keywords only enabled */
009400000000       void _Cdecl farfree( void *block );
009500000000       void *_Cdecl farmalloc( unsigned long nbytes );
009600000000#    else
009700000000#      include <alloc.h>
009800000000#    endif
009900000000#  else /* MSC or DJGPP */
010000000000#    include <malloc.h>
010100000000#  endif
010200000000#endif
010300000000
010400000000#ifdef AMIGA
010500000000#  define OS_CODE  0x01
010600000000#endif
010700000000
010800000000#if defined(VAXC) || defined(VMS)
010900000000#  define OS_CODE  0x02
011000000000#  define F_OPEN(name, mode) \
011100000000     fopen((name), (mode), "mbc=60", "ctx=stm", "rfm=fix", "mrs=512")
011200000000#endif
011300000000
011400000000#if defined(ATARI) || defined(atarist)
011500000000#  define OS_CODE  0x05
011600000000#endif
011700000000
011800000000#ifdef OS2
011900000000#  define OS_CODE  0x06
012000000000#  ifdef M_I86
012100000000     #include <malloc.h>
012200000000#  endif
012300000000#endif
012400000000
012500000000#if defined(MACOS) || defined(TARGET_OS_MAC)
012600000000#  define OS_CODE  0x07
012700000000#  if defined(__MWERKS__) && __dest_os != __be_os && __dest_os != __win32_os
012800000000#    include <unix.h> /* for fdopen */
012900000000#  else
013000000000#    ifndef fdopen
013100000000#      define fdopen(fd,mode) NULL /* No fdopen() */
013200000000#    endif
013300000000#  endif
013400000000#endif
013500000000
013600000000#ifdef TOPS20
013700000000#  define OS_CODE  0x0a
013800000000#endif
013900000000
014000000000#ifdef WIN32
014100000000#  ifndef __CYGWIN__  /* Cygwin is Unix, not Win32 */
014200000000#    define OS_CODE  0x0b
014300000000#  endif
014400000000#endif
014500000000
014600000000#ifdef __50SERIES /* Prime/PRIMOS */
014700000000#  define OS_CODE  0x0f
014800000000#endif
014900000000
015000000000#if defined(_BEOS_) || defined(RISCOS)
015100000000#  define fdopen(fd,mode) NULL /* No fdopen() */
015200000000#endif
015300000000
015400000000#if (defined(_MSC_VER) && (_MSC_VER > 600))
015500000000#  if defined(_WIN32_WCE)
015600000000#    define fdopen(fd,mode) NULL /* No fdopen() */
015700000000#    ifndef _PTRDIFF_T_DEFINED
015800000000       typedef int ptrdiff_t;
015900000000#      define _PTRDIFF_T_DEFINED
016000000000#    endif
016100000000#  else
016200000000#    define fdopen(fd,type)  _fdopen(fd,type)
016300000000#  endif
016400000000#endif
016500000000
016600000000        /* common defaults */
016700000000
016800000000#ifndef OS_CODE
016900000000#  define OS_CODE  0x03  /* assume Unix */
017000000000#endif
017100000000
017200000000#ifndef F_OPEN
017300000000#  define F_OPEN(name, mode) fopen((name), (mode))
017400000000#endif
017500000000
017600000000         /* functions */
017700000000
017800000000#if defined(STDC99) || (defined(__TURBOC__) && __TURBOC__ >= 0x550)
017900000000#  ifndef HAVE_VSNPRINTF
018000000000#    define HAVE_VSNPRINTF
018100000000#  endif
018200000000#endif
018300000000#if defined(__CYGWIN__)
018400000000#  ifndef HAVE_VSNPRINTF
018500000000#    define HAVE_VSNPRINTF
018600000000#  endif
018700000000#endif
018800000000#ifndef HAVE_VSNPRINTF
018900000000#  ifdef MSDOS
019000000000     /* vsnprintf may exist on some MS-DOS compilers (DJGPP?),
019100000000        but for now we just assume it doesn't. */
019200000000#    define NO_vsnprintf
019300000000#  endif
019400000000#  ifdef __TURBOC__
019500000000#    define NO_vsnprintf
019600000000#  endif
019700000000#  ifdef WIN32
019800000000     /* In Win32, vsnprintf is available as the "non-ANSI" _vsnprintf. */
019900000000#    if !defined(vsnprintf) && !defined(NO_vsnprintf)
020000000000#      define vsnprintf _vsnprintf
020100000000#    endif
020200000000#  endif
020300000000#  ifdef __SASC
020400000000#    define NO_vsnprintf
020500000000#  endif
020600000000#endif
020700000000#ifdef VMS
020800000000#  define NO_vsnprintf
020900000000#endif
021000000000
021100000000#if defined(pyr)
021200000000#  define NO_MEMCPY
021300000000#endif
021400000000#if defined(SMALL_MEDIUM) && !defined(_MSC_VER) && !defined(__SC__)
021500000000 /* Use our own functions for small and medium model with MSC <= 5.0.
021600000000  * You may have to use the same strategy for Borland C (untested).
021700000000  * The __SC__ check is for Symantec.
021800000000  */
021900000000#  define NO_MEMCPY
022000000000#endif
022100000000#if defined(STDC) && !defined(HAVE_MEMCPY) && !defined(NO_MEMCPY)
022200000000#  define HAVE_MEMCPY
022300000000#endif
022400000000#ifdef HAVE_MEMCPY
022500000000#  ifdef SMALL_MEDIUM /* MSDOS small or medium model */
022600000000#    define zmemcpy _fmemcpy
022700000000#    define zmemcmp _fmemcmp
022800000000#    define zmemzero(dest, len) _fmemset(dest, 0, len)
022900000000#  else
023000000000#    define zmemcpy memcpy
023100000000#    define zmemcmp memcmp
023200000000#    define zmemzero(dest, len) memset(dest, 0, len)
023300000000#  endif
023400000000#else
023500000000   extern void zmemcpy  OF((Bytef* dest, const Bytef* source, uInt len));
023600000000   extern int  zmemcmp  OF((const Bytef* s1, const Bytef* s2, uInt len));
023700000000   extern void zmemzero OF((Bytef* dest, uInt len));
023800000000#endif
023900000000
024000000000/* Diagnostic functions */
024100000000#ifdef DEBUG
024200000000#  include <stdio.h>
024300000000   extern int z_verbose;
024400000000   extern void z_error    OF((char *m));
024500000000#  define Assert(cond,msg) {if(!(cond)) z_error(msg);}
024600000000#  define Trace(x) {if (z_verbose>=0) fprintf x ;}
024700000000#  define Tracev(x) {if (z_verbose>0) fprintf x ;}
024800000000#  define Tracevv(x) {if (z_verbose>1) fprintf x ;}
024900000000#  define Tracec(c,x) {if (z_verbose>0 && (c)) fprintf x ;}
025000000000#  define Tracecv(c,x) {if (z_verbose>1 && (c)) fprintf x ;}
025100000000#else
025200000000#  define Assert(cond,msg)
025300000000#  define Trace(x)
025400000000#  define Tracev(x)
025500000000#  define Tracevv(x)
025600000000#  define Tracec(c,x)
025700000000#  define Tracecv(c,x)
025800000000#endif
025900000000
026000000000
026100000000voidpf zcalloc OF((voidpf opaque, unsigned items, unsigned size));
026200000000void   zcfree  OF((voidpf opaque, voidpf ptr));
026300000000
026400000000#define ZALLOC(strm, items, size) \
026500000000           (*((strm)->zalloc))((strm)->opaque, (items), (size))
026600000000#define ZFREE(strm, addr)  (*((strm)->zfree))((strm)->opaque, (voidpf)(addr))
026700000000#define TRY_FREE(s, p) {if (p) ZFREE(s, p);}
026800000000
026900000000#endif /* ZUTIL_H */
