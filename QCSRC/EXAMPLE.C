000100000000/* example.c -- usage example of the zlib compression library
000200000000 * Copyright (C) 1995-2004 Jean-loup Gailly.
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/* @(#) $Id$ */
000700000000
000800000000#include <stdio.h>
000900000000#include "zlib.h"
001000000000
001100000000#ifdef STDC
001200000000#  include <string.h>
001300000000#  include <stdlib.h>
001400000000#endif
001500000000
001600000000#if defined(VMS) || defined(RISCOS)
001700000000#  define TESTFILE "foo-gz"
001800000000#else
001900000000#  define TESTFILE "foo.gz"
002000000000#endif
002100000000
002200000000#define CHECK_ERR(err, msg) { \
002300000000    if (err != Z_OK) { \
002400000000        fprintf(stderr, "%s error: %d\n", msg, err); \
002500000000        exit(1); \
002600000000    } \
002700000000}
002800000000
002900000000const char hello[] = "hello, hello!";
003000000000/* "hello world" would be more standard, but the repeated "hello"
003100000000 * stresses the compression code better, sorry...
003200000000 */
003300000000
003400000000const char dictionary[] = "hello";
003500000000uLong dictId; /* Adler32 value of the dictionary */
003600000000
003700000000void test_compress      OF((Byte *compr, uLong comprLen,
003800000000                            Byte *uncompr, uLong uncomprLen));
003900000000void test_gzio          OF((const char *fname,
004000000000                            Byte *uncompr, uLong uncomprLen));
004100000000void test_deflate       OF((Byte *compr, uLong comprLen));
004200000000void test_inflate       OF((Byte *compr, uLong comprLen,
004300000000                            Byte *uncompr, uLong uncomprLen));
004400000000void test_large_deflate OF((Byte *compr, uLong comprLen,
004500000000                            Byte *uncompr, uLong uncomprLen));
004600000000void test_large_inflate OF((Byte *compr, uLong comprLen,
004700000000                            Byte *uncompr, uLong uncomprLen));
004800000000void test_flush         OF((Byte *compr, uLong *comprLen));
004900000000void test_sync          OF((Byte *compr, uLong comprLen,
005000000000                            Byte *uncompr, uLong uncomprLen));
005100000000void test_dict_deflate  OF((Byte *compr, uLong comprLen));
005200000000void test_dict_inflate  OF((Byte *compr, uLong comprLen,
005300000000                            Byte *uncompr, uLong uncomprLen));
005400000000int  main               OF((int argc, char *argv[]));
005500000000
005600000000/* ===========================================================================
005700000000 * Test compress() and uncompress()
005800000000 */
005900000000void test_compress(compr, comprLen, uncompr, uncomprLen)
006000000000    Byte *compr, *uncompr;
006100000000    uLong comprLen, uncomprLen;
006200000000{
006300000000    int err;
006400000000    uLong len = (uLong)strlen(hello)+1;
006500000000
006600000000    err = compress(compr, &comprLen, (const Bytef*)hello, len);
006700000000    CHECK_ERR(err, "compress");
006800000000
006900000000    strcpy((char*)uncompr, "garbage");
007000000000
007100000000    err = uncompress(uncompr, &uncomprLen, compr, comprLen);
007200000000    CHECK_ERR(err, "uncompress");
007300000000
007400000000    if (strcmp((char*)uncompr, hello)) {
007500000000        fprintf(stderr, "bad uncompress\n");
007600000000        exit(1);
007700000000    } else {
007800000000        printf("uncompress(): %s\n", (char *)uncompr);
007900000000    }
008000000000}
008100000000
008200000000/* ===========================================================================
008300000000 * Test read/write of .gz files
008400000000 */
008500000000void test_gzio(fname, uncompr, uncomprLen)
008600000000    const char *fname; /* compressed file name */
008700000000    Byte *uncompr;
008800000000    uLong uncomprLen;
008900000000{
009000000000#ifdef NO_GZCOMPRESS
009100000000    fprintf(stderr, "NO_GZCOMPRESS -- gz* functions cannot compress\n");
009200000000#else
009300000000    int err;
009400000000    int len = (int)strlen(hello)+1;
009500000000    gzFile file;
009600000000    z_off_t pos;
009700000000
009800000000    file = gzopen(fname, "wb");
009900000000    if (file == NULL) {
010000000000        fprintf(stderr, "gzopen error\n");
010100000000        exit(1);
010200000000    }
010300000000    gzputc(file, 'h');
010400000000    if (gzputs(file, "ello") != 4) {
010500000000        fprintf(stderr, "gzputs err: %s\n", gzerror(file, &err));
010600000000        exit(1);
010700000000    }
010800000000    if (gzprintf(file, ", %s!", "hello") != 8) {
010900000000        fprintf(stderr, "gzprintf err: %s\n", gzerror(file, &err));
011000000000        exit(1);
011100000000    }
011200000000    gzseek(file, 1L, SEEK_CUR); /* add one zero byte */
011300000000    gzclose(file);
011400000000
011500000000    file = gzopen(fname, "rb");
011600000000    if (file == NULL) {
011700000000        fprintf(stderr, "gzopen error\n");
011800000000        exit(1);
011900000000    }
012000000000    strcpy((char*)uncompr, "garbage");
012100000000
012200000000    if (gzread(file, uncompr, (unsigned)uncomprLen) != len) {
012300000000        fprintf(stderr, "gzread err: %s\n", gzerror(file, &err));
012400000000        exit(1);
012500000000    }
012600000000    if (strcmp((char*)uncompr, hello)) {
012700000000        fprintf(stderr, "bad gzread: %s\n", (char*)uncompr);
012800000000        exit(1);
012900000000    } else {
013000000000        printf("gzread(): %s\n", (char*)uncompr);
013100000000    }
013200000000
013300000000    pos = gzseek(file, -8L, SEEK_CUR);
013400000000    if (pos != 6 || gztell(file) != pos) {
013500000000        fprintf(stderr, "gzseek error, pos=%ld, gztell=%ld\n",
013600000000                (long)pos, (long)gztell(file));
013700000000        exit(1);
013800000000    }
013900000000
014000000000    if (gzgetc(file) != ' ') {
014100000000        fprintf(stderr, "gzgetc error\n");
014200000000        exit(1);
014300000000    }
014400000000
014500000000    if (gzungetc(' ', file) != ' ') {
014600000000        fprintf(stderr, "gzungetc error\n");
014700000000        exit(1);
014800000000    }
014900000000
015000000000    gzgets(file, (char*)uncompr, (int)uncomprLen);
015100000000    if (strlen((char*)uncompr) != 7) { /* " hello!" */
015200000000        fprintf(stderr, "gzgets err after gzseek: %s\n", gzerror(file, &err));
015300000000        exit(1);
015400000000    }
015500000000    if (strcmp((char*)uncompr, hello + 6)) {
015600000000        fprintf(stderr, "bad gzgets after gzseek\n");
015700000000        exit(1);
015800000000    } else {
015900000000        printf("gzgets() after gzseek: %s\n", (char*)uncompr);
016000000000    }
016100000000
016200000000    gzclose(file);
016300000000#endif
016400000000}
016500000000
016600000000/* ===========================================================================
016700000000 * Test deflate() with small buffers
016800000000 */
016900000000void test_deflate(compr, comprLen)
017000000000    Byte *compr;
017100000000    uLong comprLen;
017200000000{
017300000000    z_stream c_stream; /* compression stream */
017400000000    int err;
017500000000    uLong len = (uLong)strlen(hello)+1;
017600000000
017700000000    c_stream.zalloc = (alloc_func)0;
017800000000    c_stream.zfree = (free_func)0;
017900000000    c_stream.opaque = (voidpf)0;
018000000000
018100000000    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
018200000000    CHECK_ERR(err, "deflateInit");
018300000000
018400000000    c_stream.next_in  = (Bytef*)hello;
018500000000    c_stream.next_out = compr;
018600000000
018700000000    while (c_stream.total_in != len && c_stream.total_out < comprLen) {
018800000000        c_stream.avail_in = c_stream.avail_out = 1; /* force small buffers */
018900000000        err = deflate(&c_stream, Z_NO_FLUSH);
019000000000        CHECK_ERR(err, "deflate");
019100000000    }
019200000000    /* Finish the stream, still forcing small buffers: */
019300000000    for (;;) {
019400000000        c_stream.avail_out = 1;
019500000000        err = deflate(&c_stream, Z_FINISH);
019600000000        if (err == Z_STREAM_END) break;
019700000000        CHECK_ERR(err, "deflate");
019800000000    }
019900000000
020000000000    err = deflateEnd(&c_stream);
020100000000    CHECK_ERR(err, "deflateEnd");
020200000000}
020300000000
020400000000/* ===========================================================================
020500000000 * Test inflate() with small buffers
020600000000 */
020700000000void test_inflate(compr, comprLen, uncompr, uncomprLen)
020800000000    Byte *compr, *uncompr;
020900000000    uLong comprLen, uncomprLen;
021000000000{
021100000000    int err;
021200000000    z_stream d_stream; /* decompression stream */
021300000000
021400000000    strcpy((char*)uncompr, "garbage");
021500000000
021600000000    d_stream.zalloc = (alloc_func)0;
021700000000    d_stream.zfree = (free_func)0;
021800000000    d_stream.opaque = (voidpf)0;
021900000000
022000000000    d_stream.next_in  = compr;
022100000000    d_stream.avail_in = 0;
022200000000    d_stream.next_out = uncompr;
022300000000
022400000000    err = inflateInit(&d_stream);
022500000000    CHECK_ERR(err, "inflateInit");
022600000000
022700000000    while (d_stream.total_out < uncomprLen && d_stream.total_in < comprLen) {
022800000000        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
022900000000        err = inflate(&d_stream, Z_NO_FLUSH);
023000000000        if (err == Z_STREAM_END) break;
023100000000        CHECK_ERR(err, "inflate");
023200000000    }
023300000000
023400000000    err = inflateEnd(&d_stream);
023500000000    CHECK_ERR(err, "inflateEnd");
023600000000
023700000000    if (strcmp((char*)uncompr, hello)) {
023800000000        fprintf(stderr, "bad inflate\n");
023900000000        exit(1);
024000000000    } else {
024100000000        printf("inflate(): %s\n", (char *)uncompr);
024200000000    }
024300000000}
024400000000
024500000000/* ===========================================================================
024600000000 * Test deflate() with large buffers and dynamic change of compression level
024700000000 */
024800000000void test_large_deflate(compr, comprLen, uncompr, uncomprLen)
024900000000    Byte *compr, *uncompr;
025000000000    uLong comprLen, uncomprLen;
025100000000{
025200000000    z_stream c_stream; /* compression stream */
025300000000    int err;
025400000000
025500000000    c_stream.zalloc = (alloc_func)0;
025600000000    c_stream.zfree = (free_func)0;
025700000000    c_stream.opaque = (voidpf)0;
025800000000
025900000000    err = deflateInit(&c_stream, Z_BEST_SPEED);
026000000000    CHECK_ERR(err, "deflateInit");
026100000000
026200000000    c_stream.next_out = compr;
026300000000    c_stream.avail_out = (uInt)comprLen;
026400000000
026500000000    /* At this point, uncompr is still mostly zeroes, so it should compress
026600000000     * very well:
026700000000     */
026800000000    c_stream.next_in = uncompr;
026900000000    c_stream.avail_in = (uInt)uncomprLen;
027000000000    err = deflate(&c_stream, Z_NO_FLUSH);
027100000000    CHECK_ERR(err, "deflate");
027200000000    if (c_stream.avail_in != 0) {
027300000000        fprintf(stderr, "deflate not greedy\n");
027400000000        exit(1);
027500000000    }
027600000000
027700000000    /* Feed in already compressed data and switch to no compression: */
027800000000    deflateParams(&c_stream, Z_NO_COMPRESSION, Z_DEFAULT_STRATEGY);
027900000000    c_stream.next_in = compr;
028000000000    c_stream.avail_in = (uInt)comprLen/2;
028100000000    err = deflate(&c_stream, Z_NO_FLUSH);
028200000000    CHECK_ERR(err, "deflate");
028300000000
028400000000    /* Switch back to compressing mode: */
028500000000    deflateParams(&c_stream, Z_BEST_COMPRESSION, Z_FILTERED);
028600000000    c_stream.next_in = uncompr;
028700000000    c_stream.avail_in = (uInt)uncomprLen;
028800000000    err = deflate(&c_stream, Z_NO_FLUSH);
028900000000    CHECK_ERR(err, "deflate");
029000000000
029100000000    err = deflate(&c_stream, Z_FINISH);
029200000000    if (err != Z_STREAM_END) {
029300000000        fprintf(stderr, "deflate should report Z_STREAM_END\n");
029400000000        exit(1);
029500000000    }
029600000000    err = deflateEnd(&c_stream);
029700000000    CHECK_ERR(err, "deflateEnd");
029800000000}
029900000000
030000000000/* ===========================================================================
030100000000 * Test inflate() with large buffers
030200000000 */
030300000000void test_large_inflate(compr, comprLen, uncompr, uncomprLen)
030400000000    Byte *compr, *uncompr;
030500000000    uLong comprLen, uncomprLen;
030600000000{
030700000000    int err;
030800000000    z_stream d_stream; /* decompression stream */
030900000000
031000000000    strcpy((char*)uncompr, "garbage");
031100000000
031200000000    d_stream.zalloc = (alloc_func)0;
031300000000    d_stream.zfree = (free_func)0;
031400000000    d_stream.opaque = (voidpf)0;
031500000000
031600000000    d_stream.next_in  = compr;
031700000000    d_stream.avail_in = (uInt)comprLen;
031800000000
031900000000    err = inflateInit(&d_stream);
032000000000    CHECK_ERR(err, "inflateInit");
032100000000
032200000000    for (;;) {
032300000000        d_stream.next_out = uncompr;            /* discard the output */
032400000000        d_stream.avail_out = (uInt)uncomprLen;
032500000000        err = inflate(&d_stream, Z_NO_FLUSH);
032600000000        if (err == Z_STREAM_END) break;
032700000000        CHECK_ERR(err, "large inflate");
032800000000    }
032900000000
033000000000    err = inflateEnd(&d_stream);
033100000000    CHECK_ERR(err, "inflateEnd");
033200000000
033300000000    if (d_stream.total_out != 2*uncomprLen + comprLen/2) {
033400000000        fprintf(stderr, "bad large inflate: %ld\n", d_stream.total_out);
033500000000        exit(1);
033600000000    } else {
033700000000        printf("large_inflate(): OK\n");
033800000000    }
033900000000}
034000000000
034100000000/* ===========================================================================
034200000000 * Test deflate() with full flush
034300000000 */
034400000000void test_flush(compr, comprLen)
034500000000    Byte *compr;
034600000000    uLong *comprLen;
034700000000{
034800000000    z_stream c_stream; /* compression stream */
034900000000    int err;
035000000000    uInt len = (uInt)strlen(hello)+1;
035100000000
035200000000    c_stream.zalloc = (alloc_func)0;
035300000000    c_stream.zfree = (free_func)0;
035400000000    c_stream.opaque = (voidpf)0;
035500000000
035600000000    err = deflateInit(&c_stream, Z_DEFAULT_COMPRESSION);
035700000000    CHECK_ERR(err, "deflateInit");
035800000000
035900000000    c_stream.next_in  = (Bytef*)hello;
036000000000    c_stream.next_out = compr;
036100000000    c_stream.avail_in = 3;
036200000000    c_stream.avail_out = (uInt)*comprLen;
036300000000    err = deflate(&c_stream, Z_FULL_FLUSH);
036400000000    CHECK_ERR(err, "deflate");
036500000000
036600000000    compr[3]++; /* force an error in first compressed block */
036700000000    c_stream.avail_in = len - 3;
036800000000
036900000000    err = deflate(&c_stream, Z_FINISH);
037000000000    if (err != Z_STREAM_END) {
037100000000        CHECK_ERR(err, "deflate");
037200000000    }
037300000000    err = deflateEnd(&c_stream);
037400000000    CHECK_ERR(err, "deflateEnd");
037500000000
037600000000    *comprLen = c_stream.total_out;
037700000000}
037800000000
037900000000/* ===========================================================================
038000000000 * Test inflateSync()
038100000000 */
038200000000void test_sync(compr, comprLen, uncompr, uncomprLen)
038300000000    Byte *compr, *uncompr;
038400000000    uLong comprLen, uncomprLen;
038500000000{
038600000000    int err;
038700000000    z_stream d_stream; /* decompression stream */
038800000000
038900000000    strcpy((char*)uncompr, "garbage");
039000000000
039100000000    d_stream.zalloc = (alloc_func)0;
039200000000    d_stream.zfree = (free_func)0;
039300000000    d_stream.opaque = (voidpf)0;
039400000000
039500000000    d_stream.next_in  = compr;
039600000000    d_stream.avail_in = 2; /* just read the zlib header */
039700000000
039800000000    err = inflateInit(&d_stream);
039900000000    CHECK_ERR(err, "inflateInit");
040000000000
040100000000    d_stream.next_out = uncompr;
040200000000    d_stream.avail_out = (uInt)uncomprLen;
040300000000
040400000000    inflate(&d_stream, Z_NO_FLUSH);
040500000000    CHECK_ERR(err, "inflate");
040600000000
040700000000    d_stream.avail_in = (uInt)comprLen-2;   /* read all compressed data */
040800000000    err = inflateSync(&d_stream);           /* but skip the damaged part */
040900000000    CHECK_ERR(err, "inflateSync");
041000000000
041100000000    err = inflate(&d_stream, Z_FINISH);
041200000000    if (err != Z_DATA_ERROR) {
041300000000        fprintf(stderr, "inflate should report DATA_ERROR\n");
041400000000        /* Because of incorrect adler32 */
041500000000        exit(1);
041600000000    }
041700000000    err = inflateEnd(&d_stream);
041800000000    CHECK_ERR(err, "inflateEnd");
041900000000
042000000000    printf("after inflateSync(): hel%s\n", (char *)uncompr);
042100000000}
042200000000
042300000000/* ===========================================================================
042400000000 * Test deflate() with preset dictionary
042500000000 */
042600000000void test_dict_deflate(compr, comprLen)
042700000000    Byte *compr;
042800000000    uLong comprLen;
042900000000{
043000000000    z_stream c_stream; /* compression stream */
043100000000    int err;
043200000000
043300000000    c_stream.zalloc = (alloc_func)0;
043400000000    c_stream.zfree = (free_func)0;
043500000000    c_stream.opaque = (voidpf)0;
043600000000
043700000000    err = deflateInit(&c_stream, Z_BEST_COMPRESSION);
043800000000    CHECK_ERR(err, "deflateInit");
043900000000
044000000000    err = deflateSetDictionary(&c_stream,
044100000000                               (const Bytef*)dictionary, sizeof(dictionary));
044200000000    CHECK_ERR(err, "deflateSetDictionary");
044300000000
044400000000    dictId = c_stream.adler;
044500000000    c_stream.next_out = compr;
044600000000    c_stream.avail_out = (uInt)comprLen;
044700000000
044800000000    c_stream.next_in = (Bytef*)hello;
044900000000    c_stream.avail_in = (uInt)strlen(hello)+1;
045000000000
045100000000    err = deflate(&c_stream, Z_FINISH);
045200000000    if (err != Z_STREAM_END) {
045300000000        fprintf(stderr, "deflate should report Z_STREAM_END\n");
045400000000        exit(1);
045500000000    }
045600000000    err = deflateEnd(&c_stream);
045700000000    CHECK_ERR(err, "deflateEnd");
045800000000}
045900000000
046000000000/* ===========================================================================
046100000000 * Test inflate() with a preset dictionary
046200000000 */
046300000000void test_dict_inflate(compr, comprLen, uncompr, uncomprLen)
046400000000    Byte *compr, *uncompr;
046500000000    uLong comprLen, uncomprLen;
046600000000{
046700000000    int err;
046800000000    z_stream d_stream; /* decompression stream */
046900000000
047000000000    strcpy((char*)uncompr, "garbage");
047100000000
047200000000    d_stream.zalloc = (alloc_func)0;
047300000000    d_stream.zfree = (free_func)0;
047400000000    d_stream.opaque = (voidpf)0;
047500000000
047600000000    d_stream.next_in  = compr;
047700000000    d_stream.avail_in = (uInt)comprLen;
047800000000
047900000000    err = inflateInit(&d_stream);
048000000000    CHECK_ERR(err, "inflateInit");
048100000000
048200000000    d_stream.next_out = uncompr;
048300000000    d_stream.avail_out = (uInt)uncomprLen;
048400000000
048500000000    for (;;) {
048600000000        err = inflate(&d_stream, Z_NO_FLUSH);
048700000000        if (err == Z_STREAM_END) break;
048800000000        if (err == Z_NEED_DICT) {
048900000000            if (d_stream.adler != dictId) {
049000000000                fprintf(stderr, "unexpected dictionary");
049100000000                exit(1);
049200000000            }
049300000000            err = inflateSetDictionary(&d_stream, (const Bytef*)dictionary,
049400000000                                       sizeof(dictionary));
049500000000        }
049600000000        CHECK_ERR(err, "inflate with dict");
049700000000    }
049800000000
049900000000    err = inflateEnd(&d_stream);
050000000000    CHECK_ERR(err, "inflateEnd");
050100000000
050200000000    if (strcmp((char*)uncompr, hello)) {
050300000000        fprintf(stderr, "bad inflate with dict\n");
050400000000        exit(1);
050500000000    } else {
050600000000        printf("inflate with dictionary: %s\n", (char *)uncompr);
050700000000    }
050800000000}
050900000000
051000000000/* ===========================================================================
051100000000 * Usage:  example [output.gz  [input.gz]]
051200000000 */
051300000000
051400000000int main(argc, argv)
051500000000    int argc;
051600000000    char *argv[];
051700000000{
051800000000    Byte *compr, *uncompr;
051900000000    uLong comprLen = 10000*sizeof(int); /* don't overflow on MSDOS */
052000000000    uLong uncomprLen = comprLen;
052100000000    static const char* myVersion = ZLIB_VERSION;
052200000000
052300000000    if (zlibVersion()[0] != myVersion[0]) {
052400000000        fprintf(stderr, "incompatible zlib version\n");
052500000000        exit(1);
052600000000
052700000000    } else if (strcmp(zlibVersion(), ZLIB_VERSION) != 0) {
052800000000        fprintf(stderr, "warning: different zlib version\n");
052900000000    }
053000000000
053100000000    printf("zlib version %s = 0x%04x, compile flags = 0x%lx\n",
053200000000            ZLIB_VERSION, ZLIB_VERNUM, zlibCompileFlags());
053300000000
053400000000    compr    = (Byte*)calloc((uInt)comprLen, 1);
053500000000    uncompr  = (Byte*)calloc((uInt)uncomprLen, 1);
053600000000    /* compr and uncompr are cleared to avoid reading uninitialized
053700000000     * data and to ensure that uncompr compresses well.
053800000000     */
053900000000    if (compr == Z_NULL || uncompr == Z_NULL) {
054000000000        printf("out of memory\n");
054100000000        exit(1);
054200000000    }
054300000000    test_compress(compr, comprLen, uncompr, uncomprLen);
054400000000
054500000000    test_gzio((argc > 1 ? argv[1] : TESTFILE),
054600000000              uncompr, uncomprLen);
054700000000
054800000000    test_deflate(compr, comprLen);
054900000000    test_inflate(compr, comprLen, uncompr, uncomprLen);
055000000000
055100000000    test_large_deflate(compr, comprLen, uncompr, uncomprLen);
055200000000    test_large_inflate(compr, comprLen, uncompr, uncomprLen);
055300000000
055400000000    test_flush(compr, &comprLen);
055500000000    test_sync(compr, comprLen, uncompr, uncomprLen);
055600000000    comprLen = uncomprLen;
055700000000
055800000000    test_dict_deflate(compr, comprLen);
055900000000    test_dict_inflate(compr, comprLen, uncompr, uncomprLen);
056000000000
056100000000    free(compr);
056200000000    free(uncompr);
056300000000
056400000000    return 0;
056500000000}
