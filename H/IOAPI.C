000100000000/* ioapi.h -- IO base function header for compress/uncompress .zip
000200000000   files using zlib + zip or unzip API
000300000000
000400000000   Version 1.01e, February 12th, 2005
000500000000
000600000000   Copyright (C) 1998-2005 Gilles Vollant
000700000000*/
000800000000
000900000000#ifndef _ZLIBIOAPI_H
001000000000#define _ZLIBIOAPI_H
001100000000
001200000000
001300000000#define ZLIB_FILEFUNC_SEEK_CUR (1)
001400000000#define ZLIB_FILEFUNC_SEEK_END (2)
001500000000#define ZLIB_FILEFUNC_SEEK_SET (0)
001600000000
001700000000#define ZLIB_FILEFUNC_MODE_READ      (1)
001800000000#define ZLIB_FILEFUNC_MODE_WRITE     (2)
001900000000#define ZLIB_FILEFUNC_MODE_READWRITEFILTER (3)
002000000000
002100000000#define ZLIB_FILEFUNC_MODE_EXISTING (4)
002200000000#define ZLIB_FILEFUNC_MODE_CREATE   (8)
002300000000
002400000000
002500000000#ifndef ZCALLBACK
002600000000
002700000000#if (defined(WIN32) || defined (WINDOWS) || defined (_WINDOWS)) && defined(CALLBACK) && defined (USEWINDOWS_CALLBACK)
002800000000#define ZCALLBACK CALLBACK
002900000000#else
003000000000#define ZCALLBACK
003100000000#endif
003200000000#endif
003300000000
003400000000#ifdef __cplusplus
003500000000extern "C" {
003600000000#endif
003700000000
003800000000typedef voidpf (ZCALLBACK *open_file_func) OF((voidpf opaque, const char* filename, int mode));
003900000000typedef uLong  (ZCALLBACK *read_file_func) OF((voidpf opaque, voidpf stream, void* buf, uLong size));
004000000000typedef uLong  (ZCALLBACK *write_file_func) OF((voidpf opaque, voidpf stream, const void* buf, uLong size));
004100000000typedef long   (ZCALLBACK *tell_file_func) OF((voidpf opaque, voidpf stream));
004200000000typedef long   (ZCALLBACK *seek_file_func) OF((voidpf opaque, voidpf stream, uLong offset, int origin));
004300000000typedef int    (ZCALLBACK *close_file_func) OF((voidpf opaque, voidpf stream));
004400000000typedef int    (ZCALLBACK *testerror_file_func) OF((voidpf opaque, voidpf stream));
004500000000
004600000000typedef struct zlib_filefunc_def_s
004700000000{
004800000000    open_file_func      zopen_file;
004900000000    read_file_func      zread_file;
005000000000    write_file_func     zwrite_file;
005100000000    tell_file_func      ztell_file;
005200000000    seek_file_func      zseek_file;
005300000000    close_file_func     zclose_file;
005400000000    testerror_file_func zerror_file;
005500000000    voidpf              opaque;
005600000000} zlib_filefunc_def;
005700000000
005800000000
005900000000
006000000000void fill_fopen_filefunc OF((zlib_filefunc_def* pzlib_filefunc_def));
006100000000
006200000000#define ZREAD(filefunc,filestream,buf,size) ((*((filefunc).zread_file))((filefunc).opaque,filestream,buf,size))
006300000000#define ZWRITE(filefunc,filestream,buf,size) ((*((filefunc).zwrite_file))((filefunc).opaque,filestream,buf,size))
006400000000#define ZTELL(filefunc,filestream) ((*((filefunc).ztell_file))((filefunc).opaque,filestream))
006500000000#define ZSEEK(filefunc,filestream,pos,mode) ((*((filefunc).zseek_file))((filefunc).opaque,filestream,pos,mode))
006600000000#define ZCLOSE(filefunc,filestream) ((*((filefunc).zclose_file))((filefunc).opaque,filestream))
006700000000#define ZERROR(filefunc,filestream) ((*((filefunc).zerror_file))((filefunc).opaque,filestream))
006800000000
006900000000
007000000000#ifdef __cplusplus
007100000000}
007200000000#endif
007300000000
007400000000#endif
007500000000
