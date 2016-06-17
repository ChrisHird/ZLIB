000100000000/* unzip.c -- IO for uncompress .zip files using zlib
000200000000   Version 1.01e, February 12th, 2005
000300000000
000400000000   Copyright (C) 1998-2005 Gilles Vollant
000500000000
000600000000   Read unzip.h for more info
000700000000*/
000800000000
000900000000/* Decryption code comes from crypt.c by Info-ZIP but has been greatly reduced in terms of
001000000000compatibility with older software. The following is from the original crypt.c. Code
001100000000woven in by Terry Thorsen 1/2003.
001200000000*/
001300000000/*
001400000000  Copyright (c) 1990-2000 Info-ZIP.  All rights reserved.
001500000000
001600000000  See the accompanying file LICENSE, version 2000-Apr-09 or later
001700000000  (the contents of which are also included in zip.h) for terms of use.
001800000000  If, for some reason, all these files are missing, the Info-ZIP license
001900000000  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
002000000000*/
002100000000/*
002200000000  crypt.c (full version) by Info-ZIP.      Last revised:  [see crypt.h]
002300000000
002400000000  The encryption/decryption parts of this source code (as opposed to the
002500000000  non-echoing password parts) were originally written in Europe.  The
002600000000  whole source package can be freely distributed, including from the USA.
002700000000  (Prior to January 2000, re-export from the US was a violation of US law.)
002800000000 */
002900000000
003000000000/*
003100000000  This encryption code is a direct transcription of the algorithm from
003200000000  Roger Schlafly, described by Phil Katz in the file appnote.txt.  This
003300000000  file (appnote.txt) is distributed with the PKZIP program (even in the
003400000000  version without encryption capabilities).
003500000000 */
003600000000
003700000000
003800000000#include <stdio.h>
003900000000#include <stdlib.h>
004000000000#include <string.h>
004100000000#include "zlib.h"
004200000000#include "unzip.h"
004300000000
004400000000#ifdef STDC
004500000000#  include <stddef.h>
004600000000#  include <string.h>
004700000000#  include <stdlib.h>
004800000000#endif
004900000000#ifdef NO_ERRNO_H
005000000000    extern int errno;
005100000000#else
005200000000#   include <errno.h>
005300000000#endif
005400000000
005500000000
005600000000#ifndef local
005700000000#  define local static
005800000000#endif
005900000000/* compile with -Dlocal if your debugger can't find static symbols */
006000000000
006100000000
006200000000#ifndef CASESENSITIVITYDEFAULT_NO
006300000000#  if !defined(unix) && !defined(CASESENSITIVITYDEFAULT_YES)
006400000000#    define CASESENSITIVITYDEFAULT_NO
006500000000#  endif
006600000000#endif
006700000000
006800000000
006900000000#ifndef UNZ_BUFSIZE
007000000000#define UNZ_BUFSIZE (16384)
007100000000#endif
007200000000
007300000000#ifndef UNZ_MAXFILENAMEINZIP
007400000000#define UNZ_MAXFILENAMEINZIP (256)
007500000000#endif
007600000000
007700000000#ifndef ALLOC
007800000000# define ALLOC(size) (malloc(size))
007900000000#endif
008000000000#ifndef TRYFREE
008100000000# define TRYFREE(p) {if (p) free(p);}
008200000000#endif
008300000000
008400000000#define SIZECENTRALDIRITEM (0x2e)
008500000000#define SIZEZIPLOCALHEADER (0x1e)
008600000000
008700000000
008800000000
008900000000
009000000000const char unz_copyright[] =
009100000000   " unzip 1.01 Copyright 1998-2004 Gilles Vollant - http://www.winimage.com/zLibDll";
009200000000
009300000000/* unz_file_info_interntal contain internal info about a file in zipfile*/
009400000000typedef struct unz_file_info_internal_s
009500000000{
009600000000    uLong offset_curfile;/* relative offset of local header 4 bytes */
009700000000} unz_file_info_internal;
009800000000
009900000000
010000000000/* file_in_zip_read_info_s contain internal information about a file in zipfile,
010100000000    when reading and decompress it */
010200000000typedef struct
010300000000{
010400000000    char  *read_buffer;         /* internal buffer for compressed data */
010500000000    z_stream stream;            /* zLib stream structure for inflate */
010600000000
010700000000    uLong pos_in_zipfile;       /* position in byte on the zipfile, for fseek*/
010800000000    uLong stream_initialised;   /* flag set if stream structure is initialised*/
010900000000
011000000000    uLong offset_local_extrafield;/* offset of the local extra field */
011100000000    uInt  size_local_extrafield;/* size of the local extra field */
011200000000    uLong pos_local_extrafield;   /* position in the local extra field in read*/
011300000000
011400000000    uLong crc32;                /* crc32 of all data uncompressed */
011500000000    uLong crc32_wait;           /* crc32 we must obtain after decompress all */
011600000000    uLong rest_read_compressed; /* number of byte to be decompressed */
011700000000    uLong rest_read_uncompressed;/*number of byte to be obtained after decomp*/
011800000000    zlib_filefunc_def z_filefunc;
011900000000    voidpf filestream;        /* io structore of the zipfile */
012000000000    uLong compression_method;   /* compression method (0==store) */
012100000000    uLong byte_before_the_zipfile;/* byte before the zipfile, (>0 for sfx)*/
012200000000    int   raw;
012300000000} file_in_zip_read_info_s;
012400000000
012500000000
012600000000/* unz_s contain internal information about the zipfile
012700000000*/
012800000000typedef struct
012900000000{
013000000000    zlib_filefunc_def z_filefunc;
013100000000    voidpf filestream;        /* io structore of the zipfile */
013200000000    unz_global_info gi;       /* public global information */
013300000000    uLong byte_before_the_zipfile;/* byte before the zipfile, (>0 for sfx)*/
013400000000    uLong num_file;             /* number of the current file in the zipfile*/
013500000000    uLong pos_in_central_dir;   /* pos of the current file in the central dir*/
013600000000    uLong current_file_ok;      /* flag about the usability of the current file*/
013700000000    uLong central_pos;          /* position of the beginning of the central dir*/
013800000000
013900000000    uLong size_central_dir;     /* size of the central directory  */
014000000000    uLong offset_central_dir;   /* offset of start of central directory with
014100000000                                   respect to the starting disk number */
014200000000
014300000000    unz_file_info cur_file_info; /* public info about the current file in zip*/
014400000000    unz_file_info_internal cur_file_info_internal; /* private info about it*/
014500000000    file_in_zip_read_info_s* pfile_in_zip_read; /* structure about the current
014600000000                                        file if we are decompressing it */
014700000000    int encrypted;
014800000000#    ifndef NOUNCRYPT
014900000000    unsigned long keys[3];     /* keys defining the pseudo-random sequence */
015000000000    const unsigned long* pcrc_32_tab;
015100000000#    endif
015200000000} unz_s;
015300000000
015400000000
015500000000#ifndef NOUNCRYPT
015600000000#include "crypt.h"
015700000000#endif
015800000000
015900000000/* ===========================================================================
016000000000     Read a byte from a gz_stream; update next_in and avail_in. Return EOF
016100000000   for end of file.
016200000000   IN assertion: the stream s has been sucessfully opened for reading.
016300000000*/
016400000000
016500000000
016600000000local int unzlocal_getByte OF((
016700000000    const zlib_filefunc_def* pzlib_filefunc_def,
016800000000    voidpf filestream,
016900000000    int *pi));
017000000000
017100000000local int unzlocal_getByte(pzlib_filefunc_def,filestream,pi)
017200000000    const zlib_filefunc_def* pzlib_filefunc_def;
017300000000    voidpf filestream;
017400000000    int *pi;
017500000000{
017600000000    unsigned char c;
017700000000    int err = (int)ZREAD(*pzlib_filefunc_def,filestream,&c,1);
017800000000    if (err==1)
017900000000    {
018000000000        *pi = (int)c;
018100000000        return UNZ_OK;
018200000000    }
018300000000    else
018400000000    {
018500000000        if (ZERROR(*pzlib_filefunc_def,filestream))
018600000000            return UNZ_ERRNO;
018700000000        else
018800000000            return UNZ_EOF;
018900000000    }
019000000000}
019100000000
019200000000
019300000000/* ===========================================================================
019400000000   Reads a long in LSB order from the given gz_stream. Sets
019500000000*/
019600000000local int unzlocal_getShort OF((
019700000000    const zlib_filefunc_def* pzlib_filefunc_def,
019800000000    voidpf filestream,
019900000000    uLong *pX));
020000000000
020100000000local int unzlocal_getShort (pzlib_filefunc_def,filestream,pX)
020200000000    const zlib_filefunc_def* pzlib_filefunc_def;
020300000000    voidpf filestream;
020400000000    uLong *pX;
020500000000{
020600000000    uLong x ;
020700000000    int i;
020800000000    int err;
020900000000
021000000000    err = unzlocal_getByte(pzlib_filefunc_def,filestream,&i);
021100000000    x = (uLong)i;
021200000000
021300000000    if (err==UNZ_OK)
021400000000        err = unzlocal_getByte(pzlib_filefunc_def,filestream,&i);
021500000000    x += ((uLong)i)<<8;
021600000000
021700000000    if (err==UNZ_OK)
021800000000        *pX = x;
021900000000    else
022000000000        *pX = 0;
022100000000    return err;
022200000000}
022300000000
022400000000local int unzlocal_getLong OF((
022500000000    const zlib_filefunc_def* pzlib_filefunc_def,
022600000000    voidpf filestream,
022700000000    uLong *pX));
022800000000
022900000000local int unzlocal_getLong (pzlib_filefunc_def,filestream,pX)
023000000000    const zlib_filefunc_def* pzlib_filefunc_def;
023100000000    voidpf filestream;
023200000000    uLong *pX;
023300000000{
023400000000    uLong x ;
023500000000    int i;
023600000000    int err;
023700000000
023800000000    err = unzlocal_getByte(pzlib_filefunc_def,filestream,&i);
023900000000    x = (uLong)i;
024000000000
024100000000    if (err==UNZ_OK)
024200000000        err = unzlocal_getByte(pzlib_filefunc_def,filestream,&i);
024300000000    x += ((uLong)i)<<8;
024400000000
024500000000    if (err==UNZ_OK)
024600000000        err = unzlocal_getByte(pzlib_filefunc_def,filestream,&i);
024700000000    x += ((uLong)i)<<16;
024800000000
024900000000    if (err==UNZ_OK)
025000000000        err = unzlocal_getByte(pzlib_filefunc_def,filestream,&i);
025100000000    x += ((uLong)i)<<24;
025200000000
025300000000    if (err==UNZ_OK)
025400000000        *pX = x;
025500000000    else
025600000000        *pX = 0;
025700000000    return err;
025800000000}
025900000000
026000000000
026100000000/* My own strcmpi / strcasecmp */
026200000000local int strcmpcasenosensitive_internal (fileName1,fileName2)
026300000000    const char* fileName1;
026400000000    const char* fileName2;
026500000000{
026600000000    for (;;)
026700000000    {
026800000000        char c1=*(fileName1++);
026900000000        char c2=*(fileName2++);
027000060219#ifdef AS400
027100060219#pragma convert(850)
027200060219#endif
027300000000        if ((c1>='a') && (c1<='z'))
027400000000            c1 -= 0x20;
027500000000        if ((c2>='a') && (c2<='z'))
027600000000            c2 -= 0x20;
027700060219#ifdef AS400
027800060219#pragma convert(0)
027900060219#endif
028000000000        if (c1=='\0')
028100000000            return ((c2=='\0') ? 0 : -1);
028200000000        if (c2=='\0')
028300000000            return 1;
028400000000        if (c1<c2)
028500000000            return -1;
028600000000        if (c1>c2)
028700000000            return 1;
028800000000    }
028900000000}
029000000000
029100000000
029200000000#ifdef  CASESENSITIVITYDEFAULT_NO
029300000000#define CASESENSITIVITYDEFAULTVALUE 2
029400000000#else
029500000000#define CASESENSITIVITYDEFAULTVALUE 1
029600000000#endif
029700000000
029800000000#ifndef STRCMPCASENOSENTIVEFUNCTION
029900000000#define STRCMPCASENOSENTIVEFUNCTION strcmpcasenosensitive_internal
030000000000#endif
030100000000
030200000000/*
030300000000   Compare two filename (fileName1,fileName2).
030400000000   If iCaseSenisivity = 1, comparision is case sensitivity (like strcmp)
030500000000   If iCaseSenisivity = 2, comparision is not case sensitivity (like strcmpi
030600000000                                                                or strcasecmp)
030700000000   If iCaseSenisivity = 0, case sensitivity is defaut of your operating system
030800000000        (like 1 on Unix, 2 on Windows)
030900000000
031000000000*/
031100000000extern int ZEXPORT unzStringFileNameCompare (fileName1,fileName2,iCaseSensitivity)
031200000000    const char* fileName1;
031300000000    const char* fileName2;
031400000000    int iCaseSensitivity;
031500000000{
031600000000    if (iCaseSensitivity==0)
031700000000        iCaseSensitivity=CASESENSITIVITYDEFAULTVALUE;
031800000000
031900000000    if (iCaseSensitivity==1)
032000000000        return strcmp(fileName1,fileName2);
032100000000
032200000000    return STRCMPCASENOSENTIVEFUNCTION(fileName1,fileName2);
032300000000}
032400000000
032500000000#ifndef BUFREADCOMMENT
032600000000#define BUFREADCOMMENT (0x400)
032700000000#endif
032800000000
032900000000/*
033000000000  Locate the Central directory of a zipfile (at the end, just before
033100000000    the global comment)
033200000000*/
033300000000local uLong unzlocal_SearchCentralDir OF((
033400000000    const zlib_filefunc_def* pzlib_filefunc_def,
033500000000    voidpf filestream));
033600000000
033700000000local uLong unzlocal_SearchCentralDir(pzlib_filefunc_def,filestream)
033800000000    const zlib_filefunc_def* pzlib_filefunc_def;
033900000000    voidpf filestream;
034000000000{
034100000000    unsigned char* buf;
034200000000    uLong uSizeFile;
034300000000    uLong uBackRead;
034400000000    uLong uMaxBack=0xffff; /* maximum size of global comment */
034500000000    uLong uPosFound=0;
034600000000
034700000000    if (ZSEEK(*pzlib_filefunc_def,filestream,0,ZLIB_FILEFUNC_SEEK_END) != 0)
034800000000        return 0;
034900000000
035000000000
035100000000    uSizeFile = ZTELL(*pzlib_filefunc_def,filestream);
035200000000
035300000000    if (uMaxBack>uSizeFile)
035400000000        uMaxBack = uSizeFile;
035500000000
035600000000    buf = (unsigned char*)ALLOC(BUFREADCOMMENT+4);
035700000000    if (buf==NULL)
035800000000        return 0;
035900000000
036000000000    uBackRead = 4;
036100000000    while (uBackRead<uMaxBack)
036200000000    {
036300000000        uLong uReadSize,uReadPos ;
036400000000        int i;
036500000000        if (uBackRead+BUFREADCOMMENT>uMaxBack)
036600000000            uBackRead = uMaxBack;
036700000000        else
036800000000            uBackRead+=BUFREADCOMMENT;
036900000000        uReadPos = uSizeFile-uBackRead ;
037000000000
037100000000        uReadSize = ((BUFREADCOMMENT+4) < (uSizeFile-uReadPos)) ?
037200000000                     (BUFREADCOMMENT+4) : (uSizeFile-uReadPos);
037300000000        if (ZSEEK(*pzlib_filefunc_def,filestream,uReadPos,ZLIB_FILEFUNC_SEEK_SET)!=0)
037400000000            break;
037500000000
037600000000        if (ZREAD(*pzlib_filefunc_def,filestream,buf,uReadSize)!=uReadSize)
037700000000            break;
037800000000
037900000000        for (i=(int)uReadSize-3; (i--)>0;)
038000000000            if (((*(buf+i))==0x50) && ((*(buf+i+1))==0x4b) &&
038100000000                ((*(buf+i+2))==0x05) && ((*(buf+i+3))==0x06))
038200000000            {
038300000000                uPosFound = uReadPos+i;
038400000000                break;
038500000000            }
038600000000
038700000000        if (uPosFound!=0)
038800000000            break;
038900000000    }
039000000000    TRYFREE(buf);
039100000000    return uPosFound;
039200000000}
039300000000
039400000000/*
039500000000  Open a Zip file. path contain the full pathname (by example,
039600000000     on a Windows NT computer "c:\\test\\zlib114.zip" or on an Unix computer
039700000000     "zlib/zlib114.zip".
039800000000     If the zipfile cannot be opened (file doesn't exist or in not valid), the
039900000000       return value is NULL.
040000000000     Else, the return value is a unzFile Handle, usable with other function
040100000000       of this unzip package.
040200000000*/
040300000000extern unzFile ZEXPORT unzOpen2 (path, pzlib_filefunc_def)
040400000000    const char *path;
040500000000    zlib_filefunc_def* pzlib_filefunc_def;
040600000000{
040700000000    unz_s us;
040800000000    unz_s *s;
040900000000    uLong central_pos,uL;
041000000000
041100000000    uLong number_disk;          /* number of the current dist, used for
041200000000                                   spaning ZIP, unsupported, always 0*/
041300000000    uLong number_disk_with_CD;  /* number the the disk with central dir, used
041400000000                                   for spaning ZIP, unsupported, always 0*/
041500000000    uLong number_entry_CD;      /* total number of entries in
041600000000                                   the central dir
041700000000                                   (same than number_entry on nospan) */
041800000000
041900000000    int err=UNZ_OK;
042000000000
042100000000    if (unz_copyright[0]!=' ')
042200000000        return NULL;
042300000000
042400000000    if (pzlib_filefunc_def==NULL)
042500000000        fill_fopen_filefunc(&us.z_filefunc);
042600000000    else
042700000000        us.z_filefunc = *pzlib_filefunc_def;
042800000000
042900000000    us.filestream= (*(us.z_filefunc.zopen_file))(us.z_filefunc.opaque,
043000000000                                                 path,
043100000000                                                 ZLIB_FILEFUNC_MODE_READ |
043200000000                                                 ZLIB_FILEFUNC_MODE_EXISTING);
043300000000    if (us.filestream==NULL)
043400000000        return NULL;
043500000000
043600000000    central_pos = unzlocal_SearchCentralDir(&us.z_filefunc,us.filestream);
043700000000    if (central_pos==0)
043800000000        err=UNZ_ERRNO;
043900000000
044000000000    if (ZSEEK(us.z_filefunc, us.filestream,
044100000000                                      central_pos,ZLIB_FILEFUNC_SEEK_SET)!=0)
044200000000        err=UNZ_ERRNO;
044300000000
044400000000    /* the signature, already checked */
044500000000    if (unzlocal_getLong(&us.z_filefunc, us.filestream,&uL)!=UNZ_OK)
044600000000        err=UNZ_ERRNO;
044700000000
044800000000    /* number of this disk */
044900000000    if (unzlocal_getShort(&us.z_filefunc, us.filestream,&number_disk)!=UNZ_OK)
045000000000        err=UNZ_ERRNO;
045100000000
045200000000    /* number of the disk with the start of the central directory */
045300000000    if (unzlocal_getShort(&us.z_filefunc, us.filestream,&number_disk_with_CD)!=UNZ_OK)
045400000000        err=UNZ_ERRNO;
045500000000
045600000000    /* total number of entries in the central dir on this disk */
045700000000    if (unzlocal_getShort(&us.z_filefunc, us.filestream,&us.gi.number_entry)!=UNZ_OK)
045800000000        err=UNZ_ERRNO;
045900000000
046000000000    /* total number of entries in the central dir */
046100000000    if (unzlocal_getShort(&us.z_filefunc, us.filestream,&number_entry_CD)!=UNZ_OK)
046200000000        err=UNZ_ERRNO;
046300000000
046400000000    if ((number_entry_CD!=us.gi.number_entry) ||
046500000000        (number_disk_with_CD!=0) ||
046600000000        (number_disk!=0))
046700000000        err=UNZ_BADZIPFILE;
046800000000
046900000000    /* size of the central directory */
047000000000    if (unzlocal_getLong(&us.z_filefunc, us.filestream,&us.size_central_dir)!=UNZ_OK)
047100000000        err=UNZ_ERRNO;
047200000000
047300000000    /* offset of start of central directory with respect to the
047400000000          starting disk number */
047500000000    if (unzlocal_getLong(&us.z_filefunc, us.filestream,&us.offset_central_dir)!=UNZ_OK)
047600000000        err=UNZ_ERRNO;
047700000000
047800000000    /* zipfile comment length */
047900000000    if (unzlocal_getShort(&us.z_filefunc, us.filestream,&us.gi.size_comment)!=UNZ_OK)
048000000000        err=UNZ_ERRNO;
048100000000
048200000000    if ((central_pos<us.offset_central_dir+us.size_central_dir) &&
048300000000        (err==UNZ_OK))
048400000000        err=UNZ_BADZIPFILE;
048500000000
048600000000    if (err!=UNZ_OK)
048700000000    {
048800000000        ZCLOSE(us.z_filefunc, us.filestream);
048900000000        return NULL;
049000000000    }
049100000000
049200000000    us.byte_before_the_zipfile = central_pos -
049300000000                            (us.offset_central_dir+us.size_central_dir);
049400000000    us.central_pos = central_pos;
049500000000    us.pfile_in_zip_read = NULL;
049600000000    us.encrypted = 0;
049700000000
049800000000
049900000000    s=(unz_s*)ALLOC(sizeof(unz_s));
050000000000    *s=us;
050100000000    unzGoToFirstFile((unzFile)s);
050200000000    return (unzFile)s;
050300000000}
050400000000
050500000000
050600000000extern unzFile ZEXPORT unzOpen (path)
050700000000    const char *path;
050800000000{
050900000000    return unzOpen2(path, NULL);
051000000000}
051100000000
051200000000/*
051300000000  Close a ZipFile opened with unzipOpen.
051400000000  If there is files inside the .Zip opened with unzipOpenCurrentFile (see later),
051500000000    these files MUST be closed with unzipCloseCurrentFile before call unzipClose.
051600000000  return UNZ_OK if there is no problem. */
051700000000extern int ZEXPORT unzClose (file)
051800000000    unzFile file;
051900000000{
052000000000    unz_s* s;
052100000000    if (file==NULL)
052200000000        return UNZ_PARAMERROR;
052300000000    s=(unz_s*)file;
052400000000
052500000000    if (s->pfile_in_zip_read!=NULL)
052600000000        unzCloseCurrentFile(file);
052700000000
052800000000    ZCLOSE(s->z_filefunc, s->filestream);
052900000000    TRYFREE(s);
053000000000    return UNZ_OK;
053100000000}
053200000000
053300000000
053400000000/*
053500000000  Write info about the ZipFile in the *pglobal_info structure.
053600000000  No preparation of the structure is needed
053700000000  return UNZ_OK if there is no problem. */
053800000000extern int ZEXPORT unzGetGlobalInfo (file,pglobal_info)
053900000000    unzFile file;
054000000000    unz_global_info *pglobal_info;
054100000000{
054200000000    unz_s* s;
054300000000    if (file==NULL)
054400000000        return UNZ_PARAMERROR;
054500000000    s=(unz_s*)file;
054600000000    *pglobal_info=s->gi;
054700000000    return UNZ_OK;
054800000000}
054900000000
055000000000
055100000000/*
055200000000   Translate date/time from Dos format to tm_unz (readable more easilty)
055300000000*/
055400000000local void unzlocal_DosDateToTmuDate (ulDosDate, ptm)
055500000000    uLong ulDosDate;
055600000000    tm_unz* ptm;
055700000000{
055800000000    uLong uDate;
055900000000    uDate = (uLong)(ulDosDate>>16);
056000000000    ptm->tm_mday = (uInt)(uDate&0x1f) ;
056100000000    ptm->tm_mon =  (uInt)((((uDate)&0x1E0)/0x20)-1) ;
056200000000    ptm->tm_year = (uInt)(((uDate&0x0FE00)/0x0200)+1980) ;
056300000000
056400000000    ptm->tm_hour = (uInt) ((ulDosDate &0xF800)/0x800);
056500000000    ptm->tm_min =  (uInt) ((ulDosDate&0x7E0)/0x20) ;
056600000000    ptm->tm_sec =  (uInt) (2*(ulDosDate&0x1f)) ;
056700000000}
056800000000
056900000000/*
057000000000  Get Info about the current file in the zipfile, with internal only info
057100000000*/
057200000000local int unzlocal_GetCurrentFileInfoInternal OF((unzFile file,
057300000000                                                  unz_file_info *pfile_info,
057400000000                                                  unz_file_info_internal
057500000000                                                  *pfile_info_internal,
057600000000                                                  char *szFileName,
057700000000                                                  uLong fileNameBufferSize,
057800000000                                                  void *extraField,
057900000000                                                  uLong extraFieldBufferSize,
058000000000                                                  char *szComment,
058100000000                                                  uLong commentBufferSize));
058200000000
058300000000local int unzlocal_GetCurrentFileInfoInternal (file,
058400000000                                              pfile_info,
058500000000                                              pfile_info_internal,
058600000000                                              szFileName, fileNameBufferSize,
058700000000                                              extraField, extraFieldBufferSize,
058800000000                                              szComment,  commentBufferSize)
058900000000    unzFile file;
059000000000    unz_file_info *pfile_info;
059100000000    unz_file_info_internal *pfile_info_internal;
059200000000    char *szFileName;
059300000000    uLong fileNameBufferSize;
059400000000    void *extraField;
059500000000    uLong extraFieldBufferSize;
059600000000    char *szComment;
059700000000    uLong commentBufferSize;
059800000000{
059900000000    unz_s* s;
060000000000    unz_file_info file_info;
060100000000    unz_file_info_internal file_info_internal;
060200000000    int err=UNZ_OK;
060300000000    uLong uMagic;
060400000000    long lSeek=0;
060500000000
060600000000    if (file==NULL)
060700000000        return UNZ_PARAMERROR;
060800000000    s=(unz_s*)file;
060900000000    if (ZSEEK(s->z_filefunc, s->filestream,
061000000000              s->pos_in_central_dir+s->byte_before_the_zipfile,
061100000000              ZLIB_FILEFUNC_SEEK_SET)!=0)
061200000000        err=UNZ_ERRNO;
061300000000
061400000000
061500000000    /* we check the magic */
061600000000    if (err==UNZ_OK)
061700000000        if (unzlocal_getLong(&s->z_filefunc, s->filestream,&uMagic) != UNZ_OK)
061800000000            err=UNZ_ERRNO;
061900000000        else if (uMagic!=0x02014b50)
062000000000            err=UNZ_BADZIPFILE;
062100000000
062200000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&file_info.version) != UNZ_OK)
062300000000        err=UNZ_ERRNO;
062400000000
062500000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&file_info.version_needed) != UNZ_OK)
062600000000        err=UNZ_ERRNO;
062700000000
062800000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&file_info.flag) != UNZ_OK)
062900000000        err=UNZ_ERRNO;
063000000000
063100000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&file_info.compression_method) != UNZ_OK)
063200000000        err=UNZ_ERRNO;
063300000000
063400000000    if (unzlocal_getLong(&s->z_filefunc, s->filestream,&file_info.dosDate) != UNZ_OK)
063500000000        err=UNZ_ERRNO;
063600000000
063700000000    unzlocal_DosDateToTmuDate(file_info.dosDate,&file_info.tmu_date);
063800000000
063900000000    if (unzlocal_getLong(&s->z_filefunc, s->filestream,&file_info.crc) != UNZ_OK)
064000000000        err=UNZ_ERRNO;
064100000000
064200000000    if (unzlocal_getLong(&s->z_filefunc, s->filestream,&file_info.compressed_size) != UNZ_OK)
064300000000        err=UNZ_ERRNO;
064400000000
064500000000    if (unzlocal_getLong(&s->z_filefunc, s->filestream,&file_info.uncompressed_size) != UNZ_OK)
064600000000        err=UNZ_ERRNO;
064700000000
064800000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&file_info.size_filename) != UNZ_OK)
064900000000        err=UNZ_ERRNO;
065000000000
065100000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&file_info.size_file_extra) != UNZ_OK)
065200000000        err=UNZ_ERRNO;
065300000000
065400000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&file_info.size_file_comment) != UNZ_OK)
065500000000        err=UNZ_ERRNO;
065600000000
065700000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&file_info.disk_num_start) != UNZ_OK)
065800000000        err=UNZ_ERRNO;
065900000000
066000000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&file_info.internal_fa) != UNZ_OK)
066100000000        err=UNZ_ERRNO;
066200000000
066300000000    if (unzlocal_getLong(&s->z_filefunc, s->filestream,&file_info.external_fa) != UNZ_OK)
066400000000        err=UNZ_ERRNO;
066500000000
066600000000    if (unzlocal_getLong(&s->z_filefunc, s->filestream,&file_info_internal.offset_curfile) != UNZ_OK)
066700000000        err=UNZ_ERRNO;
066800000000
066900000000    lSeek+=file_info.size_filename;
067000000000    if ((err==UNZ_OK) && (szFileName!=NULL))
067100000000    {
067200000000        uLong uSizeRead ;
067300000000        if (file_info.size_filename<fileNameBufferSize)
067400000000        {
067500000000            *(szFileName+file_info.size_filename)='\0';
067600000000            uSizeRead = file_info.size_filename;
067700000000        }
067800000000        else
067900000000            uSizeRead = fileNameBufferSize;
068000000000
068100000000        if ((file_info.size_filename>0) && (fileNameBufferSize>0))
068200000000            if (ZREAD(s->z_filefunc, s->filestream,szFileName,uSizeRead)!=uSizeRead)
068300000000                err=UNZ_ERRNO;
068400000000        lSeek -= uSizeRead;
068500000000    }
068600000000
068700000000
068800000000    if ((err==UNZ_OK) && (extraField!=NULL))
068900000000    {
069000000000        uLong uSizeRead ;
069100000000        if (file_info.size_file_extra<extraFieldBufferSize)
069200000000            uSizeRead = file_info.size_file_extra;
069300000000        else
069400000000            uSizeRead = extraFieldBufferSize;
069500000000
069600000000        if (lSeek!=0)
069700000000            if (ZSEEK(s->z_filefunc, s->filestream,lSeek,ZLIB_FILEFUNC_SEEK_CUR)==0)
069800000000                lSeek=0;
069900000000            else
070000000000                err=UNZ_ERRNO;
070100000000        if ((file_info.size_file_extra>0) && (extraFieldBufferSize>0))
070200000000            if (ZREAD(s->z_filefunc, s->filestream,extraField,uSizeRead)!=uSizeRead)
070300000000                err=UNZ_ERRNO;
070400000000        lSeek += file_info.size_file_extra - uSizeRead;
070500000000    }
070600000000    else
070700000000        lSeek+=file_info.size_file_extra;
070800000000
070900000000
071000000000    if ((err==UNZ_OK) && (szComment!=NULL))
071100000000    {
071200000000        uLong uSizeRead ;
071300000000        if (file_info.size_file_comment<commentBufferSize)
071400000000        {
071500000000            *(szComment+file_info.size_file_comment)='\0';
071600000000            uSizeRead = file_info.size_file_comment;
071700000000        }
071800000000        else
071900000000            uSizeRead = commentBufferSize;
072000000000
072100000000        if (lSeek!=0)
072200000000            if (ZSEEK(s->z_filefunc, s->filestream,lSeek,ZLIB_FILEFUNC_SEEK_CUR)==0)
072300000000                lSeek=0;
072400000000            else
072500000000                err=UNZ_ERRNO;
072600000000        if ((file_info.size_file_comment>0) && (commentBufferSize>0))
072700000000            if (ZREAD(s->z_filefunc, s->filestream,szComment,uSizeRead)!=uSizeRead)
072800000000                err=UNZ_ERRNO;
072900000000        lSeek+=file_info.size_file_comment - uSizeRead;
073000000000    }
073100000000    else
073200000000        lSeek+=file_info.size_file_comment;
073300000000
073400000000    if ((err==UNZ_OK) && (pfile_info!=NULL))
073500000000        *pfile_info=file_info;
073600000000
073700000000    if ((err==UNZ_OK) && (pfile_info_internal!=NULL))
073800000000        *pfile_info_internal=file_info_internal;
073900000000
074000000000    return err;
074100000000}
074200000000
074300000000
074400000000
074500000000/*
074600000000  Write info about the ZipFile in the *pglobal_info structure.
074700000000  No preparation of the structure is needed
074800000000  return UNZ_OK if there is no problem.
074900000000*/
075000000000extern int ZEXPORT unzGetCurrentFileInfo (file,
075100000000                                          pfile_info,
075200000000                                          szFileName, fileNameBufferSize,
075300000000                                          extraField, extraFieldBufferSize,
075400000000                                          szComment,  commentBufferSize)
075500000000    unzFile file;
075600000000    unz_file_info *pfile_info;
075700000000    char *szFileName;
075800000000    uLong fileNameBufferSize;
075900000000    void *extraField;
076000000000    uLong extraFieldBufferSize;
076100000000    char *szComment;
076200000000    uLong commentBufferSize;
076300000000{
076400000000    return unzlocal_GetCurrentFileInfoInternal(file,pfile_info,NULL,
076500000000                                                szFileName,fileNameBufferSize,
076600000000                                                extraField,extraFieldBufferSize,
076700000000                                                szComment,commentBufferSize);
076800000000}
076900000000
077000000000/*
077100000000  Set the current file of the zipfile to the first file.
077200000000  return UNZ_OK if there is no problem
077300000000*/
077400000000extern int ZEXPORT unzGoToFirstFile (file)
077500000000    unzFile file;
077600000000{
077700000000    int err=UNZ_OK;
077800000000    unz_s* s;
077900000000    if (file==NULL)
078000000000        return UNZ_PARAMERROR;
078100000000    s=(unz_s*)file;
078200000000    s->pos_in_central_dir=s->offset_central_dir;
078300000000    s->num_file=0;
078400000000    err=unzlocal_GetCurrentFileInfoInternal(file,&s->cur_file_info,
078500000000                                             &s->cur_file_info_internal,
078600000000                                             NULL,0,NULL,0,NULL,0);
078700000000    s->current_file_ok = (err == UNZ_OK);
078800000000    return err;
078900000000}
079000000000
079100000000/*
079200000000  Set the current file of the zipfile to the next file.
079300000000  return UNZ_OK if there is no problem
079400000000  return UNZ_END_OF_LIST_OF_FILE if the actual file was the latest.
079500000000*/
079600000000extern int ZEXPORT unzGoToNextFile (file)
079700000000    unzFile file;
079800000000{
079900000000    unz_s* s;
080000000000    int err;
080100000000
080200000000    if (file==NULL)
080300000000        return UNZ_PARAMERROR;
080400000000    s=(unz_s*)file;
080500000000    if (!s->current_file_ok)
080600000000        return UNZ_END_OF_LIST_OF_FILE;
080700000000    if (s->gi.number_entry != 0xffff)    /* 2^16 files overflow hack */
080800000000      if (s->num_file+1==s->gi.number_entry)
080900000000        return UNZ_END_OF_LIST_OF_FILE;
081000000000
081100000000    s->pos_in_central_dir += SIZECENTRALDIRITEM + s->cur_file_info.size_filename +
081200000000            s->cur_file_info.size_file_extra + s->cur_file_info.size_file_comment ;
081300000000    s->num_file++;
081400000000    err = unzlocal_GetCurrentFileInfoInternal(file,&s->cur_file_info,
081500000000                                               &s->cur_file_info_internal,
081600000000                                               NULL,0,NULL,0,NULL,0);
081700000000    s->current_file_ok = (err == UNZ_OK);
081800000000    return err;
081900000000}
082000000000
082100000000
082200000000/*
082300000000  Try locate the file szFileName in the zipfile.
082400000000  For the iCaseSensitivity signification, see unzipStringFileNameCompare
082500000000
082600000000  return value :
082700000000  UNZ_OK if the file is found. It becomes the current file.
082800000000  UNZ_END_OF_LIST_OF_FILE if the file is not found
082900000000*/
083000000000extern int ZEXPORT unzLocateFile (file, szFileName, iCaseSensitivity)
083100000000    unzFile file;
083200000000    const char *szFileName;
083300000000    int iCaseSensitivity;
083400000000{
083500000000    unz_s* s;
083600000000    int err;
083700000000
083800000000    /* We remember the 'current' position in the file so that we can jump
083900000000     * back there if we fail.
084000000000     */
084100000000    unz_file_info cur_file_infoSaved;
084200000000    unz_file_info_internal cur_file_info_internalSaved;
084300000000    uLong num_fileSaved;
084400000000    uLong pos_in_central_dirSaved;
084500000000
084600000000
084700000000    if (file==NULL)
084800000000        return UNZ_PARAMERROR;
084900000000
085000000000    if (strlen(szFileName)>=UNZ_MAXFILENAMEINZIP)
085100000000        return UNZ_PARAMERROR;
085200000000
085300000000    s=(unz_s*)file;
085400000000    if (!s->current_file_ok)
085500000000        return UNZ_END_OF_LIST_OF_FILE;
085600000000
085700000000    /* Save the current state */
085800000000    num_fileSaved = s->num_file;
085900000000    pos_in_central_dirSaved = s->pos_in_central_dir;
086000000000    cur_file_infoSaved = s->cur_file_info;
086100000000    cur_file_info_internalSaved = s->cur_file_info_internal;
086200000000
086300000000    err = unzGoToFirstFile(file);
086400000000
086500000000    while (err == UNZ_OK)
086600000000    {
086700000000        char szCurrentFileName[UNZ_MAXFILENAMEINZIP+1];
086800000000        err = unzGetCurrentFileInfo(file,NULL,
086900000000                                    szCurrentFileName,sizeof(szCurrentFileName)-1,
087000000000                                    NULL,0,NULL,0);
087100000000        if (err == UNZ_OK)
087200000000        {
087300000000            if (unzStringFileNameCompare(szCurrentFileName,
087400000000                                            szFileName,iCaseSensitivity)==0)
087500000000                return UNZ_OK;
087600000000            err = unzGoToNextFile(file);
087700000000        }
087800000000    }
087900000000
088000000000    /* We failed, so restore the state of the 'current file' to where we
088100000000     * were.
088200000000     */
088300000000    s->num_file = num_fileSaved ;
088400000000    s->pos_in_central_dir = pos_in_central_dirSaved ;
088500000000    s->cur_file_info = cur_file_infoSaved;
088600000000    s->cur_file_info_internal = cur_file_info_internalSaved;
088700000000    return err;
088800000000}
088900000000
089000000000
089100000000/*
089200000000///////////////////////////////////////////
089300000000// Contributed by Ryan Haksi (mailto://cryogen@infoserve.net)
089400000000// I need random access
089500000000//
089600000000// Further optimization could be realized by adding an ability
089700000000// to cache the directory in memory. The goal being a single
089800000000// comprehensive file read to put the file I need in a memory.
089900000000*/
090000000000
090100000000/*
090200000000typedef struct unz_file_pos_s
090300000000{
090400000000    uLong pos_in_zip_directory;   // offset in file
090500000000    uLong num_of_file;            // # of file
090600000000} unz_file_pos;
090700000000*/
090800000000
090900000000extern int ZEXPORT unzGetFilePos(file, file_pos)
091000000000    unzFile file;
091100000000    unz_file_pos* file_pos;
091200000000{
091300000000    unz_s* s;
091400000000
091500000000    if (file==NULL || file_pos==NULL)
091600000000        return UNZ_PARAMERROR;
091700000000    s=(unz_s*)file;
091800000000    if (!s->current_file_ok)
091900000000        return UNZ_END_OF_LIST_OF_FILE;
092000000000
092100000000    file_pos->pos_in_zip_directory  = s->pos_in_central_dir;
092200000000    file_pos->num_of_file           = s->num_file;
092300000000
092400000000    return UNZ_OK;
092500000000}
092600000000
092700000000extern int ZEXPORT unzGoToFilePos(file, file_pos)
092800000000    unzFile file;
092900000000    unz_file_pos* file_pos;
093000000000{
093100000000    unz_s* s;
093200000000    int err;
093300000000
093400000000    if (file==NULL || file_pos==NULL)
093500000000        return UNZ_PARAMERROR;
093600000000    s=(unz_s*)file;
093700000000
093800000000    /* jump to the right spot */
093900000000    s->pos_in_central_dir = file_pos->pos_in_zip_directory;
094000000000    s->num_file           = file_pos->num_of_file;
094100000000
094200000000    /* set the current file */
094300000000    err = unzlocal_GetCurrentFileInfoInternal(file,&s->cur_file_info,
094400000000                                               &s->cur_file_info_internal,
094500000000                                               NULL,0,NULL,0,NULL,0);
094600000000    /* return results */
094700000000    s->current_file_ok = (err == UNZ_OK);
094800000000    return err;
094900000000}
095000000000
095100000000/*
095200000000// Unzip Helper Functions - should be here?
095300000000///////////////////////////////////////////
095400000000*/
095500000000
095600000000/*
095700000000  Read the local header of the current zipfile
095800000000  Check the coherency of the local header and info in the end of central
095900000000        directory about this file
096000000000  store in *piSizeVar the size of extra info in local header
096100000000        (filename and size of extra field data)
096200000000*/
096300000000local int unzlocal_CheckCurrentFileCoherencyHeader (s,piSizeVar,
096400000000                                                    poffset_local_extrafield,
096500000000                                                    psize_local_extrafield)
096600000000    unz_s* s;
096700000000    uInt* piSizeVar;
096800000000    uLong *poffset_local_extrafield;
096900000000    uInt  *psize_local_extrafield;
097000000000{
097100000000    uLong uMagic,uData,uFlags;
097200000000    uLong size_filename;
097300000000    uLong size_extra_field;
097400000000    int err=UNZ_OK;
097500000000
097600000000    *piSizeVar = 0;
097700000000    *poffset_local_extrafield = 0;
097800000000    *psize_local_extrafield = 0;
097900000000
098000000000    if (ZSEEK(s->z_filefunc, s->filestream,s->cur_file_info_internal.offset_curfile +
098100000000                                s->byte_before_the_zipfile,ZLIB_FILEFUNC_SEEK_SET)!=0)
098200000000        return UNZ_ERRNO;
098300000000
098400000000
098500000000    if (err==UNZ_OK)
098600000000        if (unzlocal_getLong(&s->z_filefunc, s->filestream,&uMagic) != UNZ_OK)
098700000000            err=UNZ_ERRNO;
098800000000        else if (uMagic!=0x04034b50)
098900000000            err=UNZ_BADZIPFILE;
099000000000
099100000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&uData) != UNZ_OK)
099200000000        err=UNZ_ERRNO;
099300000000/*
099400000000    else if ((err==UNZ_OK) && (uData!=s->cur_file_info.wVersion))
099500000000        err=UNZ_BADZIPFILE;
099600000000*/
099700000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&uFlags) != UNZ_OK)
099800000000        err=UNZ_ERRNO;
099900000000
100000000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&uData) != UNZ_OK)
100100000000        err=UNZ_ERRNO;
100200000000    else if ((err==UNZ_OK) && (uData!=s->cur_file_info.compression_method))
100300000000        err=UNZ_BADZIPFILE;
100400000000
100500000000    if ((err==UNZ_OK) && (s->cur_file_info.compression_method!=0) &&
100600000000                         (s->cur_file_info.compression_method!=Z_DEFLATED))
100700000000        err=UNZ_BADZIPFILE;
100800000000
100900000000    if (unzlocal_getLong(&s->z_filefunc, s->filestream,&uData) != UNZ_OK) /* date/time */
101000000000        err=UNZ_ERRNO;
101100000000
101200000000    if (unzlocal_getLong(&s->z_filefunc, s->filestream,&uData) != UNZ_OK) /* crc */
101300000000        err=UNZ_ERRNO;
101400000000    else if ((err==UNZ_OK) && (uData!=s->cur_file_info.crc) &&
101500000000                              ((uFlags & 8)==0))
101600000000        err=UNZ_BADZIPFILE;
101700000000
101800000000    if (unzlocal_getLong(&s->z_filefunc, s->filestream,&uData) != UNZ_OK) /* size compr */
101900000000        err=UNZ_ERRNO;
102000000000    else if ((err==UNZ_OK) && (uData!=s->cur_file_info.compressed_size) &&
102100000000                              ((uFlags & 8)==0))
102200000000        err=UNZ_BADZIPFILE;
102300000000
102400000000    if (unzlocal_getLong(&s->z_filefunc, s->filestream,&uData) != UNZ_OK) /* size uncompr */
102500000000        err=UNZ_ERRNO;
102600000000    else if ((err==UNZ_OK) && (uData!=s->cur_file_info.uncompressed_size) &&
102700000000                              ((uFlags & 8)==0))
102800000000        err=UNZ_BADZIPFILE;
102900000000
103000000000
103100000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&size_filename) != UNZ_OK)
103200000000        err=UNZ_ERRNO;
103300000000    else if ((err==UNZ_OK) && (size_filename!=s->cur_file_info.size_filename))
103400000000        err=UNZ_BADZIPFILE;
103500000000
103600000000    *piSizeVar += (uInt)size_filename;
103700000000
103800000000    if (unzlocal_getShort(&s->z_filefunc, s->filestream,&size_extra_field) != UNZ_OK)
103900000000        err=UNZ_ERRNO;
104000000000    *poffset_local_extrafield= s->cur_file_info_internal.offset_curfile +
104100000000                                    SIZEZIPLOCALHEADER + size_filename;
104200000000    *psize_local_extrafield = (uInt)size_extra_field;
104300000000
104400000000    *piSizeVar += (uInt)size_extra_field;
104500000000
104600000000    return err;
104700000000}
104800000000
104900000000/*
105000000000  Open for reading data the current file in the zipfile.
105100000000  If there is no error and the file is opened, the return value is UNZ_OK.
105200000000*/
105300000000extern int ZEXPORT unzOpenCurrentFile3 (file, method, level, raw, password)
105400000000    unzFile file;
105500000000    int* method;
105600000000    int* level;
105700000000    int raw;
105800000000    const char* password;
105900000000{
106000000000    int err=UNZ_OK;
106100000000    uInt iSizeVar;
106200000000    unz_s* s;
106300000000    file_in_zip_read_info_s* pfile_in_zip_read_info;
106400000000    uLong offset_local_extrafield;  /* offset of the local extra field */
106500000000    uInt  size_local_extrafield;    /* size of the local extra field */
106600000000#    ifndef NOUNCRYPT
106700000000    char source[12];
106800000000#    else
106900000000    if (password != NULL)
107000000000        return UNZ_PARAMERROR;
107100000000#    endif
107200000000
107300000000    if (file==NULL)
107400000000        return UNZ_PARAMERROR;
107500000000    s=(unz_s*)file;
107600000000    if (!s->current_file_ok)
107700000000        return UNZ_PARAMERROR;
107800000000
107900000000    if (s->pfile_in_zip_read != NULL)
108000000000        unzCloseCurrentFile(file);
108100000000
108200000000    if (unzlocal_CheckCurrentFileCoherencyHeader(s,&iSizeVar,
108300000000                &offset_local_extrafield,&size_local_extrafield)!=UNZ_OK)
108400000000        return UNZ_BADZIPFILE;
108500000000
108600000000    pfile_in_zip_read_info = (file_in_zip_read_info_s*)
108700000000                                        ALLOC(sizeof(file_in_zip_read_info_s));
108800000000    if (pfile_in_zip_read_info==NULL)
108900000000        return UNZ_INTERNALERROR;
109000000000
109100000000    pfile_in_zip_read_info->read_buffer=(char*)ALLOC(UNZ_BUFSIZE);
109200000000    pfile_in_zip_read_info->offset_local_extrafield = offset_local_extrafield;
109300000000    pfile_in_zip_read_info->size_local_extrafield = size_local_extrafield;
109400000000    pfile_in_zip_read_info->pos_local_extrafield=0;
109500000000    pfile_in_zip_read_info->raw=raw;
109600000000
109700000000    if (pfile_in_zip_read_info->read_buffer==NULL)
109800000000    {
109900000000        TRYFREE(pfile_in_zip_read_info);
110000000000        return UNZ_INTERNALERROR;
110100000000    }
110200000000
110300000000    pfile_in_zip_read_info->stream_initialised=0;
110400000000
110500000000    if (method!=NULL)
110600000000        *method = (int)s->cur_file_info.compression_method;
110700000000
110800000000    if (level!=NULL)
110900000000    {
111000000000        *level = 6;
111100000000        switch (s->cur_file_info.flag & 0x06)
111200000000        {
111300000000          case 6 : *level = 1; break;
111400000000          case 4 : *level = 2; break;
111500000000          case 2 : *level = 9; break;
111600000000        }
111700000000    }
111800000000
111900000000    if ((s->cur_file_info.compression_method!=0) &&
112000000000        (s->cur_file_info.compression_method!=Z_DEFLATED))
112100000000        err=UNZ_BADZIPFILE;
112200000000
112300000000    pfile_in_zip_read_info->crc32_wait=s->cur_file_info.crc;
112400000000    pfile_in_zip_read_info->crc32=0;
112500000000    pfile_in_zip_read_info->compression_method =
112600000000            s->cur_file_info.compression_method;
112700000000    pfile_in_zip_read_info->filestream=s->filestream;
112800000000    pfile_in_zip_read_info->z_filefunc=s->z_filefunc;
112900000000    pfile_in_zip_read_info->byte_before_the_zipfile=s->byte_before_the_zipfile;
113000000000
113100000000    pfile_in_zip_read_info->stream.total_out = 0;
113200000000
113300000000    if ((s->cur_file_info.compression_method==Z_DEFLATED) &&
113400000000        (!raw))
113500000000    {
113600000000      pfile_in_zip_read_info->stream.zalloc = (alloc_func)0;
113700000000      pfile_in_zip_read_info->stream.zfree = (free_func)0;
113800000000      pfile_in_zip_read_info->stream.opaque = (voidpf)0;
113900000000      pfile_in_zip_read_info->stream.next_in = (voidpf)0;
114000000000      pfile_in_zip_read_info->stream.avail_in = 0;
114100000000
114200000000      err=inflateInit2(&pfile_in_zip_read_info->stream, -MAX_WBITS);
114300000000      if (err == Z_OK)
114400000000        pfile_in_zip_read_info->stream_initialised=1;
114500000000      else
114600000000      {
114700000000        TRYFREE(pfile_in_zip_read_info);
114800000000        return err;
114900000000      }
115000000000        /* windowBits is passed < 0 to tell that there is no zlib header.
115100000000         * Note that in this case inflate *requires* an extra "dummy" byte
115200000000         * after the compressed stream in order to complete decompression and
115300000000         * return Z_STREAM_END.
115400000000         * In unzip, i don't wait absolutely Z_STREAM_END because I known the
115500000000         * size of both compressed and uncompressed data
115600000000         */
115700000000    }
115800000000    pfile_in_zip_read_info->rest_read_compressed =
115900000000            s->cur_file_info.compressed_size ;
116000000000    pfile_in_zip_read_info->rest_read_uncompressed =
116100000000            s->cur_file_info.uncompressed_size ;
116200000000
116300000000
116400000000    pfile_in_zip_read_info->pos_in_zipfile =
116500000000            s->cur_file_info_internal.offset_curfile + SIZEZIPLOCALHEADER +
116600000000              iSizeVar;
116700000000
116800000000    pfile_in_zip_read_info->stream.avail_in = (uInt)0;
116900000000
117000000000    s->pfile_in_zip_read = pfile_in_zip_read_info;
117100000000
117200000000#    ifndef NOUNCRYPT
117300000000    if (password != NULL)
117400000000    {
117500000000        int i;
117600000000        s->pcrc_32_tab = get_crc_table();
117700000000        init_keys(password,s->keys,s->pcrc_32_tab);
117800000000        if (ZSEEK(s->z_filefunc, s->filestream,
117900000000                  s->pfile_in_zip_read->pos_in_zipfile +
118000000000                     s->pfile_in_zip_read->byte_before_the_zipfile,
118100000000                  SEEK_SET)!=0)
118200000000            return UNZ_INTERNALERROR;
118300000000        if(ZREAD(s->z_filefunc, s->filestream,source, 12)<12)
118400000000            return UNZ_INTERNALERROR;
118500000000
118600000000        for (i = 0; i<12; i++)
118700000000            zdecode(s->keys,s->pcrc_32_tab,source[i]);
118800000000
118900000000        s->pfile_in_zip_read->pos_in_zipfile+=12;
119000000000        s->encrypted=1;
119100000000    }
119200000000#    endif
119300000000
119400000000
119500000000    return UNZ_OK;
119600000000}
119700000000
119800000000extern int ZEXPORT unzOpenCurrentFile (file)
119900000000    unzFile file;
120000000000{
120100000000    return unzOpenCurrentFile3(file, NULL, NULL, 0, NULL);
120200000000}
120300000000
120400000000extern int ZEXPORT unzOpenCurrentFilePassword (file, password)
120500000000    unzFile file;
120600000000    const char* password;
120700000000{
120800000000    return unzOpenCurrentFile3(file, NULL, NULL, 0, password);
120900000000}
121000000000
121100000000extern int ZEXPORT unzOpenCurrentFile2 (file,method,level,raw)
121200000000    unzFile file;
121300000000    int* method;
121400000000    int* level;
121500000000    int raw;
121600000000{
121700000000    return unzOpenCurrentFile3(file, method, level, raw, NULL);
121800000000}
121900000000
122000000000/*
122100000000  Read bytes from the current file.
122200000000  buf contain buffer where data must be copied
122300000000  len the size of buf.
122400000000
122500000000  return the number of byte copied if somes bytes are copied
122600000000  return 0 if the end of file was reached
122700000000  return <0 with error code if there is an error
122800000000    (UNZ_ERRNO for IO error, or zLib error for uncompress error)
122900000000*/
123000000000extern int ZEXPORT unzReadCurrentFile  (file, buf, len)
123100000000    unzFile file;
123200000000    voidp buf;
123300000000    unsigned len;
123400000000{
123500000000    int err=UNZ_OK;
123600000000    uInt iRead = 0;
123700000000    unz_s* s;
123800000000    file_in_zip_read_info_s* pfile_in_zip_read_info;
123900000000    if (file==NULL)
124000000000        return UNZ_PARAMERROR;
124100000000    s=(unz_s*)file;
124200000000    pfile_in_zip_read_info=s->pfile_in_zip_read;
124300000000
124400000000    if (pfile_in_zip_read_info==NULL)
124500000000        return UNZ_PARAMERROR;
124600000000
124700000000
124800000000    if ((pfile_in_zip_read_info->read_buffer == NULL))
124900000000        return UNZ_END_OF_LIST_OF_FILE;
125000000000    if (len==0)
125100000000        return 0;
125200000000
125300000000    pfile_in_zip_read_info->stream.next_out = (Bytef*)buf;
125400000000
125500000000    pfile_in_zip_read_info->stream.avail_out = (uInt)len;
125600000000
125700000000    if ((len>pfile_in_zip_read_info->rest_read_uncompressed) &&
125800000000        (!(pfile_in_zip_read_info->raw)))
125900000000        pfile_in_zip_read_info->stream.avail_out =
126000000000            (uInt)pfile_in_zip_read_info->rest_read_uncompressed;
126100000000
126200000000    if ((len>pfile_in_zip_read_info->rest_read_compressed+
126300000000           pfile_in_zip_read_info->stream.avail_in) &&
126400000000         (pfile_in_zip_read_info->raw))
126500000000        pfile_in_zip_read_info->stream.avail_out =
126600000000            (uInt)pfile_in_zip_read_info->rest_read_compressed+
126700000000            pfile_in_zip_read_info->stream.avail_in;
126800000000
126900000000    while (pfile_in_zip_read_info->stream.avail_out>0)
127000000000    {
127100000000        if ((pfile_in_zip_read_info->stream.avail_in==0) &&
127200000000            (pfile_in_zip_read_info->rest_read_compressed>0))
127300000000        {
127400000000            uInt uReadThis = UNZ_BUFSIZE;
127500000000            if (pfile_in_zip_read_info->rest_read_compressed<uReadThis)
127600000000                uReadThis = (uInt)pfile_in_zip_read_info->rest_read_compressed;
127700000000            if (uReadThis == 0)
127800000000                return UNZ_EOF;
127900000000            if (ZSEEK(pfile_in_zip_read_info->z_filefunc,
128000000000                      pfile_in_zip_read_info->filestream,
128100000000                      pfile_in_zip_read_info->pos_in_zipfile +
128200000000                         pfile_in_zip_read_info->byte_before_the_zipfile,
128300000000                         ZLIB_FILEFUNC_SEEK_SET)!=0)
128400000000                return UNZ_ERRNO;
128500000000            if (ZREAD(pfile_in_zip_read_info->z_filefunc,
128600000000                      pfile_in_zip_read_info->filestream,
128700000000                      pfile_in_zip_read_info->read_buffer,
128800000000                      uReadThis)!=uReadThis)
128900000000                return UNZ_ERRNO;
129000000000
129100000000
129200000000#            ifndef NOUNCRYPT
129300000000            if(s->encrypted)
129400000000            {
129500000000                uInt i;
129600000000                for(i=0;i<uReadThis;i++)
129700000000                  pfile_in_zip_read_info->read_buffer[i] =
129800000000                      zdecode(s->keys,s->pcrc_32_tab,
129900000000                              pfile_in_zip_read_info->read_buffer[i]);
130000000000            }
130100000000#            endif
130200000000
130300000000
130400000000            pfile_in_zip_read_info->pos_in_zipfile += uReadThis;
130500000000
130600000000            pfile_in_zip_read_info->rest_read_compressed-=uReadThis;
130700000000
130800000000            pfile_in_zip_read_info->stream.next_in =
130900000000                (Bytef*)pfile_in_zip_read_info->read_buffer;
131000000000            pfile_in_zip_read_info->stream.avail_in = (uInt)uReadThis;
131100000000        }
131200000000
131300000000        if ((pfile_in_zip_read_info->compression_method==0) || (pfile_in_zip_read_info->raw))
131400000000        {
131500000000            uInt uDoCopy,i ;
131600000000
131700000000            if ((pfile_in_zip_read_info->stream.avail_in == 0) &&
131800000000                (pfile_in_zip_read_info->rest_read_compressed == 0))
131900000000                return (iRead==0) ? UNZ_EOF : iRead;
132000000000
132100000000            if (pfile_in_zip_read_info->stream.avail_out <
132200000000                            pfile_in_zip_read_info->stream.avail_in)
132300000000                uDoCopy = pfile_in_zip_read_info->stream.avail_out ;
132400000000            else
132500000000                uDoCopy = pfile_in_zip_read_info->stream.avail_in ;
132600000000
132700000000            for (i=0;i<uDoCopy;i++)
132800000000                *(pfile_in_zip_read_info->stream.next_out+i) =
132900000000                        *(pfile_in_zip_read_info->stream.next_in+i);
133000000000
133100000000            pfile_in_zip_read_info->crc32 = crc32(pfile_in_zip_read_info->crc32,
133200000000                                pfile_in_zip_read_info->stream.next_out,
133300000000                                uDoCopy);
133400000000            pfile_in_zip_read_info->rest_read_uncompressed-=uDoCopy;
133500000000            pfile_in_zip_read_info->stream.avail_in -= uDoCopy;
133600000000            pfile_in_zip_read_info->stream.avail_out -= uDoCopy;
133700000000            pfile_in_zip_read_info->stream.next_out += uDoCopy;
133800000000            pfile_in_zip_read_info->stream.next_in += uDoCopy;
133900000000            pfile_in_zip_read_info->stream.total_out += uDoCopy;
134000000000            iRead += uDoCopy;
134100000000        }
134200000000        else
134300000000        {
134400000000            uLong uTotalOutBefore,uTotalOutAfter;
134500000000            const Bytef *bufBefore;
134600000000            uLong uOutThis;
134700000000            int flush=Z_SYNC_FLUSH;
134800000000
134900000000            uTotalOutBefore = pfile_in_zip_read_info->stream.total_out;
135000000000            bufBefore = pfile_in_zip_read_info->stream.next_out;
135100000000
135200000000            /*
135300000000            if ((pfile_in_zip_read_info->rest_read_uncompressed ==
135400000000                     pfile_in_zip_read_info->stream.avail_out) &&
135500000000                (pfile_in_zip_read_info->rest_read_compressed == 0))
135600000000                flush = Z_FINISH;
135700000000            */
135800000000            err=inflate(&pfile_in_zip_read_info->stream,flush);
135900000000
136000000000            if ((err>=0) && (pfile_in_zip_read_info->stream.msg!=NULL))
136100000000              err = Z_DATA_ERROR;
136200000000
136300000000            uTotalOutAfter = pfile_in_zip_read_info->stream.total_out;
136400000000            uOutThis = uTotalOutAfter-uTotalOutBefore;
136500000000
136600000000            pfile_in_zip_read_info->crc32 =
136700000000                crc32(pfile_in_zip_read_info->crc32,bufBefore,
136800000000                        (uInt)(uOutThis));
136900000000
137000000000            pfile_in_zip_read_info->rest_read_uncompressed -=
137100000000                uOutThis;
137200000000
137300000000            iRead += (uInt)(uTotalOutAfter - uTotalOutBefore);
137400000000
137500000000            if (err==Z_STREAM_END)
137600000000                return (iRead==0) ? UNZ_EOF : iRead;
137700000000            if (err!=Z_OK)
137800000000                break;
137900000000        }
138000000000    }
138100000000
138200000000    if (err==Z_OK)
138300000000        return iRead;
138400000000    return err;
138500000000}
138600000000
138700000000
138800000000/*
138900000000  Give the current position in uncompressed data
139000000000*/
139100000000extern z_off_t ZEXPORT unztell (file)
139200000000    unzFile file;
139300000000{
139400000000    unz_s* s;
139500000000    file_in_zip_read_info_s* pfile_in_zip_read_info;
139600000000    if (file==NULL)
139700000000        return UNZ_PARAMERROR;
139800000000    s=(unz_s*)file;
139900000000    pfile_in_zip_read_info=s->pfile_in_zip_read;
140000000000
140100000000    if (pfile_in_zip_read_info==NULL)
140200000000        return UNZ_PARAMERROR;
140300000000
140400000000    return (z_off_t)pfile_in_zip_read_info->stream.total_out;
140500000000}
140600000000
140700000000
140800000000/*
140900000000  return 1 if the end of file was reached, 0 elsewhere
141000000000*/
141100000000extern int ZEXPORT unzeof (file)
141200000000    unzFile file;
141300000000{
141400000000    unz_s* s;
141500000000    file_in_zip_read_info_s* pfile_in_zip_read_info;
141600000000    if (file==NULL)
141700000000        return UNZ_PARAMERROR;
141800000000    s=(unz_s*)file;
141900000000    pfile_in_zip_read_info=s->pfile_in_zip_read;
142000000000
142100000000    if (pfile_in_zip_read_info==NULL)
142200000000        return UNZ_PARAMERROR;
142300000000
142400000000    if (pfile_in_zip_read_info->rest_read_uncompressed == 0)
142500000000        return 1;
142600000000    else
142700000000        return 0;
142800000000}
142900000000
143000000000
143100000000
143200000000/*
143300000000  Read extra field from the current file (opened by unzOpenCurrentFile)
143400000000  This is the local-header version of the extra field (sometimes, there is
143500000000    more info in the local-header version than in the central-header)
143600000000
143700000000  if buf==NULL, it return the size of the local extra field that can be read
143800000000
143900000000  if buf!=NULL, len is the size of the buffer, the extra header is copied in
144000000000    buf.
144100000000  the return value is the number of bytes copied in buf, or (if <0)
144200000000    the error code
144300000000*/
144400000000extern int ZEXPORT unzGetLocalExtrafield (file,buf,len)
144500000000    unzFile file;
144600000000    voidp buf;
144700000000    unsigned len;
144800000000{
144900000000    unz_s* s;
145000000000    file_in_zip_read_info_s* pfile_in_zip_read_info;
145100000000    uInt read_now;
145200000000    uLong size_to_read;
145300000000
145400000000    if (file==NULL)
145500000000        return UNZ_PARAMERROR;
145600000000    s=(unz_s*)file;
145700000000    pfile_in_zip_read_info=s->pfile_in_zip_read;
145800000000
145900000000    if (pfile_in_zip_read_info==NULL)
146000000000        return UNZ_PARAMERROR;
146100000000
146200000000    size_to_read = (pfile_in_zip_read_info->size_local_extrafield -
146300000000                pfile_in_zip_read_info->pos_local_extrafield);
146400000000
146500000000    if (buf==NULL)
146600000000        return (int)size_to_read;
146700000000
146800000000    if (len>size_to_read)
146900000000        read_now = (uInt)size_to_read;
147000000000    else
147100000000        read_now = (uInt)len ;
147200000000
147300000000    if (read_now==0)
147400000000        return 0;
147500000000
147600000000    if (ZSEEK(pfile_in_zip_read_info->z_filefunc,
147700000000              pfile_in_zip_read_info->filestream,
147800000000              pfile_in_zip_read_info->offset_local_extrafield +
147900000000              pfile_in_zip_read_info->pos_local_extrafield,
148000000000              ZLIB_FILEFUNC_SEEK_SET)!=0)
148100000000        return UNZ_ERRNO;
148200000000
148300000000    if (ZREAD(pfile_in_zip_read_info->z_filefunc,
148400000000              pfile_in_zip_read_info->filestream,
148500000000              buf,read_now)!=read_now)
148600000000        return UNZ_ERRNO;
148700000000
148800000000    return (int)read_now;
148900000000}
149000000000
149100000000/*
149200000000  Close the file in zip opened with unzipOpenCurrentFile
149300000000  Return UNZ_CRCERROR if all the file was read but the CRC is not good
149400000000*/
149500000000extern int ZEXPORT unzCloseCurrentFile (file)
149600000000    unzFile file;
149700000000{
149800000000    int err=UNZ_OK;
149900000000
150000000000    unz_s* s;
150100000000    file_in_zip_read_info_s* pfile_in_zip_read_info;
150200000000    if (file==NULL)
150300000000        return UNZ_PARAMERROR;
150400000000    s=(unz_s*)file;
150500000000    pfile_in_zip_read_info=s->pfile_in_zip_read;
150600000000
150700000000    if (pfile_in_zip_read_info==NULL)
150800000000        return UNZ_PARAMERROR;
150900000000
151000000000
151100000000    if ((pfile_in_zip_read_info->rest_read_uncompressed == 0) &&
151200000000        (!pfile_in_zip_read_info->raw))
151300000000    {
151400000000        if (pfile_in_zip_read_info->crc32 != pfile_in_zip_read_info->crc32_wait)
151500000000            err=UNZ_CRCERROR;
151600000000    }
151700000000
151800000000
151900000000    TRYFREE(pfile_in_zip_read_info->read_buffer);
152000000000    pfile_in_zip_read_info->read_buffer = NULL;
152100000000    if (pfile_in_zip_read_info->stream_initialised)
152200000000        inflateEnd(&pfile_in_zip_read_info->stream);
152300000000
152400000000    pfile_in_zip_read_info->stream_initialised = 0;
152500000000    TRYFREE(pfile_in_zip_read_info);
152600000000
152700000000    s->pfile_in_zip_read=NULL;
152800000000
152900000000    return err;
153000000000}
153100000000
153200000000
153300000000/*
153400000000  Get the global comment string of the ZipFile, in the szComment buffer.
153500000000  uSizeBuf is the size of the szComment buffer.
153600000000  return the number of byte copied or an error code <0
153700000000*/
153800000000extern int ZEXPORT unzGetGlobalComment (file, szComment, uSizeBuf)
153900000000    unzFile file;
154000000000    char *szComment;
154100000000    uLong uSizeBuf;
154200000000{
154300000000    int err=UNZ_OK;
154400000000    unz_s* s;
154500000000    uLong uReadThis ;
154600000000    if (file==NULL)
154700000000        return UNZ_PARAMERROR;
154800000000    s=(unz_s*)file;
154900000000
155000000000    uReadThis = uSizeBuf;
155100000000    if (uReadThis>s->gi.size_comment)
155200000000        uReadThis = s->gi.size_comment;
155300000000
155400000000    if (ZSEEK(s->z_filefunc,s->filestream,s->central_pos+22,ZLIB_FILEFUNC_SEEK_SET)!=0)
155500000000        return UNZ_ERRNO;
155600000000
155700000000    if (uReadThis>0)
155800000000    {
155900000000      *szComment='\0';
156000000000      if (ZREAD(s->z_filefunc,s->filestream,szComment,uReadThis)!=uReadThis)
156100000000        return UNZ_ERRNO;
156200000000    }
156300000000
156400000000    if ((szComment != NULL) && (uSizeBuf > s->gi.size_comment))
156500000000        *(szComment+s->gi.size_comment)='\0';
156600000000    return (int)uReadThis;
156700000000}
156800000000
156900000000/* Additions by RX '2004 */
157000000000extern uLong ZEXPORT unzGetOffset (file)
157100000000    unzFile file;
157200000000{
157300000000    unz_s* s;
157400000000
157500000000    if (file==NULL)
157600000000          return UNZ_PARAMERROR;
157700000000    s=(unz_s*)file;
157800000000    if (!s->current_file_ok)
157900000000      return 0;
158000000000    if (s->gi.number_entry != 0 && s->gi.number_entry != 0xffff)
158100000000      if (s->num_file==s->gi.number_entry)
158200000000         return 0;
158300000000    return s->pos_in_central_dir;
158400000000}
158500000000
158600000000extern int ZEXPORT unzSetOffset (file, pos)
158700000000        unzFile file;
158800000000        uLong pos;
158900000000{
159000000000    unz_s* s;
159100000000    int err;
159200000000
159300000000    if (file==NULL)
159400000000        return UNZ_PARAMERROR;
159500000000    s=(unz_s*)file;
159600000000
159700000000    s->pos_in_central_dir = pos;
159800000000    s->num_file = s->gi.number_entry;      /* hack */
159900000000    err = unzlocal_GetCurrentFileInfoInternal(file,&s->cur_file_info,
160000000000                                              &s->cur_file_info_internal,
160100000000                                              NULL,0,NULL,0,NULL,0);
160200000000    s->current_file_ok = (err == UNZ_OK);
160300000000    return err;
160400000000}
