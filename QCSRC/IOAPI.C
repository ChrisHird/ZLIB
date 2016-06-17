000100000000/* ioapi.c -- IO base function header for compress/uncompress .zip
000200000000   files using zlib + zip or unzip API
000300000000
000400000000   Version 1.01e, February 12th, 2005
000500000000
000600000000   Copyright (C) 1998-2005 Gilles Vollant
000700000000*/
000800000000
000900000000#include <stdio.h>
001000000000#include <stdlib.h>
001100000000#include <string.h>
001200000000
001300000000#include "zlib.h"
001400000000#include "ioapi.h"
001500000000
001600000000
001700000000
001800000000/* I've found an old Unix (a SunOS 4.1.3_U1) without all SEEK_* defined.... */
001900000000
002000000000#ifndef SEEK_CUR
002100000000#define SEEK_CUR    1
002200000000#endif
002300000000
002400000000#ifndef SEEK_END
002500000000#define SEEK_END    2
002600000000#endif
002700000000
002800000000#ifndef SEEK_SET
002900000000#define SEEK_SET    0
003000000000#endif
003100000000
003200000000voidpf ZCALLBACK fopen_file_func OF((
003300000000   voidpf opaque,
003400000000   const char* filename,
003500000000   int mode));
003600000000
003700000000uLong ZCALLBACK fread_file_func OF((
003800000000   voidpf opaque,
003900000000   voidpf stream,
004000000000   void* buf,
004100000000   uLong size));
004200000000
004300000000uLong ZCALLBACK fwrite_file_func OF((
004400000000   voidpf opaque,
004500000000   voidpf stream,
004600000000   const void* buf,
004700000000   uLong size));
004800000000
004900000000long ZCALLBACK ftell_file_func OF((
005000000000   voidpf opaque,
005100000000   voidpf stream));
005200000000
005300000000long ZCALLBACK fseek_file_func OF((
005400000000   voidpf opaque,
005500000000   voidpf stream,
005600000000   uLong offset,
005700000000   int origin));
005800000000
005900000000int ZCALLBACK fclose_file_func OF((
006000000000   voidpf opaque,
006100000000   voidpf stream));
006200000000
006300000000int ZCALLBACK ferror_file_func OF((
006400000000   voidpf opaque,
006500000000   voidpf stream));
006600000000
006700000000
006800000000voidpf ZCALLBACK fopen_file_func (opaque, filename, mode)
006900000000   voidpf opaque;
007000000000   const char* filename;
007100000000   int mode;
007200000000{
007300000000    FILE* file = NULL;
007400000000    const char* mode_fopen = NULL;
007500000000    if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ)
007600000000        mode_fopen = "rb";
007700000000    else
007800000000    if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
007900000000        mode_fopen = "r+b";
008000000000    else
008100000000    if (mode & ZLIB_FILEFUNC_MODE_CREATE)
008200000000        mode_fopen = "wb";
008300000000
008400000000    if ((filename!=NULL) && (mode_fopen != NULL))
008500000000        file = fopen(filename, mode_fopen);
008600000000    return file;
008700000000}
008800000000
008900000000
009000000000uLong ZCALLBACK fread_file_func (opaque, stream, buf, size)
009100000000   voidpf opaque;
009200000000   voidpf stream;
009300000000   void* buf;
009400000000   uLong size;
009500000000{
009600000000    uLong ret;
009700000000    ret = (uLong)fread(buf, 1, (size_t)size, (FILE *)stream);
009800000000    return ret;
009900000000}
010000000000
010100000000
010200000000uLong ZCALLBACK fwrite_file_func (opaque, stream, buf, size)
010300000000   voidpf opaque;
010400000000   voidpf stream;
010500000000   const void* buf;
010600000000   uLong size;
010700000000{
010800000000    uLong ret;
010900000000    ret = (uLong)fwrite(buf, 1, (size_t)size, (FILE *)stream);
011000000000    return ret;
011100000000}
011200000000
011300000000long ZCALLBACK ftell_file_func (opaque, stream)
011400000000   voidpf opaque;
011500000000   voidpf stream;
011600000000{
011700000000    long ret;
011800000000    ret = ftell((FILE *)stream);
011900000000    return ret;
012000000000}
012100000000
012200000000long ZCALLBACK fseek_file_func (opaque, stream, offset, origin)
012300000000   voidpf opaque;
012400000000   voidpf stream;
012500000000   uLong offset;
012600000000   int origin;
012700000000{
012800000000    int fseek_origin=0;
012900000000    long ret;
013000000000    switch (origin)
013100000000    {
013200000000    case ZLIB_FILEFUNC_SEEK_CUR :
013300000000        fseek_origin = SEEK_CUR;
013400000000        break;
013500000000    case ZLIB_FILEFUNC_SEEK_END :
013600000000        fseek_origin = SEEK_END;
013700000000        break;
013800000000    case ZLIB_FILEFUNC_SEEK_SET :
013900000000        fseek_origin = SEEK_SET;
014000000000        break;
014100000000    default: return -1;
014200000000    }
014300000000    ret = 0;
014400000000    fseek((FILE *)stream, offset, fseek_origin);
014500000000    return ret;
014600000000}
014700000000
014800000000int ZCALLBACK fclose_file_func (opaque, stream)
014900000000   voidpf opaque;
015000000000   voidpf stream;
015100000000{
015200000000    int ret;
015300000000    ret = fclose((FILE *)stream);
015400000000    return ret;
015500000000}
015600000000
015700000000int ZCALLBACK ferror_file_func (opaque, stream)
015800000000   voidpf opaque;
015900000000   voidpf stream;
016000000000{
016100000000    int ret;
016200000000    ret = ferror((FILE *)stream);
016300000000    return ret;
016400000000}
016500000000
016600000000void fill_fopen_filefunc (pzlib_filefunc_def)
016700000000  zlib_filefunc_def* pzlib_filefunc_def;
016800000000{
016900000000    pzlib_filefunc_def->zopen_file = fopen_file_func;
017000000000    pzlib_filefunc_def->zread_file = fread_file_func;
017100000000    pzlib_filefunc_def->zwrite_file = fwrite_file_func;
017200000000    pzlib_filefunc_def->ztell_file = ftell_file_func;
017300000000    pzlib_filefunc_def->zseek_file = fseek_file_func;
017400000000    pzlib_filefunc_def->zclose_file = fclose_file_func;
017500000000    pzlib_filefunc_def->zerror_file = ferror_file_func;
017600000000    pzlib_filefunc_def->opaque = NULL;
017700000000}
