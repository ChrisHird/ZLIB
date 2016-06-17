000100000000/* zconf.h -- configuration of the zlib compression library
000200000000 * Copyright (C) 1995-2005 Jean-loup Gailly.
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/* @(#) $Id$ */
000700000000
000800000000#ifndef ZCONF_H
000900000000#define ZCONF_H
001000000000
001100000000/*
001200000000 * If you *really* need a unique prefix for all types and library functions,
001300000000 * compile with -DZ_PREFIX. The "standard" zlib should be compiled without it.
001400000000 */
001500000000#ifdef Z_PREFIX
001600000000#  define deflateInit_          z_deflateInit_
001700000000#  define deflate               z_deflate
001800000000#  define deflateEnd            z_deflateEnd
001900000000#  define inflateInit_          z_inflateInit_
002000000000#  define inflate               z_inflate
002100000000#  define inflateEnd            z_inflateEnd
002200000000#  define deflateInit2_         z_deflateInit2_
002300000000#  define deflateSetDictionary  z_deflateSetDictionary
002400000000#  define deflateCopy           z_deflateCopy
002500000000#  define deflateReset          z_deflateReset
002600000000#  define deflateParams         z_deflateParams
002700000000#  define deflateBound          z_deflateBound
002800000000#  define deflatePrime          z_deflatePrime
002900000000#  define inflateInit2_         z_inflateInit2_
003000000000#  define inflateSetDictionary  z_inflateSetDictionary
003100000000#  define inflateSync           z_inflateSync
003200000000#  define inflateSyncPoint      z_inflateSyncPoint
003300000000#  define inflateCopy           z_inflateCopy
003400000000#  define inflateReset          z_inflateReset
003500000000#  define inflateBack           z_inflateBack
003600000000#  define inflateBackEnd        z_inflateBackEnd
003700000000#  define compress              z_compress
003800000000#  define compress2             z_compress2
003900000000#  define compressBound         z_compressBound
004000000000#  define uncompress            z_uncompress
004100000000#  define adler32               z_adler32
004200000000#  define crc32                 z_crc32
004300000000#  define get_crc_table         z_get_crc_table
004400000000#  define zError                z_zError
004500000000
004600000000#  define alloc_func            z_alloc_func
004700000000#  define free_func             z_free_func
004800000000#  define in_func               z_in_func
004900000000#  define out_func              z_out_func
005000000000#  define Byte                  z_Byte
005100000000#  define uInt                  z_uInt
005200000000#  define uLong                 z_uLong
005300000000#  define Bytef                 z_Bytef
005400000000#  define charf                 z_charf
005500000000#  define intf                  z_intf
005600000000#  define uIntf                 z_uIntf
005700000000#  define uLongf                z_uLongf
005800000000#  define voidpf                z_voidpf
005900000000#  define voidp                 z_voidp
006000000000#endif
006100000000
006200000000#if defined(__MSDOS__) && !defined(MSDOS)
006300000000#  define MSDOS
006400000000#endif
006500000000#if (defined(OS_2) || defined(__OS2__)) && !defined(OS2)
006600000000#  define OS2
006700000000#endif
006800000000#if defined(_WINDOWS) && !defined(WINDOWS)
006900000000#  define WINDOWS
007000000000#endif
007100000000#if defined(_WIN32) || defined(_WIN32_WCE) || defined(__WIN32__)
007200000000#  ifndef WIN32
007300000000#    define WIN32
007400000000#  endif
007500000000#endif
007600000000#if (defined(MSDOS) || defined(OS2) || defined(WINDOWS)) && !defined(WIN32)
007700000000#  if !defined(__GNUC__) && !defined(__FLAT__) && !defined(__386__)
007800000000#    ifndef SYS16BIT
007900000000#      define SYS16BIT
008000000000#    endif
008100000000#  endif
008200000000#endif
008300000000
008400000000/*
008500000000 * Compile with -DMAXSEG_64K if the alloc function cannot allocate more
008600000000 * than 64k bytes at a time (needed on systems with 16-bit int).
008700000000 */
008800000000#ifdef SYS16BIT
008900000000#  define MAXSEG_64K
009000000000#endif
009100000000#ifdef MSDOS
009200000000#  define UNALIGNED_OK
009300000000#endif
009400000000
009500000000#ifdef __STDC_VERSION__
009600000000#  ifndef STDC
009700000000#    define STDC
009800000000#  endif
009900000000#  if __STDC_VERSION__ >= 199901L
010000000000#    ifndef STDC99
010100000000#      define STDC99
010200000000#    endif
010300000000#  endif
010400000000#endif
010500000000#if !defined(STDC) && (defined(__STDC__) || defined(__cplusplus))
010600000000#  define STDC
010700000000#endif
010800000000#if !defined(STDC) && (defined(__GNUC__) || defined(__BORLANDC__))
010900000000#  define STDC
011000000000#endif
011100000000#if !defined(STDC) && (defined(MSDOS) || defined(WINDOWS) || defined(WIN32))
011200000000#  define STDC
011300000000#endif
011400000000#if !defined(STDC) && (defined(OS2) || defined(__HOS_AIX__))
011500000000#  define STDC
011600000000#endif
011700000000
011800000000#if defined(__OS400__) && !defined(STDC)    /* iSeries (formerly AS/400). */
011900000000#  define STDC
012000000000#endif
012100000000
012200000000#ifndef STDC
012300000000#  ifndef const /* cannot use !defined(STDC) && !defined(const) on Mac */
012400000000#    define const       /* note: need a more gentle solution here */
012500000000#  endif
012600000000#endif
012700000000
012800000000/* Some Mac compilers merge all .h files incorrectly: */
012900000000#if defined(__MWERKS__)||defined(applec)||defined(THINK_C)||defined(__SC__)
013000000000#  define NO_DUMMY_DECL
013100000000#endif
013200000000
013300000000/* Maximum value for memLevel in deflateInit2 */
013400000000#ifndef MAX_MEM_LEVEL
013500000000#  ifdef MAXSEG_64K
013600000000#    define MAX_MEM_LEVEL 8
013700000000#  else
013800000000#    define MAX_MEM_LEVEL 9
013900000000#  endif
014000000000#endif
014100000000
014200000000/* Maximum value for windowBits in deflateInit2 and inflateInit2.
014300000000 * WARNING: reducing MAX_WBITS makes minigzip unable to extract .gz files
014400000000 * created by gzip. (Files created by minigzip can still be extracted by
014500000000 * gzip.)
014600000000 */
014700000000#ifndef MAX_WBITS
014800000000#  define MAX_WBITS   15 /* 32K LZ77 window */
014900000000#endif
015000000000
015100000000/* The memory requirements for deflate are (in bytes):
015200000000            (1 << (windowBits+2)) +  (1 << (memLevel+9))
015300000000 that is: 128K for windowBits=15  +  128K for memLevel = 8  (default values)
015400000000 plus a few kilobytes for small objects. For example, if you want to reduce
015500000000 the default memory requirements from 256K to 128K, compile with
015600000000     make CFLAGS="-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7"
015700000000 Of course this will generally degrade compression (there's no free lunch).
015800000000
015900000000   The memory requirements for inflate are (in bytes) 1 << windowBits
016000000000 that is, 32K for windowBits=15 (default value) plus a few kilobytes
016100000000 for small objects.
016200000000*/
016300000000
016400000000                        /* Type declarations */
016500000000
016600000000#ifndef OF /* function prototypes */
016700000000#  ifdef STDC
016800000000#    define OF(args)  args
016900000000#  else
017000000000#    define OF(args)  ()
017100000000#  endif
017200000000#endif
017300000000
017400000000/* The following definitions for FAR are needed only for MSDOS mixed
017500000000 * model programming (small or medium model with some far allocations).
017600000000 * This was tested only with MSC; for other MSDOS compilers you may have
017700000000 * to define NO_MEMCPY in zutil.h.  If you don't need the mixed model,
017800000000 * just define FAR to be empty.
017900000000 */
018000000000#ifdef SYS16BIT
018100000000#  if defined(M_I86SM) || defined(M_I86MM)
018200000000     /* MSC small or medium model */
018300000000#    define SMALL_MEDIUM
018400000000#    ifdef _MSC_VER
018500000000#      define FAR _far
018600000000#    else
018700000000#      define FAR far
018800000000#    endif
018900000000#  endif
019000000000#  if (defined(__SMALL__) || defined(__MEDIUM__))
019100000000     /* Turbo C small or medium model */
019200000000#    define SMALL_MEDIUM
019300000000#    ifdef __BORLANDC__
019400000000#      define FAR _far
019500000000#    else
019600000000#      define FAR far
019700000000#    endif
019800000000#  endif
019900000000#endif
020000000000
020100000000#if defined(WINDOWS) || defined(WIN32)
020200000000   /* If building or using zlib as a DLL, define ZLIB_DLL.
020300000000    * This is not mandatory, but it offers a little performance increase.
020400000000    */
020500000000#  ifdef ZLIB_DLL
020600000000#    if defined(WIN32) && (!defined(__BORLANDC__) || (__BORLANDC__ >= 0x500))
020700000000#      ifdef ZLIB_INTERNAL
020800000000#        define ZEXTERN extern __declspec(dllexport)
020900000000#      else
021000000000#        define ZEXTERN extern __declspec(dllimport)
021100000000#      endif
021200000000#    endif
021300000000#  endif  /* ZLIB_DLL */
021400000000   /* If building or using zlib with the WINAPI/WINAPIV calling convention,
021500000000    * define ZLIB_WINAPI.
021600000000    * Caution: the standard ZLIB1.DLL is NOT compiled using ZLIB_WINAPI.
021700000000    */
021800000000#  ifdef ZLIB_WINAPI
021900000000#    ifdef FAR
022000000000#      undef FAR
022100000000#    endif
022200000000#    include <windows.h>
022300000000     /* No need for _export, use ZLIB.DEF instead. */
022400000000     /* For complete Windows compatibility, use WINAPI, not __stdcall. */
022500000000#    define ZEXPORT WINAPI
022600000000#    ifdef WIN32
022700000000#      define ZEXPORTVA WINAPIV
022800000000#    else
022900000000#      define ZEXPORTVA FAR CDECL
023000000000#    endif
023100000000#  endif
023200000000#endif
023300000000
023400000000#if defined (__BEOS__)
023500000000#  ifdef ZLIB_DLL
023600000000#    ifdef ZLIB_INTERNAL
023700000000#      define ZEXPORT   __declspec(dllexport)
023800000000#      define ZEXPORTVA __declspec(dllexport)
023900000000#    else
024000000000#      define ZEXPORT   __declspec(dllimport)
024100000000#      define ZEXPORTVA __declspec(dllimport)
024200000000#    endif
024300000000#  endif
024400000000#endif
024500000000
024600000000#ifndef ZEXTERN
024700000000#  define ZEXTERN extern
024800000000#endif
024900000000#ifndef ZEXPORT
025000000000#  define ZEXPORT
025100000000#endif
025200000000#ifndef ZEXPORTVA
025300000000#  define ZEXPORTVA
025400000000#endif
025500000000
025600000000#ifndef FAR
025700000000#  define FAR
025800000000#endif
025900000000
026000000000#if !defined(__MACTYPES__)
026100000000typedef unsigned char  Byte;  /* 8 bits */
026200000000#endif
026300000000typedef unsigned int   uInt;  /* 16 bits or more */
026400000000typedef unsigned long  uLong; /* 32 bits or more */
026500000000
026600000000#ifdef SMALL_MEDIUM
026700000000   /* Borland C/C++ and some old MSC versions ignore FAR inside typedef */
026800000000#  define Bytef Byte FAR
026900000000#else
027000000000   typedef Byte  FAR Bytef;
027100000000#endif
027200000000typedef char  FAR charf;
027300000000typedef int   FAR intf;
027400000000typedef uInt  FAR uIntf;
027500000000typedef uLong FAR uLongf;
027600000000
027700000000#ifdef STDC
027800000000   typedef void const *voidpc;
027900000000   typedef void FAR   *voidpf;
028000000000   typedef void       *voidp;
028100000000#else
028200000000   typedef Byte const *voidpc;
028300000000   typedef Byte FAR   *voidpf;
028400000000   typedef Byte       *voidp;
028500000000#endif
028600000000
028700000000#if 0           /* HAVE_UNISTD_H -- this line is updated by ./configure */
028800000000#  include <sys/types.h> /* for off_t */
028900000000#  include <unistd.h>    /* for SEEK_* and off_t */
029000000000#  ifdef VMS
029100000000#    include <unixio.h>   /* for off_t */
029200000000#  endif
029300000000#  define z_off_t off_t
029400000000#endif
029500000000#ifndef SEEK_SET
029600000000#  define SEEK_SET        0       /* Seek from beginning of file.  */
029700000000#  define SEEK_CUR        1       /* Seek from current position.  */
029800000000#  define SEEK_END        2       /* Set file pointer to EOF plus "offset" */
029900000000#endif
030000000000#ifndef z_off_t
030100000000#  define z_off_t long
030200000000#endif
030300000000
030400000000#if defined(__OS400__)
030500000000#  define NO_vsnprintf
030600000000#endif
030700000000
030800000000#if defined(__MVS__)
030900000000#  define NO_vsnprintf
031000000000#  ifdef FAR
031100000000#    undef FAR
031200000000#  endif
031300000000#endif
031400000000
031500000000/* MVS linker does not support external names larger than 8 bytes */
031600000000#if defined(__MVS__)
031700000000#   pragma map(deflateInit_,"DEIN")
031800000000#   pragma map(deflateInit2_,"DEIN2")
031900000000#   pragma map(deflateEnd,"DEEND")
032000000000#   pragma map(deflateBound,"DEBND")
032100000000#   pragma map(inflateInit_,"ININ")
032200000000#   pragma map(inflateInit2_,"ININ2")
032300000000#   pragma map(inflateEnd,"INEND")
032400000000#   pragma map(inflateSync,"INSY")
032500000000#   pragma map(inflateSetDictionary,"INSEDI")
032600000000#   pragma map(compressBound,"CMBND")
032700000000#   pragma map(inflate_table,"INTABL")
032800000000#   pragma map(inflate_fast,"INFA")
032900000000#   pragma map(inflate_copyright,"INCOPY")
033000000000#endif
033100000000
033200000000#endif /* ZCONF_H */
