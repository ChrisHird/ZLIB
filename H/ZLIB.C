000100000000/* zlib.h -- interface of the 'zlib' general purpose compression library
000200000000  version 1.2.3, July 18th, 2005
000300000000
000400000000  Copyright (C) 1995-2005 Jean-loup Gailly and Mark Adler
000500000000
000600000000  This software is provided 'as-is', without any express or implied
000700000000  warranty.  In no event will the authors be held liable for any damages
000800000000  arising from the use of this software.
000900000000
001000000000  Permission is granted to anyone to use this software for any purpose,
001100000000  including commercial applications, and to alter it and redistribute it
001200000000  freely, subject to the following restrictions:
001300000000
001400000000  1. The origin of this software must not be misrepresented; you must not
001500000000     claim that you wrote the original software. If you use this software
001600000000     in a product, an acknowledgment in the product documentation would be
001700000000     appreciated but is not required.
001800000000  2. Altered source versions must be plainly marked as such, and must not be
001900000000     misrepresented as being the original software.
002000000000  3. This notice may not be removed or altered from any source distribution.
002100000000
002200000000  Jean-loup Gailly        Mark Adler
002300000000  jloup@gzip.org          madler@alumni.caltech.edu
002400000000
002500000000
002600000000  The data format used by the zlib library is described by RFCs (Request for
002700000000  Comments) 1950 to 1952 in the files http://www.ietf.org/rfc/rfc1950.txt
002800000000  (zlib format), rfc1951.txt (deflate format) and rfc1952.txt (gzip format).
002900000000*/
003000000000
003100000000#ifndef ZLIB_H
003200000000#define ZLIB_H
003300000000
003400000000#include "zconf.h"
003500000000
003600000000#ifdef __cplusplus
003700000000extern "C" {
003800000000#endif
003900000000
004000000000#define ZLIB_VERSION "1.2.3"
004100000000#define ZLIB_VERNUM 0x1230
004200000000
004300000000/*
004400000000     The 'zlib' compression library provides in-memory compression and
004500000000  decompression functions, including integrity checks of the uncompressed
004600000000  data.  This version of the library supports only one compression method
004700000000  (deflation) but other algorithms will be added later and will have the same
004800000000  stream interface.
004900000000
005000000000     Compression can be done in a single step if the buffers are large
005100000000  enough (for example if an input file is mmap'ed), or can be done by
005200000000  repeated calls of the compression function.  In the latter case, the
005300000000  application must provide more input and/or consume the output
005400000000  (providing more output space) before each call.
005500000000
005600000000     The compressed data format used by default by the in-memory functions is
005700000000  the zlib format, which is a zlib wrapper documented in RFC 1950, wrapped
005800000000  around a deflate stream, which is itself documented in RFC 1951.
005900000000
006000000000     The library also supports reading and writing files in gzip (.gz) format
006100000000  with an interface similar to that of stdio using the functions that start
006200000000  with "gz".  The gzip format is different from the zlib format.  gzip is a
006300000000  gzip wrapper, documented in RFC 1952, wrapped around a deflate stream.
006400000000
006500000000     This library can optionally read and write gzip streams in memory as well.
006600000000
006700000000     The zlib format was designed to be compact and fast for use in memory
006800000000  and on communications channels.  The gzip format was designed for single-
006900000000  file compression on file systems, has a larger header than zlib to maintain
007000000000  directory information, and uses a different, slower check method than zlib.
007100000000
007200000000     The library does not install any signal handler. The decoder checks
007300000000  the consistency of the compressed data, so the library should never
007400000000  crash even in case of corrupted input.
007500000000*/
007600000000
007700000000typedef voidpf (*alloc_func) OF((voidpf opaque, uInt items, uInt size));
007800000000typedef void   (*free_func)  OF((voidpf opaque, voidpf address));
007900000000
008000000000struct internal_state;
008100000000
008200000000typedef struct z_stream_s {
008300000000    Bytef    *next_in;  /* next input byte */
008400000000    uInt     avail_in;  /* number of bytes available at next_in */
008500000000    uLong    total_in;  /* total nb of input bytes read so far */
008600000000
008700000000    Bytef    *next_out; /* next output byte should be put there */
008800000000    uInt     avail_out; /* remaining free space at next_out */
008900000000    uLong    total_out; /* total nb of bytes output so far */
009000000000
009100000000    char     *msg;      /* last error message, NULL if no error */
009200000000    struct internal_state FAR *state; /* not visible by applications */
009300000000
009400000000    alloc_func zalloc;  /* used to allocate the internal state */
009500000000    free_func  zfree;   /* used to free the internal state */
009600000000    voidpf     opaque;  /* private data object passed to zalloc and zfree */
009700000000
009800000000    int     data_type;  /* best guess about the data type: binary or text */
009900000000    uLong   adler;      /* adler32 value of the uncompressed data */
010000000000    uLong   reserved;   /* reserved for future use */
010100000000} z_stream;
010200000000
010300000000typedef z_stream FAR *z_streamp;
010400000000
010500000000/*
010600000000     gzip header information passed to and from zlib routines.  See RFC 1952
010700000000  for more details on the meanings of these fields.
010800000000*/
010900000000typedef struct gz_header_s {
011000000000    int     text;       /* true if compressed data believed to be text */
011100000000    uLong   time;       /* modification time */
011200000000    int     xflags;     /* extra flags (not used when writing a gzip file) */
011300000000    int     os;         /* operating system */
011400000000    Bytef   *extra;     /* pointer to extra field or Z_NULL if none */
011500000000    uInt    extra_len;  /* extra field length (valid if extra != Z_NULL) */
011600000000    uInt    extra_max;  /* space at extra (only when reading header) */
011700000000    Bytef   *name;      /* pointer to zero-terminated file name or Z_NULL */
011800000000    uInt    name_max;   /* space at name (only when reading header) */
011900000000    Bytef   *comment;   /* pointer to zero-terminated comment or Z_NULL */
012000000000    uInt    comm_max;   /* space at comment (only when reading header) */
012100000000    int     hcrc;       /* true if there was or will be a header crc */
012200000000    int     done;       /* true when done reading gzip header (not used
012300000000                           when writing a gzip file) */
012400000000} gz_header;
012500000000
012600000000typedef gz_header FAR *gz_headerp;
012700000000
012800000000/*
012900000000   The application must update next_in and avail_in when avail_in has
013000000000   dropped to zero. It must update next_out and avail_out when avail_out
013100000000   has dropped to zero. The application must initialize zalloc, zfree and
013200000000   opaque before calling the init function. All other fields are set by the
013300000000   compression library and must not be updated by the application.
013400000000
013500000000   The opaque value provided by the application will be passed as the first
013600000000   parameter for calls of zalloc and zfree. This can be useful for custom
013700000000   memory management. The compression library attaches no meaning to the
013800000000   opaque value.
013900000000
014000000000   zalloc must return Z_NULL if there is not enough memory for the object.
014100000000   If zlib is used in a multi-threaded application, zalloc and zfree must be
014200000000   thread safe.
014300000000
014400000000   On 16-bit systems, the functions zalloc and zfree must be able to allocate
014500000000   exactly 65536 bytes, but will not be required to allocate more than this
014600000000   if the symbol MAXSEG_64K is defined (see zconf.h). WARNING: On MSDOS,
014700000000   pointers returned by zalloc for objects of exactly 65536 bytes *must*
014800000000   have their offset normalized to zero. The default allocation function
014900000000   provided by this library ensures this (see zutil.c). To reduce memory
015000000000   requirements and avoid any allocation of 64K objects, at the expense of
015100000000   compression ratio, compile the library with -DMAX_WBITS=14 (see zconf.h).
015200000000
015300000000   The fields total_in and total_out can be used for statistics or
015400000000   progress reports. After compression, total_in holds the total size of
015500000000   the uncompressed data and may be saved for use in the decompressor
015600000000   (particularly if the decompressor wants to decompress everything in
015700000000   a single step).
015800000000*/
015900000000
016000000000                        /* constants */
016100000000
016200000000#define Z_NO_FLUSH      0
016300000000#define Z_PARTIAL_FLUSH 1 /* will be removed, use Z_SYNC_FLUSH instead */
016400000000#define Z_SYNC_FLUSH    2
016500000000#define Z_FULL_FLUSH    3
016600000000#define Z_FINISH        4
016700000000#define Z_BLOCK         5
016800000000/* Allowed flush values; see deflate() and inflate() below for details */
016900000000
017000000000#define Z_OK            0
017100000000#define Z_STREAM_END    1
017200000000#define Z_NEED_DICT     2
017300000000#define Z_ERRNO        (-1)
017400000000#define Z_STREAM_ERROR (-2)
017500000000#define Z_DATA_ERROR   (-3)
017600000000#define Z_MEM_ERROR    (-4)
017700000000#define Z_BUF_ERROR    (-5)
017800000000#define Z_VERSION_ERROR (-6)
017900000000/* Return codes for the compression/decompression functions. Negative
018000000000 * values are errors, positive values are used for special but normal events.
018100000000 */
018200000000
018300000000#define Z_NO_COMPRESSION         0
018400000000#define Z_BEST_SPEED             1
018500000000#define Z_BEST_COMPRESSION       9
018600000000#define Z_DEFAULT_COMPRESSION  (-1)
018700000000/* compression levels */
018800000000
018900000000#define Z_FILTERED            1
019000000000#define Z_HUFFMAN_ONLY        2
019100000000#define Z_RLE                 3
019200000000#define Z_FIXED               4
019300000000#define Z_DEFAULT_STRATEGY    0
019400000000/* compression strategy; see deflateInit2() below for details */
019500000000
019600000000#define Z_BINARY   0
019700000000#define Z_TEXT     1
019800000000#define Z_ASCII    Z_TEXT   /* for compatibility with 1.2.2 and earlier */
019900000000#define Z_UNKNOWN  2
020000000000/* Possible values of the data_type field (though see inflate()) */
020100000000
020200000000#define Z_DEFLATED   8
020300000000/* The deflate compression method (the only one supported in this version) */
020400000000
020500000000#define Z_NULL  0  /* for initializing zalloc, zfree, opaque */
020600000000
020700000000#define zlib_version zlibVersion()
020800000000/* for compatibility with versions < 1.0.2 */
020900000000
021000000000                        /* basic functions */
021100000000
021200000000ZEXTERN const char * ZEXPORT zlibVersion OF((void));
021300000000/* The application can compare zlibVersion and ZLIB_VERSION for consistency.
021400000000   If the first character differs, the library code actually used is
021500000000   not compatible with the zlib.h header file used by the application.
021600000000   This check is automatically made by deflateInit and inflateInit.
021700000000 */
021800000000
021900000000/*
022000000000ZEXTERN int ZEXPORT deflateInit OF((z_streamp strm, int level));
022100000000
022200000000     Initializes the internal stream state for compression. The fields
022300000000   zalloc, zfree and opaque must be initialized before by the caller.
022400000000   If zalloc and zfree are set to Z_NULL, deflateInit updates them to
022500000000   use default allocation functions.
022600000000
022700000000     The compression level must be Z_DEFAULT_COMPRESSION, or between 0 and 9:
022800000000   1 gives best speed, 9 gives best compression, 0 gives no compression at
022900000000   all (the input data is simply copied a block at a time).
023000000000   Z_DEFAULT_COMPRESSION requests a default compromise between speed and
023100000000   compression (currently equivalent to level 6).
023200000000
023300000000     deflateInit returns Z_OK if success, Z_MEM_ERROR if there was not
023400000000   enough memory, Z_STREAM_ERROR if level is not a valid compression level,
023500000000   Z_VERSION_ERROR if the zlib library version (zlib_version) is incompatible
023600000000   with the version assumed by the caller (ZLIB_VERSION).
023700000000   msg is set to null if there is no error message.  deflateInit does not
023800000000   perform any compression: this will be done by deflate().
023900000000*/
024000000000
024100000000
024200000000ZEXTERN int ZEXPORT deflate OF((z_streamp strm, int flush));
024300000000/*
024400000000    deflate compresses as much data as possible, and stops when the input
024500000000  buffer becomes empty or the output buffer becomes full. It may introduce some
024600000000  output latency (reading input without producing any output) except when
024700000000  forced to flush.
024800000000
024900000000    The detailed semantics are as follows. deflate performs one or both of the
025000000000  following actions:
025100000000
025200000000  - Compress more input starting at next_in and update next_in and avail_in
025300000000    accordingly. If not all input can be processed (because there is not
025400000000    enough room in the output buffer), next_in and avail_in are updated and
025500000000    processing will resume at this point for the next call of deflate().
025600000000
025700000000  - Provide more output starting at next_out and update next_out and avail_out
025800000000    accordingly. This action is forced if the parameter flush is non zero.
025900000000    Forcing flush frequently degrades the compression ratio, so this parameter
026000000000    should be set only when necessary (in interactive applications).
026100000000    Some output may be provided even if flush is not set.
026200000000
026300000000  Before the call of deflate(), the application should ensure that at least
026400000000  one of the actions is possible, by providing more input and/or consuming
026500000000  more output, and updating avail_in or avail_out accordingly; avail_out
026600000000  should never be zero before the call. The application can consume the
026700000000  compressed output when it wants, for example when the output buffer is full
026800000000  (avail_out == 0), or after each call of deflate(). If deflate returns Z_OK
026900000000  and with zero avail_out, it must be called again after making room in the
027000000000  output buffer because there might be more output pending.
027100000000
027200000000    Normally the parameter flush is set to Z_NO_FLUSH, which allows deflate to
027300000000  decide how much data to accumualte before producing output, in order to
027400000000  maximize compression.
027500000000
027600000000    If the parameter flush is set to Z_SYNC_FLUSH, all pending output is
027700000000  flushed to the output buffer and the output is aligned on a byte boundary, so
027800000000  that the decompressor can get all input data available so far. (In particular
027900000000  avail_in is zero after the call if enough output space has been provided
028000000000  before the call.)  Flushing may degrade compression for some compression
028100000000  algorithms and so it should be used only when necessary.
028200000000
028300000000    If flush is set to Z_FULL_FLUSH, all output is flushed as with
028400000000  Z_SYNC_FLUSH, and the compression state is reset so that decompression can
028500000000  restart from this point if previous compressed data has been damaged or if
028600000000  random access is desired. Using Z_FULL_FLUSH too often can seriously degrade
028700000000  compression.
028800000000
028900000000    If deflate returns with avail_out == 0, this function must be called again
029000000000  with the same value of the flush parameter and more output space (updated
029100000000  avail_out), until the flush is complete (deflate returns with non-zero
029200000000  avail_out). In the case of a Z_FULL_FLUSH or Z_SYNC_FLUSH, make sure that
029300000000  avail_out is greater than six to avoid repeated flush markers due to
029400000000  avail_out == 0 on return.
029500000000
029600000000    If the parameter flush is set to Z_FINISH, pending input is processed,
029700000000  pending output is flushed and deflate returns with Z_STREAM_END if there
029800000000  was enough output space; if deflate returns with Z_OK, this function must be
029900000000  called again with Z_FINISH and more output space (updated avail_out) but no
030000000000  more input data, until it returns with Z_STREAM_END or an error. After
030100000000  deflate has returned Z_STREAM_END, the only possible operations on the
030200000000  stream are deflateReset or deflateEnd.
030300000000
030400000000    Z_FINISH can be used immediately after deflateInit if all the compression
030500000000  is to be done in a single step. In this case, avail_out must be at least
030600000000  the value returned by deflateBound (see below). If deflate does not return
030700000000  Z_STREAM_END, then it must be called again as described above.
030800000000
030900000000    deflate() sets strm->adler to the adler32 checksum of all input read
031000000000  so far (that is, total_in bytes).
031100000000
031200000000    deflate() may update strm->data_type if it can make a good guess about
031300000000  the input data type (Z_BINARY or Z_TEXT). In doubt, the data is considered
031400000000  binary. This field is only for information purposes and does not affect
031500000000  the compression algorithm in any manner.
031600000000
031700000000    deflate() returns Z_OK if some progress has been made (more input
031800000000  processed or more output produced), Z_STREAM_END if all input has been
031900000000  consumed and all output has been produced (only when flush is set to
032000000000  Z_FINISH), Z_STREAM_ERROR if the stream state was inconsistent (for example
032100000000  if next_in or next_out was NULL), Z_BUF_ERROR if no progress is possible
032200000000  (for example avail_in or avail_out was zero). Note that Z_BUF_ERROR is not
032300000000  fatal, and deflate() can be called again with more input and more output
032400000000  space to continue compressing.
032500000000*/
032600000000
032700000000
032800000000ZEXTERN int ZEXPORT deflateEnd OF((z_streamp strm));
032900000000/*
033000000000     All dynamically allocated data structures for this stream are freed.
033100000000   This function discards any unprocessed input and does not flush any
033200000000   pending output.
033300000000
033400000000     deflateEnd returns Z_OK if success, Z_STREAM_ERROR if the
033500000000   stream state was inconsistent, Z_DATA_ERROR if the stream was freed
033600000000   prematurely (some input or output was discarded). In the error case,
033700000000   msg may be set but then points to a static string (which must not be
033800000000   deallocated).
033900000000*/
034000000000
034100000000
034200000000/*
034300000000ZEXTERN int ZEXPORT inflateInit OF((z_streamp strm));
034400000000
034500000000     Initializes the internal stream state for decompression. The fields
034600000000   next_in, avail_in, zalloc, zfree and opaque must be initialized before by
034700000000   the caller. If next_in is not Z_NULL and avail_in is large enough (the exact
034800000000   value depends on the compression method), inflateInit determines the
034900000000   compression method from the zlib header and allocates all data structures
035000000000   accordingly; otherwise the allocation will be deferred to the first call of
035100000000   inflate.  If zalloc and zfree are set to Z_NULL, inflateInit updates them to
035200000000   use default allocation functions.
035300000000
035400000000     inflateInit returns Z_OK if success, Z_MEM_ERROR if there was not enough
035500000000   memory, Z_VERSION_ERROR if the zlib library version is incompatible with the
035600000000   version assumed by the caller.  msg is set to null if there is no error
035700000000   message. inflateInit does not perform any decompression apart from reading
035800000000   the zlib header if present: this will be done by inflate().  (So next_in and
035900000000   avail_in may be modified, but next_out and avail_out are unchanged.)
036000000000*/
036100000000
036200000000
036300000000ZEXTERN int ZEXPORT inflate OF((z_streamp strm, int flush));
036400000000/*
036500000000    inflate decompresses as much data as possible, and stops when the input
036600000000  buffer becomes empty or the output buffer becomes full. It may introduce
036700000000  some output latency (reading input without producing any output) except when
036800000000  forced to flush.
036900000000
037000000000  The detailed semantics are as follows. inflate performs one or both of the
037100000000  following actions:
037200000000
037300000000  - Decompress more input starting at next_in and update next_in and avail_in
037400000000    accordingly. If not all input can be processed (because there is not
037500000000    enough room in the output buffer), next_in is updated and processing
037600000000    will resume at this point for the next call of inflate().
037700000000
037800000000  - Provide more output starting at next_out and update next_out and avail_out
037900000000    accordingly.  inflate() provides as much output as possible, until there
038000000000    is no more input data or no more space in the output buffer (see below
038100000000    about the flush parameter).
038200000000
038300000000  Before the call of inflate(), the application should ensure that at least
038400000000  one of the actions is possible, by providing more input and/or consuming
038500000000  more output, and updating the next_* and avail_* values accordingly.
038600000000  The application can consume the uncompressed output when it wants, for
038700000000  example when the output buffer is full (avail_out == 0), or after each
038800000000  call of inflate(). If inflate returns Z_OK and with zero avail_out, it
038900000000  must be called again after making room in the output buffer because there
039000000000  might be more output pending.
039100000000
039200000000    The flush parameter of inflate() can be Z_NO_FLUSH, Z_SYNC_FLUSH,
039300000000  Z_FINISH, or Z_BLOCK. Z_SYNC_FLUSH requests that inflate() flush as much
039400000000  output as possible to the output buffer. Z_BLOCK requests that inflate() stop
039500000000  if and when it gets to the next deflate block boundary. When decoding the
039600000000  zlib or gzip format, this will cause inflate() to return immediately after
039700000000  the header and before the first block. When doing a raw inflate, inflate()
039800000000  will go ahead and process the first block, and will return when it gets to
039900000000  the end of that block, or when it runs out of data.
040000000000
040100000000    The Z_BLOCK option assists in appending to or combining deflate streams.
040200000000  Also to assist in this, on return inflate() will set strm->data_type to the
040300000000  number of unused bits in the last byte taken from strm->next_in, plus 64
040400000000  if inflate() is currently decoding the last block in the deflate stream,
040500000000  plus 128 if inflate() returned immediately after decoding an end-of-block
040600000000  code or decoding the complete header up to just before the first byte of the
040700000000  deflate stream. The end-of-block will not be indicated until all of the
040800000000  uncompressed data from that block has been written to strm->next_out.  The
040900000000  number of unused bits may in general be greater than seven, except when
041000000000  bit 7 of data_type is set, in which case the number of unused bits will be
041100000000  less than eight.
041200000000
041300000000    inflate() should normally be called until it returns Z_STREAM_END or an
041400000000  error. However if all decompression is to be performed in a single step
041500000000  (a single call of inflate), the parameter flush should be set to
041600000000  Z_FINISH. In this case all pending input is processed and all pending
041700000000  output is flushed; avail_out must be large enough to hold all the
041800000000  uncompressed data. (The size of the uncompressed data may have been saved
041900000000  by the compressor for this purpose.) The next operation on this stream must
042000000000  be inflateEnd to deallocate the decompression state. The use of Z_FINISH
042100000000  is never required, but can be used to inform inflate that a faster approach
042200000000  may be used for the single inflate() call.
042300000000
042400000000     In this implementation, inflate() always flushes as much output as
042500000000  possible to the output buffer, and always uses the faster approach on the
042600000000  first call. So the only effect of the flush parameter in this implementation
042700000000  is on the return value of inflate(), as noted below, or when it returns early
042800000000  because Z_BLOCK is used.
042900000000
043000000000     If a preset dictionary is needed after this call (see inflateSetDictionary
043100000000  below), inflate sets strm->adler to the adler32 checksum of the dictionary
043200000000  chosen by the compressor and returns Z_NEED_DICT; otherwise it sets
043300000000  strm->adler to the adler32 checksum of all output produced so far (that is,
043400000000  total_out bytes) and returns Z_OK, Z_STREAM_END or an error code as described
043500000000  below. At the end of the stream, inflate() checks that its computed adler32
043600000000  checksum is equal to that saved by the compressor and returns Z_STREAM_END
043700000000  only if the checksum is correct.
043800000000
043900000000    inflate() will decompress and check either zlib-wrapped or gzip-wrapped
044000000000  deflate data.  The header type is detected automatically.  Any information
044100000000  contained in the gzip header is not retained, so applications that need that
044200000000  information should instead use raw inflate, see inflateInit2() below, or
044300000000  inflateBack() and perform their own processing of the gzip header and
044400000000  trailer.
044500000000
044600000000    inflate() returns Z_OK if some progress has been made (more input processed
044700000000  or more output produced), Z_STREAM_END if the end of the compressed data has
044800000000  been reached and all uncompressed output has been produced, Z_NEED_DICT if a
044900000000  preset dictionary is needed at this point, Z_DATA_ERROR if the input data was
045000000000  corrupted (input stream not conforming to the zlib format or incorrect check
045100000000  value), Z_STREAM_ERROR if the stream structure was inconsistent (for example
045200000000  if next_in or next_out was NULL), Z_MEM_ERROR if there was not enough memory,
045300000000  Z_BUF_ERROR if no progress is possible or if there was not enough room in the
045400000000  output buffer when Z_FINISH is used. Note that Z_BUF_ERROR is not fatal, and
045500000000  inflate() can be called again with more input and more output space to
045600000000  continue decompressing. If Z_DATA_ERROR is returned, the application may then
045700000000  call inflateSync() to look for a good compression block if a partial recovery
045800000000  of the data is desired.
045900000000*/
046000000000
046100000000
046200000000ZEXTERN int ZEXPORT inflateEnd OF((z_streamp strm));
046300000000/*
046400000000     All dynamically allocated data structures for this stream are freed.
046500000000   This function discards any unprocessed input and does not flush any
046600000000   pending output.
046700000000
046800000000     inflateEnd returns Z_OK if success, Z_STREAM_ERROR if the stream state
046900000000   was inconsistent. In the error case, msg may be set but then points to a
047000000000   static string (which must not be deallocated).
047100000000*/
047200000000
047300000000                        /* Advanced functions */
047400000000
047500000000/*
047600000000    The following functions are needed only in some special applications.
047700000000*/
047800000000
047900000000/*
048000000000ZEXTERN int ZEXPORT deflateInit2 OF((z_streamp strm,
048100000000                                     int  level,
048200000000                                     int  method,
048300000000                                     int  windowBits,
048400000000                                     int  memLevel,
048500000000                                     int  strategy));
048600000000
048700000000     This is another version of deflateInit with more compression options. The
048800000000   fields next_in, zalloc, zfree and opaque must be initialized before by
048900000000   the caller.
049000000000
049100000000     The method parameter is the compression method. It must be Z_DEFLATED in
049200000000   this version of the library.
049300000000
049400000000     The windowBits parameter is the base two logarithm of the window size
049500000000   (the size of the history buffer). It should be in the range 8..15 for this
049600000000   version of the library. Larger values of this parameter result in better
049700000000   compression at the expense of memory usage. The default value is 15 if
049800000000   deflateInit is used instead.
049900000000
050000000000     windowBits can also be -8..-15 for raw deflate. In this case, -windowBits
050100000000   determines the window size. deflate() will then generate raw deflate data
050200000000   with no zlib header or trailer, and will not compute an adler32 check value.
050300000000
050400000000     windowBits can also be greater than 15 for optional gzip encoding. Add
050500000000   16 to windowBits to write a simple gzip header and trailer around the
050600000000   compressed data instead of a zlib wrapper. The gzip header will have no
050700000000   file name, no extra data, no comment, no modification time (set to zero),
050800000000   no header crc, and the operating system will be set to 255 (unknown).  If a
050900000000   gzip stream is being written, strm->adler is a crc32 instead of an adler32.
051000000000
051100000000     The memLevel parameter specifies how much memory should be allocated
051200000000   for the internal compression state. memLevel=1 uses minimum memory but
051300000000   is slow and reduces compression ratio; memLevel=9 uses maximum memory
051400000000   for optimal speed. The default value is 8. See zconf.h for total memory
051500000000   usage as a function of windowBits and memLevel.
051600000000
051700000000     The strategy parameter is used to tune the compression algorithm. Use the
051800000000   value Z_DEFAULT_STRATEGY for normal data, Z_FILTERED for data produced by a
051900000000   filter (or predictor), Z_HUFFMAN_ONLY to force Huffman encoding only (no
052000000000   string match), or Z_RLE to limit match distances to one (run-length
052100000000   encoding). Filtered data consists mostly of small values with a somewhat
052200000000   random distribution. In this case, the compression algorithm is tuned to
052300000000   compress them better. The effect of Z_FILTERED is to force more Huffman
052400000000   coding and less string matching; it is somewhat intermediate between
052500000000   Z_DEFAULT and Z_HUFFMAN_ONLY. Z_RLE is designed to be almost as fast as
052600000000   Z_HUFFMAN_ONLY, but give better compression for PNG image data. The strategy
052700000000   parameter only affects the compression ratio but not the correctness of the
052800000000   compressed output even if it is not set appropriately.  Z_FIXED prevents the
052900000000   use of dynamic Huffman codes, allowing for a simpler decoder for special
053000000000   applications.
053100000000
053200000000      deflateInit2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
053300000000   memory, Z_STREAM_ERROR if a parameter is invalid (such as an invalid
053400000000   method). msg is set to null if there is no error message.  deflateInit2 does
053500000000   not perform any compression: this will be done by deflate().
053600000000*/
053700000000
053800000000ZEXTERN int ZEXPORT deflateSetDictionary OF((z_streamp strm,
053900000000                                             const Bytef *dictionary,
054000000000                                             uInt  dictLength));
054100000000/*
054200000000     Initializes the compression dictionary from the given byte sequence
054300000000   without producing any compressed output. This function must be called
054400000000   immediately after deflateInit, deflateInit2 or deflateReset, before any
054500000000   call of deflate. The compressor and decompressor must use exactly the same
054600000000   dictionary (see inflateSetDictionary).
054700000000
054800000000     The dictionary should consist of strings (byte sequences) that are likely
054900000000   to be encountered later in the data to be compressed, with the most commonly
055000000000   used strings preferably put towards the end of the dictionary. Using a
055100000000   dictionary is most useful when the data to be compressed is short and can be
055200000000   predicted with good accuracy; the data can then be compressed better than
055300000000   with the default empty dictionary.
055400000000
055500000000     Depending on the size of the compression data structures selected by
055600000000   deflateInit or deflateInit2, a part of the dictionary may in effect be
055700000000   discarded, for example if the dictionary is larger than the window size in
055800000000   deflate or deflate2. Thus the strings most likely to be useful should be
055900000000   put at the end of the dictionary, not at the front. In addition, the
056000000000   current implementation of deflate will use at most the window size minus
056100000000   262 bytes of the provided dictionary.
056200000000
056300000000     Upon return of this function, strm->adler is set to the adler32 value
056400000000   of the dictionary; the decompressor may later use this value to determine
056500000000   which dictionary has been used by the compressor. (The adler32 value
056600000000   applies to the whole dictionary even if only a subset of the dictionary is
056700000000   actually used by the compressor.) If a raw deflate was requested, then the
056800000000   adler32 value is not computed and strm->adler is not set.
056900000000
057000000000     deflateSetDictionary returns Z_OK if success, or Z_STREAM_ERROR if a
057100000000   parameter is invalid (such as NULL dictionary) or the stream state is
057200000000   inconsistent (for example if deflate has already been called for this stream
057300000000   or if the compression method is bsort). deflateSetDictionary does not
057400000000   perform any compression: this will be done by deflate().
057500000000*/
057600000000
057700000000ZEXTERN int ZEXPORT deflateCopy OF((z_streamp dest,
057800000000                                    z_streamp source));
057900000000/*
058000000000     Sets the destination stream as a complete copy of the source stream.
058100000000
058200000000     This function can be useful when several compression strategies will be
058300000000   tried, for example when there are several ways of pre-processing the input
058400000000   data with a filter. The streams that will be discarded should then be freed
058500000000   by calling deflateEnd.  Note that deflateCopy duplicates the internal
058600000000   compression state which can be quite large, so this strategy is slow and
058700000000   can consume lots of memory.
058800000000
058900000000     deflateCopy returns Z_OK if success, Z_MEM_ERROR if there was not
059000000000   enough memory, Z_STREAM_ERROR if the source stream state was inconsistent
059100000000   (such as zalloc being NULL). msg is left unchanged in both source and
059200000000   destination.
059300000000*/
059400000000
059500000000ZEXTERN int ZEXPORT deflateReset OF((z_streamp strm));
059600000000/*
059700000000     This function is equivalent to deflateEnd followed by deflateInit,
059800000000   but does not free and reallocate all the internal compression state.
059900000000   The stream will keep the same compression level and any other attributes
060000000000   that may have been set by deflateInit2.
060100000000
060200000000      deflateReset returns Z_OK if success, or Z_STREAM_ERROR if the source
060300000000   stream state was inconsistent (such as zalloc or state being NULL).
060400000000*/
060500000000
060600000000ZEXTERN int ZEXPORT deflateParams OF((z_streamp strm,
060700000000                                      int level,
060800000000                                      int strategy));
060900000000/*
061000000000     Dynamically update the compression level and compression strategy.  The
061100000000   interpretation of level and strategy is as in deflateInit2.  This can be
061200000000   used to switch between compression and straight copy of the input data, or
061300000000   to switch to a different kind of input data requiring a different
061400000000   strategy. If the compression level is changed, the input available so far
061500000000   is compressed with the old level (and may be flushed); the new level will
061600000000   take effect only at the next call of deflate().
061700000000
061800000000     Before the call of deflateParams, the stream state must be set as for
061900000000   a call of deflate(), since the currently available input may have to
062000000000   be compressed and flushed. In particular, strm->avail_out must be non-zero.
062100000000
062200000000     deflateParams returns Z_OK if success, Z_STREAM_ERROR if the source
062300000000   stream state was inconsistent or if a parameter was invalid, Z_BUF_ERROR
062400000000   if strm->avail_out was zero.
062500000000*/
062600000000
062700000000ZEXTERN int ZEXPORT deflateTune OF((z_streamp strm,
062800000000                                    int good_length,
062900000000                                    int max_lazy,
063000000000                                    int nice_length,
063100000000                                    int max_chain));
063200000000/*
063300000000     Fine tune deflate's internal compression parameters.  This should only be
063400000000   used by someone who understands the algorithm used by zlib's deflate for
063500000000   searching for the best matching string, and even then only by the most
063600000000   fanatic optimizer trying to squeeze out the last compressed bit for their
063700000000   specific input data.  Read the deflate.c source code for the meaning of the
063800000000   max_lazy, good_length, nice_length, and max_chain parameters.
063900000000
064000000000     deflateTune() can be called after deflateInit() or deflateInit2(), and
064100000000   returns Z_OK on success, or Z_STREAM_ERROR for an invalid deflate stream.
064200000000 */
064300000000
064400000000ZEXTERN uLong ZEXPORT deflateBound OF((z_streamp strm,
064500000000                                       uLong sourceLen));
064600000000/*
064700000000     deflateBound() returns an upper bound on the compressed size after
064800000000   deflation of sourceLen bytes.  It must be called after deflateInit()
064900000000   or deflateInit2().  This would be used to allocate an output buffer
065000000000   for deflation in a single pass, and so would be called before deflate().
065100000000*/
065200000000
065300000000ZEXTERN int ZEXPORT deflatePrime OF((z_streamp strm,
065400000000                                     int bits,
065500000000                                     int value));
065600000000/*
065700000000     deflatePrime() inserts bits in the deflate output stream.  The intent
065800000000  is that this function is used to start off the deflate output with the
065900000000  bits leftover from a previous deflate stream when appending to it.  As such,
066000000000  this function can only be used for raw deflate, and must be used before the
066100000000  first deflate() call after a deflateInit2() or deflateReset().  bits must be
066200000000  less than or equal to 16, and that many of the least significant bits of
066300000000  value will be inserted in the output.
066400000000
066500000000      deflatePrime returns Z_OK if success, or Z_STREAM_ERROR if the source
066600000000   stream state was inconsistent.
066700000000*/
066800000000
066900000000ZEXTERN int ZEXPORT deflateSetHeader OF((z_streamp strm,
067000000000                                         gz_headerp head));
067100000000/*
067200000000      deflateSetHeader() provides gzip header information for when a gzip
067300000000   stream is requested by deflateInit2().  deflateSetHeader() may be called
067400000000   after deflateInit2() or deflateReset() and before the first call of
067500000000   deflate().  The text, time, os, extra field, name, and comment information
067600000000   in the provided gz_header structure are written to the gzip header (xflag is
067700000000   ignored -- the extra flags are set according to the compression level).  The
067800000000   caller must assure that, if not Z_NULL, name and comment are terminated with
067900000000   a zero byte, and that if extra is not Z_NULL, that extra_len bytes are
068000000000   available there.  If hcrc is true, a gzip header crc is included.  Note that
068100000000   the current versions of the command-line version of gzip (up through version
068200000000   1.3.x) do not support header crc's, and will report that it is a "multi-part
068300000000   gzip file" and give up.
068400000000
068500000000      If deflateSetHeader is not used, the default gzip header has text false,
068600000000   the time set to zero, and os set to 255, with no extra, name, or comment
068700000000   fields.  The gzip header is returned to the default state by deflateReset().
068800000000
068900000000      deflateSetHeader returns Z_OK if success, or Z_STREAM_ERROR if the source
069000000000   stream state was inconsistent.
069100000000*/
069200000000
069300000000/*
069400000000ZEXTERN int ZEXPORT inflateInit2 OF((z_streamp strm,
069500000000                                     int  windowBits));
069600000000
069700000000     This is another version of inflateInit with an extra parameter. The
069800000000   fields next_in, avail_in, zalloc, zfree and opaque must be initialized
069900000000   before by the caller.
070000000000
070100000000     The windowBits parameter is the base two logarithm of the maximum window
070200000000   size (the size of the history buffer).  It should be in the range 8..15 for
070300000000   this version of the library. The default value is 15 if inflateInit is used
070400000000   instead. windowBits must be greater than or equal to the windowBits value
070500000000   provided to deflateInit2() while compressing, or it must be equal to 15 if
070600000000   deflateInit2() was not used. If a compressed stream with a larger window
070700000000   size is given as input, inflate() will return with the error code
070800000000   Z_DATA_ERROR instead of trying to allocate a larger window.
070900000000
071000000000     windowBits can also be -8..-15 for raw inflate. In this case, -windowBits
071100000000   determines the window size. inflate() will then process raw deflate data,
071200000000   not looking for a zlib or gzip header, not generating a check value, and not
071300000000   looking for any check values for comparison at the end of the stream. This
071400000000   is for use with other formats that use the deflate compressed data format
071500000000   such as zip.  Those formats provide their own check values. If a custom
071600000000   format is developed using the raw deflate format for compressed data, it is
071700000000   recommended that a check value such as an adler32 or a crc32 be applied to
071800000000   the uncompressed data as is done in the zlib, gzip, and zip formats.  For
071900000000   most applications, the zlib format should be used as is. Note that comments
072000000000   above on the use in deflateInit2() applies to the magnitude of windowBits.
072100000000
072200000000     windowBits can also be greater than 15 for optional gzip decoding. Add
072300000000   32 to windowBits to enable zlib and gzip decoding with automatic header
072400000000   detection, or add 16 to decode only the gzip format (the zlib format will
072500000000   return a Z_DATA_ERROR).  If a gzip stream is being decoded, strm->adler is
072600000000   a crc32 instead of an adler32.
072700000000
072800000000     inflateInit2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
072900000000   memory, Z_STREAM_ERROR if a parameter is invalid (such as a null strm). msg
073000000000   is set to null if there is no error message.  inflateInit2 does not perform
073100000000   any decompression apart from reading the zlib header if present: this will
073200000000   be done by inflate(). (So next_in and avail_in may be modified, but next_out
073300000000   and avail_out are unchanged.)
073400000000*/
073500000000
073600000000ZEXTERN int ZEXPORT inflateSetDictionary OF((z_streamp strm,
073700000000                                             const Bytef *dictionary,
073800000000                                             uInt  dictLength));
073900000000/*
074000000000     Initializes the decompression dictionary from the given uncompressed byte
074100000000   sequence. This function must be called immediately after a call of inflate,
074200000000   if that call returned Z_NEED_DICT. The dictionary chosen by the compressor
074300000000   can be determined from the adler32 value returned by that call of inflate.
074400000000   The compressor and decompressor must use exactly the same dictionary (see
074500000000   deflateSetDictionary).  For raw inflate, this function can be called
074600000000   immediately after inflateInit2() or inflateReset() and before any call of
074700000000   inflate() to set the dictionary.  The application must insure that the
074800000000   dictionary that was used for compression is provided.
074900000000
075000000000     inflateSetDictionary returns Z_OK if success, Z_STREAM_ERROR if a
075100000000   parameter is invalid (such as NULL dictionary) or the stream state is
075200000000   inconsistent, Z_DATA_ERROR if the given dictionary doesn't match the
075300000000   expected one (incorrect adler32 value). inflateSetDictionary does not
075400000000   perform any decompression: this will be done by subsequent calls of
075500000000   inflate().
075600000000*/
075700000000
075800000000ZEXTERN int ZEXPORT inflateSync OF((z_streamp strm));
075900000000/*
076000000000    Skips invalid compressed data until a full flush point (see above the
076100000000  description of deflate with Z_FULL_FLUSH) can be found, or until all
076200000000  available input is skipped. No output is provided.
076300000000
076400000000    inflateSync returns Z_OK if a full flush point has been found, Z_BUF_ERROR
076500000000  if no more input was provided, Z_DATA_ERROR if no flush point has been found,
076600000000  or Z_STREAM_ERROR if the stream structure was inconsistent. In the success
076700000000  case, the application may save the current current value of total_in which
076800000000  indicates where valid compressed data was found. In the error case, the
076900000000  application may repeatedly call inflateSync, providing more input each time,
077000000000  until success or end of the input data.
077100000000*/
077200000000
077300000000ZEXTERN int ZEXPORT inflateCopy OF((z_streamp dest,
077400000000                                    z_streamp source));
077500000000/*
077600000000     Sets the destination stream as a complete copy of the source stream.
077700000000
077800000000     This function can be useful when randomly accessing a large stream.  The
077900000000   first pass through the stream can periodically record the inflate state,
078000000000   allowing restarting inflate at those points when randomly accessing the
078100000000   stream.
078200000000
078300000000     inflateCopy returns Z_OK if success, Z_MEM_ERROR if there was not
078400000000   enough memory, Z_STREAM_ERROR if the source stream state was inconsistent
078500000000   (such as zalloc being NULL). msg is left unchanged in both source and
078600000000   destination.
078700000000*/
078800000000
078900000000ZEXTERN int ZEXPORT inflateReset OF((z_streamp strm));
079000000000/*
079100000000     This function is equivalent to inflateEnd followed by inflateInit,
079200000000   but does not free and reallocate all the internal decompression state.
079300000000   The stream will keep attributes that may have been set by inflateInit2.
079400000000
079500000000      inflateReset returns Z_OK if success, or Z_STREAM_ERROR if the source
079600000000   stream state was inconsistent (such as zalloc or state being NULL).
079700000000*/
079800000000
079900000000ZEXTERN int ZEXPORT inflatePrime OF((z_streamp strm,
080000000000                                     int bits,
080100000000                                     int value));
080200000000/*
080300000000     This function inserts bits in the inflate input stream.  The intent is
080400000000  that this function is used to start inflating at a bit position in the
080500000000  middle of a byte.  The provided bits will be used before any bytes are used
080600000000  from next_in.  This function should only be used with raw inflate, and
080700000000  should be used before the first inflate() call after inflateInit2() or
080800000000  inflateReset().  bits must be less than or equal to 16, and that many of the
080900000000  least significant bits of value will be inserted in the input.
081000000000
081100000000      inflatePrime returns Z_OK if success, or Z_STREAM_ERROR if the source
081200000000   stream state was inconsistent.
081300000000*/
081400000000
081500000000ZEXTERN int ZEXPORT inflateGetHeader OF((z_streamp strm,
081600000000                                         gz_headerp head));
081700000000/*
081800000000      inflateGetHeader() requests that gzip header information be stored in the
081900000000   provided gz_header structure.  inflateGetHeader() may be called after
082000000000   inflateInit2() or inflateReset(), and before the first call of inflate().
082100000000   As inflate() processes the gzip stream, head->done is zero until the header
082200000000   is completed, at which time head->done is set to one.  If a zlib stream is
082300000000   being decoded, then head->done is set to -1 to indicate that there will be
082400000000   no gzip header information forthcoming.  Note that Z_BLOCK can be used to
082500000000   force inflate() to return immediately after header processing is complete
082600000000   and before any actual data is decompressed.
082700000000
082800000000      The text, time, xflags, and os fields are filled in with the gzip header
082900000000   contents.  hcrc is set to true if there is a header CRC.  (The header CRC
083000000000   was valid if done is set to one.)  If extra is not Z_NULL, then extra_max
083100000000   contains the maximum number of bytes to write to extra.  Once done is true,
083200000000   extra_len contains the actual extra field length, and extra contains the
083300000000   extra field, or that field truncated if extra_max is less than extra_len.
083400000000   If name is not Z_NULL, then up to name_max characters are written there,
083500000000   terminated with a zero unless the length is greater than name_max.  If
083600000000   comment is not Z_NULL, then up to comm_max characters are written there,
083700000000   terminated with a zero unless the length is greater than comm_max.  When
083800000000   any of extra, name, or comment are not Z_NULL and the respective field is
083900000000   not present in the header, then that field is set to Z_NULL to signal its
084000000000   absence.  This allows the use of deflateSetHeader() with the returned
084100000000   structure to duplicate the header.  However if those fields are set to
084200000000   allocated memory, then the application will need to save those pointers
084300000000   elsewhere so that they can be eventually freed.
084400000000
084500000000      If inflateGetHeader is not used, then the header information is simply
084600000000   discarded.  The header is always checked for validity, including the header
084700000000   CRC if present.  inflateReset() will reset the process to discard the header
084800000000   information.  The application would need to call inflateGetHeader() again to
084900000000   retrieve the header from the next gzip stream.
085000000000
085100000000      inflateGetHeader returns Z_OK if success, or Z_STREAM_ERROR if the source
085200000000   stream state was inconsistent.
085300000000*/
085400000000
085500000000/*
085600000000ZEXTERN int ZEXPORT inflateBackInit OF((z_streamp strm, int windowBits,
085700000000                                        unsigned char FAR *window));
085800000000
085900000000     Initialize the internal stream state for decompression using inflateBack()
086000000000   calls.  The fields zalloc, zfree and opaque in strm must be initialized
086100000000   before the call.  If zalloc and zfree are Z_NULL, then the default library-
086200000000   derived memory allocation routines are used.  windowBits is the base two
086300000000   logarithm of the window size, in the range 8..15.  window is a caller
086400000000   supplied buffer of that size.  Except for special applications where it is
086500000000   assured that deflate was used with small window sizes, windowBits must be 15
086600000000   and a 32K byte window must be supplied to be able to decompress general
086700000000   deflate streams.
086800000000
086900000000     See inflateBack() for the usage of these routines.
087000000000
087100000000     inflateBackInit will return Z_OK on success, Z_STREAM_ERROR if any of
087200000000   the paramaters are invalid, Z_MEM_ERROR if the internal state could not
087300000000   be allocated, or Z_VERSION_ERROR if the version of the library does not
087400000000   match the version of the header file.
087500000000*/
087600000000
087700000000typedef unsigned (*in_func) OF((void FAR *, unsigned char FAR * FAR *));
087800000000typedef int (*out_func) OF((void FAR *, unsigned char FAR *, unsigned));
087900000000
088000000000ZEXTERN int ZEXPORT inflateBack OF((z_streamp strm,
088100000000                                    in_func in, void FAR *in_desc,
088200000000                                    out_func out, void FAR *out_desc));
088300000000/*
088400000000     inflateBack() does a raw inflate with a single call using a call-back
088500000000   interface for input and output.  This is more efficient than inflate() for
088600000000   file i/o applications in that it avoids copying between the output and the
088700000000   sliding window by simply making the window itself the output buffer.  This
088800000000   function trusts the application to not change the output buffer passed by
088900000000   the output function, at least until inflateBack() returns.
089000000000
089100000000     inflateBackInit() must be called first to allocate the internal state
089200000000   and to initialize the state with the user-provided window buffer.
089300000000   inflateBack() may then be used multiple times to inflate a complete, raw
089400000000   deflate stream with each call.  inflateBackEnd() is then called to free
089500000000   the allocated state.
089600000000
089700000000     A raw deflate stream is one with no zlib or gzip header or trailer.
089800000000   This routine would normally be used in a utility that reads zip or gzip
089900000000   files and writes out uncompressed files.  The utility would decode the
090000000000   header and process the trailer on its own, hence this routine expects
090100000000   only the raw deflate stream to decompress.  This is different from the
090200000000   normal behavior of inflate(), which expects either a zlib or gzip header and
090300000000   trailer around the deflate stream.
090400000000
090500000000     inflateBack() uses two subroutines supplied by the caller that are then
090600000000   called by inflateBack() for input and output.  inflateBack() calls those
090700000000   routines until it reads a complete deflate stream and writes out all of the
090800000000   uncompressed data, or until it encounters an error.  The function's
090900000000   parameters and return types are defined above in the in_func and out_func
091000000000   typedefs.  inflateBack() will call in(in_desc, &buf) which should return the
091100000000   number of bytes of provided input, and a pointer to that input in buf.  If
091200000000   there is no input available, in() must return zero--buf is ignored in that
091300000000   case--and inflateBack() will return a buffer error.  inflateBack() will call
091400000000   out(out_desc, buf, len) to write the uncompressed data buf[0..len-1].  out()
091500000000   should return zero on success, or non-zero on failure.  If out() returns
091600000000   non-zero, inflateBack() will return with an error.  Neither in() nor out()
091700000000   are permitted to change the contents of the window provided to
091800000000   inflateBackInit(), which is also the buffer that out() uses to write from.
091900000000   The length written by out() will be at most the window size.  Any non-zero
092000000000   amount of input may be provided by in().
092100000000
092200000000     For convenience, inflateBack() can be provided input on the first call by
092300000000   setting strm->next_in and strm->avail_in.  If that input is exhausted, then
092400000000   in() will be called.  Therefore strm->next_in must be initialized before
092500000000   calling inflateBack().  If strm->next_in is Z_NULL, then in() will be called
092600000000   immediately for input.  If strm->next_in is not Z_NULL, then strm->avail_in
092700000000   must also be initialized, and then if strm->avail_in is not zero, input will
092800000000   initially be taken from strm->next_in[0 .. strm->avail_in - 1].
092900000000
093000000000     The in_desc and out_desc parameters of inflateBack() is passed as the
093100000000   first parameter of in() and out() respectively when they are called.  These
093200000000   descriptors can be optionally used to pass any information that the caller-
093300000000   supplied in() and out() functions need to do their job.
093400000000
093500000000     On return, inflateBack() will set strm->next_in and strm->avail_in to
093600000000   pass back any unused input that was provided by the last in() call.  The
093700000000   return values of inflateBack() can be Z_STREAM_END on success, Z_BUF_ERROR
093800000000   if in() or out() returned an error, Z_DATA_ERROR if there was a format
093900000000   error in the deflate stream (in which case strm->msg is set to indicate the
094000000000   nature of the error), or Z_STREAM_ERROR if the stream was not properly
094100000000   initialized.  In the case of Z_BUF_ERROR, an input or output error can be
094200000000   distinguished using strm->next_in which will be Z_NULL only if in() returned
094300000000   an error.  If strm->next is not Z_NULL, then the Z_BUF_ERROR was due to
094400000000   out() returning non-zero.  (in() will always be called before out(), so
094500000000   strm->next_in is assured to be defined if out() returns non-zero.)  Note
094600000000   that inflateBack() cannot return Z_OK.
094700000000*/
094800000000
094900000000ZEXTERN int ZEXPORT inflateBackEnd OF((z_streamp strm));
095000000000/*
095100000000     All memory allocated by inflateBackInit() is freed.
095200000000
095300000000     inflateBackEnd() returns Z_OK on success, or Z_STREAM_ERROR if the stream
095400000000   state was inconsistent.
095500000000*/
095600000000
095700000000ZEXTERN uLong ZEXPORT zlibCompileFlags OF((void));
095800000000/* Return flags indicating compile-time options.
095900000000
096000000000    Type sizes, two bits each, 00 = 16 bits, 01 = 32, 10 = 64, 11 = other:
096100000000     1.0: size of uInt
096200000000     3.2: size of uLong
096300000000     5.4: size of voidpf (pointer)
096400000000     7.6: size of z_off_t
096500000000
096600000000    Compiler, assembler, and debug options:
096700000000     8: DEBUG
096800000000     9: ASMV or ASMINF -- use ASM code
096900000000     10: ZLIB_WINAPI -- exported functions use the WINAPI calling convention
097000000000     11: 0 (reserved)
097100000000
097200000000    One-time table building (smaller code, but not thread-safe if true):
097300000000     12: BUILDFIXED -- build static block decoding tables when needed
097400000000     13: DYNAMIC_CRC_TABLE -- build CRC calculation tables when needed
097500000000     14,15: 0 (reserved)
097600000000
097700000000    Library content (indicates missing functionality):
097800000000     16: NO_GZCOMPRESS -- gz* functions cannot compress (to avoid linking
097900000000                          deflate code when not needed)
098000000000     17: NO_GZIP -- deflate can't write gzip streams, and inflate can't detect
098100000000                    and decode gzip streams (to avoid linking crc code)
098200000000     18-19: 0 (reserved)
098300000000
098400000000    Operation variations (changes in library functionality):
098500000000     20: PKZIP_BUG_WORKAROUND -- slightly more permissive inflate
098600000000     21: FASTEST -- deflate algorithm with only one, lowest compression level
098700000000     22,23: 0 (reserved)
098800000000
098900000000    The sprintf variant used by gzprintf (zero is best):
099000000000     24: 0 = vs*, 1 = s* -- 1 means limited to 20 arguments after the format
099100000000     25: 0 = *nprintf, 1 = *printf -- 1 means gzprintf() not secure!
099200000000     26: 0 = returns value, 1 = void -- 1 means inferred string length returned
099300000000
099400000000    Remainder:
099500000000     27-31: 0 (reserved)
099600000000 */
099700000000
099800000000
099900000000                        /* utility functions */
100000000000
100100000000/*
100200000000     The following utility functions are implemented on top of the
100300000000   basic stream-oriented functions. To simplify the interface, some
100400000000   default options are assumed (compression level and memory usage,
100500000000   standard memory allocation functions). The source code of these
100600000000   utility functions can easily be modified if you need special options.
100700000000*/
100800000000
100900000000ZEXTERN int ZEXPORT compress OF((Bytef *dest,   uLongf *destLen,
101000000000                                 const Bytef *source, uLong sourceLen));
101100000000/*
101200000000     Compresses the source buffer into the destination buffer.  sourceLen is
101300000000   the byte length of the source buffer. Upon entry, destLen is the total
101400000000   size of the destination buffer, which must be at least the value returned
101500000000   by compressBound(sourceLen). Upon exit, destLen is the actual size of the
101600000000   compressed buffer.
101700000000     This function can be used to compress a whole file at once if the
101800000000   input file is mmap'ed.
101900000000     compress returns Z_OK if success, Z_MEM_ERROR if there was not
102000000000   enough memory, Z_BUF_ERROR if there was not enough room in the output
102100000000   buffer.
102200000000*/
102300000000
102400000000ZEXTERN int ZEXPORT compress2 OF((Bytef *dest,   uLongf *destLen,
102500000000                                  const Bytef *source, uLong sourceLen,
102600000000                                  int level));
102700000000/*
102800000000     Compresses the source buffer into the destination buffer. The level
102900000000   parameter has the same meaning as in deflateInit.  sourceLen is the byte
103000000000   length of the source buffer. Upon entry, destLen is the total size of the
103100000000   destination buffer, which must be at least the value returned by
103200000000   compressBound(sourceLen). Upon exit, destLen is the actual size of the
103300000000   compressed buffer.
103400000000
103500000000     compress2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
103600000000   memory, Z_BUF_ERROR if there was not enough room in the output buffer,
103700000000   Z_STREAM_ERROR if the level parameter is invalid.
103800000000*/
103900000000
104000000000ZEXTERN uLong ZEXPORT compressBound OF((uLong sourceLen));
104100000000/*
104200000000     compressBound() returns an upper bound on the compressed size after
104300000000   compress() or compress2() on sourceLen bytes.  It would be used before
104400000000   a compress() or compress2() call to allocate the destination buffer.
104500000000*/
104600000000
104700000000ZEXTERN int ZEXPORT uncompress OF((Bytef *dest,   uLongf *destLen,
104800000000                                   const Bytef *source, uLong sourceLen));
104900000000/*
105000000000     Decompresses the source buffer into the destination buffer.  sourceLen is
105100000000   the byte length of the source buffer. Upon entry, destLen is the total
105200000000   size of the destination buffer, which must be large enough to hold the
105300000000   entire uncompressed data. (The size of the uncompressed data must have
105400000000   been saved previously by the compressor and transmitted to the decompressor
105500000000   by some mechanism outside the scope of this compression library.)
105600000000   Upon exit, destLen is the actual size of the compressed buffer.
105700000000     This function can be used to decompress a whole file at once if the
105800000000   input file is mmap'ed.
105900000000
106000000000     uncompress returns Z_OK if success, Z_MEM_ERROR if there was not
106100000000   enough memory, Z_BUF_ERROR if there was not enough room in the output
106200000000   buffer, or Z_DATA_ERROR if the input data was corrupted or incomplete.
106300000000*/
106400000000
106500000000
106600000000typedef voidp gzFile;
106700000000
106800000000ZEXTERN gzFile ZEXPORT gzopen  OF((const char *path, const char *mode));
106900000000/*
107000000000     Opens a gzip (.gz) file for reading or writing. The mode parameter
107100000000   is as in fopen ("rb" or "wb") but can also include a compression level
107200000000   ("wb9") or a strategy: 'f' for filtered data as in "wb6f", 'h' for
107300000000   Huffman only compression as in "wb1h", or 'R' for run-length encoding
107400000000   as in "wb1R". (See the description of deflateInit2 for more information
107500000000   about the strategy parameter.)
107600000000
107700000000     gzopen can be used to read a file which is not in gzip format; in this
107800000000   case gzread will directly read from the file without decompression.
107900000000
108000000000     gzopen returns NULL if the file could not be opened or if there was
108100000000   insufficient memory to allocate the (de)compression state; errno
108200000000   can be checked to distinguish the two cases (if errno is zero, the
108300000000   zlib error is Z_MEM_ERROR).  */
108400000000
108500000000ZEXTERN gzFile ZEXPORT gzdopen  OF((int fd, const char *mode));
108600000000/*
108700000000     gzdopen() associates a gzFile with the file descriptor fd.  File
108800000000   descriptors are obtained from calls like open, dup, creat, pipe or
108900000000   fileno (in the file has been previously opened with fopen).
109000000000   The mode parameter is as in gzopen.
109100000000     The next call of gzclose on the returned gzFile will also close the
109200000000   file descriptor fd, just like fclose(fdopen(fd), mode) closes the file
109300000000   descriptor fd. If you want to keep fd open, use gzdopen(dup(fd), mode).
109400000000     gzdopen returns NULL if there was insufficient memory to allocate
109500000000   the (de)compression state.
109600000000*/
109700000000
109800000000ZEXTERN int ZEXPORT gzsetparams OF((gzFile file, int level, int strategy));
109900000000/*
110000000000     Dynamically update the compression level or strategy. See the description
110100000000   of deflateInit2 for the meaning of these parameters.
110200000000     gzsetparams returns Z_OK if success, or Z_STREAM_ERROR if the file was not
110300000000   opened for writing.
110400000000*/
110500000000
110600000000ZEXTERN int ZEXPORT    gzread  OF((gzFile file, voidp buf, unsigned len));
110700000000/*
110800000000     Reads the given number of uncompressed bytes from the compressed file.
110900000000   If the input file was not in gzip format, gzread copies the given number
111000000000   of bytes into the buffer.
111100000000     gzread returns the number of uncompressed bytes actually read (0 for
111200000000   end of file, -1 for error). */
111300000000
111400000000ZEXTERN int ZEXPORT    gzwrite OF((gzFile file,
111500000000                                   voidpc buf, unsigned len));
111600000000/*
111700000000     Writes the given number of uncompressed bytes into the compressed file.
111800000000   gzwrite returns the number of uncompressed bytes actually written
111900000000   (0 in case of error).
112000000000*/
112100000000
112200000000ZEXTERN int ZEXPORTVA   gzprintf OF((gzFile file, const char *format, ...));
112300000000/*
112400000000     Converts, formats, and writes the args to the compressed file under
112500000000   control of the format string, as in fprintf. gzprintf returns the number of
112600000000   uncompressed bytes actually written (0 in case of error).  The number of
112700000000   uncompressed bytes written is limited to 4095. The caller should assure that
112800000000   this limit is not exceeded. If it is exceeded, then gzprintf() will return
112900000000   return an error (0) with nothing written. In this case, there may also be a
113000000000   buffer overflow with unpredictable consequences, which is possible only if
113100000000   zlib was compiled with the insecure functions sprintf() or vsprintf()
113200000000   because the secure snprintf() or vsnprintf() functions were not available.
113300000000*/
113400000000
113500000000ZEXTERN int ZEXPORT gzputs OF((gzFile file, const char *s));
113600000000/*
113700000000      Writes the given null-terminated string to the compressed file, excluding
113800000000   the terminating null character.
113900000000      gzputs returns the number of characters written, or -1 in case of error.
114000000000*/
114100000000
114200000000ZEXTERN char * ZEXPORT gzgets OF((gzFile file, char *buf, int len));
114300000000/*
114400000000      Reads bytes from the compressed file until len-1 characters are read, or
114500000000   a newline character is read and transferred to buf, or an end-of-file
114600000000   condition is encountered.  The string is then terminated with a null
114700000000   character.
114800000000      gzgets returns buf, or Z_NULL in case of error.
114900000000*/
115000000000
115100000000ZEXTERN int ZEXPORT    gzputc OF((gzFile file, int c));
115200000000/*
115300000000      Writes c, converted to an unsigned char, into the compressed file.
115400000000   gzputc returns the value that was written, or -1 in case of error.
115500000000*/
115600000000
115700000000ZEXTERN int ZEXPORT    gzgetc OF((gzFile file));
115800000000/*
115900000000      Reads one byte from the compressed file. gzgetc returns this byte
116000000000   or -1 in case of end of file or error.
116100000000*/
116200000000
116300000000ZEXTERN int ZEXPORT    gzungetc OF((int c, gzFile file));
116400000000/*
116500000000      Push one character back onto the stream to be read again later.
116600000000   Only one character of push-back is allowed.  gzungetc() returns the
116700000000   character pushed, or -1 on failure.  gzungetc() will fail if a
116800000000   character has been pushed but not read yet, or if c is -1. The pushed
116900000000   character will be discarded if the stream is repositioned with gzseek()
117000000000   or gzrewind().
117100000000*/
117200000000
117300000000ZEXTERN int ZEXPORT    gzflush OF((gzFile file, int flush));
117400000000/*
117500000000     Flushes all pending output into the compressed file. The parameter
117600000000   flush is as in the deflate() function. The return value is the zlib
117700000000   error number (see function gzerror below). gzflush returns Z_OK if
117800000000   the flush parameter is Z_FINISH and all output could be flushed.
117900000000     gzflush should be called only when strictly necessary because it can
118000000000   degrade compression.
118100000000*/
118200000000
118300000000ZEXTERN z_off_t ZEXPORT    gzseek OF((gzFile file,
118400000000                                      z_off_t offset, int whence));
118500000000/*
118600000000      Sets the starting position for the next gzread or gzwrite on the
118700000000   given compressed file. The offset represents a number of bytes in the
118800000000   uncompressed data stream. The whence parameter is defined as in lseek(2);
118900000000   the value SEEK_END is not supported.
119000000000     If the file is opened for reading, this function is emulated but can be
119100000000   extremely slow. If the file is opened for writing, only forward seeks are
119200000000   supported; gzseek then compresses a sequence of zeroes up to the new
119300000000   starting position.
119400000000
119500000000      gzseek returns the resulting offset location as measured in bytes from
119600000000   the beginning of the uncompressed stream, or -1 in case of error, in
119700000000   particular if the file is opened for writing and the new starting position
119800000000   would be before the current position.
119900000000*/
120000000000
120100000000ZEXTERN int ZEXPORT    gzrewind OF((gzFile file));
120200000000/*
120300000000     Rewinds the given file. This function is supported only for reading.
120400000000
120500000000   gzrewind(file) is equivalent to (int)gzseek(file, 0L, SEEK_SET)
120600000000*/
120700000000
120800000000ZEXTERN z_off_t ZEXPORT    gztell OF((gzFile file));
120900000000/*
121000000000     Returns the starting position for the next gzread or gzwrite on the
121100000000   given compressed file. This position represents a number of bytes in the
121200000000   uncompressed data stream.
121300000000
121400000000   gztell(file) is equivalent to gzseek(file, 0L, SEEK_CUR)
121500000000*/
121600000000
121700000000ZEXTERN int ZEXPORT gzeof OF((gzFile file));
121800000000/*
121900000000     Returns 1 when EOF has previously been detected reading the given
122000000000   input stream, otherwise zero.
122100000000*/
122200000000
122300000000ZEXTERN int ZEXPORT gzdirect OF((gzFile file));
122400000000/*
122500000000     Returns 1 if file is being read directly without decompression, otherwise
122600000000   zero.
122700000000*/
122800000000
122900000000ZEXTERN int ZEXPORT    gzclose OF((gzFile file));
123000000000/*
123100000000     Flushes all pending output if necessary, closes the compressed file
123200000000   and deallocates all the (de)compression state. The return value is the zlib
123300000000   error number (see function gzerror below).
123400000000*/
123500000000
123600000000ZEXTERN const char * ZEXPORT gzerror OF((gzFile file, int *errnum));
123700000000/*
123800000000     Returns the error message for the last error which occurred on the
123900000000   given compressed file. errnum is set to zlib error number. If an
124000000000   error occurred in the file system and not in the compression library,
124100000000   errnum is set to Z_ERRNO and the application may consult errno
124200000000   to get the exact error code.
124300000000*/
124400000000
124500000000ZEXTERN void ZEXPORT gzclearerr OF((gzFile file));
124600000000/*
124700000000     Clears the error and end-of-file flags for file. This is analogous to the
124800000000   clearerr() function in stdio. This is useful for continuing to read a gzip
124900000000   file that is being written concurrently.
125000000000*/
125100000000
125200000000                        /* checksum functions */
125300000000
125400000000/*
125500000000     These functions are not related to compression but are exported
125600000000   anyway because they might be useful in applications using the
125700000000   compression library.
125800000000*/
125900000000
126000000000ZEXTERN uLong ZEXPORT adler32 OF((uLong adler, const Bytef *buf, uInt len));
126100000000/*
126200000000     Update a running Adler-32 checksum with the bytes buf[0..len-1] and
126300000000   return the updated checksum. If buf is NULL, this function returns
126400000000   the required initial value for the checksum.
126500000000   An Adler-32 checksum is almost as reliable as a CRC32 but can be computed
126600000000   much faster. Usage example:
126700000000
126800000000     uLong adler = adler32(0L, Z_NULL, 0);
126900000000
127000000000     while (read_buffer(buffer, length) != EOF) {
127100000000       adler = adler32(adler, buffer, length);
127200000000     }
127300000000     if (adler != original_adler) error();
127400000000*/
127500000000
127600000000ZEXTERN uLong ZEXPORT adler32_combine OF((uLong adler1, uLong adler2,
127700000000                                          z_off_t len2));
127800000000/*
127900000000     Combine two Adler-32 checksums into one.  For two sequences of bytes, seq1
128000000000   and seq2 with lengths len1 and len2, Adler-32 checksums were calculated for
128100000000   each, adler1 and adler2.  adler32_combine() returns the Adler-32 checksum of
128200000000   seq1 and seq2 concatenated, requiring only adler1, adler2, and len2.
128300000000*/
128400000000
128500000000ZEXTERN uLong ZEXPORT crc32   OF((uLong crc, const Bytef *buf, uInt len));
128600000000/*
128700000000     Update a running CRC-32 with the bytes buf[0..len-1] and return the
128800000000   updated CRC-32. If buf is NULL, this function returns the required initial
128900000000   value for the for the crc. Pre- and post-conditioning (one's complement) is
129000000000   performed within this function so it shouldn't be done by the application.
129100000000   Usage example:
129200000000
129300000000     uLong crc = crc32(0L, Z_NULL, 0);
129400000000
129500000000     while (read_buffer(buffer, length) != EOF) {
129600000000       crc = crc32(crc, buffer, length);
129700000000     }
129800000000     if (crc != original_crc) error();
129900000000*/
130000000000
130100000000ZEXTERN uLong ZEXPORT crc32_combine OF((uLong crc1, uLong crc2, z_off_t len2));
130200000000
130300000000/*
130400000000     Combine two CRC-32 check values into one.  For two sequences of bytes,
130500000000   seq1 and seq2 with lengths len1 and len2, CRC-32 check values were
130600000000   calculated for each, crc1 and crc2.  crc32_combine() returns the CRC-32
130700000000   check value of seq1 and seq2 concatenated, requiring only crc1, crc2, and
130800000000   len2.
130900000000*/
131000000000
131100000000
131200000000                        /* various hacks, don't look :) */
131300000000
131400000000/* deflateInit and inflateInit are macros to allow checking the zlib version
131500000000 * and the compiler's view of z_stream:
131600000000 */
131700000000ZEXTERN int ZEXPORT deflateInit_ OF((z_streamp strm, int level,
131800000000                                     const char *version, int stream_size));
131900000000ZEXTERN int ZEXPORT inflateInit_ OF((z_streamp strm,
132000000000                                     const char *version, int stream_size));
132100000000ZEXTERN int ZEXPORT deflateInit2_ OF((z_streamp strm, int  level, int  method,
132200000000                                      int windowBits, int memLevel,
132300000000                                      int strategy, const char *version,
132400000000                                      int stream_size));
132500000000ZEXTERN int ZEXPORT inflateInit2_ OF((z_streamp strm, int  windowBits,
132600000000                                      const char *version, int stream_size));
132700000000ZEXTERN int ZEXPORT inflateBackInit_ OF((z_streamp strm, int windowBits,
132800000000                                         unsigned char FAR *window,
132900000000                                         const char *version,
133000000000                                         int stream_size));
133100000000#define deflateInit(strm, level) \
133200000000        deflateInit_((strm), (level),       ZLIB_VERSION, sizeof(z_stream))
133300000000#define inflateInit(strm) \
133400000000        inflateInit_((strm),                ZLIB_VERSION, sizeof(z_stream))
133500000000#define deflateInit2(strm, level, method, windowBits, memLevel, strategy) \
133600000000        deflateInit2_((strm),(level),(method),(windowBits),(memLevel),\
133700000000                      (strategy),           ZLIB_VERSION, sizeof(z_stream))
133800000000#define inflateInit2(strm, windowBits) \
133900000000        inflateInit2_((strm), (windowBits), ZLIB_VERSION, sizeof(z_stream))
134000000000#define inflateBackInit(strm, windowBits, window) \
134100000000        inflateBackInit_((strm), (windowBits), (window), \
134200000000        ZLIB_VERSION, sizeof(z_stream))
134300000000
134400000000
134500000000#if !defined(ZUTIL_H) && !defined(NO_DUMMY_DECL)
134600000000    struct internal_state {int dummy;}; /* hack for buggy compilers */
134700000000#endif
134800000000
134900000000ZEXTERN const char   * ZEXPORT zError           OF((int));
135000000000ZEXTERN int            ZEXPORT inflateSyncPoint OF((z_streamp z));
135100000000ZEXTERN const uLongf * ZEXPORT get_crc_table    OF((void));
135200000000
135300000000#ifdef __cplusplus
135400000000}
135500000000#endif
135600000000
135700000000#endif /* ZLIB_H */
