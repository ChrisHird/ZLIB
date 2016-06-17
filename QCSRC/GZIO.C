000100000000/* gzio.c -- IO on .gz files
000200000000 * Copyright (C) 1995-2005 Jean-loup Gailly.
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 *
000500000000 * Compile this file with -DNO_GZCOMPRESS to avoid the compression code.
000600000000 */
000700000000
000800000000/* @(#) $Id$ */
000900000000
001000000000#include <stdio.h>
001100000000
001200000000#include "zutil.h"
001300000000
001400000000#ifdef NO_DEFLATE       /* for compatibility with old definition */
001500000000#  define NO_GZCOMPRESS
001600000000#endif
001700000000
001800000000#ifndef NO_DUMMY_DECL
001900000000struct internal_state {int dummy;}; /* for buggy compilers */
002000000000#endif
002100000000
002200000000#ifndef Z_BUFSIZE
002300000000#  ifdef MAXSEG_64K
002400000000#    define Z_BUFSIZE 4096 /* minimize memory usage for 16-bit DOS */
002500000000#  else
002600000000#    define Z_BUFSIZE 16384
002700000000#  endif
002800000000#endif
002900000000#ifndef Z_PRINTF_BUFSIZE
003000000000#  define Z_PRINTF_BUFSIZE 4096
003100000000#endif
003200000000
003300000000#ifdef __MVS__
003400000000#  pragma map (fdopen , "\174\174FDOPEN")
003500000000   FILE *fdopen(int, const char *);
003600000000#endif
003700000000
003800000000#ifndef STDC
003900000000extern voidp  malloc OF((uInt size));
004000000000extern void   free   OF((voidpf ptr));
004100000000#endif
004200000000
004300000000#define ALLOC(size) malloc(size)
004400000000#define TRYFREE(p) {if (p) free(p);}
004500000000
004600000000static int const gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */
004700000000
004800000000/* gzip flag byte */
004900000000#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
005000000000#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
005100000000#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
005200000000#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
005300000000#define COMMENT      0x10 /* bit 4 set: file comment present */
005400000000#define RESERVED     0xE0 /* bits 5..7: reserved */
005500000000
005600000000typedef struct gz_stream {
005700000000    z_stream stream;
005800000000    int      z_err;   /* error code for last stream operation */
005900000000    int      z_eof;   /* set if end of input file */
006000000000    FILE     *file;   /* .gz file */
006100000000    Byte     *inbuf;  /* input buffer */
006200000000    Byte     *outbuf; /* output buffer */
006300000000    uLong    crc;     /* crc32 of uncompressed data */
006400000000    char     *msg;    /* error message */
006500000000    char     *path;   /* path name for debugging only */
006600000000    int      transparent; /* 1 if input file is not a .gz file */
006700000000    char     mode;    /* 'w' or 'r' */
006800000000    z_off_t  start;   /* start of compressed data in file (header skipped) */
006900000000    z_off_t  in;      /* bytes into deflate or inflate */
007000000000    z_off_t  out;     /* bytes out of deflate or inflate */
007100000000    int      back;    /* one character push-back */
007200000000    int      last;    /* true if push-back is last character */
007300000000} gz_stream;
007400000000
007500000000
007600000000local gzFile gz_open      OF((const char *path, const char *mode, int  fd));
007700000000local int do_flush        OF((gzFile file, int flush));
007800000000local int    get_byte     OF((gz_stream *s));
007900000000local void   check_header OF((gz_stream *s));
008000000000local int    destroy      OF((gz_stream *s));
008100000000local void   putLong      OF((FILE *file, uLong x));
008200000000local uLong  getLong      OF((gz_stream *s));
008300000000
008400000000/* ===========================================================================
008500000000     Opens a gzip (.gz) file for reading or writing. The mode parameter
008600000000   is as in fopen ("rb" or "wb"). The file is given either by file descriptor
008700000000   or path name (if fd == -1).
008800000000     gz_open returns NULL if the file could not be opened or if there was
008900000000   insufficient memory to allocate the (de)compression state; errno
009000000000   can be checked to distinguish the two cases (if errno is zero, the
009100000000   zlib error is Z_MEM_ERROR).
009200000000*/
009300000000local gzFile gz_open (path, mode, fd)
009400000000    const char *path;
009500000000    const char *mode;
009600000000    int  fd;
009700000000{
009800000000    int err;
009900000000    int level = Z_DEFAULT_COMPRESSION; /* compression level */
010000000000    int strategy = Z_DEFAULT_STRATEGY; /* compression strategy */
010100000000    char *p = (char*)mode;
010200000000    gz_stream *s;
010300000000    char fmode[80]; /* copy of mode, without the compression level */
010400000000    char *m = fmode;
010500000000
010600000000    if (!path || !mode) return Z_NULL;
010700000000
010800000000    s = (gz_stream *)ALLOC(sizeof(gz_stream));
010900000000    if (!s) return Z_NULL;
011000000000
011100000000    s->stream.zalloc = (alloc_func)0;
011200000000    s->stream.zfree = (free_func)0;
011300000000    s->stream.opaque = (voidpf)0;
011400000000    s->stream.next_in = s->inbuf = Z_NULL;
011500000000    s->stream.next_out = s->outbuf = Z_NULL;
011600000000    s->stream.avail_in = s->stream.avail_out = 0;
011700000000    s->file = NULL;
011800000000    s->z_err = Z_OK;
011900000000    s->z_eof = 0;
012000000000    s->in = 0;
012100000000    s->out = 0;
012200000000    s->back = EOF;
012300000000    s->crc = crc32(0L, Z_NULL, 0);
012400000000    s->msg = NULL;
012500000000    s->transparent = 0;
012600000000
012700000000    s->path = (char*)ALLOC(strlen(path)+1);
012800000000    if (s->path == NULL) {
012900000000        return destroy(s), (gzFile)Z_NULL;
013000000000    }
013100000000    strcpy(s->path, path); /* do this early for debugging */
013200000000
013300000000    s->mode = '\0';
013400000000    do {
013500000000        if (*p == 'r') s->mode = 'r';
013600000000        if (*p == 'w' || *p == 'a') s->mode = 'w';
013700000000        if (*p >= '0' && *p <= '9') {
013800000000            level = *p - '0';
013900000000        } else if (*p == 'f') {
014000000000          strategy = Z_FILTERED;
014100000000        } else if (*p == 'h') {
014200000000          strategy = Z_HUFFMAN_ONLY;
014300000000        } else if (*p == 'R') {
014400000000          strategy = Z_RLE;
014500000000        } else {
014600000000            *m++ = *p; /* copy the mode */
014700000000        }
014800000000    } while (*p++ && m != fmode + sizeof(fmode));
014900000000    if (s->mode == '\0') return destroy(s), (gzFile)Z_NULL;
015000000000
015100000000    if (s->mode == 'w') {
015200000000#ifdef NO_GZCOMPRESS
015300000000        err = Z_STREAM_ERROR;
015400000000#else
015500000000        err = deflateInit2(&(s->stream), level,
015600000000                           Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, strategy);
015700000000        /* windowBits is passed < 0 to suppress zlib header */
015800000000
015900000000        s->stream.next_out = s->outbuf = (Byte*)ALLOC(Z_BUFSIZE);
016000000000#endif
016100000000        if (err != Z_OK || s->outbuf == Z_NULL) {
016200000000            return destroy(s), (gzFile)Z_NULL;
016300000000        }
016400000000    } else {
016500000000        s->stream.next_in  = s->inbuf = (Byte*)ALLOC(Z_BUFSIZE);
016600000000
016700000000        err = inflateInit2(&(s->stream), -MAX_WBITS);
016800000000        /* windowBits is passed < 0 to tell that there is no zlib header.
016900000000         * Note that in this case inflate *requires* an extra "dummy" byte
017000000000         * after the compressed stream in order to complete decompression and
017100000000         * return Z_STREAM_END. Here the gzip CRC32 ensures that 4 bytes are
017200000000         * present after the compressed stream.
017300000000         */
017400000000        if (err != Z_OK || s->inbuf == Z_NULL) {
017500000000            return destroy(s), (gzFile)Z_NULL;
017600000000        }
017700000000    }
017800000000    s->stream.avail_out = Z_BUFSIZE;
017900000000
018000000000    errno = 0;
018100000000    s->file = fd < 0 ? F_OPEN(path, fmode) : (FILE*)fdopen(fd, fmode);
018200000000
018300000000    if (s->file == NULL) {
018400000000        return destroy(s), (gzFile)Z_NULL;
018500000000    }
018600000000    if (s->mode == 'w') {
018700000000        /* Write a very simple .gz header:
018800000000         */
018900000000        fprintf(s->file, "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
019000000000             Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, OS_CODE);
019100000000        s->start = 10L;
019200000000        /* We use 10L instead of ftell(s->file) to because ftell causes an
019300000000         * fflush on some systems. This version of the library doesn't use
019400000000         * start anyway in write mode, so this initialization is not
019500000000         * necessary.
019600000000         */
019700000000    } else {
019800000000        check_header(s); /* skip the .gz header */
019900000000        s->start = ftell(s->file) - s->stream.avail_in;
020000000000    }
020100000000
020200000000    return (gzFile)s;
020300000000}
020400000000
020500000000/* ===========================================================================
020600000000     Opens a gzip (.gz) file for reading or writing.
020700000000*/
020800000000gzFile ZEXPORT gzopen (path, mode)
020900000000    const char *path;
021000000000    const char *mode;
021100000000{
021200000000    return gz_open (path, mode, -1);
021300000000}
021400000000
021500000000/* ===========================================================================
021600000000     Associate a gzFile with the file descriptor fd. fd is not dup'ed here
021700000000   to mimic the behavio(u)r of fdopen.
021800000000*/
021900000000gzFile ZEXPORT gzdopen (fd, mode)
022000000000    int fd;
022100000000    const char *mode;
022200000000{
022300000000    char name[46];      /* allow for up to 128-bit integers */
022400000000
022500000000    if (fd < 0) return (gzFile)Z_NULL;
022600000000    sprintf(name, "<fd:%d>", fd); /* for debugging */
022700000000
022800000000    return gz_open (name, mode, fd);
022900000000}
023000000000
023100000000/* ===========================================================================
023200000000 * Update the compression level and strategy
023300000000 */
023400000000int ZEXPORT gzsetparams (file, level, strategy)
023500000000    gzFile file;
023600000000    int level;
023700000000    int strategy;
023800000000{
023900000000    gz_stream *s = (gz_stream*)file;
024000000000
024100000000    if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;
024200000000
024300000000    /* Make room to allow flushing */
024400000000    if (s->stream.avail_out == 0) {
024500000000
024600000000        s->stream.next_out = s->outbuf;
024700000000        if (fwrite(s->outbuf, 1, Z_BUFSIZE, s->file) != Z_BUFSIZE) {
024800000000            s->z_err = Z_ERRNO;
024900000000        }
025000000000        s->stream.avail_out = Z_BUFSIZE;
025100000000    }
025200000000
025300000000    return deflateParams (&(s->stream), level, strategy);
025400000000}
025500000000
025600000000/* ===========================================================================
025700000000     Read a byte from a gz_stream; update next_in and avail_in. Return EOF
025800000000   for end of file.
025900000000   IN assertion: the stream s has been sucessfully opened for reading.
026000000000*/
026100000000local int get_byte(s)
026200000000    gz_stream *s;
026300000000{
026400000000    if (s->z_eof) return EOF;
026500000000    if (s->stream.avail_in == 0) {
026600000000        errno = 0;
026700000000        s->stream.avail_in = (uInt)fread(s->inbuf, 1, Z_BUFSIZE, s->file);
026800000000        if (s->stream.avail_in == 0) {
026900000000            s->z_eof = 1;
027000000000            if (ferror(s->file)) s->z_err = Z_ERRNO;
027100000000            return EOF;
027200000000        }
027300000000        s->stream.next_in = s->inbuf;
027400000000    }
027500000000    s->stream.avail_in--;
027600000000    return *(s->stream.next_in)++;
027700000000}
027800000000
027900000000/* ===========================================================================
028000000000      Check the gzip header of a gz_stream opened for reading. Set the stream
028100000000    mode to transparent if the gzip magic header is not present; set s->err
028200000000    to Z_DATA_ERROR if the magic header is present but the rest of the header
028300000000    is incorrect.
028400000000    IN assertion: the stream s has already been created sucessfully;
028500000000       s->stream.avail_in is zero for the first time, but may be non-zero
028600000000       for concatenated .gz files.
028700000000*/
028800000000local void check_header(s)
028900000000    gz_stream *s;
029000000000{
029100000000    int method; /* method byte */
029200000000    int flags;  /* flags byte */
029300000000    uInt len;
029400000000    int c;
029500000000
029600000000    /* Assure two bytes in the buffer so we can peek ahead -- handle case
029700000000       where first byte of header is at the end of the buffer after the last
029800000000       gzip segment */
029900000000    len = s->stream.avail_in;
030000000000    if (len < 2) {
030100000000        if (len) s->inbuf[0] = s->stream.next_in[0];
030200000000        errno = 0;
030300000000        len = (uInt)fread(s->inbuf + len, 1, Z_BUFSIZE >> len, s->file);
030400000000        if (len == 0 && ferror(s->file)) s->z_err = Z_ERRNO;
030500000000        s->stream.avail_in += len;
030600000000        s->stream.next_in = s->inbuf;
030700000000        if (s->stream.avail_in < 2) {
030800000000            s->transparent = s->stream.avail_in;
030900000000            return;
031000000000        }
031100000000    }
031200000000
031300000000    /* Peek ahead to check the gzip magic header */
031400000000    if (s->stream.next_in[0] != gz_magic[0] ||
031500000000        s->stream.next_in[1] != gz_magic[1]) {
031600000000        s->transparent = 1;
031700000000        return;
031800000000    }
031900000000    s->stream.avail_in -= 2;
032000000000    s->stream.next_in += 2;
032100000000
032200000000    /* Check the rest of the gzip header */
032300000000    method = get_byte(s);
032400000000    flags = get_byte(s);
032500000000    if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
032600000000        s->z_err = Z_DATA_ERROR;
032700000000        return;
032800000000    }
032900000000
033000000000    /* Discard time, xflags and OS code: */
033100000000    for (len = 0; len < 6; len++) (void)get_byte(s);
033200000000
033300000000    if ((flags & EXTRA_FIELD) != 0) { /* skip the extra field */
033400000000        len  =  (uInt)get_byte(s);
033500000000        len += ((uInt)get_byte(s))<<8;
033600000000        /* len is garbage if EOF but the loop below will quit anyway */
033700000000        while (len-- != 0 && get_byte(s) != EOF) ;
033800000000    }
033900000000    if ((flags & ORIG_NAME) != 0) { /* skip the original file name */
034000000000        while ((c = get_byte(s)) != 0 && c != EOF) ;
034100000000    }
034200000000    if ((flags & COMMENT) != 0) {   /* skip the .gz file comment */
034300000000        while ((c = get_byte(s)) != 0 && c != EOF) ;
034400000000    }
034500000000    if ((flags & HEAD_CRC) != 0) {  /* skip the header crc */
034600000000        for (len = 0; len < 2; len++) (void)get_byte(s);
034700000000    }
034800000000    s->z_err = s->z_eof ? Z_DATA_ERROR : Z_OK;
034900000000}
035000000000
035100000000 /* ===========================================================================
035200000000 * Cleanup then free the given gz_stream. Return a zlib error code.
035300000000   Try freeing in the reverse order of allocations.
035400000000 */
035500000000local int destroy (s)
035600000000    gz_stream *s;
035700000000{
035800000000    int err = Z_OK;
035900000000
036000000000    if (!s) return Z_STREAM_ERROR;
036100000000
036200000000    TRYFREE(s->msg);
036300000000
036400000000    if (s->stream.state != NULL) {
036500000000        if (s->mode == 'w') {
036600000000#ifdef NO_GZCOMPRESS
036700000000            err = Z_STREAM_ERROR;
036800000000#else
036900000000            err = deflateEnd(&(s->stream));
037000000000#endif
037100000000        } else if (s->mode == 'r') {
037200000000            err = inflateEnd(&(s->stream));
037300000000        }
037400000000    }
037500000000    if (s->file != NULL && fclose(s->file)) {
037600000000#ifdef ESPIPE
037700000000        if (errno != ESPIPE) /* fclose is broken for pipes in HP/UX */
037800000000#endif
037900000000            err = Z_ERRNO;
038000000000    }
038100000000    if (s->z_err < 0) err = s->z_err;
038200000000
038300000000    TRYFREE(s->inbuf);
038400000000    TRYFREE(s->outbuf);
038500000000    TRYFREE(s->path);
038600000000    TRYFREE(s);
038700000000    return err;
038800000000}
038900000000
039000000000/* ===========================================================================
039100000000     Reads the given number of uncompressed bytes from the compressed file.
039200000000   gzread returns the number of bytes actually read (0 for end of file).
039300000000*/
039400000000int ZEXPORT gzread (file, buf, len)
039500000000    gzFile file;
039600000000    voidp buf;
039700000000    unsigned len;
039800000000{
039900000000    gz_stream *s = (gz_stream*)file;
040000000000    Bytef *start = (Bytef*)buf; /* starting point for crc computation */
040100000000    Byte  *next_out; /* == stream.next_out but not forced far (for MSDOS) */
040200000000
040300000000    if (s == NULL || s->mode != 'r') return Z_STREAM_ERROR;
040400000000
040500000000    if (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO) return -1;
040600000000    if (s->z_err == Z_STREAM_END) return 0;  /* EOF */
040700000000
040800000000    next_out = (Byte*)buf;
040900000000    s->stream.next_out = (Bytef*)buf;
041000000000    s->stream.avail_out = len;
041100000000
041200000000    if (s->stream.avail_out && s->back != EOF) {
041300000000        *next_out++ = s->back;
041400000000        s->stream.next_out++;
041500000000        s->stream.avail_out--;
041600000000        s->back = EOF;
041700000000        s->out++;
041800000000        start++;
041900000000        if (s->last) {
042000000000            s->z_err = Z_STREAM_END;
042100000000            return 1;
042200000000        }
042300000000    }
042400000000
042500000000    while (s->stream.avail_out != 0) {
042600000000
042700000000        if (s->transparent) {
042800000000            /* Copy first the lookahead bytes: */
042900000000            uInt n = s->stream.avail_in;
043000000000            if (n > s->stream.avail_out) n = s->stream.avail_out;
043100000000            if (n > 0) {
043200000000                zmemcpy(s->stream.next_out, s->stream.next_in, n);
043300000000                next_out += n;
043400000000                s->stream.next_out = next_out;
043500000000                s->stream.next_in   += n;
043600000000                s->stream.avail_out -= n;
043700000000                s->stream.avail_in  -= n;
043800000000            }
043900000000            if (s->stream.avail_out > 0) {
044000000000                s->stream.avail_out -=
044100000000                    (uInt)fread(next_out, 1, s->stream.avail_out, s->file);
044200000000            }
044300000000            len -= s->stream.avail_out;
044400000000            s->in  += len;
044500000000            s->out += len;
044600000000            if (len == 0) s->z_eof = 1;
044700000000            return (int)len;
044800000000        }
044900000000        if (s->stream.avail_in == 0 && !s->z_eof) {
045000000000
045100000000            errno = 0;
045200000000            s->stream.avail_in = (uInt)fread(s->inbuf, 1, Z_BUFSIZE, s->file);
045300000000            if (s->stream.avail_in == 0) {
045400000000                s->z_eof = 1;
045500000000                if (ferror(s->file)) {
045600000000                    s->z_err = Z_ERRNO;
045700000000                    break;
045800000000                }
045900000000            }
046000000000            s->stream.next_in = s->inbuf;
046100000000        }
046200000000        s->in += s->stream.avail_in;
046300000000        s->out += s->stream.avail_out;
046400000000        s->z_err = inflate(&(s->stream), Z_NO_FLUSH);
046500000000        s->in -= s->stream.avail_in;
046600000000        s->out -= s->stream.avail_out;
046700000000
046800000000        if (s->z_err == Z_STREAM_END) {
046900000000            /* Check CRC and original size */
047000000000            s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));
047100000000            start = s->stream.next_out;
047200000000
047300000000            if (getLong(s) != s->crc) {
047400000000                s->z_err = Z_DATA_ERROR;
047500000000            } else {
047600000000                (void)getLong(s);
047700000000                /* The uncompressed length returned by above getlong() may be
047800000000                 * different from s->out in case of concatenated .gz files.
047900000000                 * Check for such files:
048000000000                 */
048100000000                check_header(s);
048200000000                if (s->z_err == Z_OK) {
048300000000                    inflateReset(&(s->stream));
048400000000                    s->crc = crc32(0L, Z_NULL, 0);
048500000000                }
048600000000            }
048700000000        }
048800000000        if (s->z_err != Z_OK || s->z_eof) break;
048900000000    }
049000000000    s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));
049100000000
049200000000    if (len == s->stream.avail_out &&
049300000000        (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO))
049400000000        return -1;
049500000000    return (int)(len - s->stream.avail_out);
049600000000}
049700000000
049800000000
049900000000/* ===========================================================================
050000000000      Reads one byte from the compressed file. gzgetc returns this byte
050100000000   or -1 in case of end of file or error.
050200000000*/
050300000000int ZEXPORT gzgetc(file)
050400000000    gzFile file;
050500000000{
050600000000    unsigned char c;
050700000000
050800000000    return gzread(file, &c, 1) == 1 ? c : -1;
050900000000}
051000000000
051100000000
051200000000/* ===========================================================================
051300000000      Push one byte back onto the stream.
051400000000*/
051500000000int ZEXPORT gzungetc(c, file)
051600000000    int c;
051700000000    gzFile file;
051800000000{
051900000000    gz_stream *s = (gz_stream*)file;
052000000000
052100000000    if (s == NULL || s->mode != 'r' || c == EOF || s->back != EOF) return EOF;
052200000000    s->back = c;
052300000000    s->out--;
052400000000    s->last = (s->z_err == Z_STREAM_END);
052500000000    if (s->last) s->z_err = Z_OK;
052600000000    s->z_eof = 0;
052700000000    return c;
052800000000}
052900000000
053000000000
053100000000/* ===========================================================================
053200000000      Reads bytes from the compressed file until len-1 characters are
053300000000   read, or a newline character is read and transferred to buf, or an
053400000000   end-of-file condition is encountered.  The string is then terminated
053500000000   with a null character.
053600000000      gzgets returns buf, or Z_NULL in case of error.
053700000000
053800000000      The current implementation is not optimized at all.
053900000000*/
054000000000char * ZEXPORT gzgets(file, buf, len)
054100000000    gzFile file;
054200000000    char *buf;
054300000000    int len;
054400000000{
054500000000    char *b = buf;
054600000000    if (buf == Z_NULL || len <= 0) return Z_NULL;
054700000000
054800000000    while (--len > 0 && gzread(file, buf, 1) == 1 && *buf++ != '\n') ;
054900000000    *buf = '\0';
055000000000    return b == buf && len > 0 ? Z_NULL : b;
055100000000}
055200000000
055300000000
055400000000#ifndef NO_GZCOMPRESS
055500000000/* ===========================================================================
055600000000     Writes the given number of uncompressed bytes into the compressed file.
055700000000   gzwrite returns the number of bytes actually written (0 in case of error).
055800000000*/
055900000000int ZEXPORT gzwrite (file, buf, len)
056000000000    gzFile file;
056100000000    voidpc buf;
056200000000    unsigned len;
056300000000{
056400000000    gz_stream *s = (gz_stream*)file;
056500000000
056600000000    if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;
056700000000
056800000000    s->stream.next_in = (Bytef*)buf;
056900000000    s->stream.avail_in = len;
057000000000
057100000000    while (s->stream.avail_in != 0) {
057200000000
057300000000        if (s->stream.avail_out == 0) {
057400000000
057500000000            s->stream.next_out = s->outbuf;
057600000000            if (fwrite(s->outbuf, 1, Z_BUFSIZE, s->file) != Z_BUFSIZE) {
057700000000                s->z_err = Z_ERRNO;
057800000000                break;
057900000000            }
058000000000            s->stream.avail_out = Z_BUFSIZE;
058100000000        }
058200000000        s->in += s->stream.avail_in;
058300000000        s->out += s->stream.avail_out;
058400000000        s->z_err = deflate(&(s->stream), Z_NO_FLUSH);
058500000000        s->in -= s->stream.avail_in;
058600000000        s->out -= s->stream.avail_out;
058700000000        if (s->z_err != Z_OK) break;
058800000000    }
058900000000    s->crc = crc32(s->crc, (const Bytef *)buf, len);
059000000000
059100000000    return (int)(len - s->stream.avail_in);
059200000000}
059300000000
059400000000
059500000000/* ===========================================================================
059600000000     Converts, formats, and writes the args to the compressed file under
059700000000   control of the format string, as in fprintf. gzprintf returns the number of
059800000000   uncompressed bytes actually written (0 in case of error).
059900000000*/
060000000000#ifdef STDC
060100000000#include <stdarg.h>
060200000000
060300000000int ZEXPORTVA gzprintf (gzFile file, const char *format, /* args */ ...)
060400000000{
060500000000    char buf[Z_PRINTF_BUFSIZE];
060600000000    va_list va;
060700000000    int len;
060800000000
060900000000    buf[sizeof(buf) - 1] = 0;
061000000000    va_start(va, format);
061100000000#ifdef NO_vsnprintf
061200000000#  ifdef HAS_vsprintf_void
061300000000    (void)vsprintf(buf, format, va);
061400000000    va_end(va);
061500000000    for (len = 0; len < sizeof(buf); len++)
061600000000        if (buf[len] == 0) break;
061700000000#  else
061800000000    len = vsprintf(buf, format, va);
061900000000    va_end(va);
062000000000#  endif
062100000000#else
062200000000#  ifdef HAS_vsnprintf_void
062300000000    (void)vsnprintf(buf, sizeof(buf), format, va);
062400000000    va_end(va);
062500000000    len = strlen(buf);
062600000000#  else
062700000000    len = vsnprintf(buf, sizeof(buf), format, va);
062800000000    va_end(va);
062900000000#  endif
063000000000#endif
063100000000    if (len <= 0 || len >= (int)sizeof(buf) || buf[sizeof(buf) - 1] != 0)
063200000000        return 0;
063300000000    return gzwrite(file, buf, (unsigned)len);
063400000000}
063500000000#else /* not ANSI C */
063600000000
063700000000int ZEXPORTVA gzprintf (file, format, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10,
063800000000                       a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)
063900000000    gzFile file;
064000000000    const char *format;
064100000000    int a1, a2, a3, a4, a5, a6, a7, a8, a9, a10,
064200000000        a11, a12, a13, a14, a15, a16, a17, a18, a19, a20;
064300000000{
064400000000    char buf[Z_PRINTF_BUFSIZE];
064500000000    int len;
064600000000
064700000000    buf[sizeof(buf) - 1] = 0;
064800000000#ifdef NO_snprintf
064900000000#  ifdef HAS_sprintf_void
065000000000    sprintf(buf, format, a1, a2, a3, a4, a5, a6, a7, a8,
065100000000            a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
065200000000    for (len = 0; len < sizeof(buf); len++)
065300000000        if (buf[len] == 0) break;
065400000000#  else
065500000000    len = sprintf(buf, format, a1, a2, a3, a4, a5, a6, a7, a8,
065600000000                a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
065700000000#  endif
065800000000#else
065900000000#  ifdef HAS_snprintf_void
066000000000    snprintf(buf, sizeof(buf), format, a1, a2, a3, a4, a5, a6, a7, a8,
066100000000             a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
066200000000    len = strlen(buf);
066300000000#  else
066400000000    len = snprintf(buf, sizeof(buf), format, a1, a2, a3, a4, a5, a6, a7, a8,
066500000000                 a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20);
066600000000#  endif
066700000000#endif
066800000000    if (len <= 0 || len >= sizeof(buf) || buf[sizeof(buf) - 1] != 0)
066900000000        return 0;
067000000000    return gzwrite(file, buf, len);
067100000000}
067200000000#endif
067300000000
067400000000/* ===========================================================================
067500000000      Writes c, converted to an unsigned char, into the compressed file.
067600000000   gzputc returns the value that was written, or -1 in case of error.
067700000000*/
067800000000int ZEXPORT gzputc(file, c)
067900000000    gzFile file;
068000000000    int c;
068100000000{
068200000000    unsigned char cc = (unsigned char) c; /* required for big endian systems */
068300000000
068400000000    return gzwrite(file, &cc, 1) == 1 ? (int)cc : -1;
068500000000}
068600000000
068700000000
068800000000/* ===========================================================================
068900000000      Writes the given null-terminated string to the compressed file, excluding
069000000000   the terminating null character.
069100000000      gzputs returns the number of characters written, or -1 in case of error.
069200000000*/
069300000000int ZEXPORT gzputs(file, s)
069400000000    gzFile file;
069500000000    const char *s;
069600000000{
069700000000    return gzwrite(file, (char*)s, (unsigned)strlen(s));
069800000000}
069900000000
070000000000
070100000000/* ===========================================================================
070200000000     Flushes all pending output into the compressed file. The parameter
070300000000   flush is as in the deflate() function.
070400000000*/
070500000000local int do_flush (file, flush)
070600000000    gzFile file;
070700000000    int flush;
070800000000{
070900000000    uInt len;
071000000000    int done = 0;
071100000000    gz_stream *s = (gz_stream*)file;
071200000000
071300000000    if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;
071400000000
071500000000    s->stream.avail_in = 0; /* should be zero already anyway */
071600000000
071700000000    for (;;) {
071800000000        len = Z_BUFSIZE - s->stream.avail_out;
071900000000
072000000000        if (len != 0) {
072100000000            if ((uInt)fwrite(s->outbuf, 1, len, s->file) != len) {
072200000000                s->z_err = Z_ERRNO;
072300000000                return Z_ERRNO;
072400000000            }
072500000000            s->stream.next_out = s->outbuf;
072600000000            s->stream.avail_out = Z_BUFSIZE;
072700000000        }
072800000000        if (done) break;
072900000000        s->out += s->stream.avail_out;
073000000000        s->z_err = deflate(&(s->stream), flush);
073100000000        s->out -= s->stream.avail_out;
073200000000
073300000000        /* Ignore the second of two consecutive flushes: */
073400000000        if (len == 0 && s->z_err == Z_BUF_ERROR) s->z_err = Z_OK;
073500000000
073600000000        /* deflate has finished flushing only when it hasn't used up
073700000000         * all the available space in the output buffer:
073800000000         */
073900000000        done = (s->stream.avail_out != 0 || s->z_err == Z_STREAM_END);
074000000000
074100000000        if (s->z_err != Z_OK && s->z_err != Z_STREAM_END) break;
074200000000    }
074300000000    return  s->z_err == Z_STREAM_END ? Z_OK : s->z_err;
074400000000}
074500000000
074600000000int ZEXPORT gzflush (file, flush)
074700000000     gzFile file;
074800000000     int flush;
074900000000{
075000000000    gz_stream *s = (gz_stream*)file;
075100000000    int err = do_flush (file, flush);
075200000000
075300000000    if (err) return err;
075400000000    fflush(s->file);
075500000000    return  s->z_err == Z_STREAM_END ? Z_OK : s->z_err;
075600000000}
075700000000#endif /* NO_GZCOMPRESS */
075800000000
075900000000/* ===========================================================================
076000000000      Sets the starting position for the next gzread or gzwrite on the given
076100000000   compressed file. The offset represents a number of bytes in the
076200000000      gzseek returns the resulting offset location as measured in bytes from
076300000000   the beginning of the uncompressed stream, or -1 in case of error.
076400000000      SEEK_END is not implemented, returns error.
076500000000      In this version of the library, gzseek can be extremely slow.
076600000000*/
076700000000z_off_t ZEXPORT gzseek (file, offset, whence)
076800000000    gzFile file;
076900000000    z_off_t offset;
077000000000    int whence;
077100000000{
077200000000    gz_stream *s = (gz_stream*)file;
077300000000
077400000000    if (s == NULL || whence == SEEK_END ||
077500000000        s->z_err == Z_ERRNO || s->z_err == Z_DATA_ERROR) {
077600000000        return -1L;
077700000000    }
077800000000
077900000000    if (s->mode == 'w') {
078000000000#ifdef NO_GZCOMPRESS
078100000000        return -1L;
078200000000#else
078300000000        if (whence == SEEK_SET) {
078400000000            offset -= s->in;
078500000000        }
078600000000        if (offset < 0) return -1L;
078700000000
078800000000        /* At this point, offset is the number of zero bytes to write. */
078900000000        if (s->inbuf == Z_NULL) {
079000000000            s->inbuf = (Byte*)ALLOC(Z_BUFSIZE); /* for seeking */
079100000000            if (s->inbuf == Z_NULL) return -1L;
079200000000            zmemzero(s->inbuf, Z_BUFSIZE);
079300000000        }
079400000000        while (offset > 0)  {
079500000000            uInt size = Z_BUFSIZE;
079600000000            if (offset < Z_BUFSIZE) size = (uInt)offset;
079700000000
079800000000            size = gzwrite(file, s->inbuf, size);
079900000000            if (size == 0) return -1L;
080000000000
080100000000            offset -= size;
080200000000        }
080300000000        return s->in;
080400000000#endif
080500000000    }
080600000000    /* Rest of function is for reading only */
080700000000
080800000000    /* compute absolute position */
080900000000    if (whence == SEEK_CUR) {
081000000000        offset += s->out;
081100000000    }
081200000000    if (offset < 0) return -1L;
081300000000
081400000000    if (s->transparent) {
081500000000        /* map to fseek */
081600000000        s->back = EOF;
081700000000        s->stream.avail_in = 0;
081800000000        s->stream.next_in = s->inbuf;
081900000000        if (fseek(s->file, offset, SEEK_SET) < 0) return -1L;
082000000000
082100000000        s->in = s->out = offset;
082200000000        return offset;
082300000000    }
082400000000
082500000000    /* For a negative seek, rewind and use positive seek */
082600000000    if (offset >= s->out) {
082700000000        offset -= s->out;
082800000000    } else if (gzrewind(file) < 0) {
082900000000        return -1L;
083000000000    }
083100000000    /* offset is now the number of bytes to skip. */
083200000000
083300000000    if (offset != 0 && s->outbuf == Z_NULL) {
083400000000        s->outbuf = (Byte*)ALLOC(Z_BUFSIZE);
083500000000        if (s->outbuf == Z_NULL) return -1L;
083600000000    }
083700000000    if (offset && s->back != EOF) {
083800000000        s->back = EOF;
083900000000        s->out++;
084000000000        offset--;
084100000000        if (s->last) s->z_err = Z_STREAM_END;
084200000000    }
084300000000    while (offset > 0)  {
084400000000        int size = Z_BUFSIZE;
084500000000        if (offset < Z_BUFSIZE) size = (int)offset;
084600000000
084700000000        size = gzread(file, s->outbuf, (uInt)size);
084800000000        if (size <= 0) return -1L;
084900000000        offset -= size;
085000000000    }
085100000000    return s->out;
085200000000}
085300000000
085400000000/* ===========================================================================
085500000000     Rewinds input file.
085600000000*/
085700000000int ZEXPORT gzrewind (file)
085800000000    gzFile file;
085900000000{
086000000000    gz_stream *s = (gz_stream*)file;
086100000000
086200000000    if (s == NULL || s->mode != 'r') return -1;
086300000000
086400000000    s->z_err = Z_OK;
086500000000    s->z_eof = 0;
086600000000    s->back = EOF;
086700000000    s->stream.avail_in = 0;
086800000000    s->stream.next_in = s->inbuf;
086900000000    s->crc = crc32(0L, Z_NULL, 0);
087000000000    if (!s->transparent) (void)inflateReset(&s->stream);
087100000000    s->in = 0;
087200000000    s->out = 0;
087300000000    return fseek(s->file, s->start, SEEK_SET);
087400000000}
087500000000
087600000000/* ===========================================================================
087700000000     Returns the starting position for the next gzread or gzwrite on the
087800000000   given compressed file. This position represents a number of bytes in the
087900000000   uncompressed data stream.
088000000000*/
088100000000z_off_t ZEXPORT gztell (file)
088200000000    gzFile file;
088300000000{
088400000000    return gzseek(file, 0L, SEEK_CUR);
088500000000}
088600000000
088700000000/* ===========================================================================
088800000000     Returns 1 when EOF has previously been detected reading the given
088900000000   input stream, otherwise zero.
089000000000*/
089100000000int ZEXPORT gzeof (file)
089200000000    gzFile file;
089300000000{
089400000000    gz_stream *s = (gz_stream*)file;
089500000000
089600000000    /* With concatenated compressed files that can have embedded
089700000000     * crc trailers, z_eof is no longer the only/best indicator of EOF
089800000000     * on a gz_stream. Handle end-of-stream error explicitly here.
089900000000     */
090000000000    if (s == NULL || s->mode != 'r') return 0;
090100000000    if (s->z_eof) return 1;
090200000000    return s->z_err == Z_STREAM_END;
090300000000}
090400000000
090500000000/* ===========================================================================
090600000000     Returns 1 if reading and doing so transparently, otherwise zero.
090700000000*/
090800000000int ZEXPORT gzdirect (file)
090900000000    gzFile file;
091000000000{
091100000000    gz_stream *s = (gz_stream*)file;
091200000000
091300000000    if (s == NULL || s->mode != 'r') return 0;
091400000000    return s->transparent;
091500000000}
091600000000
091700000000/* ===========================================================================
091800000000   Outputs a long in LSB order to the given file
091900000000*/
092000000000local void putLong (file, x)
092100000000    FILE *file;
092200000000    uLong x;
092300000000{
092400000000    int n;
092500000000    for (n = 0; n < 4; n++) {
092600000000        fputc((int)(x & 0xff), file);
092700000000        x >>= 8;
092800000000    }
092900000000}
093000000000
093100000000/* ===========================================================================
093200000000   Reads a long in LSB order from the given gz_stream. Sets z_err in case
093300000000   of error.
093400000000*/
093500000000local uLong getLong (s)
093600000000    gz_stream *s;
093700000000{
093800000000    uLong x = (uLong)get_byte(s);
093900000000    int c;
094000000000
094100000000    x += ((uLong)get_byte(s))<<8;
094200000000    x += ((uLong)get_byte(s))<<16;
094300000000    c = get_byte(s);
094400000000    if (c == EOF) s->z_err = Z_DATA_ERROR;
094500000000    x += ((uLong)c)<<24;
094600000000    return x;
094700000000}
094800000000
094900000000/* ===========================================================================
095000000000     Flushes all pending output if necessary, closes the compressed file
095100000000   and deallocates all the (de)compression state.
095200000000*/
095300000000int ZEXPORT gzclose (file)
095400000000    gzFile file;
095500000000{
095600000000    gz_stream *s = (gz_stream*)file;
095700000000
095800000000    if (s == NULL) return Z_STREAM_ERROR;
095900000000
096000000000    if (s->mode == 'w') {
096100000000#ifdef NO_GZCOMPRESS
096200000000        return Z_STREAM_ERROR;
096300000000#else
096400000000        if (do_flush (file, Z_FINISH) != Z_OK)
096500000000            return destroy((gz_stream*)file);
096600000000
096700000000        putLong (s->file, s->crc);
096800000000        putLong (s->file, (uLong)(s->in & 0xffffffff));
096900000000#endif
097000000000    }
097100000000    return destroy((gz_stream*)file);
097200000000}
097300000000
097400000000#ifdef STDC
097500000000#  define zstrerror(errnum) strerror(errnum)
097600000000#else
097700000000#  define zstrerror(errnum) ""
097800000000#endif
097900000000
098000000000/* ===========================================================================
098100000000     Returns the error message for the last error which occurred on the
098200000000   given compressed file. errnum is set to zlib error number. If an
098300000000   error occurred in the file system and not in the compression library,
098400000000   errnum is set to Z_ERRNO and the application may consult errno
098500000000   to get the exact error code.
098600000000*/
098700000000const char * ZEXPORT gzerror (file, errnum)
098800000000    gzFile file;
098900000000    int *errnum;
099000000000{
099100000000    char *m;
099200000000    gz_stream *s = (gz_stream*)file;
099300000000
099400000000    if (s == NULL) {
099500000000        *errnum = Z_STREAM_ERROR;
099600000000        return (const char*)ERR_MSG(Z_STREAM_ERROR);
099700000000    }
099800000000    *errnum = s->z_err;
099900000000    if (*errnum == Z_OK) return (const char*)"";
100000000000
100100000000    m = (char*)(*errnum == Z_ERRNO ? zstrerror(errno) : s->stream.msg);
100200000000
100300000000    if (m == NULL || *m == '\0') m = (char*)ERR_MSG(s->z_err);
100400000000
100500000000    TRYFREE(s->msg);
100600000000    s->msg = (char*)ALLOC(strlen(s->path) + strlen(m) + 3);
100700000000    if (s->msg == Z_NULL) return (const char*)ERR_MSG(Z_MEM_ERROR);
100800000000    strcpy(s->msg, s->path);
100900000000    strcat(s->msg, ": ");
101000000000    strcat(s->msg, m);
101100000000    return (const char*)s->msg;
101200000000}
101300000000
101400000000/* ===========================================================================
101500000000     Clear the error and end-of-file flags, and do the same for the real file.
101600000000*/
101700000000void ZEXPORT gzclearerr (file)
101800000000    gzFile file;
101900000000{
102000000000    gz_stream *s = (gz_stream*)file;
102100000000
102200000000    if (s == NULL) return;
102300000000    if (s->z_err != Z_STREAM_END) s->z_err = Z_OK;
102400000000    s->z_eof = 0;
102500000000    clearerr(s->file);
102600000000}
