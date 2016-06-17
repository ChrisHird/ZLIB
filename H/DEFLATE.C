000100000000/* deflate.h -- internal compression state
000200000000 * Copyright (C) 1995-2004 Jean-loup Gailly
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/* WARNING: this file should *not* be used by applications. It is
000700000000   part of the implementation of the compression library and is
000800000000   subject to change. Applications should only use zlib.h.
000900000000 */
001000000000
001100000000/* @(#) $Id$ */
001200000000
001300000000#ifndef DEFLATE_H
001400000000#define DEFLATE_H
001500000000
001600000000#include "zutil.h"
001700000000
001800000000/* define NO_GZIP when compiling if you want to disable gzip header and
001900000000   trailer creation by deflate().  NO_GZIP would be used to avoid linking in
002000000000   the crc code when it is not needed.  For shared libraries, gzip encoding
002100000000   should be left enabled. */
002200000000#ifndef NO_GZIP
002300000000#  define GZIP
002400000000#endif
002500000000
002600000000/* ===========================================================================
002700000000 * Internal compression state.
002800000000 */
002900000000
003000000000#define LENGTH_CODES 29
003100000000/* number of length codes, not counting the special END_BLOCK code */
003200000000
003300000000#define LITERALS  256
003400000000/* number of literal bytes 0..255 */
003500000000
003600000000#define L_CODES (LITERALS+1+LENGTH_CODES)
003700000000/* number of Literal or Length codes, including the END_BLOCK code */
003800000000
003900000000#define D_CODES   30
004000000000/* number of distance codes */
004100000000
004200000000#define BL_CODES  19
004300000000/* number of codes used to transfer the bit lengths */
004400000000
004500000000#define HEAP_SIZE (2*L_CODES+1)
004600000000/* maximum heap size */
004700000000
004800000000#define MAX_BITS 15
004900000000/* All codes must not exceed MAX_BITS bits */
005000000000
005100000000#define INIT_STATE    42
005200000000#define EXTRA_STATE   69
005300000000#define NAME_STATE    73
005400000000#define COMMENT_STATE 91
005500000000#define HCRC_STATE   103
005600000000#define BUSY_STATE   113
005700000000#define FINISH_STATE 666
005800000000/* Stream status */
005900000000
006000000000
006100000000/* Data structure describing a single value and its code string. */
006200000000typedef struct ct_data_s {
006300000000    union {
006400000000        ush  freq;       /* frequency count */
006500000000        ush  code;       /* bit string */
006600000000    } fc;
006700000000    union {
006800000000        ush  dad;        /* father node in Huffman tree */
006900000000        ush  len;        /* length of bit string */
007000000000    } dl;
007100000000} FAR ct_data;
007200000000
007300000000#define Freq fc.freq
007400000000#define Code fc.code
007500000000#define Dad  dl.dad
007600000000#define Len  dl.len
007700000000
007800000000typedef struct static_tree_desc_s  static_tree_desc;
007900000000
008000000000typedef struct tree_desc_s {
008100000000    ct_data *dyn_tree;           /* the dynamic tree */
008200000000    int     max_code;            /* largest code with non zero frequency */
008300000000    static_tree_desc *stat_desc; /* the corresponding static tree */
008400000000} FAR tree_desc;
008500000000
008600000000typedef ush Pos;
008700000000typedef Pos FAR Posf;
008800000000typedef unsigned IPos;
008900000000
009000000000/* A Pos is an index in the character window. We use short instead of int to
009100000000 * save space in the various tables. IPos is used only for parameter passing.
009200000000 */
009300000000
009400000000typedef struct internal_state {
009500000000    z_streamp strm;      /* pointer back to this zlib stream */
009600000000    int   status;        /* as the name implies */
009700000000    Bytef *pending_buf;  /* output still pending */
009800000000    ulg   pending_buf_size; /* size of pending_buf */
009900000000    Bytef *pending_out;  /* next pending byte to output to the stream */
010000000000    uInt   pending;      /* nb of bytes in the pending buffer */
010100000000    int   wrap;          /* bit 0 true for zlib, bit 1 true for gzip */
010200000000    gz_headerp  gzhead;  /* gzip header information to write */
010300000000    uInt   gzindex;      /* where in extra, name, or comment */
010400000000    Byte  method;        /* STORED (for zip only) or DEFLATED */
010500000000    int   last_flush;    /* value of flush param for previous deflate call */
010600000000
010700000000                /* used by deflate.c: */
010800000000
010900000000    uInt  w_size;        /* LZ77 window size (32K by default) */
011000000000    uInt  w_bits;        /* log2(w_size)  (8..16) */
011100000000    uInt  w_mask;        /* w_size - 1 */
011200000000
011300000000    Bytef *window;
011400000000    /* Sliding window. Input bytes are read into the second half of the window,
011500000000     * and move to the first half later to keep a dictionary of at least wSize
011600000000     * bytes. With this organization, matches are limited to a distance of
011700000000     * wSize-MAX_MATCH bytes, but this ensures that IO is always
011800000000     * performed with a length multiple of the block size. Also, it limits
011900000000     * the window size to 64K, which is quite useful on MSDOS.
012000000000     * To do: use the user input buffer as sliding window.
012100000000     */
012200000000
012300000000    ulg window_size;
012400000000    /* Actual size of window: 2*wSize, except when the user input buffer
012500000000     * is directly used as sliding window.
012600000000     */
012700000000
012800000000    Posf *prev;
012900000000    /* Link to older string with same hash index. To limit the size of this
013000000000     * array to 64K, this link is maintained only for the last 32K strings.
013100000000     * An index in this array is thus a window index modulo 32K.
013200000000     */
013300000000
013400000000    Posf *head; /* Heads of the hash chains or NIL. */
013500000000
013600000000    uInt  ins_h;          /* hash index of string to be inserted */
013700000000    uInt  hash_size;      /* number of elements in hash table */
013800000000    uInt  hash_bits;      /* log2(hash_size) */
013900000000    uInt  hash_mask;      /* hash_size-1 */
014000000000
014100000000    uInt  hash_shift;
014200000000    /* Number of bits by which ins_h must be shifted at each input
014300000000     * step. It must be such that after MIN_MATCH steps, the oldest
014400000000     * byte no longer takes part in the hash key, that is:
014500000000     *   hash_shift * MIN_MATCH >= hash_bits
014600000000     */
014700000000
014800000000    long block_start;
014900000000    /* Window position at the beginning of the current output block. Gets
015000000000     * negative when the window is moved backwards.
015100000000     */
015200000000
015300000000    uInt match_length;           /* length of best match */
015400000000    IPos prev_match;             /* previous match */
015500000000    int match_available;         /* set if previous match exists */
015600000000    uInt strstart;               /* start of string to insert */
015700000000    uInt match_start;            /* start of matching string */
015800000000    uInt lookahead;              /* number of valid bytes ahead in window */
015900000000
016000000000    uInt prev_length;
016100000000    /* Length of the best match at previous step. Matches not greater than this
016200000000     * are discarded. This is used in the lazy match evaluation.
016300000000     */
016400000000
016500000000    uInt max_chain_length;
016600000000    /* To speed up deflation, hash chains are never searched beyond this
016700000000     * length.  A higher limit improves compression ratio but degrades the
016800000000     * speed.
016900000000     */
017000000000
017100000000    uInt max_lazy_match;
017200000000    /* Attempt to find a better match only when the current match is strictly
017300000000     * smaller than this value. This mechanism is used only for compression
017400000000     * levels >= 4.
017500000000     */
017600000000#   define max_insert_length  max_lazy_match
017700000000    /* Insert new strings in the hash table only if the match length is not
017800000000     * greater than this length. This saves time but degrades compression.
017900000000     * max_insert_length is used only for compression levels <= 3.
018000000000     */
018100000000
018200000000    int level;    /* compression level (1..9) */
018300000000    int strategy; /* favor or force Huffman coding*/
018400000000
018500000000    uInt good_match;
018600000000    /* Use a faster search when the previous match is longer than this */
018700000000
018800000000    int nice_match; /* Stop searching when current match exceeds this */
018900000000
019000000000                /* used by trees.c: */
019100000000    /* Didn't use ct_data typedef below to supress compiler warning */
019200000000    struct ct_data_s dyn_ltree[HEAP_SIZE];   /* literal and length tree */
019300000000    struct ct_data_s dyn_dtree[2*D_CODES+1]; /* distance tree */
019400000000    struct ct_data_s bl_tree[2*BL_CODES+1];  /* Huffman tree for bit lengths */
019500000000
019600000000    struct tree_desc_s l_desc;               /* desc. for literal tree */
019700000000    struct tree_desc_s d_desc;               /* desc. for distance tree */
019800000000    struct tree_desc_s bl_desc;              /* desc. for bit length tree */
019900000000
020000000000    ush bl_count[MAX_BITS+1];
020100000000    /* number of codes at each bit length for an optimal tree */
020200000000
020300000000    int heap[2*L_CODES+1];      /* heap used to build the Huffman trees */
020400000000    int heap_len;               /* number of elements in the heap */
020500000000    int heap_max;               /* element of largest frequency */
020600000000    /* The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
020700000000     * The same heap array is used to build all trees.
020800000000     */
020900000000
021000000000    uch depth[2*L_CODES+1];
021100000000    /* Depth of each subtree used as tie breaker for trees of equal frequency
021200000000     */
021300000000
021400000000    uchf *l_buf;          /* buffer for literals or lengths */
021500000000
021600000000    uInt  lit_bufsize;
021700000000    /* Size of match buffer for literals/lengths.  There are 4 reasons for
021800000000     * limiting lit_bufsize to 64K:
021900000000     *   - frequencies can be kept in 16 bit counters
022000000000     *   - if compression is not successful for the first block, all input
022100000000     *     data is still in the window so we can still emit a stored block even
022200000000     *     when input comes from standard input.  (This can also be done for
022300000000     *     all blocks if lit_bufsize is not greater than 32K.)
022400000000     *   - if compression is not successful for a file smaller than 64K, we can
022500000000     *     even emit a stored file instead of a stored block (saving 5 bytes).
022600000000     *     This is applicable only for zip (not gzip or zlib).
022700000000     *   - creating new Huffman trees less frequently may not provide fast
022800000000     *     adaptation to changes in the input data statistics. (Take for
022900000000     *     example a binary file with poorly compressible code followed by
023000000000     *     a highly compressible string table.) Smaller buffer sizes give
023100000000     *     fast adaptation but have of course the overhead of transmitting
023200000000     *     trees more frequently.
023300000000     *   - I can't count above 4
023400000000     */
023500000000
023600000000    uInt last_lit;      /* running index in l_buf */
023700000000
023800000000    ushf *d_buf;
023900000000    /* Buffer for distances. To simplify the code, d_buf and l_buf have
024000000000     * the same number of elements. To use different lengths, an extra flag
024100000000     * array would be necessary.
024200000000     */
024300000000
024400000000    ulg opt_len;        /* bit length of current block with optimal trees */
024500000000    ulg static_len;     /* bit length of current block with static trees */
024600000000    uInt matches;       /* number of string matches in current block */
024700000000    int last_eob_len;   /* bit length of EOB code for last block */
024800000000
024900000000#ifdef DEBUG
025000000000    ulg compressed_len; /* total bit length of compressed file mod 2^32 */
025100000000    ulg bits_sent;      /* bit length of compressed data sent mod 2^32 */
025200000000#endif
025300000000
025400000000    ush bi_buf;
025500000000    /* Output buffer. bits are inserted starting at the bottom (least
025600000000     * significant bits).
025700000000     */
025800000000    int bi_valid;
025900000000    /* Number of valid bits in bi_buf.  All bits above the last valid bit
026000000000     * are always zero.
026100000000     */
026200000000
026300000000} FAR deflate_state;
026400000000
026500000000/* Output a byte on the stream.
026600000000 * IN assertion: there is enough room in pending_buf.
026700000000 */
026800000000#define put_byte(s, c) {s->pending_buf[s->pending++] = (c);}
026900000000
027000000000
027100000000#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
027200000000/* Minimum amount of lookahead, except at the end of the input file.
027300000000 * See deflate.c for comments about the MIN_MATCH+1.
027400000000 */
027500000000
027600000000#define MAX_DIST(s)  ((s)->w_size-MIN_LOOKAHEAD)
027700000000/* In order to simplify the code, particularly on 16 bit machines, match
027800000000 * distances are limited to MAX_DIST instead of WSIZE.
027900000000 */
028000000000
028100000000        /* in trees.c */
028200000000void _tr_init         OF((deflate_state *s));
028300000000int  _tr_tally        OF((deflate_state *s, unsigned dist, unsigned lc));
028400000000void _tr_flush_block  OF((deflate_state *s, charf *buf, ulg stored_len,
028500000000                          int eof));
028600000000void _tr_align        OF((deflate_state *s));
028700000000void _tr_stored_block OF((deflate_state *s, charf *buf, ulg stored_len,
028800000000                          int eof));
028900000000
029000000000#define d_code(dist) \
029100000000   ((dist) < 256 ? _dist_code[dist] : _dist_code[256+((dist)>>7)])
029200000000/* Mapping from a distance to a distance code. dist is the distance - 1 and
029300000000 * must not have side effects. _dist_code[256] and _dist_code[257] are never
029400000000 * used.
029500000000 */
029600000000
029700000000#ifndef DEBUG
029800000000/* Inline versions of _tr_tally for speed: */
029900000000
030000000000#if defined(GEN_TREES_H) || !defined(STDC)
030100000000  extern uch _length_code[];
030200000000  extern uch _dist_code[];
030300000000#else
030400000000  extern const uch _length_code[];
030500000000  extern const uch _dist_code[];
030600000000#endif
030700000000
030800000000# define _tr_tally_lit(s, c, flush) \
030900000000  { uch cc = (c); \
031000000000    s->d_buf[s->last_lit] = 0; \
031100000000    s->l_buf[s->last_lit++] = cc; \
031200000000    s->dyn_ltree[cc].Freq++; \
031300000000    flush = (s->last_lit == s->lit_bufsize-1); \
031400000000   }
031500000000# define _tr_tally_dist(s, distance, length, flush) \
031600000000  { uch len = (length); \
031700000000    ush dist = (distance); \
031800000000    s->d_buf[s->last_lit] = dist; \
031900000000    s->l_buf[s->last_lit++] = len; \
032000000000    dist--; \
032100000000    s->dyn_ltree[_length_code[len]+LITERALS+1].Freq++; \
032200000000    s->dyn_dtree[d_code(dist)].Freq++; \
032300000000    flush = (s->last_lit == s->lit_bufsize-1); \
032400000000  }
032500000000#else
032600000000# define _tr_tally_lit(s, c, flush) flush = _tr_tally(s, 0, c)
032700000000# define _tr_tally_dist(s, distance, length, flush) \
032800000000              flush = _tr_tally(s, distance, length)
032900000000#endif
033000000000
033100000000#endif /* DEFLATE_H */
