000100000000/* deflate.c -- compress data using the deflation algorithm
000200000000 * Copyright (C) 1995-2005 Jean-loup Gailly.
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/*
000700000000 *  ALGORITHM
000800000000 *
000900000000 *      The "deflation" process depends on being able to identify portions
001000000000 *      of the input text which are identical to earlier input (within a
001100000000 *      sliding window trailing behind the input currently being processed).
001200000000 *
001300000000 *      The most straightforward technique turns out to be the fastest for
001400000000 *      most input files: try all possible matches and select the longest.
001500000000 *      The key feature of this algorithm is that insertions into the string
001600000000 *      dictionary are very simple and thus fast, and deletions are avoided
001700000000 *      completely. Insertions are performed at each input character, whereas
001800000000 *      string matches are performed only when the previous match ends. So it
001900000000 *      is preferable to spend more time in matches to allow very fast string
002000000000 *      insertions and avoid deletions. The matching algorithm for small
002100000000 *      strings is inspired from that of Rabin & Karp. A brute force approach
002200000000 *      is used to find longer strings when a small match has been found.
002300000000 *      A similar algorithm is used in comic (by Jan-Mark Wams) and freeze
002400000000 *      (by Leonid Broukhis).
002500000000 *         A previous version of this file used a more sophisticated algorithm
002600000000 *      (by Fiala and Greene) which is guaranteed to run in linear amortized
002700000000 *      time, but has a larger average cost, uses more memory and is patented.
002800000000 *      However the F&G algorithm may be faster for some highly redundant
002900000000 *      files if the parameter max_chain_length (described below) is too large.
003000000000 *
003100000000 *  ACKNOWLEDGEMENTS
003200000000 *
003300000000 *      The idea of lazy evaluation of matches is due to Jan-Mark Wams, and
003400000000 *      I found it in 'freeze' written by Leonid Broukhis.
003500000000 *      Thanks to many people for bug reports and testing.
003600000000 *
003700000000 *  REFERENCES
003800000000 *
003900000000 *      Deutsch, L.P.,"DEFLATE Compressed Data Format Specification".
004000000000 *      Available in http://www.ietf.org/rfc/rfc1951.txt
004100000000 *
004200000000 *      A description of the Rabin and Karp algorithm is given in the book
004300000000 *         "Algorithms" by R. Sedgewick, Addison-Wesley, p252.
004400000000 *
004500000000 *      Fiala,E.R., and Greene,D.H.
004600000000 *         Data Compression with Finite Windows, Comm.ACM, 32,4 (1989) 490-595
004700000000 *
004800000000 */
004900000000
005000000000/* @(#) $Id$ */
005100000000
005200000000#include "deflate.h"
005300000000
005400000000const char deflate_copyright[] =
005500000000   " deflate 1.2.3 Copyright 1995-2005 Jean-loup Gailly ";
005600000000/*
005700000000  If you use the zlib library in a product, an acknowledgment is welcome
005800000000  in the documentation of your product. If for some reason you cannot
005900000000  include such an acknowledgment, I would appreciate that you keep this
006000000000  copyright string in the executable of your product.
006100000000 */
006200000000
006300000000/* ===========================================================================
006400000000 *  Function prototypes.
006500000000 */
006600000000typedef enum {
006700000000    need_more,      /* block not completed, need more input or more output */
006800000000    block_done,     /* block flush performed */
006900000000    finish_started, /* finish started, need only more output at next deflate */
007000000000    finish_done     /* finish done, accept no more input or output */
007100000000} block_state;
007200000000
007300000000typedef block_state (*compress_func) OF((deflate_state *s, int flush));
007400000000/* Compression function. Returns the block state after the call. */
007500000000
007600000000local void fill_window    OF((deflate_state *s));
007700000000local block_state deflate_stored OF((deflate_state *s, int flush));
007800000000local block_state deflate_fast   OF((deflate_state *s, int flush));
007900000000#ifndef FASTEST
008000000000local block_state deflate_slow   OF((deflate_state *s, int flush));
008100000000#endif
008200000000local void lm_init        OF((deflate_state *s));
008300000000local void putShortMSB    OF((deflate_state *s, uInt b));
008400000000local void flush_pending  OF((z_streamp strm));
008500000000local int read_buf        OF((z_streamp strm, Bytef *buf, unsigned size));
008600000000#ifndef FASTEST
008700000000#ifdef ASMV
008800000000      void match_init OF((void)); /* asm code initialization */
008900000000      uInt longest_match  OF((deflate_state *s, IPos cur_match));
009000000000#else
009100000000local uInt longest_match  OF((deflate_state *s, IPos cur_match));
009200000000#endif
009300000000#endif
009400000000local uInt longest_match_fast OF((deflate_state *s, IPos cur_match));
009500000000
009600000000#ifdef DEBUG
009700000000local  void check_match OF((deflate_state *s, IPos start, IPos match,
009800000000                            int length));
009900000000#endif
010000000000
010100000000/* ===========================================================================
010200000000 * Local data
010300000000 */
010400000000
010500000000#define NIL 0
010600000000/* Tail of hash chains */
010700000000
010800000000#ifndef TOO_FAR
010900000000#  define TOO_FAR 4096
011000000000#endif
011100000000/* Matches of length 3 are discarded if their distance exceeds TOO_FAR */
011200000000
011300000000#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
011400000000/* Minimum amount of lookahead, except at the end of the input file.
011500000000 * See deflate.c for comments about the MIN_MATCH+1.
011600000000 */
011700000000
011800000000/* Values for max_lazy_match, good_match and max_chain_length, depending on
011900000000 * the desired pack level (0..9). The values given below have been tuned to
012000000000 * exclude worst case performance for pathological files. Better values may be
012100000000 * found for specific files.
012200000000 */
012300000000typedef struct config_s {
012400000000   ush good_length; /* reduce lazy search above this match length */
012500000000   ush max_lazy;    /* do not perform lazy search above this match length */
012600000000   ush nice_length; /* quit search above this match length */
012700000000   ush max_chain;
012800000000   compress_func func;
012900000000} config;
013000000000
013100000000#ifdef FASTEST
013200000000local const config configuration_table[2] = {
013300000000/*      good lazy nice chain */
013400000000/* 0 */ {0,    0,  0,    0, deflate_stored},  /* store only */
013500000000/* 1 */ {4,    4,  8,    4, deflate_fast}}; /* max speed, no lazy matches */
013600000000#else
013700000000local const config configuration_table[10] = {
013800000000/*      good lazy nice chain */
013900000000/* 0 */ {0,    0,  0,    0, deflate_stored},  /* store only */
014000000000/* 1 */ {4,    4,  8,    4, deflate_fast}, /* max speed, no lazy matches */
014100000000/* 2 */ {4,    5, 16,    8, deflate_fast},
014200000000/* 3 */ {4,    6, 32,   32, deflate_fast},
014300000000
014400000000/* 4 */ {4,    4, 16,   16, deflate_slow},  /* lazy matches */
014500000000/* 5 */ {8,   16, 32,   32, deflate_slow},
014600000000/* 6 */ {8,   16, 128, 128, deflate_slow},
014700000000/* 7 */ {8,   32, 128, 256, deflate_slow},
014800000000/* 8 */ {32, 128, 258, 1024, deflate_slow},
014900000000/* 9 */ {32, 258, 258, 4096, deflate_slow}}; /* max compression */
015000000000#endif
015100000000
015200000000/* Note: the deflate() code requires max_lazy >= MIN_MATCH and max_chain >= 4
015300000000 * For deflate_fast() (levels <= 3) good is ignored and lazy has a different
015400000000 * meaning.
015500000000 */
015600000000
015700000000#define EQUAL 0
015800000000/* result of memcmp for equal strings */
015900000000
016000000000#ifndef NO_DUMMY_DECL
016100000000struct static_tree_desc_s {int dummy;}; /* for buggy compilers */
016200000000#endif
016300000000
016400000000/* ===========================================================================
016500000000 * Update a hash value with the given input byte
016600000000 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
016700000000 *    input characters, so that a running hash key can be computed from the
016800000000 *    previous key instead of complete recalculation each time.
016900000000 */
017000000000#define UPDATE_HASH(s,h,c) (h = (((h)<<s->hash_shift) ^ (c)) & s->hash_mask)
017100000000
017200000000
017300000000/* ===========================================================================
017400000000 * Insert string str in the dictionary and set match_head to the previous head
017500000000 * of the hash chain (the most recent string with same hash key). Return
017600000000 * the previous length of the hash chain.
017700000000 * If this file is compiled with -DFASTEST, the compression level is forced
017800000000 * to 1, and no hash chains are maintained.
017900000000 * IN  assertion: all calls to to INSERT_STRING are made with consecutive
018000000000 *    input characters and the first MIN_MATCH bytes of str are valid
018100000000 *    (except for the last MIN_MATCH-1 bytes of the input file).
018200000000 */
018300000000#ifdef FASTEST
018400000000#define INSERT_STRING(s, str, match_head) \
018500000000   (UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)]), \
018600000000    match_head = s->head[s->ins_h], \
018700000000    s->head[s->ins_h] = (Pos)(str))
018800000000#else
018900000000#define INSERT_STRING(s, str, match_head) \
019000000000   (UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)]), \
019100000000    match_head = s->prev[(str) & s->w_mask] = s->head[s->ins_h], \
019200000000    s->head[s->ins_h] = (Pos)(str))
019300000000#endif
019400000000
019500000000/* ===========================================================================
019600000000 * Initialize the hash table (avoiding 64K overflow for 16 bit systems).
019700000000 * prev[] will be initialized on the fly.
019800000000 */
019900000000#define CLEAR_HASH(s) \
020000000000    s->head[s->hash_size-1] = NIL; \
020100000000    zmemzero((Bytef *)s->head, (unsigned)(s->hash_size-1)*sizeof(*s->head));
020200000000
020300000000/* ========================================================================= */
020400000000int ZEXPORT deflateInit_(strm, level, version, stream_size)
020500000000    z_streamp strm;
020600000000    int level;
020700000000    const char *version;
020800000000    int stream_size;
020900000000{
021000000000    return deflateInit2_(strm, level, Z_DEFLATED, MAX_WBITS, DEF_MEM_LEVEL,
021100000000                         Z_DEFAULT_STRATEGY, version, stream_size);
021200000000    /* To do: ignore strm->next_in if we use it as window */
021300000000}
021400000000
021500000000/* ========================================================================= */
021600000000int ZEXPORT deflateInit2_(strm, level, method, windowBits, memLevel, strategy,
021700000000                  version, stream_size)
021800000000    z_streamp strm;
021900000000    int  level;
022000000000    int  method;
022100000000    int  windowBits;
022200000000    int  memLevel;
022300000000    int  strategy;
022400000000    const char *version;
022500000000    int stream_size;
022600000000{
022700000000    deflate_state *s;
022800000000    int wrap = 1;
022900000000    static const char my_version[] = ZLIB_VERSION;
023000000000
023100000000    ushf *overlay;
023200000000    /* We overlay pending_buf and d_buf+l_buf. This works since the average
023300000000     * output size for (length,distance) codes is <= 24 bits.
023400000000     */
023500000000
023600000000    if (version == Z_NULL || version[0] != my_version[0] ||
023700000000        stream_size != sizeof(z_stream)) {
023800000000        return Z_VERSION_ERROR;
023900000000    }
024000000000    if (strm == Z_NULL) return Z_STREAM_ERROR;
024100000000
024200000000    strm->msg = Z_NULL;
024300000000    if (strm->zalloc == (alloc_func)0) {
024400000000        strm->zalloc = zcalloc;
024500000000        strm->opaque = (voidpf)0;
024600000000    }
024700000000    if (strm->zfree == (free_func)0) strm->zfree = zcfree;
024800000000
024900000000#ifdef FASTEST
025000000000    if (level != 0) level = 1;
025100000000#else
025200000000    if (level == Z_DEFAULT_COMPRESSION) level = 6;
025300000000#endif
025400000000
025500000000    if (windowBits < 0) { /* suppress zlib wrapper */
025600000000        wrap = 0;
025700000000        windowBits = -windowBits;
025800000000    }
025900000000#ifdef GZIP
026000000000    else if (windowBits > 15) {
026100000000        wrap = 2;       /* write gzip wrapper instead */
026200000000        windowBits -= 16;
026300000000    }
026400000000#endif
026500000000    if (memLevel < 1 || memLevel > MAX_MEM_LEVEL || method != Z_DEFLATED ||
026600000000        windowBits < 8 || windowBits > 15 || level < 0 || level > 9 ||
026700000000        strategy < 0 || strategy > Z_FIXED) {
026800000000        return Z_STREAM_ERROR;
026900000000    }
027000000000    if (windowBits == 8) windowBits = 9;  /* until 256-byte window bug fixed */
027100000000    s = (deflate_state *) ZALLOC(strm, 1, sizeof(deflate_state));
027200000000    if (s == Z_NULL) return Z_MEM_ERROR;
027300000000    strm->state = (struct internal_state FAR *)s;
027400000000    s->strm = strm;
027500000000
027600000000    s->wrap = wrap;
027700000000    s->gzhead = Z_NULL;
027800000000    s->w_bits = windowBits;
027900000000    s->w_size = 1 << s->w_bits;
028000000000    s->w_mask = s->w_size - 1;
028100000000
028200000000    s->hash_bits = memLevel + 7;
028300000000    s->hash_size = 1 << s->hash_bits;
028400000000    s->hash_mask = s->hash_size - 1;
028500000000    s->hash_shift =  ((s->hash_bits+MIN_MATCH-1)/MIN_MATCH);
028600000000
028700000000    s->window = (Bytef *) ZALLOC(strm, s->w_size, 2*sizeof(Byte));
028800000000    s->prev   = (Posf *)  ZALLOC(strm, s->w_size, sizeof(Pos));
028900000000    s->head   = (Posf *)  ZALLOC(strm, s->hash_size, sizeof(Pos));
029000000000
029100000000    s->lit_bufsize = 1 << (memLevel + 6); /* 16K elements by default */
029200000000
029300000000    overlay = (ushf *) ZALLOC(strm, s->lit_bufsize, sizeof(ush)+2);
029400000000    s->pending_buf = (uchf *) overlay;
029500000000    s->pending_buf_size = (ulg)s->lit_bufsize * (sizeof(ush)+2L);
029600000000
029700000000    if (s->window == Z_NULL || s->prev == Z_NULL || s->head == Z_NULL ||
029800000000        s->pending_buf == Z_NULL) {
029900000000        s->status = FINISH_STATE;
030000000000        strm->msg = (char*)ERR_MSG(Z_MEM_ERROR);
030100000000        deflateEnd (strm);
030200000000        return Z_MEM_ERROR;
030300000000    }
030400000000    s->d_buf = overlay + s->lit_bufsize/sizeof(ush);
030500000000    s->l_buf = s->pending_buf + (1+sizeof(ush))*s->lit_bufsize;
030600000000
030700000000    s->level = level;
030800000000    s->strategy = strategy;
030900000000    s->method = (Byte)method;
031000000000
031100000000    return deflateReset(strm);
031200000000}
031300000000
031400000000/* ========================================================================= */
031500000000int ZEXPORT deflateSetDictionary (strm, dictionary, dictLength)
031600000000    z_streamp strm;
031700000000    const Bytef *dictionary;
031800000000    uInt  dictLength;
031900000000{
032000000000    deflate_state *s;
032100000000    uInt length = dictLength;
032200000000    uInt n;
032300000000    IPos hash_head = 0;
032400000000
032500000000    if (strm == Z_NULL || strm->state == Z_NULL || dictionary == Z_NULL ||
032600000000        strm->state->wrap == 2 ||
032700000000        (strm->state->wrap == 1 && strm->state->status != INIT_STATE))
032800000000        return Z_STREAM_ERROR;
032900000000
033000000000    s = strm->state;
033100000000    if (s->wrap)
033200000000        strm->adler = adler32(strm->adler, dictionary, dictLength);
033300000000
033400000000    if (length < MIN_MATCH) return Z_OK;
033500000000    if (length > MAX_DIST(s)) {
033600000000        length = MAX_DIST(s);
033700000000        dictionary += dictLength - length; /* use the tail of the dictionary */
033800000000    }
033900000000    zmemcpy(s->window, dictionary, length);
034000000000    s->strstart = length;
034100000000    s->block_start = (long)length;
034200000000
034300000000    /* Insert all strings in the hash table (except for the last two bytes).
034400000000     * s->lookahead stays null, so s->ins_h will be recomputed at the next
034500000000     * call of fill_window.
034600000000     */
034700000000    s->ins_h = s->window[0];
034800000000    UPDATE_HASH(s, s->ins_h, s->window[1]);
034900000000    for (n = 0; n <= length - MIN_MATCH; n++) {
035000000000        INSERT_STRING(s, n, hash_head);
035100000000    }
035200000000    if (hash_head) hash_head = 0;  /* to make compiler happy */
035300000000    return Z_OK;
035400000000}
035500000000
035600000000/* ========================================================================= */
035700000000int ZEXPORT deflateReset (strm)
035800000000    z_streamp strm;
035900000000{
036000000000    deflate_state *s;
036100000000
036200000000    if (strm == Z_NULL || strm->state == Z_NULL ||
036300000000        strm->zalloc == (alloc_func)0 || strm->zfree == (free_func)0) {
036400000000        return Z_STREAM_ERROR;
036500000000    }
036600000000
036700000000    strm->total_in = strm->total_out = 0;
036800000000    strm->msg = Z_NULL; /* use zfree if we ever allocate msg dynamically */
036900000000    strm->data_type = Z_UNKNOWN;
037000000000
037100000000    s = (deflate_state *)strm->state;
037200000000    s->pending = 0;
037300000000    s->pending_out = s->pending_buf;
037400000000
037500000000    if (s->wrap < 0) {
037600000000        s->wrap = -s->wrap; /* was made negative by deflate(..., Z_FINISH); */
037700000000    }
037800000000    s->status = s->wrap ? INIT_STATE : BUSY_STATE;
037900000000    strm->adler =
038000000000#ifdef GZIP
038100000000        s->wrap == 2 ? crc32(0L, Z_NULL, 0) :
038200000000#endif
038300000000        adler32(0L, Z_NULL, 0);
038400000000    s->last_flush = Z_NO_FLUSH;
038500000000
038600000000    _tr_init(s);
038700000000    lm_init(s);
038800000000
038900000000    return Z_OK;
039000000000}
039100000000
039200000000/* ========================================================================= */
039300000000int ZEXPORT deflateSetHeader (strm, head)
039400000000    z_streamp strm;
039500000000    gz_headerp head;
039600000000{
039700000000    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
039800000000    if (strm->state->wrap != 2) return Z_STREAM_ERROR;
039900000000    strm->state->gzhead = head;
040000000000    return Z_OK;
040100000000}
040200000000
040300000000/* ========================================================================= */
040400000000int ZEXPORT deflatePrime (strm, bits, value)
040500000000    z_streamp strm;
040600000000    int bits;
040700000000    int value;
040800000000{
040900000000    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
041000000000    strm->state->bi_valid = bits;
041100000000    strm->state->bi_buf = (ush)(value & ((1 << bits) - 1));
041200000000    return Z_OK;
041300000000}
041400000000
041500000000/* ========================================================================= */
041600000000int ZEXPORT deflateParams(strm, level, strategy)
041700000000    z_streamp strm;
041800000000    int level;
041900000000    int strategy;
042000000000{
042100000000    deflate_state *s;
042200000000    compress_func func;
042300000000    int err = Z_OK;
042400000000
042500000000    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
042600000000    s = strm->state;
042700000000
042800000000#ifdef FASTEST
042900000000    if (level != 0) level = 1;
043000000000#else
043100000000    if (level == Z_DEFAULT_COMPRESSION) level = 6;
043200000000#endif
043300000000    if (level < 0 || level > 9 || strategy < 0 || strategy > Z_FIXED) {
043400000000        return Z_STREAM_ERROR;
043500000000    }
043600000000    func = configuration_table[s->level].func;
043700000000
043800000000    if (func != configuration_table[level].func && strm->total_in != 0) {
043900000000        /* Flush the last buffer: */
044000000000        err = deflate(strm, Z_PARTIAL_FLUSH);
044100000000    }
044200000000    if (s->level != level) {
044300000000        s->level = level;
044400000000        s->max_lazy_match   = configuration_table[level].max_lazy;
044500000000        s->good_match       = configuration_table[level].good_length;
044600000000        s->nice_match       = configuration_table[level].nice_length;
044700000000        s->max_chain_length = configuration_table[level].max_chain;
044800000000    }
044900000000    s->strategy = strategy;
045000000000    return err;
045100000000}
045200000000
045300000000/* ========================================================================= */
045400000000int ZEXPORT deflateTune(strm, good_length, max_lazy, nice_length, max_chain)
045500000000    z_streamp strm;
045600000000    int good_length;
045700000000    int max_lazy;
045800000000    int nice_length;
045900000000    int max_chain;
046000000000{
046100000000    deflate_state *s;
046200000000
046300000000    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
046400000000    s = strm->state;
046500000000    s->good_match = good_length;
046600000000    s->max_lazy_match = max_lazy;
046700000000    s->nice_match = nice_length;
046800000000    s->max_chain_length = max_chain;
046900000000    return Z_OK;
047000000000}
047100000000
047200000000/* =========================================================================
047300000000 * For the default windowBits of 15 and memLevel of 8, this function returns
047400000000 * a close to exact, as well as small, upper bound on the compressed size.
047500000000 * They are coded as constants here for a reason--if the #define's are
047600000000 * changed, then this function needs to be changed as well.  The return
047700000000 * value for 15 and 8 only works for those exact settings.
047800000000 *
047900000000 * For any setting other than those defaults for windowBits and memLevel,
048000000000 * the value returned is a conservative worst case for the maximum expansion
048100000000 * resulting from using fixed blocks instead of stored blocks, which deflate
048200000000 * can emit on compressed data for some combinations of the parameters.
048300000000 *
048400000000 * This function could be more sophisticated to provide closer upper bounds
048500000000 * for every combination of windowBits and memLevel, as well as wrap.
048600000000 * But even the conservative upper bound of about 14% expansion does not
048700000000 * seem onerous for output buffer allocation.
048800000000 */
048900000000uLong ZEXPORT deflateBound(strm, sourceLen)
049000000000    z_streamp strm;
049100000000    uLong sourceLen;
049200000000{
049300000000    deflate_state *s;
049400000000    uLong destLen;
049500000000
049600000000    /* conservative upper bound */
049700000000    destLen = sourceLen +
049800000000              ((sourceLen + 7) >> 3) + ((sourceLen + 63) >> 6) + 11;
049900000000
050000000000    /* if can't get parameters, return conservative bound */
050100000000    if (strm == Z_NULL || strm->state == Z_NULL)
050200000000        return destLen;
050300000000
050400000000    /* if not default parameters, return conservative bound */
050500000000    s = strm->state;
050600000000    if (s->w_bits != 15 || s->hash_bits != 8 + 7)
050700000000        return destLen;
050800000000
050900000000    /* default settings: return tight bound for that case */
051000000000    return compressBound(sourceLen);
051100000000}
051200000000
051300000000/* =========================================================================
051400000000 * Put a short in the pending buffer. The 16-bit value is put in MSB order.
051500000000 * IN assertion: the stream state is correct and there is enough room in
051600000000 * pending_buf.
051700000000 */
051800000000local void putShortMSB (s, b)
051900000000    deflate_state *s;
052000000000    uInt b;
052100000000{
052200000000    put_byte(s, (Byte)(b >> 8));
052300000000    put_byte(s, (Byte)(b & 0xff));
052400000000}
052500000000
052600000000/* =========================================================================
052700000000 * Flush as much pending output as possible. All deflate() output goes
052800000000 * through this function so some applications may wish to modify it
052900000000 * to avoid allocating a large strm->next_out buffer and copying into it.
053000000000 * (See also read_buf()).
053100000000 */
053200000000local void flush_pending(strm)
053300000000    z_streamp strm;
053400000000{
053500000000    unsigned len = strm->state->pending;
053600000000
053700000000    if (len > strm->avail_out) len = strm->avail_out;
053800000000    if (len == 0) return;
053900000000
054000000000    zmemcpy(strm->next_out, strm->state->pending_out, len);
054100000000    strm->next_out  += len;
054200000000    strm->state->pending_out  += len;
054300000000    strm->total_out += len;
054400000000    strm->avail_out  -= len;
054500000000    strm->state->pending -= len;
054600000000    if (strm->state->pending == 0) {
054700000000        strm->state->pending_out = strm->state->pending_buf;
054800000000    }
054900000000}
055000000000
055100000000/* ========================================================================= */
055200000000int ZEXPORT deflate (strm, flush)
055300000000    z_streamp strm;
055400000000    int flush;
055500000000{
055600000000    int old_flush; /* value of flush param for previous deflate call */
055700000000    deflate_state *s;
055800000000
055900000000    if (strm == Z_NULL || strm->state == Z_NULL ||
056000000000        flush > Z_FINISH || flush < 0) {
056100000000        return Z_STREAM_ERROR;
056200000000    }
056300000000    s = strm->state;
056400000000
056500000000    if (strm->next_out == Z_NULL ||
056600000000        (strm->next_in == Z_NULL && strm->avail_in != 0) ||
056700000000        (s->status == FINISH_STATE && flush != Z_FINISH)) {
056800000000        ERR_RETURN(strm, Z_STREAM_ERROR);
056900000000    }
057000000000    if (strm->avail_out == 0) ERR_RETURN(strm, Z_BUF_ERROR);
057100000000
057200000000    s->strm = strm; /* just in case */
057300000000    old_flush = s->last_flush;
057400000000    s->last_flush = flush;
057500000000
057600000000    /* Write the header */
057700000000    if (s->status == INIT_STATE) {
057800000000#ifdef GZIP
057900000000        if (s->wrap == 2) {
058000000000            strm->adler = crc32(0L, Z_NULL, 0);
058100000000            put_byte(s, 31);
058200000000            put_byte(s, 139);
058300000000            put_byte(s, 8);
058400000000            if (s->gzhead == NULL) {
058500000000                put_byte(s, 0);
058600000000                put_byte(s, 0);
058700000000                put_byte(s, 0);
058800000000                put_byte(s, 0);
058900000000                put_byte(s, 0);
059000000000                put_byte(s, s->level == 9 ? 2 :
059100000000                            (s->strategy >= Z_HUFFMAN_ONLY || s->level < 2 ?
059200000000                             4 : 0));
059300000000                put_byte(s, OS_CODE);
059400000000                s->status = BUSY_STATE;
059500000000            }
059600000000            else {
059700000000                put_byte(s, (s->gzhead->text ? 1 : 0) +
059800000000                            (s->gzhead->hcrc ? 2 : 0) +
059900000000                            (s->gzhead->extra == Z_NULL ? 0 : 4) +
060000000000                            (s->gzhead->name == Z_NULL ? 0 : 8) +
060100000000                            (s->gzhead->comment == Z_NULL ? 0 : 16)
060200000000                        );
060300000000                put_byte(s, (Byte)(s->gzhead->time & 0xff));
060400000000                put_byte(s, (Byte)((s->gzhead->time >> 8) & 0xff));
060500000000                put_byte(s, (Byte)((s->gzhead->time >> 16) & 0xff));
060600000000                put_byte(s, (Byte)((s->gzhead->time >> 24) & 0xff));
060700000000                put_byte(s, s->level == 9 ? 2 :
060800000000                            (s->strategy >= Z_HUFFMAN_ONLY || s->level < 2 ?
060900000000                             4 : 0));
061000000000                put_byte(s, s->gzhead->os & 0xff);
061100000000                if (s->gzhead->extra != NULL) {
061200000000                    put_byte(s, s->gzhead->extra_len & 0xff);
061300000000                    put_byte(s, (s->gzhead->extra_len >> 8) & 0xff);
061400000000                }
061500000000                if (s->gzhead->hcrc)
061600000000                    strm->adler = crc32(strm->adler, s->pending_buf,
061700000000                                        s->pending);
061800000000                s->gzindex = 0;
061900000000                s->status = EXTRA_STATE;
062000000000            }
062100000000        }
062200000000        else
062300000000#endif
062400000000        {
062500000000            uInt header = (Z_DEFLATED + ((s->w_bits-8)<<4)) << 8;
062600000000            uInt level_flags;
062700000000
062800000000            if (s->strategy >= Z_HUFFMAN_ONLY || s->level < 2)
062900000000                level_flags = 0;
063000000000            else if (s->level < 6)
063100000000                level_flags = 1;
063200000000            else if (s->level == 6)
063300000000                level_flags = 2;
063400000000            else
063500000000                level_flags = 3;
063600000000            header |= (level_flags << 6);
063700000000            if (s->strstart != 0) header |= PRESET_DICT;
063800000000            header += 31 - (header % 31);
063900000000
064000000000            s->status = BUSY_STATE;
064100000000            putShortMSB(s, header);
064200000000
064300000000            /* Save the adler32 of the preset dictionary: */
064400000000            if (s->strstart != 0) {
064500000000                putShortMSB(s, (uInt)(strm->adler >> 16));
064600000000                putShortMSB(s, (uInt)(strm->adler & 0xffff));
064700000000            }
064800000000            strm->adler = adler32(0L, Z_NULL, 0);
064900000000        }
065000000000    }
065100000000#ifdef GZIP
065200000000    if (s->status == EXTRA_STATE) {
065300000000        if (s->gzhead->extra != NULL) {
065400000000            uInt beg = s->pending;  /* start of bytes to update crc */
065500000000
065600000000            while (s->gzindex < (s->gzhead->extra_len & 0xffff)) {
065700000000                if (s->pending == s->pending_buf_size) {
065800000000                    if (s->gzhead->hcrc && s->pending > beg)
065900000000                        strm->adler = crc32(strm->adler, s->pending_buf + beg,
066000000000                                            s->pending - beg);
066100000000                    flush_pending(strm);
066200000000                    beg = s->pending;
066300000000                    if (s->pending == s->pending_buf_size)
066400000000                        break;
066500000000                }
066600000000                put_byte(s, s->gzhead->extra[s->gzindex]);
066700000000                s->gzindex++;
066800000000            }
066900000000            if (s->gzhead->hcrc && s->pending > beg)
067000000000                strm->adler = crc32(strm->adler, s->pending_buf + beg,
067100000000                                    s->pending - beg);
067200000000            if (s->gzindex == s->gzhead->extra_len) {
067300000000                s->gzindex = 0;
067400000000                s->status = NAME_STATE;
067500000000            }
067600000000        }
067700000000        else
067800000000            s->status = NAME_STATE;
067900000000    }
068000000000    if (s->status == NAME_STATE) {
068100000000        if (s->gzhead->name != NULL) {
068200000000            uInt beg = s->pending;  /* start of bytes to update crc */
068300000000            int val;
068400000000
068500000000            do {
068600000000                if (s->pending == s->pending_buf_size) {
068700000000                    if (s->gzhead->hcrc && s->pending > beg)
068800000000                        strm->adler = crc32(strm->adler, s->pending_buf + beg,
068900000000                                            s->pending - beg);
069000000000                    flush_pending(strm);
069100000000                    beg = s->pending;
069200000000                    if (s->pending == s->pending_buf_size) {
069300000000                        val = 1;
069400000000                        break;
069500000000                    }
069600000000                }
069700000000                val = s->gzhead->name[s->gzindex++];
069800000000                put_byte(s, val);
069900000000            } while (val != 0);
070000000000            if (s->gzhead->hcrc && s->pending > beg)
070100000000                strm->adler = crc32(strm->adler, s->pending_buf + beg,
070200000000                                    s->pending - beg);
070300000000            if (val == 0) {
070400000000                s->gzindex = 0;
070500000000                s->status = COMMENT_STATE;
070600000000            }
070700000000        }
070800000000        else
070900000000            s->status = COMMENT_STATE;
071000000000    }
071100000000    if (s->status == COMMENT_STATE) {
071200000000        if (s->gzhead->comment != NULL) {
071300000000            uInt beg = s->pending;  /* start of bytes to update crc */
071400000000            int val;
071500000000
071600000000            do {
071700000000                if (s->pending == s->pending_buf_size) {
071800000000                    if (s->gzhead->hcrc && s->pending > beg)
071900000000                        strm->adler = crc32(strm->adler, s->pending_buf + beg,
072000000000                                            s->pending - beg);
072100000000                    flush_pending(strm);
072200000000                    beg = s->pending;
072300000000                    if (s->pending == s->pending_buf_size) {
072400000000                        val = 1;
072500000000                        break;
072600000000                    }
072700000000                }
072800000000                val = s->gzhead->comment[s->gzindex++];
072900000000                put_byte(s, val);
073000000000            } while (val != 0);
073100000000            if (s->gzhead->hcrc && s->pending > beg)
073200000000                strm->adler = crc32(strm->adler, s->pending_buf + beg,
073300000000                                    s->pending - beg);
073400000000            if (val == 0)
073500000000                s->status = HCRC_STATE;
073600000000        }
073700000000        else
073800000000            s->status = HCRC_STATE;
073900000000    }
074000000000    if (s->status == HCRC_STATE) {
074100000000        if (s->gzhead->hcrc) {
074200000000            if (s->pending + 2 > s->pending_buf_size)
074300000000                flush_pending(strm);
074400000000            if (s->pending + 2 <= s->pending_buf_size) {
074500000000                put_byte(s, (Byte)(strm->adler & 0xff));
074600000000                put_byte(s, (Byte)((strm->adler >> 8) & 0xff));
074700000000                strm->adler = crc32(0L, Z_NULL, 0);
074800000000                s->status = BUSY_STATE;
074900000000            }
075000000000        }
075100000000        else
075200000000            s->status = BUSY_STATE;
075300000000    }
075400000000#endif
075500000000
075600000000    /* Flush as much pending output as possible */
075700000000    if (s->pending != 0) {
075800000000        flush_pending(strm);
075900000000        if (strm->avail_out == 0) {
076000000000            /* Since avail_out is 0, deflate will be called again with
076100000000             * more output space, but possibly with both pending and
076200000000             * avail_in equal to zero. There won't be anything to do,
076300000000             * but this is not an error situation so make sure we
076400000000             * return OK instead of BUF_ERROR at next call of deflate:
076500000000             */
076600000000            s->last_flush = -1;
076700000000            return Z_OK;
076800000000        }
076900000000
077000000000    /* Make sure there is something to do and avoid duplicate consecutive
077100000000     * flushes. For repeated and useless calls with Z_FINISH, we keep
077200000000     * returning Z_STREAM_END instead of Z_BUF_ERROR.
077300000000     */
077400000000    } else if (strm->avail_in == 0 && flush <= old_flush &&
077500000000               flush != Z_FINISH) {
077600000000        ERR_RETURN(strm, Z_BUF_ERROR);
077700000000    }
077800000000
077900000000    /* User must not provide more input after the first FINISH: */
078000000000    if (s->status == FINISH_STATE && strm->avail_in != 0) {
078100000000        ERR_RETURN(strm, Z_BUF_ERROR);
078200000000    }
078300000000
078400000000    /* Start a new block or continue the current one.
078500000000     */
078600000000    if (strm->avail_in != 0 || s->lookahead != 0 ||
078700000000        (flush != Z_NO_FLUSH && s->status != FINISH_STATE)) {
078800000000        block_state bstate;
078900000000
079000000000        bstate = (*(configuration_table[s->level].func))(s, flush);
079100000000
079200000000        if (bstate == finish_started || bstate == finish_done) {
079300000000            s->status = FINISH_STATE;
079400000000        }
079500000000        if (bstate == need_more || bstate == finish_started) {
079600000000            if (strm->avail_out == 0) {
079700000000                s->last_flush = -1; /* avoid BUF_ERROR next call, see above */
079800000000            }
079900000000            return Z_OK;
080000000000            /* If flush != Z_NO_FLUSH && avail_out == 0, the next call
080100000000             * of deflate should use the same flush parameter to make sure
080200000000             * that the flush is complete. So we don't have to output an
080300000000             * empty block here, this will be done at next call. This also
080400000000             * ensures that for a very small output buffer, we emit at most
080500000000             * one empty block.
080600000000             */
080700000000        }
080800000000        if (bstate == block_done) {
080900000000            if (flush == Z_PARTIAL_FLUSH) {
081000000000                _tr_align(s);
081100000000            } else { /* FULL_FLUSH or SYNC_FLUSH */
081200000000                _tr_stored_block(s, (char*)0, 0L, 0);
081300000000                /* For a full flush, this empty block will be recognized
081400000000                 * as a special marker by inflate_sync().
081500000000                 */
081600000000                if (flush == Z_FULL_FLUSH) {
081700000000                    CLEAR_HASH(s);             /* forget history */
081800000000                }
081900000000            }
082000000000            flush_pending(strm);
082100000000            if (strm->avail_out == 0) {
082200000000              s->last_flush = -1; /* avoid BUF_ERROR at next call, see above */
082300000000              return Z_OK;
082400000000            }
082500000000        }
082600000000    }
082700000000    Assert(strm->avail_out > 0, "bug2");
082800000000
082900000000    if (flush != Z_FINISH) return Z_OK;
083000000000    if (s->wrap <= 0) return Z_STREAM_END;
083100000000
083200000000    /* Write the trailer */
083300000000#ifdef GZIP
083400000000    if (s->wrap == 2) {
083500000000        put_byte(s, (Byte)(strm->adler & 0xff));
083600000000        put_byte(s, (Byte)((strm->adler >> 8) & 0xff));
083700000000        put_byte(s, (Byte)((strm->adler >> 16) & 0xff));
083800000000        put_byte(s, (Byte)((strm->adler >> 24) & 0xff));
083900000000        put_byte(s, (Byte)(strm->total_in & 0xff));
084000000000        put_byte(s, (Byte)((strm->total_in >> 8) & 0xff));
084100000000        put_byte(s, (Byte)((strm->total_in >> 16) & 0xff));
084200000000        put_byte(s, (Byte)((strm->total_in >> 24) & 0xff));
084300000000    }
084400000000    else
084500000000#endif
084600000000    {
084700000000        putShortMSB(s, (uInt)(strm->adler >> 16));
084800000000        putShortMSB(s, (uInt)(strm->adler & 0xffff));
084900000000    }
085000000000    flush_pending(strm);
085100000000    /* If avail_out is zero, the application will call deflate again
085200000000     * to flush the rest.
085300000000     */
085400000000    if (s->wrap > 0) s->wrap = -s->wrap; /* write the trailer only once! */
085500000000    return s->pending != 0 ? Z_OK : Z_STREAM_END;
085600000000}
085700000000
085800000000/* ========================================================================= */
085900000000int ZEXPORT deflateEnd (strm)
086000000000    z_streamp strm;
086100000000{
086200000000    int status;
086300000000
086400000000    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
086500000000
086600000000    status = strm->state->status;
086700000000    if (status != INIT_STATE &&
086800000000        status != EXTRA_STATE &&
086900000000        status != NAME_STATE &&
087000000000        status != COMMENT_STATE &&
087100000000        status != HCRC_STATE &&
087200000000        status != BUSY_STATE &&
087300000000        status != FINISH_STATE) {
087400000000      return Z_STREAM_ERROR;
087500000000    }
087600000000
087700000000    /* Deallocate in reverse order of allocations: */
087800000000    TRY_FREE(strm, strm->state->pending_buf);
087900000000    TRY_FREE(strm, strm->state->head);
088000000000    TRY_FREE(strm, strm->state->prev);
088100000000    TRY_FREE(strm, strm->state->window);
088200000000
088300000000    ZFREE(strm, strm->state);
088400000000    strm->state = Z_NULL;
088500000000
088600000000    return status == BUSY_STATE ? Z_DATA_ERROR : Z_OK;
088700000000}
088800000000
088900000000/* =========================================================================
089000000000 * Copy the source state to the destination state.
089100000000 * To simplify the source, this is not supported for 16-bit MSDOS (which
089200000000 * doesn't have enough memory anyway to duplicate compression states).
089300000000 */
089400000000int ZEXPORT deflateCopy (dest, source)
089500000000    z_streamp dest;
089600000000    z_streamp source;
089700000000{
089800000000#ifdef MAXSEG_64K
089900000000    return Z_STREAM_ERROR;
090000000000#else
090100000000    deflate_state *ds;
090200000000    deflate_state *ss;
090300000000    ushf *overlay;
090400000000
090500000000
090600000000    if (source == Z_NULL || dest == Z_NULL || source->state == Z_NULL) {
090700000000        return Z_STREAM_ERROR;
090800000000    }
090900000000
091000000000    ss = source->state;
091100000000
091200000000    zmemcpy(dest, source, sizeof(z_stream));
091300000000
091400000000    ds = (deflate_state *) ZALLOC(dest, 1, sizeof(deflate_state));
091500000000    if (ds == Z_NULL) return Z_MEM_ERROR;
091600000000    dest->state = (struct internal_state FAR *) ds;
091700000000    zmemcpy(ds, ss, sizeof(deflate_state));
091800000000    ds->strm = dest;
091900000000
092000000000    ds->window = (Bytef *) ZALLOC(dest, ds->w_size, 2*sizeof(Byte));
092100000000    ds->prev   = (Posf *)  ZALLOC(dest, ds->w_size, sizeof(Pos));
092200000000    ds->head   = (Posf *)  ZALLOC(dest, ds->hash_size, sizeof(Pos));
092300000000    overlay = (ushf *) ZALLOC(dest, ds->lit_bufsize, sizeof(ush)+2);
092400000000    ds->pending_buf = (uchf *) overlay;
092500000000
092600000000    if (ds->window == Z_NULL || ds->prev == Z_NULL || ds->head == Z_NULL ||
092700000000        ds->pending_buf == Z_NULL) {
092800000000        deflateEnd (dest);
092900000000        return Z_MEM_ERROR;
093000000000    }
093100000000    /* following zmemcpy do not work for 16-bit MSDOS */
093200000000    zmemcpy(ds->window, ss->window, ds->w_size * 2 * sizeof(Byte));
093300000000    zmemcpy(ds->prev, ss->prev, ds->w_size * sizeof(Pos));
093400000000    zmemcpy(ds->head, ss->head, ds->hash_size * sizeof(Pos));
093500000000    zmemcpy(ds->pending_buf, ss->pending_buf, (uInt)ds->pending_buf_size);
093600000000
093700000000    ds->pending_out = ds->pending_buf + (ss->pending_out - ss->pending_buf);
093800000000    ds->d_buf = overlay + ds->lit_bufsize/sizeof(ush);
093900000000    ds->l_buf = ds->pending_buf + (1+sizeof(ush))*ds->lit_bufsize;
094000000000
094100000000    ds->l_desc.dyn_tree = ds->dyn_ltree;
094200000000    ds->d_desc.dyn_tree = ds->dyn_dtree;
094300000000    ds->bl_desc.dyn_tree = ds->bl_tree;
094400000000
094500000000    return Z_OK;
094600000000#endif /* MAXSEG_64K */
094700000000}
094800000000
094900000000/* ===========================================================================
095000000000 * Read a new buffer from the current input stream, update the adler32
095100000000 * and total number of bytes read.  All deflate() input goes through
095200000000 * this function so some applications may wish to modify it to avoid
095300000000 * allocating a large strm->next_in buffer and copying from it.
095400000000 * (See also flush_pending()).
095500000000 */
095600000000local int read_buf(strm, buf, size)
095700000000    z_streamp strm;
095800000000    Bytef *buf;
095900000000    unsigned size;
096000000000{
096100000000    unsigned len = strm->avail_in;
096200000000
096300000000    if (len > size) len = size;
096400000000    if (len == 0) return 0;
096500000000
096600000000    strm->avail_in  -= len;
096700000000
096800000000    if (strm->state->wrap == 1) {
096900000000        strm->adler = adler32(strm->adler, strm->next_in, len);
097000000000    }
097100000000#ifdef GZIP
097200000000    else if (strm->state->wrap == 2) {
097300000000        strm->adler = crc32(strm->adler, strm->next_in, len);
097400000000    }
097500000000#endif
097600000000    zmemcpy(buf, strm->next_in, len);
097700000000    strm->next_in  += len;
097800000000    strm->total_in += len;
097900000000
098000000000    return (int)len;
098100000000}
098200000000
098300000000/* ===========================================================================
098400000000 * Initialize the "longest match" routines for a new zlib stream
098500000000 */
098600000000local void lm_init (s)
098700000000    deflate_state *s;
098800000000{
098900000000    s->window_size = (ulg)2L*s->w_size;
099000000000
099100000000    CLEAR_HASH(s);
099200000000
099300000000    /* Set the default configuration parameters:
099400000000     */
099500000000    s->max_lazy_match   = configuration_table[s->level].max_lazy;
099600000000    s->good_match       = configuration_table[s->level].good_length;
099700000000    s->nice_match       = configuration_table[s->level].nice_length;
099800000000    s->max_chain_length = configuration_table[s->level].max_chain;
099900000000
100000000000    s->strstart = 0;
100100000000    s->block_start = 0L;
100200000000    s->lookahead = 0;
100300000000    s->match_length = s->prev_length = MIN_MATCH-1;
100400000000    s->match_available = 0;
100500000000    s->ins_h = 0;
100600000000#ifndef FASTEST
100700000000#ifdef ASMV
100800000000    match_init(); /* initialize the asm code */
100900000000#endif
101000000000#endif
101100000000}
101200000000
101300000000#ifndef FASTEST
101400000000/* ===========================================================================
101500000000 * Set match_start to the longest match starting at the given string and
101600000000 * return its length. Matches shorter or equal to prev_length are discarded,
101700000000 * in which case the result is equal to prev_length and match_start is
101800000000 * garbage.
101900000000 * IN assertions: cur_match is the head of the hash chain for the current
102000000000 *   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
102100000000 * OUT assertion: the match length is not greater than s->lookahead.
102200000000 */
102300000000#ifndef ASMV
102400000000/* For 80x86 and 680x0, an optimized version will be provided in match.asm or
102500000000 * match.S. The code will be functionally equivalent.
102600000000 */
102700000000local uInt longest_match(s, cur_match)
102800000000    deflate_state *s;
102900000000    IPos cur_match;                             /* current match */
103000000000{
103100000000    unsigned chain_length = s->max_chain_length;/* max hash chain length */
103200000000    register Bytef *scan = s->window + s->strstart; /* current string */
103300000000    register Bytef *match;                       /* matched string */
103400000000    register int len;                           /* length of current match */
103500000000    int best_len = s->prev_length;              /* best match length so far */
103600000000    int nice_match = s->nice_match;             /* stop if match long enough */
103700000000    IPos limit = s->strstart > (IPos)MAX_DIST(s) ?
103800000000        s->strstart - (IPos)MAX_DIST(s) : NIL;
103900000000    /* Stop when cur_match becomes <= limit. To simplify the code,
104000000000     * we prevent matches with the string of window index 0.
104100000000     */
104200000000    Posf *prev = s->prev;
104300000000    uInt wmask = s->w_mask;
104400000000
104500000000#ifdef UNALIGNED_OK
104600000000    /* Compare two bytes at a time. Note: this is not always beneficial.
104700000000     * Try with and without -DUNALIGNED_OK to check.
104800000000     */
104900000000    register Bytef *strend = s->window + s->strstart + MAX_MATCH - 1;
105000000000    register ush scan_start = *(ushf*)scan;
105100000000    register ush scan_end   = *(ushf*)(scan+best_len-1);
105200000000#else
105300000000    register Bytef *strend = s->window + s->strstart + MAX_MATCH;
105400000000    register Byte scan_end1  = scan[best_len-1];
105500000000    register Byte scan_end   = scan[best_len];
105600000000#endif
105700000000
105800000000    /* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
105900000000     * It is easy to get rid of this optimization if necessary.
106000000000     */
106100000000    Assert(s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever");
106200000000
106300000000    /* Do not waste too much time if we already have a good match: */
106400000000    if (s->prev_length >= s->good_match) {
106500000000        chain_length >>= 2;
106600000000    }
106700000000    /* Do not look for matches beyond the end of the input. This is necessary
106800000000     * to make deflate deterministic.
106900000000     */
107000000000    if ((uInt)nice_match > s->lookahead) nice_match = s->lookahead;
107100000000
107200000000    Assert((ulg)s->strstart <= s->window_size-MIN_LOOKAHEAD, "need lookahead");
107300000000
107400000000    do {
107500000000        Assert(cur_match < s->strstart, "no future");
107600000000        match = s->window + cur_match;
107700000000
107800000000        /* Skip to next match if the match length cannot increase
107900000000         * or if the match length is less than 2.  Note that the checks below
108000000000         * for insufficient lookahead only occur occasionally for performance
108100000000         * reasons.  Therefore uninitialized memory will be accessed, and
108200000000         * conditional jumps will be made that depend on those values.
108300000000         * However the length of the match is limited to the lookahead, so
108400000000         * the output of deflate is not affected by the uninitialized values.
108500000000         */
108600000000#if (defined(UNALIGNED_OK) && MAX_MATCH == 258)
108700000000        /* This code assumes sizeof(unsigned short) == 2. Do not use
108800000000         * UNALIGNED_OK if your compiler uses a different size.
108900000000         */
109000000000        if (*(ushf*)(match+best_len-1) != scan_end ||
109100000000            *(ushf*)match != scan_start) continue;
109200000000
109300000000        /* It is not necessary to compare scan[2] and match[2] since they are
109400000000         * always equal when the other bytes match, given that the hash keys
109500000000         * are equal and that HASH_BITS >= 8. Compare 2 bytes at a time at
109600000000         * strstart+3, +5, ... up to strstart+257. We check for insufficient
109700000000         * lookahead only every 4th comparison; the 128th check will be made
109800000000         * at strstart+257. If MAX_MATCH-2 is not a multiple of 8, it is
109900000000         * necessary to put more guard bytes at the end of the window, or
110000000000         * to check more often for insufficient lookahead.
110100000000         */
110200000000        Assert(scan[2] == match[2], "scan[2]?");
110300000000        scan++, match++;
110400000000        do {
110500000000        } while (*(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
110600000000                 *(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
110700000000                 *(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
110800000000                 *(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
110900000000                 scan < strend);
111000000000        /* The funny "do {}" generates better code on most compilers */
111100000000
111200000000        /* Here, scan <= window+strstart+257 */
111300000000        Assert(scan <= s->window+(unsigned)(s->window_size-1), "wild scan");
111400000000        if (*scan == *match) scan++;
111500000000
111600000000        len = (MAX_MATCH - 1) - (int)(strend-scan);
111700000000        scan = strend - (MAX_MATCH-1);
111800000000
111900000000#else /* UNALIGNED_OK */
112000000000
112100000000        if (match[best_len]   != scan_end  ||
112200000000            match[best_len-1] != scan_end1 ||
112300000000            *match            != *scan     ||
112400000000            *++match          != scan[1])      continue;
112500000000
112600000000        /* The check at best_len-1 can be removed because it will be made
112700000000         * again later. (This heuristic is not always a win.)
112800000000         * It is not necessary to compare scan[2] and match[2] since they
112900000000         * are always equal when the other bytes match, given that
113000000000         * the hash keys are equal and that HASH_BITS >= 8.
113100000000         */
113200000000        scan += 2, match++;
113300000000        Assert(*scan == *match, "match[2]?");
113400000000
113500000000        /* We check for insufficient lookahead only every 8th comparison;
113600000000         * the 256th check will be made at strstart+258.
113700000000         */
113800000000        do {
113900000000        } while (*++scan == *++match && *++scan == *++match &&
114000000000                 *++scan == *++match && *++scan == *++match &&
114100000000                 *++scan == *++match && *++scan == *++match &&
114200000000                 *++scan == *++match && *++scan == *++match &&
114300000000                 scan < strend);
114400000000
114500000000        Assert(scan <= s->window+(unsigned)(s->window_size-1), "wild scan");
114600000000
114700000000        len = MAX_MATCH - (int)(strend - scan);
114800000000        scan = strend - MAX_MATCH;
114900000000
115000000000#endif /* UNALIGNED_OK */
115100000000
115200000000        if (len > best_len) {
115300000000            s->match_start = cur_match;
115400000000            best_len = len;
115500000000            if (len >= nice_match) break;
115600000000#ifdef UNALIGNED_OK
115700000000            scan_end = *(ushf*)(scan+best_len-1);
115800000000#else
115900000000            scan_end1  = scan[best_len-1];
116000000000            scan_end   = scan[best_len];
116100000000#endif
116200000000        }
116300000000    } while ((cur_match = prev[cur_match & wmask]) > limit
116400000000             && --chain_length != 0);
116500000000
116600000000    if ((uInt)best_len <= s->lookahead) return (uInt)best_len;
116700000000    return s->lookahead;
116800000000}
116900000000#endif /* ASMV */
117000000000#endif /* FASTEST */
117100000000
117200000000/* ---------------------------------------------------------------------------
117300000000 * Optimized version for level == 1 or strategy == Z_RLE only
117400000000 */
117500000000local uInt longest_match_fast(s, cur_match)
117600000000    deflate_state *s;
117700000000    IPos cur_match;                             /* current match */
117800000000{
117900000000    register Bytef *scan = s->window + s->strstart; /* current string */
118000000000    register Bytef *match;                       /* matched string */
118100000000    register int len;                           /* length of current match */
118200000000    register Bytef *strend = s->window + s->strstart + MAX_MATCH;
118300000000
118400000000    /* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
118500000000     * It is easy to get rid of this optimization if necessary.
118600000000     */
118700000000    Assert(s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever");
118800000000
118900000000    Assert((ulg)s->strstart <= s->window_size-MIN_LOOKAHEAD, "need lookahead");
119000000000
119100000000    Assert(cur_match < s->strstart, "no future");
119200000000
119300000000    match = s->window + cur_match;
119400000000
119500000000    /* Return failure if the match length is less than 2:
119600000000     */
119700000000    if (match[0] != scan[0] || match[1] != scan[1]) return MIN_MATCH-1;
119800000000
119900000000    /* The check at best_len-1 can be removed because it will be made
120000000000     * again later. (This heuristic is not always a win.)
120100000000     * It is not necessary to compare scan[2] and match[2] since they
120200000000     * are always equal when the other bytes match, given that
120300000000     * the hash keys are equal and that HASH_BITS >= 8.
120400000000     */
120500000000    scan += 2, match += 2;
120600000000    Assert(*scan == *match, "match[2]?");
120700000000
120800000000    /* We check for insufficient lookahead only every 8th comparison;
120900000000     * the 256th check will be made at strstart+258.
121000000000     */
121100000000    do {
121200000000    } while (*++scan == *++match && *++scan == *++match &&
121300000000             *++scan == *++match && *++scan == *++match &&
121400000000             *++scan == *++match && *++scan == *++match &&
121500000000             *++scan == *++match && *++scan == *++match &&
121600000000             scan < strend);
121700000000
121800000000    Assert(scan <= s->window+(unsigned)(s->window_size-1), "wild scan");
121900000000
122000000000    len = MAX_MATCH - (int)(strend - scan);
122100000000
122200000000    if (len < MIN_MATCH) return MIN_MATCH - 1;
122300000000
122400000000    s->match_start = cur_match;
122500000000    return (uInt)len <= s->lookahead ? (uInt)len : s->lookahead;
122600000000}
122700000000
122800000000#ifdef DEBUG
122900000000/* ===========================================================================
123000000000 * Check that the match at match_start is indeed a match.
123100000000 */
123200000000local void check_match(s, start, match, length)
123300000000    deflate_state *s;
123400000000    IPos start, match;
123500000000    int length;
123600000000{
123700000000    /* check that the match is indeed a match */
123800000000    if (zmemcmp(s->window + match,
123900000000                s->window + start, length) != EQUAL) {
124000000000        fprintf(stderr, " start %u, match %u, length %d\n",
124100000000                start, match, length);
124200000000        do {
124300000000            fprintf(stderr, "%c%c", s->window[match++], s->window[start++]);
124400000000        } while (--length != 0);
124500000000        z_error("invalid match");
124600000000    }
124700000000    if (z_verbose > 1) {
124800000000        fprintf(stderr,"\\[%d,%d]", start-match, length);
124900000000        do { putc(s->window[start++], stderr); } while (--length != 0);
125000000000    }
125100000000}
125200000000#else
125300000000#  define check_match(s, start, match, length)
125400000000#endif /* DEBUG */
125500000000
125600000000/* ===========================================================================
125700000000 * Fill the window when the lookahead becomes insufficient.
125800000000 * Updates strstart and lookahead.
125900000000 *
126000000000 * IN assertion: lookahead < MIN_LOOKAHEAD
126100000000 * OUT assertions: strstart <= window_size-MIN_LOOKAHEAD
126200000000 *    At least one byte has been read, or avail_in == 0; reads are
126300000000 *    performed for at least two bytes (required for the zip translate_eol
126400000000 *    option -- not supported here).
126500000000 */
126600000000local void fill_window(s)
126700000000    deflate_state *s;
126800000000{
126900000000    register unsigned n, m;
127000000000    register Posf *p;
127100000000    unsigned more;    /* Amount of free space at the end of the window. */
127200000000    uInt wsize = s->w_size;
127300000000
127400000000    do {
127500000000        more = (unsigned)(s->window_size -(ulg)s->lookahead -(ulg)s->strstart);
127600000000
127700000000        /* Deal with !@#$% 64K limit: */
127800000000        if (sizeof(int) <= 2) {
127900000000            if (more == 0 && s->strstart == 0 && s->lookahead == 0) {
128000000000                more = wsize;
128100000000
128200000000            } else if (more == (unsigned)(-1)) {
128300000000                /* Very unlikely, but possible on 16 bit machine if
128400000000                 * strstart == 0 && lookahead == 1 (input done a byte at time)
128500000000                 */
128600000000                more--;
128700000000            }
128800000000        }
128900000000
129000000000        /* If the window is almost full and there is insufficient lookahead,
129100000000         * move the upper half to the lower one to make room in the upper half.
129200000000         */
129300000000        if (s->strstart >= wsize+MAX_DIST(s)) {
129400000000
129500000000            zmemcpy(s->window, s->window+wsize, (unsigned)wsize);
129600000000            s->match_start -= wsize;
129700000000            s->strstart    -= wsize; /* we now have strstart >= MAX_DIST */
129800000000            s->block_start -= (long) wsize;
129900000000
130000000000            /* Slide the hash table (could be avoided with 32 bit values
130100000000               at the expense of memory usage). We slide even when level == 0
130200000000               to keep the hash table consistent if we switch back to level > 0
130300000000               later. (Using level 0 permanently is not an optimal usage of
130400000000               zlib, so we don't care about this pathological case.)
130500000000             */
130600000000            /* %%% avoid this when Z_RLE */
130700000000            n = s->hash_size;
130800000000            p = &s->head[n];
130900000000            do {
131000000000                m = *--p;
131100000000                *p = (Pos)(m >= wsize ? m-wsize : NIL);
131200000000            } while (--n);
131300000000
131400000000            n = wsize;
131500000000#ifndef FASTEST
131600000000            p = &s->prev[n];
131700000000            do {
131800000000                m = *--p;
131900000000                *p = (Pos)(m >= wsize ? m-wsize : NIL);
132000000000                /* If n is not on any hash chain, prev[n] is garbage but
132100000000                 * its value will never be used.
132200000000                 */
132300000000            } while (--n);
132400000000#endif
132500000000            more += wsize;
132600000000        }
132700000000        if (s->strm->avail_in == 0) return;
132800000000
132900000000        /* If there was no sliding:
133000000000         *    strstart <= WSIZE+MAX_DIST-1 && lookahead <= MIN_LOOKAHEAD - 1 &&
133100000000         *    more == window_size - lookahead - strstart
133200000000         * => more >= window_size - (MIN_LOOKAHEAD-1 + WSIZE + MAX_DIST-1)
133300000000         * => more >= window_size - 2*WSIZE + 2
133400000000         * In the BIG_MEM or MMAP case (not yet supported),
133500000000         *   window_size == input_size + MIN_LOOKAHEAD  &&
133600000000         *   strstart + s->lookahead <= input_size => more >= MIN_LOOKAHEAD.
133700000000         * Otherwise, window_size == 2*WSIZE so more >= 2.
133800000000         * If there was sliding, more >= WSIZE. So in all cases, more >= 2.
133900000000         */
134000000000        Assert(more >= 2, "more < 2");
134100000000
134200000000        n = read_buf(s->strm, s->window + s->strstart + s->lookahead, more);
134300000000        s->lookahead += n;
134400000000
134500000000        /* Initialize the hash value now that we have some input: */
134600000000        if (s->lookahead >= MIN_MATCH) {
134700000000            s->ins_h = s->window[s->strstart];
134800000000            UPDATE_HASH(s, s->ins_h, s->window[s->strstart+1]);
134900000000#if MIN_MATCH != 3
135000000000            Call UPDATE_HASH() MIN_MATCH-3 more times
135100000000#endif
135200000000        }
135300000000        /* If the whole input has less than MIN_MATCH bytes, ins_h is garbage,
135400000000         * but this is not important since only literal bytes will be emitted.
135500000000         */
135600000000
135700000000    } while (s->lookahead < MIN_LOOKAHEAD && s->strm->avail_in != 0);
135800000000}
135900000000
136000000000/* ===========================================================================
136100000000 * Flush the current block, with given end-of-file flag.
136200000000 * IN assertion: strstart is set to the end of the current match.
136300000000 */
136400000000#define FLUSH_BLOCK_ONLY(s, eof) { \
136500000000   _tr_flush_block(s, (s->block_start >= 0L ? \
136600000000                   (charf *)&s->window[(unsigned)s->block_start] : \
136700000000                   (charf *)Z_NULL), \
136800000000                (ulg)((long)s->strstart - s->block_start), \
136900000000                (eof)); \
137000000000   s->block_start = s->strstart; \
137100000000   flush_pending(s->strm); \
137200000000   Tracev((stderr,"[FLUSH]")); \
137300000000}
137400000000
137500000000/* Same but force premature exit if necessary. */
137600000000#define FLUSH_BLOCK(s, eof) { \
137700000000   FLUSH_BLOCK_ONLY(s, eof); \
137800000000   if (s->strm->avail_out == 0) return (eof) ? finish_started : need_more; \
137900000000}
138000000000
138100000000/* ===========================================================================
138200000000 * Copy without compression as much as possible from the input stream, return
138300000000 * the current block state.
138400000000 * This function does not insert new strings in the dictionary since
138500000000 * uncompressible data is probably not useful. This function is used
138600000000 * only for the level=0 compression option.
138700000000 * NOTE: this function should be optimized to avoid extra copying from
138800000000 * window to pending_buf.
138900000000 */
139000000000local block_state deflate_stored(s, flush)
139100000000    deflate_state *s;
139200000000    int flush;
139300000000{
139400000000    /* Stored blocks are limited to 0xffff bytes, pending_buf is limited
139500000000     * to pending_buf_size, and each stored block has a 5 byte header:
139600000000     */
139700000000    ulg max_block_size = 0xffff;
139800000000    ulg max_start;
139900000000
140000000000    if (max_block_size > s->pending_buf_size - 5) {
140100000000        max_block_size = s->pending_buf_size - 5;
140200000000    }
140300000000
140400000000    /* Copy as much as possible from input to output: */
140500000000    for (;;) {
140600000000        /* Fill the window as much as possible: */
140700000000        if (s->lookahead <= 1) {
140800000000
140900000000            Assert(s->strstart < s->w_size+MAX_DIST(s) ||
141000000000                   s->block_start >= (long)s->w_size, "slide too late");
141100000000
141200000000            fill_window(s);
141300000000            if (s->lookahead == 0 && flush == Z_NO_FLUSH) return need_more;
141400000000
141500000000            if (s->lookahead == 0) break; /* flush the current block */
141600000000        }
141700000000        Assert(s->block_start >= 0L, "block gone");
141800000000
141900000000        s->strstart += s->lookahead;
142000000000        s->lookahead = 0;
142100000000
142200000000        /* Emit a stored block if pending_buf will be full: */
142300000000        max_start = s->block_start + max_block_size;
142400000000        if (s->strstart == 0 || (ulg)s->strstart >= max_start) {
142500000000            /* strstart == 0 is possible when wraparound on 16-bit machine */
142600000000            s->lookahead = (uInt)(s->strstart - max_start);
142700000000            s->strstart = (uInt)max_start;
142800000000            FLUSH_BLOCK(s, 0);
142900000000        }
143000000000        /* Flush if we may have to slide, otherwise block_start may become
143100000000         * negative and the data will be gone:
143200000000         */
143300000000        if (s->strstart - (uInt)s->block_start >= MAX_DIST(s)) {
143400000000            FLUSH_BLOCK(s, 0);
143500000000        }
143600000000    }
143700000000    FLUSH_BLOCK(s, flush == Z_FINISH);
143800000000    return flush == Z_FINISH ? finish_done : block_done;
143900000000}
144000000000
144100000000/* ===========================================================================
144200000000 * Compress as much as possible from the input stream, return the current
144300000000 * block state.
144400000000 * This function does not perform lazy evaluation of matches and inserts
144500000000 * new strings in the dictionary only for unmatched strings or for short
144600000000 * matches. It is used only for the fast compression options.
144700000000 */
144800000000local block_state deflate_fast(s, flush)
144900000000    deflate_state *s;
145000000000    int flush;
145100000000{
145200000000    IPos hash_head = NIL; /* head of the hash chain */
145300000000    int bflush;           /* set if current block must be flushed */
145400000000
145500000000    for (;;) {
145600000000        /* Make sure that we always have enough lookahead, except
145700000000         * at the end of the input file. We need MAX_MATCH bytes
145800000000         * for the next match, plus MIN_MATCH bytes to insert the
145900000000         * string following the next match.
146000000000         */
146100000000        if (s->lookahead < MIN_LOOKAHEAD) {
146200000000            fill_window(s);
146300000000            if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH) {
146400000000                return need_more;
146500000000            }
146600000000            if (s->lookahead == 0) break; /* flush the current block */
146700000000        }
146800000000
146900000000        /* Insert the string window[strstart .. strstart+2] in the
147000000000         * dictionary, and set hash_head to the head of the hash chain:
147100000000         */
147200000000        if (s->lookahead >= MIN_MATCH) {
147300000000            INSERT_STRING(s, s->strstart, hash_head);
147400000000        }
147500000000
147600000000        /* Find the longest match, discarding those <= prev_length.
147700000000         * At this point we have always match_length < MIN_MATCH
147800000000         */
147900000000        if (hash_head != NIL && s->strstart - hash_head <= MAX_DIST(s)) {
148000000000            /* To simplify the code, we prevent matches with the string
148100000000             * of window index 0 (in particular we have to avoid a match
148200000000             * of the string with itself at the start of the input file).
148300000000             */
148400000000#ifdef FASTEST
148500000000            if ((s->strategy != Z_HUFFMAN_ONLY && s->strategy != Z_RLE) ||
148600000000                (s->strategy == Z_RLE && s->strstart - hash_head == 1)) {
148700000000                s->match_length = longest_match_fast (s, hash_head);
148800000000            }
148900000000#else
149000000000            if (s->strategy != Z_HUFFMAN_ONLY && s->strategy != Z_RLE) {
149100000000                s->match_length = longest_match (s, hash_head);
149200000000            } else if (s->strategy == Z_RLE && s->strstart - hash_head == 1) {
149300000000                s->match_length = longest_match_fast (s, hash_head);
149400000000            }
149500000000#endif
149600000000            /* longest_match() or longest_match_fast() sets match_start */
149700000000        }
149800000000        if (s->match_length >= MIN_MATCH) {
149900000000            check_match(s, s->strstart, s->match_start, s->match_length);
150000000000
150100000000            _tr_tally_dist(s, s->strstart - s->match_start,
150200000000                           s->match_length - MIN_MATCH, bflush);
150300000000
150400000000            s->lookahead -= s->match_length;
150500000000
150600000000            /* Insert new strings in the hash table only if the match length
150700000000             * is not too large. This saves time but degrades compression.
150800000000             */
150900000000#ifndef FASTEST
151000000000            if (s->match_length <= s->max_insert_length &&
151100000000                s->lookahead >= MIN_MATCH) {
151200000000                s->match_length--; /* string at strstart already in table */
151300000000                do {
151400000000                    s->strstart++;
151500000000                    INSERT_STRING(s, s->strstart, hash_head);
151600000000                    /* strstart never exceeds WSIZE-MAX_MATCH, so there are
151700000000                     * always MIN_MATCH bytes ahead.
151800000000                     */
151900000000                } while (--s->match_length != 0);
152000000000                s->strstart++;
152100000000            } else
152200000000#endif
152300000000            {
152400000000                s->strstart += s->match_length;
152500000000                s->match_length = 0;
152600000000                s->ins_h = s->window[s->strstart];
152700000000                UPDATE_HASH(s, s->ins_h, s->window[s->strstart+1]);
152800000000#if MIN_MATCH != 3
152900000000                Call UPDATE_HASH() MIN_MATCH-3 more times
153000000000#endif
153100000000                /* If lookahead < MIN_MATCH, ins_h is garbage, but it does not
153200000000                 * matter since it will be recomputed at next deflate call.
153300000000                 */
153400000000            }
153500000000        } else {
153600000000            /* No match, output a literal byte */
153700000000            Tracevv((stderr,"%c", s->window[s->strstart]));
153800000000            _tr_tally_lit (s, s->window[s->strstart], bflush);
153900000000            s->lookahead--;
154000000000            s->strstart++;
154100000000        }
154200000000        if (bflush) FLUSH_BLOCK(s, 0);
154300000000    }
154400000000    FLUSH_BLOCK(s, flush == Z_FINISH);
154500000000    return flush == Z_FINISH ? finish_done : block_done;
154600000000}
154700000000
154800000000#ifndef FASTEST
154900000000/* ===========================================================================
155000000000 * Same as above, but achieves better compression. We use a lazy
155100000000 * evaluation for matches: a match is finally adopted only if there is
155200000000 * no better match at the next window position.
155300000000 */
155400000000local block_state deflate_slow(s, flush)
155500000000    deflate_state *s;
155600000000    int flush;
155700000000{
155800000000    IPos hash_head = NIL;    /* head of hash chain */
155900000000    int bflush;              /* set if current block must be flushed */
156000000000
156100000000    /* Process the input block. */
156200000000    for (;;) {
156300000000        /* Make sure that we always have enough lookahead, except
156400000000         * at the end of the input file. We need MAX_MATCH bytes
156500000000         * for the next match, plus MIN_MATCH bytes to insert the
156600000000         * string following the next match.
156700000000         */
156800000000        if (s->lookahead < MIN_LOOKAHEAD) {
156900000000            fill_window(s);
157000000000            if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH) {
157100000000                return need_more;
157200000000            }
157300000000            if (s->lookahead == 0) break; /* flush the current block */
157400000000        }
157500000000
157600000000        /* Insert the string window[strstart .. strstart+2] in the
157700000000         * dictionary, and set hash_head to the head of the hash chain:
157800000000         */
157900000000        if (s->lookahead >= MIN_MATCH) {
158000000000            INSERT_STRING(s, s->strstart, hash_head);
158100000000        }
158200000000
158300000000        /* Find the longest match, discarding those <= prev_length.
158400000000         */
158500000000        s->prev_length = s->match_length, s->prev_match = s->match_start;
158600000000        s->match_length = MIN_MATCH-1;
158700000000
158800000000        if (hash_head != NIL && s->prev_length < s->max_lazy_match &&
158900000000            s->strstart - hash_head <= MAX_DIST(s)) {
159000000000            /* To simplify the code, we prevent matches with the string
159100000000             * of window index 0 (in particular we have to avoid a match
159200000000             * of the string with itself at the start of the input file).
159300000000             */
159400000000            if (s->strategy != Z_HUFFMAN_ONLY && s->strategy != Z_RLE) {
159500000000                s->match_length = longest_match (s, hash_head);
159600000000            } else if (s->strategy == Z_RLE && s->strstart - hash_head == 1) {
159700000000                s->match_length = longest_match_fast (s, hash_head);
159800000000            }
159900000000            /* longest_match() or longest_match_fast() sets match_start */
160000000000
160100000000            if (s->match_length <= 5 && (s->strategy == Z_FILTERED
160200000000#if TOO_FAR <= 32767
160300000000                || (s->match_length == MIN_MATCH &&
160400000000                    s->strstart - s->match_start > TOO_FAR)
160500000000#endif
160600000000                )) {
160700000000
160800000000                /* If prev_match is also MIN_MATCH, match_start is garbage
160900000000                 * but we will ignore the current match anyway.
161000000000                 */
161100000000                s->match_length = MIN_MATCH-1;
161200000000            }
161300000000        }
161400000000        /* If there was a match at the previous step and the current
161500000000         * match is not better, output the previous match:
161600000000         */
161700000000        if (s->prev_length >= MIN_MATCH && s->match_length <= s->prev_length) {
161800000000            uInt max_insert = s->strstart + s->lookahead - MIN_MATCH;
161900000000            /* Do not insert strings in hash table beyond this. */
162000000000
162100000000            check_match(s, s->strstart-1, s->prev_match, s->prev_length);
162200000000
162300000000            _tr_tally_dist(s, s->strstart -1 - s->prev_match,
162400000000                           s->prev_length - MIN_MATCH, bflush);
162500000000
162600000000            /* Insert in hash table all strings up to the end of the match.
162700000000             * strstart-1 and strstart are already inserted. If there is not
162800000000             * enough lookahead, the last two strings are not inserted in
162900000000             * the hash table.
163000000000             */
163100000000            s->lookahead -= s->prev_length-1;
163200000000            s->prev_length -= 2;
163300000000            do {
163400000000                if (++s->strstart <= max_insert) {
163500000000                    INSERT_STRING(s, s->strstart, hash_head);
163600000000                }
163700000000            } while (--s->prev_length != 0);
163800000000            s->match_available = 0;
163900000000            s->match_length = MIN_MATCH-1;
164000000000            s->strstart++;
164100000000
164200000000            if (bflush) FLUSH_BLOCK(s, 0);
164300000000
164400000000        } else if (s->match_available) {
164500000000            /* If there was no match at the previous position, output a
164600000000             * single literal. If there was a match but the current match
164700000000             * is longer, truncate the previous match to a single literal.
164800000000             */
164900000000            Tracevv((stderr,"%c", s->window[s->strstart-1]));
165000000000            _tr_tally_lit(s, s->window[s->strstart-1], bflush);
165100000000            if (bflush) {
165200000000                FLUSH_BLOCK_ONLY(s, 0);
165300000000            }
165400000000            s->strstart++;
165500000000            s->lookahead--;
165600000000            if (s->strm->avail_out == 0) return need_more;
165700000000        } else {
165800000000            /* There is no previous match to compare with, wait for
165900000000             * the next step to decide.
166000000000             */
166100000000            s->match_available = 1;
166200000000            s->strstart++;
166300000000            s->lookahead--;
166400000000        }
166500000000    }
166600000000    Assert (flush != Z_NO_FLUSH, "no flush?");
166700000000    if (s->match_available) {
166800000000        Tracevv((stderr,"%c", s->window[s->strstart-1]));
166900000000        _tr_tally_lit(s, s->window[s->strstart-1], bflush);
167000000000        s->match_available = 0;
167100000000    }
167200000000    FLUSH_BLOCK(s, flush == Z_FINISH);
167300000000    return flush == Z_FINISH ? finish_done : block_done;
167400000000}
167500000000#endif /* FASTEST */
167600000000
167700000000#if 0
167800000000/* ===========================================================================
167900000000 * For Z_RLE, simply look for runs of bytes, generate matches only of distance
168000000000 * one.  Do not maintain a hash table.  (It will be regenerated if this run of
168100000000 * deflate switches away from Z_RLE.)
168200000000 */
168300000000local block_state deflate_rle(s, flush)
168400000000    deflate_state *s;
168500000000    int flush;
168600000000{
168700000000    int bflush;         /* set if current block must be flushed */
168800000000    uInt run;           /* length of run */
168900000000    uInt max;           /* maximum length of run */
169000000000    uInt prev;          /* byte at distance one to match */
169100000000    Bytef *scan;        /* scan for end of run */
169200000000
169300000000    for (;;) {
169400000000        /* Make sure that we always have enough lookahead, except
169500000000         * at the end of the input file. We need MAX_MATCH bytes
169600000000         * for the longest encodable run.
169700000000         */
169800000000        if (s->lookahead < MAX_MATCH) {
169900000000            fill_window(s);
170000000000            if (s->lookahead < MAX_MATCH && flush == Z_NO_FLUSH) {
170100000000                return need_more;
170200000000            }
170300000000            if (s->lookahead == 0) break; /* flush the current block */
170400000000        }
170500000000
170600000000        /* See how many times the previous byte repeats */
170700000000        run = 0;
170800000000        if (s->strstart > 0) {      /* if there is a previous byte, that is */
170900000000            max = s->lookahead < MAX_MATCH ? s->lookahead : MAX_MATCH;
171000000000            scan = s->window + s->strstart - 1;
171100000000            prev = *scan++;
171200000000            do {
171300000000                if (*scan++ != prev)
171400000000                    break;
171500000000            } while (++run < max);
171600000000        }
171700000000
171800000000        /* Emit match if have run of MIN_MATCH or longer, else emit literal */
171900000000        if (run >= MIN_MATCH) {
172000000000            check_match(s, s->strstart, s->strstart - 1, run);
172100000000            _tr_tally_dist(s, 1, run - MIN_MATCH, bflush);
172200000000            s->lookahead -= run;
172300000000            s->strstart += run;
172400000000        } else {
172500000000            /* No match, output a literal byte */
172600000000            Tracevv((stderr,"%c", s->window[s->strstart]));
172700000000            _tr_tally_lit (s, s->window[s->strstart], bflush);
172800000000            s->lookahead--;
172900000000            s->strstart++;
173000000000        }
173100000000        if (bflush) FLUSH_BLOCK(s, 0);
173200000000    }
173300000000    FLUSH_BLOCK(s, flush == Z_FINISH);
173400000000    return flush == Z_FINISH ? finish_done : block_done;
173500000000}
173600000000#endif
