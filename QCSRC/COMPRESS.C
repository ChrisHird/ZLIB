000100000000/* compress.c -- compress a memory buffer
000200000000 * Copyright (C) 1995-2003 Jean-loup Gailly.
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/* @(#) $Id$ */
000700000000
000800000000#define ZLIB_INTERNAL
000900000000#include "zlib.h"
001000000000
001100000000/* ===========================================================================
001200000000     Compresses the source buffer into the destination buffer. The level
001300000000   parameter has the same meaning as in deflateInit.  sourceLen is the byte
001400000000   length of the source buffer. Upon entry, destLen is the total size of the
001500000000   destination buffer, which must be at least 0.1% larger than sourceLen plus
001600000000   12 bytes. Upon exit, destLen is the actual size of the compressed buffer.
001700000000
001800000000     compress2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
001900000000   memory, Z_BUF_ERROR if there was not enough room in the output buffer,
002000000000   Z_STREAM_ERROR if the level parameter is invalid.
002100000000*/
002200000000int ZEXPORT compress2 (dest, destLen, source, sourceLen, level)
002300000000    Bytef *dest;
002400000000    uLongf *destLen;
002500000000    const Bytef *source;
002600000000    uLong sourceLen;
002700000000    int level;
002800000000{
002900000000    z_stream stream;
003000000000    int err;
003100000000
003200000000    stream.next_in = (Bytef*)source;
003300000000    stream.avail_in = (uInt)sourceLen;
003400000000#ifdef MAXSEG_64K
003500000000    /* Check for source > 64K on 16-bit machine: */
003600000000    if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;
003700000000#endif
003800000000    stream.next_out = dest;
003900000000    stream.avail_out = (uInt)*destLen;
004000000000    if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;
004100000000
004200000000    stream.zalloc = (alloc_func)0;
004300000000    stream.zfree = (free_func)0;
004400000000    stream.opaque = (voidpf)0;
004500000000
004600000000    err = deflateInit(&stream, level);
004700000000    if (err != Z_OK) return err;
004800000000
004900000000    err = deflate(&stream, Z_FINISH);
005000000000    if (err != Z_STREAM_END) {
005100000000        deflateEnd(&stream);
005200000000        return err == Z_OK ? Z_BUF_ERROR : err;
005300000000    }
005400000000    *destLen = stream.total_out;
005500000000
005600000000    err = deflateEnd(&stream);
005700000000    return err;
005800000000}
005900000000
006000000000/* ===========================================================================
006100000000 */
006200000000int ZEXPORT compress (dest, destLen, source, sourceLen)
006300000000    Bytef *dest;
006400000000    uLongf *destLen;
006500000000    const Bytef *source;
006600000000    uLong sourceLen;
006700000000{
006800000000    return compress2(dest, destLen, source, sourceLen, Z_DEFAULT_COMPRESSION);
006900000000}
007000000000
007100000000/* ===========================================================================
007200000000     If the default memLevel or windowBits for deflateInit() is changed, then
007300000000   this function needs to be updated.
007400000000 */
007500000000uLong ZEXPORT compressBound (sourceLen)
007600000000    uLong sourceLen;
007700000000{
007800000000    return sourceLen + (sourceLen >> 12) + (sourceLen >> 14) + 11;
007900000000}
