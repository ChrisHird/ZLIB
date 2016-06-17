000100000000/* zip.h -- IO for compress .zip files using zlib
000200000000   Version 1.01e, February 12th, 2005
000300000000
000400000000   Copyright (C) 1998-2005 Gilles Vollant
000500000000
000600000000   This unzip package allow creates .ZIP file, compatible with PKZip 2.04g
000700000000     WinZip, InfoZip tools and compatible.
000800000000   Multi volume ZipFile (span) are not supported.
000900000000   Encryption compatible with pkzip 2.04g only supported
001000000000   Old compressions used by old PKZip 1.x are not supported
001100000000
001200000000  For uncompress .zip file, look at unzip.h
001300000000
001400000000
001500000000   I WAIT FEEDBACK at mail info@winimage.com
001600000000   Visit also http://www.winimage.com/zLibDll/unzip.html for evolution
001700000000
001800000000   Condition of use and distribution are the same than zlib :
001900000000
002000000000  This software is provided 'as-is', without any express or implied
002100000000  warranty.  In no event will the authors be held liable for any damages
002200000000  arising from the use of this software.
002300000000
002400000000  Permission is granted to anyone to use this software for any purpose,
002500000000  including commercial applications, and to alter it and redistribute it
002600000000  freely, subject to the following restrictions:
002700000000
002800000000  1. The origin of this software must not be misrepresented; you must not
002900000000     claim that you wrote the original software. If you use this software
003000000000     in a product, an acknowledgment in the product documentation would be
003100000000     appreciated but is not required.
003200000000  2. Altered source versions must be plainly marked as such, and must not be
003300000000     misrepresented as being the original software.
003400000000  3. This notice may not be removed or altered from any source distribution.
003500000000
003600000000
003700000000*/
003800000000
003900000000/* for more info about .ZIP format, see
004000000000      http://www.info-zip.org/pub/infozip/doc/appnote-981119-iz.zip
004100000000      http://www.info-zip.org/pub/infozip/doc/
004200000000   PkWare has also a specification at :
004300000000      ftp://ftp.pkware.com/probdesc.zip
004400000000*/
004500000000
004600000000#ifndef _zip_H
004700000000#define _zip_H
004800000000
004900000000#ifdef __cplusplus
005000000000extern "C" {
005100000000#endif
005200000000
005300000000#ifndef _ZLIB_H
005400000000#include "zlib.h"
005500000000#endif
005600000000
005700000000#ifndef _ZLIBIOAPI_H
005800000000#include "ioapi.h"
005900000000#endif
006000000000
006100000000#if defined(STRICTZIP) || defined(STRICTZIPUNZIP)
006200000000/* like the STRICT of WIN32, we define a pointer that cannot be converted
006300000000    from (void*) without cast */
006400000000typedef struct TagzipFile__ { int unused; } zipFile__;
006500000000typedef zipFile__ *zipFile;
006600000000#else
006700000000typedef voidp zipFile;
006800000000#endif
006900000000
007000000000#define ZIP_OK                          (0)
007100000000#define ZIP_EOF                         (0)
007200000000#define ZIP_ERRNO                       (Z_ERRNO)
007300000000#define ZIP_PARAMERROR                  (-102)
007400000000#define ZIP_BADZIPFILE                  (-103)
007500000000#define ZIP_INTERNALERROR               (-104)
007600000000
007700000000#ifndef DEF_MEM_LEVEL
007800000000#  if MAX_MEM_LEVEL >= 8
007900000000#    define DEF_MEM_LEVEL 8
008000000000#  else
008100000000#    define DEF_MEM_LEVEL  MAX_MEM_LEVEL
008200000000#  endif
008300000000#endif
008400000000/* default memLevel */
008500000000
008600000000/* tm_zip contain date/time info */
008700000000typedef struct tm_zip_s
008800000000{
008900000000    uInt tm_sec;            /* seconds after the minute - [0,59] */
009000000000    uInt tm_min;            /* minutes after the hour - [0,59] */
009100000000    uInt tm_hour;           /* hours since midnight - [0,23] */
009200000000    uInt tm_mday;           /* day of the month - [1,31] */
009300000000    uInt tm_mon;            /* months since January - [0,11] */
009400000000    uInt tm_year;           /* years - [1980..2044] */
009500000000} tm_zip;
009600000000
009700000000typedef struct
009800000000{
009900000000    tm_zip      tmz_date;       /* date in understandable format           */
010000000000    uLong       dosDate;       /* if dos_date == 0, tmu_date is used      */
010100000000/*    uLong       flag;        */   /* general purpose bit flag        2 bytes */
010200000000
010300000000    uLong       internal_fa;    /* internal file attributes        2 bytes */
010400000000    uLong       external_fa;    /* external file attributes        4 bytes */
010500000000} zip_fileinfo;
010600000000
010700000000typedef const char* zipcharpc;
010800000000
010900000000
011000000000#define APPEND_STATUS_CREATE        (0)
011100000000#define APPEND_STATUS_CREATEAFTER   (1)
011200000000#define APPEND_STATUS_ADDINZIP      (2)
011300000000
011400000000extern zipFile ZEXPORT zipOpen OF((const char *pathname, int append));
011500000000/*
011600000000  Create a zipfile.
011700000000     pathname contain on Windows XP a filename like "c:\\zlib\\zlib113.zip" or on
011800000000       an Unix computer "zlib/zlib113.zip".
011900000000     if the file pathname exist and append==APPEND_STATUS_CREATEAFTER, the zip
012000000000       will be created at the end of the file.
012100000000         (useful if the file contain a self extractor code)
012200000000     if the file pathname exist and append==APPEND_STATUS_ADDINZIP, we will
012300000000       add files in existing zip (be sure you don't add file that doesn't exist)
012400000000     If the zipfile cannot be opened, the return value is NULL.
012500000000     Else, the return value is a zipFile Handle, usable with other function
012600000000       of this zip package.
012700000000*/
012800000000
012900000000/* Note : there is no delete function into a zipfile.
013000000000   If you want delete file into a zipfile, you must open a zipfile, and create another
013100000000   Of couse, you can use RAW reading and writing to copy the file you did not want delte
013200000000*/
013300000000
013400000000extern zipFile ZEXPORT zipOpen2 OF((const char *pathname,
013500000000                                   int append,
013600000000                                   zipcharpc* globalcomment,
013700000000                                   zlib_filefunc_def* pzlib_filefunc_def));
013800000000
013900000000extern int ZEXPORT zipOpenNewFileInZip OF((zipFile file,
014000000000                       const char* filename,
014100000000                       const zip_fileinfo* zipfi,
014200000000                       const void* extrafield_local,
014300000000                       uInt size_extrafield_local,
014400000000                       const void* extrafield_global,
014500000000                       uInt size_extrafield_global,
014600000000                       const char* comment,
014700000000                       int method,
014800000000                       int level));
014900000000/*
015000000000  Open a file in the ZIP for writing.
015100000000  filename : the filename in zip (if NULL, '-' without quote will be used
015200000000  *zipfi contain supplemental information
015300000000  if extrafield_local!=NULL and size_extrafield_local>0, extrafield_local
015400000000    contains the extrafield data the the local header
015500000000  if extrafield_global!=NULL and size_extrafield_global>0, extrafield_global
015600000000    contains the extrafield data the the local header
015700000000  if comment != NULL, comment contain the comment string
015800000000  method contain the compression method (0 for store, Z_DEFLATED for deflate)
015900000000  level contain the level of compression (can be Z_DEFAULT_COMPRESSION)
016000000000*/
016100000000
016200000000
016300000000extern int ZEXPORT zipOpenNewFileInZip2 OF((zipFile file,
016400000000                                            const char* filename,
016500000000                                            const zip_fileinfo* zipfi,
016600000000                                            const void* extrafield_local,
016700000000                                            uInt size_extrafield_local,
016800000000                                            const void* extrafield_global,
016900000000                                            uInt size_extrafield_global,
017000000000                                            const char* comment,
017100000000                                            int method,
017200000000                                            int level,
017300000000                                            int raw));
017400000000
017500000000/*
017600000000  Same than zipOpenNewFileInZip, except if raw=1, we write raw file
017700000000 */
017800000000
017900000000extern int ZEXPORT zipOpenNewFileInZip3 OF((zipFile file,
018000000000                                            const char* filename,
018100000000                                            const zip_fileinfo* zipfi,
018200000000                                            const void* extrafield_local,
018300000000                                            uInt size_extrafield_local,
018400000000                                            const void* extrafield_global,
018500000000                                            uInt size_extrafield_global,
018600000000                                            const char* comment,
018700000000                                            int method,
018800000000                                            int level,
018900000000                                            int raw,
019000000000                                            int windowBits,
019100000000                                            int memLevel,
019200000000                                            int strategy,
019300000000                                            const char* password,
019400000000                                            uLong crcForCtypting));
019500000000
019600000000/*
019700000000  Same than zipOpenNewFileInZip2, except
019800000000    windowBits,memLevel,,strategy : see parameter strategy in deflateInit2
019900000000    password : crypting password (NULL for no crypting)
020000000000    crcForCtypting : crc of file to compress (needed for crypting)
020100000000 */
020200000000
020300000000
020400000000extern int ZEXPORT zipWriteInFileInZip OF((zipFile file,
020500000000                       const void* buf,
020600000000                       unsigned len));
020700000000/*
020800000000  Write data in the zipfile
020900000000*/
021000000000
021100000000extern int ZEXPORT zipCloseFileInZip OF((zipFile file));
021200000000/*
021300000000  Close the current file in the zipfile
021400000000*/
021500000000
021600000000extern int ZEXPORT zipCloseFileInZipRaw OF((zipFile file,
021700000000                                            uLong uncompressed_size,
021800000000                                            uLong crc32));
021900000000/*
022000000000  Close the current file in the zipfile, for fiel opened with
022100000000    parameter raw=1 in zipOpenNewFileInZip2
022200000000  uncompressed_size and crc32 are value for the uncompressed size
022300000000*/
022400000000
022500000000extern int ZEXPORT zipClose OF((zipFile file,
022600000000                const char* global_comment));
022700000000/*
022800000000  Close the zipfile
022900000000*/
023000000000
023100000000#ifdef __cplusplus
023200000000}
023300000000#endif
023400000000
023500000000#endif /* _zip_H */
