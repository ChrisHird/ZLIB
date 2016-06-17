000100000000/* uncompr.c -- decompress a memory buffer
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
001200000000     Decompresses the source buffer into the destination buffer.  sourceLen is
001300000000   the byte length of the source buffer. Upon entry, destLen is the total
001400000000   size of the destination buffer, which must be large enough to hold the
001500000000   entire uncompressed data. (The size of the uncompressed data must have
001600000000   been saved previously by the compressor and transmitted to the decompressor
001700000000   by some mechanism outside the scope of this compression library.)
001800000000   Upon exit, destLen is the actual size of the compressed buffer.
001900000000     This function can be used to decompress a whole file at once if the
002000000000   input file is mmap'ed.
002100000000
002200000000     uncompress returns Z_OK if success, Z_MEM_ERROR if there was not
002300000000   enough memory, Z_BUF_ERROR if there was not enough room in the output
002400000000   buffer, or Z_DATA_ERROR if the input data was corrupted.
002500000000*/
002600000000int ZEXPORT uncompress (dest, destLen, source, sourceLen)
002700000000    Bytef *dest;
002800000000    uLongf *destLen;
002900000000    const Bytef *source;
003000000000    uLong sourceLen;
003100000000{
003200000000    z_stream stream;
003300000000    int err;
003400000000
003500000000    stream.next_in = (Bytef*)source;
003600000000    stream.avail_in = (uInt)sourceLen;
003700000000    /* Check for source > 64K on 16-bit machine: */
003800000000    if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;
003900000000
004000000000    stream.next_out = dest;
004100000000    stream.avail_out = (uInt)*destLen;
004200000000    if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;
004300000000
004400000000    stream.zalloc = (alloc_func)0;
004500000000    stream.zfree = (free_func)0;
004600000000
004700000000    err = inflateInit(&stream);
004800000000    if (err != Z_OK) return err;
004900000000
005000000000    err = inflate(&stream, Z_FINISH);
005100000000    if (err != Z_STREAM_END) {
005200000000        inflateEnd(&stream);
005300000000        if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
005400000000            return Z_DATA_ERROR;
005500000000        return err;
005600000000    }
005700000000    *destLen = stream.total_out;
005800000000
005900000000    err = inflateEnd(&stream);
006000000000    return err;
006100000000}
