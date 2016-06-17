000100000000/* unzip.h -- IO for uncompress .zip files using zlib
000200000000   Version 1.01e, February 12th, 2005
000300000000
000400000000   Copyright (C) 1998-2005 Gilles Vollant
000500000000
000600000000   This unzip package allow extract file from .ZIP file, compatible with PKZip 2.04g
000700000000     WinZip, InfoZip tools and compatible.
000800000000
000900000000   Multi volume ZipFile (span) are not supported.
001000000000   Encryption compatible with pkzip 2.04g only supported
001100000000   Old compressions used by old PKZip 1.x are not supported
001200000000
001300000000
001400000000   I WAIT FEEDBACK at mail info@winimage.com
001500000000   Visit also http://www.winimage.com/zLibDll/unzip.htm for evolution
001600000000
001700000000   Condition of use and distribution are the same than zlib :
001800000000
001900000000  This software is provided 'as-is', without any express or implied
002000000000  warranty.  In no event will the authors be held liable for any damages
002100000000  arising from the use of this software.
002200000000
002300000000  Permission is granted to anyone to use this software for any purpose,
002400000000  including commercial applications, and to alter it and redistribute it
002500000000  freely, subject to the following restrictions:
002600000000
002700000000  1. The origin of this software must not be misrepresented; you must not
002800000000     claim that you wrote the original software. If you use this software
002900000000     in a product, an acknowledgment in the product documentation would be
003000000000     appreciated but is not required.
003100000000  2. Altered source versions must be plainly marked as such, and must not be
003200000000     misrepresented as being the original software.
003300000000  3. This notice may not be removed or altered from any source distribution.
003400000000
003500000000
003600000000*/
003700000000
003800000000/* for more info about .ZIP format, see
003900000000      http://www.info-zip.org/pub/infozip/doc/appnote-981119-iz.zip
004000000000      http://www.info-zip.org/pub/infozip/doc/
004100000000   PkWare has also a specification at :
004200000000      ftp://ftp.pkware.com/probdesc.zip
004300000000*/
004400000000
004500000000#ifndef _unz_H
004600000000#define _unz_H
004700000000
004800000000#ifdef __cplusplus
004900000000extern "C" {
005000000000#endif
005100000000
005200000000#ifndef _ZLIB_H
005300000000#include "zlib.h"
005400000000#endif
005500000000
005600000000#ifndef _ZLIBIOAPI_H
005700000000#include "ioapi.h"
005800000000#endif
005900000000
006000000000#if defined(STRICTUNZIP) || defined(STRICTZIPUNZIP)
006100000000/* like the STRICT of WIN32, we define a pointer that cannot be converted
006200000000    from (void*) without cast */
006300000000typedef struct TagunzFile__ { int unused; } unzFile__;
006400000000typedef unzFile__ *unzFile;
006500000000#else
006600000000typedef voidp unzFile;
006700000000#endif
006800000000
006900000000
007000000000#define UNZ_OK                          (0)
007100000000#define UNZ_END_OF_LIST_OF_FILE         (-100)
007200000000#define UNZ_ERRNO                       (Z_ERRNO)
007300000000#define UNZ_EOF                         (0)
007400000000#define UNZ_PARAMERROR                  (-102)
007500000000#define UNZ_BADZIPFILE                  (-103)
007600000000#define UNZ_INTERNALERROR               (-104)
007700000000#define UNZ_CRCERROR                    (-105)
007800000000
007900000000/* tm_unz contain date/time info */
008000000000typedef struct tm_unz_s
008100000000{
008200000000    uInt tm_sec;            /* seconds after the minute - [0,59] */
008300000000    uInt tm_min;            /* minutes after the hour - [0,59] */
008400000000    uInt tm_hour;           /* hours since midnight - [0,23] */
008500000000    uInt tm_mday;           /* day of the month - [1,31] */
008600000000    uInt tm_mon;            /* months since January - [0,11] */
008700000000    uInt tm_year;           /* years - [1980..2044] */
008800000000} tm_unz;
008900000000
009000000000/* unz_global_info structure contain global data about the ZIPfile
009100000000   These data comes from the end of central dir */
009200000000typedef struct unz_global_info_s
009300000000{
009400000000    uLong number_entry;         /* total number of entries in
009500000000                       the central dir on this disk */
009600000000    uLong size_comment;         /* size of the global comment of the zipfile */
009700000000} unz_global_info;
009800000000
009900000000
010000000000/* unz_file_info contain information about a file in the zipfile */
010100000000typedef struct unz_file_info_s
010200000000{
010300000000    uLong version;              /* version made by                 2 bytes */
010400000000    uLong version_needed;       /* version needed to extract       2 bytes */
010500000000    uLong flag;                 /* general purpose bit flag        2 bytes */
010600000000    uLong compression_method;   /* compression method              2 bytes */
010700000000    uLong dosDate;              /* last mod file date in Dos fmt   4 bytes */
010800000000    uLong crc;                  /* crc-32                          4 bytes */
010900000000    uLong compressed_size;      /* compressed size                 4 bytes */
011000000000    uLong uncompressed_size;    /* uncompressed size               4 bytes */
011100000000    uLong size_filename;        /* filename length                 2 bytes */
011200000000    uLong size_file_extra;      /* extra field length              2 bytes */
011300000000    uLong size_file_comment;    /* file comment length             2 bytes */
011400000000
011500000000    uLong disk_num_start;       /* disk number start               2 bytes */
011600000000    uLong internal_fa;          /* internal file attributes        2 bytes */
011700000000    uLong external_fa;          /* external file attributes        4 bytes */
011800000000
011900000000    tm_unz tmu_date;
012000000000} unz_file_info;
012100000000
012200000000extern int ZEXPORT unzStringFileNameCompare OF ((const char* fileName1,
012300000000                                                 const char* fileName2,
012400000000                                                 int iCaseSensitivity));
012500000000/*
012600000000   Compare two filename (fileName1,fileName2).
012700000000   If iCaseSenisivity = 1, comparision is case sensitivity (like strcmp)
012800000000   If iCaseSenisivity = 2, comparision is not case sensitivity (like strcmpi
012900000000                                or strcasecmp)
013000000000   If iCaseSenisivity = 0, case sensitivity is defaut of your operating system
013100000000    (like 1 on Unix, 2 on Windows)
013200000000*/
013300000000
013400000000
013500000000extern unzFile ZEXPORT unzOpen OF((const char *path));
013600000000/*
013700000000  Open a Zip file. path contain the full pathname (by example,
013800000000     on a Windows XP computer "c:\\zlib\\zlib113.zip" or on an Unix computer
013900000000     "zlib/zlib113.zip".
014000000000     If the zipfile cannot be opened (file don't exist or in not valid), the
014100000000       return value is NULL.
014200000000     Else, the return value is a unzFile Handle, usable with other function
014300000000       of this unzip package.
014400000000*/
014500000000
014600000000extern unzFile ZEXPORT unzOpen2 OF((const char *path,
014700000000                                    zlib_filefunc_def* pzlib_filefunc_def));
014800000000/*
014900000000   Open a Zip file, like unzOpen, but provide a set of file low level API
015000000000      for read/write the zip file (see ioapi.h)
015100000000*/
015200000000
015300000000extern int ZEXPORT unzClose OF((unzFile file));
015400000000/*
015500000000  Close a ZipFile opened with unzipOpen.
015600000000  If there is files inside the .Zip opened with unzOpenCurrentFile (see later),
015700000000    these files MUST be closed with unzipCloseCurrentFile before call unzipClose.
015800000000  return UNZ_OK if there is no problem. */
015900000000
016000000000extern int ZEXPORT unzGetGlobalInfo OF((unzFile file,
016100000000                                        unz_global_info *pglobal_info));
016200000000/*
016300000000  Write info about the ZipFile in the *pglobal_info structure.
016400000000  No preparation of the structure is needed
016500000000  return UNZ_OK if there is no problem. */
016600000000
016700000000
016800000000extern int ZEXPORT unzGetGlobalComment OF((unzFile file,
016900000000                                           char *szComment,
017000000000                                           uLong uSizeBuf));
017100000000/*
017200000000  Get the global comment string of the ZipFile, in the szComment buffer.
017300000000  uSizeBuf is the size of the szComment buffer.
017400000000  return the number of byte copied or an error code <0
017500000000*/
017600000000
017700000000
017800000000/***************************************************************************/
017900000000/* Unzip package allow you browse the directory of the zipfile */
018000000000
018100000000extern int ZEXPORT unzGoToFirstFile OF((unzFile file));
018200000000/*
018300000000  Set the current file of the zipfile to the first file.
018400000000  return UNZ_OK if there is no problem
018500000000*/
018600000000
018700000000extern int ZEXPORT unzGoToNextFile OF((unzFile file));
018800000000/*
018900000000  Set the current file of the zipfile to the next file.
019000000000  return UNZ_OK if there is no problem
019100000000  return UNZ_END_OF_LIST_OF_FILE if the actual file was the latest.
019200000000*/
019300000000
019400000000extern int ZEXPORT unzLocateFile OF((unzFile file,
019500000000                     const char *szFileName,
019600000000                     int iCaseSensitivity));
019700000000/*
019800000000  Try locate the file szFileName in the zipfile.
019900000000  For the iCaseSensitivity signification, see unzStringFileNameCompare
020000000000
020100000000  return value :
020200000000  UNZ_OK if the file is found. It becomes the current file.
020300000000  UNZ_END_OF_LIST_OF_FILE if the file is not found
020400000000*/
020500000000
020600000000
020700000000/* ****************************************** */
020800000000/* Ryan supplied functions */
020900000000/* unz_file_info contain information about a file in the zipfile */
021000000000typedef struct unz_file_pos_s
021100000000{
021200000000    uLong pos_in_zip_directory;   /* offset in zip file directory */
021300000000    uLong num_of_file;            /* # of file */
021400000000} unz_file_pos;
021500000000
021600000000extern int ZEXPORT unzGetFilePos(
021700000000    unzFile file,
021800000000    unz_file_pos* file_pos);
021900000000
022000000000extern int ZEXPORT unzGoToFilePos(
022100000000    unzFile file,
022200000000    unz_file_pos* file_pos);
022300000000
022400000000/* ****************************************** */
022500000000
022600000000extern int ZEXPORT unzGetCurrentFileInfo OF((unzFile file,
022700000000                         unz_file_info *pfile_info,
022800000000                         char *szFileName,
022900000000                         uLong fileNameBufferSize,
023000000000                         void *extraField,
023100000000                         uLong extraFieldBufferSize,
023200000000                         char *szComment,
023300000000                         uLong commentBufferSize));
023400000000/*
023500000000  Get Info about the current file
023600000000  if pfile_info!=NULL, the *pfile_info structure will contain somes info about
023700000000        the current file
023800000000  if szFileName!=NULL, the filemane string will be copied in szFileName
023900000000            (fileNameBufferSize is the size of the buffer)
024000000000  if extraField!=NULL, the extra field information will be copied in extraField
024100000000            (extraFieldBufferSize is the size of the buffer).
024200000000            This is the Central-header version of the extra field
024300000000  if szComment!=NULL, the comment string of the file will be copied in szComment
024400000000            (commentBufferSize is the size of the buffer)
024500000000*/
024600000000
024700000000/***************************************************************************/
024800000000/* for reading the content of the current zipfile, you can open it, read data
024900000000   from it, and close it (you can close it before reading all the file)
025000000000   */
025100000000
025200000000extern int ZEXPORT unzOpenCurrentFile OF((unzFile file));
025300000000/*
025400000000  Open for reading data the current file in the zipfile.
025500000000  If there is no error, the return value is UNZ_OK.
025600000000*/
025700000000
025800000000extern int ZEXPORT unzOpenCurrentFilePassword OF((unzFile file,
025900000000                                                  const char* password));
026000000000/*
026100000000  Open for reading data the current file in the zipfile.
026200000000  password is a crypting password
026300000000  If there is no error, the return value is UNZ_OK.
026400000000*/
026500000000
026600000000extern int ZEXPORT unzOpenCurrentFile2 OF((unzFile file,
026700000000                                           int* method,
026800000000                                           int* level,
026900000000                                           int raw));
027000000000/*
027100000000  Same than unzOpenCurrentFile, but open for read raw the file (not uncompress)
027200000000    if raw==1
027300000000  *method will receive method of compression, *level will receive level of
027400000000     compression
027500000000  note : you can set level parameter as NULL (if you did not want known level,
027600000000         but you CANNOT set method parameter as NULL
027700000000*/
027800000000
027900000000extern int ZEXPORT unzOpenCurrentFile3 OF((unzFile file,
028000000000                                           int* method,
028100000000                                           int* level,
028200000000                                           int raw,
028300000000                                           const char* password));
028400000000/*
028500000000  Same than unzOpenCurrentFile, but open for read raw the file (not uncompress)
028600000000    if raw==1
028700000000  *method will receive method of compression, *level will receive level of
028800000000     compression
028900000000  note : you can set level parameter as NULL (if you did not want known level,
029000000000         but you CANNOT set method parameter as NULL
029100000000*/
029200000000
029300000000
029400000000extern int ZEXPORT unzCloseCurrentFile OF((unzFile file));
029500000000/*
029600000000  Close the file in zip opened with unzOpenCurrentFile
029700000000  Return UNZ_CRCERROR if all the file was read but the CRC is not good
029800000000*/
029900000000
030000000000extern int ZEXPORT unzReadCurrentFile OF((unzFile file,
030100000000                      voidp buf,
030200000000                      unsigned len));
030300000000/*
030400000000  Read bytes from the current file (opened by unzOpenCurrentFile)
030500000000  buf contain buffer where data must be copied
030600000000  len the size of buf.
030700000000
030800000000  return the number of byte copied if somes bytes are copied
030900000000  return 0 if the end of file was reached
031000000000  return <0 with error code if there is an error
031100000000    (UNZ_ERRNO for IO error, or zLib error for uncompress error)
031200000000*/
031300000000
031400000000extern z_off_t ZEXPORT unztell OF((unzFile file));
031500000000/*
031600000000  Give the current position in uncompressed data
031700000000*/
031800000000
031900000000extern int ZEXPORT unzeof OF((unzFile file));
032000000000/*
032100000000  return 1 if the end of file was reached, 0 elsewhere
032200000000*/
032300000000
032400000000extern int ZEXPORT unzGetLocalExtrafield OF((unzFile file,
032500000000                                             voidp buf,
032600000000                                             unsigned len));
032700000000/*
032800000000  Read extra field from the current file (opened by unzOpenCurrentFile)
032900000000  This is the local-header version of the extra field (sometimes, there is
033000000000    more info in the local-header version than in the central-header)
033100000000
033200000000  if buf==NULL, it return the size of the local extra field
033300000000
033400000000  if buf!=NULL, len is the size of the buffer, the extra header is copied in
033500000000    buf.
033600000000  the return value is the number of bytes copied in buf, or (if <0)
033700000000    the error code
033800000000*/
033900000000
034000000000/***************************************************************************/
034100000000
034200000000/* Get the current file offset */
034300000000extern uLong ZEXPORT unzGetOffset (unzFile file);
034400000000
034500000000/* Set the current file offset */
034600000000extern int ZEXPORT unzSetOffset (unzFile file, uLong pos);
034700000000
034800000000
034900000000
035000000000#ifdef __cplusplus
035100000000}
035200000000#endif
035300000000
035400000000#endif /* _unz_H */
