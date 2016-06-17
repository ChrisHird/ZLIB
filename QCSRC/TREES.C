000100000000/* trees.c -- output deflated data using Huffman coding
000200000000 * Copyright (C) 1995-2005 Jean-loup Gailly
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/*
000700000000 *  ALGORITHM
000800000000 *
000900000000 *      The "deflation" process uses several Huffman trees. The more
001000000000 *      common source values are represented by shorter bit sequences.
001100000000 *
001200000000 *      Each code tree is stored in a compressed form which is itself
001300000000 * a Huffman encoding of the lengths of all the code strings (in
001400000000 * ascending order by source values).  The actual code strings are
001500000000 * reconstructed from the lengths in the inflate process, as described
001600000000 * in the deflate specification.
001700000000 *
001800000000 *  REFERENCES
001900000000 *
002000000000 *      Deutsch, L.P.,"'Deflate' Compressed Data Format Specification".
002100000000 *      Available in ftp.uu.net:/pub/archiving/zip/doc/deflate-1.1.doc
002200000000 *
002300000000 *      Storer, James A.
002400000000 *          Data Compression:  Methods and Theory, pp. 49-50.
002500000000 *          Computer Science Press, 1988.  ISBN 0-7167-8156-5.
002600000000 *
002700000000 *      Sedgewick, R.
002800000000 *          Algorithms, p290.
002900000000 *          Addison-Wesley, 1983. ISBN 0-201-06672-6.
003000000000 */
003100000000
003200000000/* @(#) $Id$ */
003300000000
003400000000/* #define GEN_TREES_H */
003500000000
003600000000#include "deflate.h"
003700000000
003800000000#ifdef DEBUG
003900000000#  include <ctype.h>
004000000000#endif
004100000000
004200000000/* ===========================================================================
004300000000 * Constants
004400000000 */
004500000000
004600000000#define MAX_BL_BITS 7
004700000000/* Bit length codes must not exceed MAX_BL_BITS bits */
004800000000
004900000000#define END_BLOCK 256
005000000000/* end of block literal code */
005100000000
005200000000#define REP_3_6      16
005300000000/* repeat previous bit length 3-6 times (2 bits of repeat count) */
005400000000
005500000000#define REPZ_3_10    17
005600000000/* repeat a zero length 3-10 times  (3 bits of repeat count) */
005700000000
005800000000#define REPZ_11_138  18
005900000000/* repeat a zero length 11-138 times  (7 bits of repeat count) */
006000000000
006100000000local const int extra_lbits[LENGTH_CODES] /* extra bits for each length code */
006200000000   = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
006300000000
006400000000local const int extra_dbits[D_CODES] /* extra bits for each distance code */
006500000000   = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
006600000000
006700000000local const int extra_blbits[BL_CODES]/* extra bits for each bit length code */
006800000000   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,7};
006900000000
007000000000local const uch bl_order[BL_CODES]
007100000000   = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
007200000000/* The lengths of the bit length codes are sent in order of decreasing
007300000000 * probability, to avoid transmitting the lengths for unused bit length codes.
007400000000 */
007500000000
007600000000#define Buf_size (8 * 2*sizeof(char))
007700000000/* Number of bits used within bi_buf. (bi_buf might be implemented on
007800000000 * more than 16 bits on some systems.)
007900000000 */
008000000000
008100000000/* ===========================================================================
008200000000 * Local data. These are initialized only once.
008300000000 */
008400000000
008500000000#define DIST_CODE_LEN  512 /* see definition of array dist_code below */
008600000000
008700000000#if defined(GEN_TREES_H) || !defined(STDC)
008800000000/* non ANSI compilers may not accept trees.h */
008900000000
009000000000local ct_data static_ltree[L_CODES+2];
009100000000/* The static literal tree. Since the bit lengths are imposed, there is no
009200000000 * need for the L_CODES extra codes used during heap construction. However
009300000000 * The codes 286 and 287 are needed to build a canonical tree (see _tr_init
009400000000 * below).
009500000000 */
009600000000
009700000000local ct_data static_dtree[D_CODES];
009800000000/* The static distance tree. (Actually a trivial tree since all codes use
009900000000 * 5 bits.)
010000000000 */
010100000000
010200000000uch _dist_code[DIST_CODE_LEN];
010300000000/* Distance codes. The first 256 values correspond to the distances
010400000000 * 3 .. 258, the last 256 values correspond to the top 8 bits of
010500000000 * the 15 bit distances.
010600000000 */
010700000000
010800000000uch _length_code[MAX_MATCH-MIN_MATCH+1];
010900000000/* length code for each normalized match length (0 == MIN_MATCH) */
011000000000
011100000000local int base_length[LENGTH_CODES];
011200000000/* First normalized length for each code (0 = MIN_MATCH) */
011300000000
011400000000local int base_dist[D_CODES];
011500000000/* First normalized distance for each code (0 = distance of 1) */
011600000000
011700000000#else
011800000000#  include "trees.h"
011900000000#endif /* GEN_TREES_H */
012000000000
012100000000struct static_tree_desc_s {
012200000000    const ct_data *static_tree;  /* static tree or NULL */
012300000000    const intf *extra_bits;      /* extra bits for each code or NULL */
012400000000    int     extra_base;          /* base index for extra_bits */
012500000000    int     elems;               /* max number of elements in the tree */
012600000000    int     max_length;          /* max bit length for the codes */
012700000000};
012800000000
012900000000local static_tree_desc  static_l_desc =
013000000000{static_ltree, extra_lbits, LITERALS+1, L_CODES, MAX_BITS};
013100000000
013200000000local static_tree_desc  static_d_desc =
013300000000{static_dtree, extra_dbits, 0,          D_CODES, MAX_BITS};
013400000000
013500000000local static_tree_desc  static_bl_desc =
013600000000{(const ct_data *)0, extra_blbits, 0,   BL_CODES, MAX_BL_BITS};
013700000000
013800000000/* ===========================================================================
013900000000 * Local (static) routines in this file.
014000000000 */
014100000000
014200000000local void tr_static_init OF((void));
014300000000local void init_block     OF((deflate_state *s));
014400000000local void pqdownheap     OF((deflate_state *s, ct_data *tree, int k));
014500000000local void gen_bitlen     OF((deflate_state *s, tree_desc *desc));
014600000000local void gen_codes      OF((ct_data *tree, int max_code, ushf *bl_count));
014700000000local void build_tree     OF((deflate_state *s, tree_desc *desc));
014800000000local void scan_tree      OF((deflate_state *s, ct_data *tree, int max_code));
014900000000local void send_tree      OF((deflate_state *s, ct_data *tree, int max_code));
015000000000local int  build_bl_tree  OF((deflate_state *s));
015100000000local void send_all_trees OF((deflate_state *s, int lcodes, int dcodes,
015200000000                              int blcodes));
015300000000local void compress_block OF((deflate_state *s, ct_data *ltree,
015400000000                              ct_data *dtree));
015500000000local void set_data_type  OF((deflate_state *s));
015600000000local unsigned bi_reverse OF((unsigned value, int length));
015700000000local void bi_windup      OF((deflate_state *s));
015800000000local void bi_flush       OF((deflate_state *s));
015900000000local void copy_block     OF((deflate_state *s, charf *buf, unsigned len,
016000000000                              int header));
016100000000
016200000000#ifdef GEN_TREES_H
016300000000local void gen_trees_header OF((void));
016400000000#endif
016500000000
016600000000#ifndef DEBUG
016700000000#  define send_code(s, c, tree) send_bits(s, tree[c].Code, tree[c].Len)
016800000000   /* Send a code of the given tree. c and tree must not have side effects */
016900000000
017000000000#else /* DEBUG */
017100000000#  define send_code(s, c, tree) \
017200000000     { if (z_verbose>2) fprintf(stderr,"\ncd %3d ",(c)); \
017300000000       send_bits(s, tree[c].Code, tree[c].Len); }
017400000000#endif
017500000000
017600000000/* ===========================================================================
017700000000 * Output a short LSB first on the stream.
017800000000 * IN assertion: there is enough room in pendingBuf.
017900000000 */
018000000000#define put_short(s, w) { \
018100000000    put_byte(s, (uch)((w) & 0xff)); \
018200000000    put_byte(s, (uch)((ush)(w) >> 8)); \
018300000000}
018400000000
018500000000/* ===========================================================================
018600000000 * Send a value on a given number of bits.
018700000000 * IN assertion: length <= 16 and value fits in length bits.
018800000000 */
018900000000#ifdef DEBUG
019000000000local void send_bits      OF((deflate_state *s, int value, int length));
019100000000
019200000000local void send_bits(s, value, length)
019300000000    deflate_state *s;
019400000000    int value;  /* value to send */
019500000000    int length; /* number of bits */
019600000000{
019700000000    Tracevv((stderr," l %2d v %4x ", length, value));
019800000000    Assert(length > 0 && length <= 15, "invalid length");
019900000000    s->bits_sent += (ulg)length;
020000000000
020100000000    /* If not enough room in bi_buf, use (valid) bits from bi_buf and
020200000000     * (16 - bi_valid) bits from value, leaving (width - (16-bi_valid))
020300000000     * unused bits in value.
020400000000     */
020500000000    if (s->bi_valid > (int)Buf_size - length) {
020600000000        s->bi_buf |= (value << s->bi_valid);
020700000000        put_short(s, s->bi_buf);
020800000000        s->bi_buf = (ush)value >> (Buf_size - s->bi_valid);
020900000000        s->bi_valid += length - Buf_size;
021000000000    } else {
021100000000        s->bi_buf |= value << s->bi_valid;
021200000000        s->bi_valid += length;
021300000000    }
021400000000}
021500000000#else /* !DEBUG */
021600000000
021700000000#define send_bits(s, value, length) \
021800000000{ int len = length;\
021900000000  if (s->bi_valid > (int)Buf_size - len) {\
022000000000    int val = value;\
022100000000    s->bi_buf |= (val << s->bi_valid);\
022200000000    put_short(s, s->bi_buf);\
022300000000    s->bi_buf = (ush)val >> (Buf_size - s->bi_valid);\
022400000000    s->bi_valid += len - Buf_size;\
022500000000  } else {\
022600000000    s->bi_buf |= (value) << s->bi_valid;\
022700000000    s->bi_valid += len;\
022800000000  }\
022900000000}
023000000000#endif /* DEBUG */
023100000000
023200000000
023300000000/* the arguments must not have side effects */
023400000000
023500000000/* ===========================================================================
023600000000 * Initialize the various 'constant' tables.
023700000000 */
023800000000local void tr_static_init()
023900000000{
024000000000#if defined(GEN_TREES_H) || !defined(STDC)
024100000000    static int static_init_done = 0;
024200000000    int n;        /* iterates over tree elements */
024300000000    int bits;     /* bit counter */
024400000000    int length;   /* length value */
024500000000    int code;     /* code value */
024600000000    int dist;     /* distance index */
024700000000    ush bl_count[MAX_BITS+1];
024800000000    /* number of codes at each bit length for an optimal tree */
024900000000
025000000000    if (static_init_done) return;
025100000000
025200000000    /* For some embedded targets, global variables are not initialized: */
025300000000    static_l_desc.static_tree = static_ltree;
025400000000    static_l_desc.extra_bits = extra_lbits;
025500000000    static_d_desc.static_tree = static_dtree;
025600000000    static_d_desc.extra_bits = extra_dbits;
025700000000    static_bl_desc.extra_bits = extra_blbits;
025800000000
025900000000    /* Initialize the mapping length (0..255) -> length code (0..28) */
026000000000    length = 0;
026100000000    for (code = 0; code < LENGTH_CODES-1; code++) {
026200000000        base_length[code] = length;
026300000000        for (n = 0; n < (1<<extra_lbits[code]); n++) {
026400000000            _length_code[length++] = (uch)code;
026500000000        }
026600000000    }
026700000000    Assert (length == 256, "tr_static_init: length != 256");
026800000000    /* Note that the length 255 (match length 258) can be represented
026900000000     * in two different ways: code 284 + 5 bits or code 285, so we
027000000000     * overwrite length_code[255] to use the best encoding:
027100000000     */
027200000000    _length_code[length-1] = (uch)code;
027300000000
027400000000    /* Initialize the mapping dist (0..32K) -> dist code (0..29) */
027500000000    dist = 0;
027600000000    for (code = 0 ; code < 16; code++) {
027700000000        base_dist[code] = dist;
027800000000        for (n = 0; n < (1<<extra_dbits[code]); n++) {
027900000000            _dist_code[dist++] = (uch)code;
028000000000        }
028100000000    }
028200000000    Assert (dist == 256, "tr_static_init: dist != 256");
028300000000    dist >>= 7; /* from now on, all distances are divided by 128 */
028400000000    for ( ; code < D_CODES; code++) {
028500000000        base_dist[code] = dist << 7;
028600000000        for (n = 0; n < (1<<(extra_dbits[code]-7)); n++) {
028700000000            _dist_code[256 + dist++] = (uch)code;
028800000000        }
028900000000    }
029000000000    Assert (dist == 256, "tr_static_init: 256+dist != 512");
029100000000
029200000000    /* Construct the codes of the static literal tree */
029300000000    for (bits = 0; bits <= MAX_BITS; bits++) bl_count[bits] = 0;
029400000000    n = 0;
029500000000    while (n <= 143) static_ltree[n++].Len = 8, bl_count[8]++;
029600000000    while (n <= 255) static_ltree[n++].Len = 9, bl_count[9]++;
029700000000    while (n <= 279) static_ltree[n++].Len = 7, bl_count[7]++;
029800000000    while (n <= 287) static_ltree[n++].Len = 8, bl_count[8]++;
029900000000    /* Codes 286 and 287 do not exist, but we must include them in the
030000000000     * tree construction to get a canonical Huffman tree (longest code
030100000000     * all ones)
030200000000     */
030300000000    gen_codes((ct_data *)static_ltree, L_CODES+1, bl_count);
030400000000
030500000000    /* The static distance tree is trivial: */
030600000000    for (n = 0; n < D_CODES; n++) {
030700000000        static_dtree[n].Len = 5;
030800000000        static_dtree[n].Code = bi_reverse((unsigned)n, 5);
030900000000    }
031000000000    static_init_done = 1;
031100000000
031200000000#  ifdef GEN_TREES_H
031300000000    gen_trees_header();
031400000000#  endif
031500000000#endif /* defined(GEN_TREES_H) || !defined(STDC) */
031600000000}
031700000000
031800000000/* ===========================================================================
031900000000 * Genererate the file trees.h describing the static trees.
032000000000 */
032100000000#ifdef GEN_TREES_H
032200000000#  ifndef DEBUG
032300000000#    include <stdio.h>
032400000000#  endif
032500000000
032600000000#  define SEPARATOR(i, last, width) \
032700000000      ((i) == (last)? "\n};\n\n" :    \
032800000000       ((i) % (width) == (width)-1 ? ",\n" : ", "))
032900000000
033000000000void gen_trees_header()
033100000000{
033200000000    FILE *header = fopen("trees.h", "w");
033300000000    int i;
033400000000
033500000000    Assert (header != NULL, "Can't open trees.h");
033600000000    fprintf(header,
033700000000            "/* header created automatically with -DGEN_TREES_H */\n\n");
033800000000
033900000000    fprintf(header, "local const ct_data static_ltree[L_CODES+2] = {\n");
034000000000    for (i = 0; i < L_CODES+2; i++) {
034100000000        fprintf(header, "{{%3u},{%3u}}%s", static_ltree[i].Code,
034200000000                static_ltree[i].Len, SEPARATOR(i, L_CODES+1, 5));
034300000000    }
034400000000
034500000000    fprintf(header, "local const ct_data static_dtree[D_CODES] = {\n");
034600000000    for (i = 0; i < D_CODES; i++) {
034700000000        fprintf(header, "{{%2u},{%2u}}%s", static_dtree[i].Code,
034800000000                static_dtree[i].Len, SEPARATOR(i, D_CODES-1, 5));
034900000000    }
035000000000
035100000000    fprintf(header, "const uch _dist_code[DIST_CODE_LEN] = {\n");
035200000000    for (i = 0; i < DIST_CODE_LEN; i++) {
035300000000        fprintf(header, "%2u%s", _dist_code[i],
035400000000                SEPARATOR(i, DIST_CODE_LEN-1, 20));
035500000000    }
035600000000
035700000000    fprintf(header, "const uch _length_code[MAX_MATCH-MIN_MATCH+1]= {\n");
035800000000    for (i = 0; i < MAX_MATCH-MIN_MATCH+1; i++) {
035900000000        fprintf(header, "%2u%s", _length_code[i],
036000000000                SEPARATOR(i, MAX_MATCH-MIN_MATCH, 20));
036100000000    }
036200000000
036300000000    fprintf(header, "local const int base_length[LENGTH_CODES] = {\n");
036400000000    for (i = 0; i < LENGTH_CODES; i++) {
036500000000        fprintf(header, "%1u%s", base_length[i],
036600000000                SEPARATOR(i, LENGTH_CODES-1, 20));
036700000000    }
036800000000
036900000000    fprintf(header, "local const int base_dist[D_CODES] = {\n");
037000000000    for (i = 0; i < D_CODES; i++) {
037100000000        fprintf(header, "%5u%s", base_dist[i],
037200000000                SEPARATOR(i, D_CODES-1, 10));
037300000000    }
037400000000
037500000000    fclose(header);
037600000000}
037700000000#endif /* GEN_TREES_H */
037800000000
037900000000/* ===========================================================================
038000000000 * Initialize the tree data structures for a new zlib stream.
038100000000 */
038200000000void _tr_init(s)
038300000000    deflate_state *s;
038400000000{
038500000000    tr_static_init();
038600000000
038700000000    s->l_desc.dyn_tree = s->dyn_ltree;
038800000000    s->l_desc.stat_desc = &static_l_desc;
038900000000
039000000000    s->d_desc.dyn_tree = s->dyn_dtree;
039100000000    s->d_desc.stat_desc = &static_d_desc;
039200000000
039300000000    s->bl_desc.dyn_tree = s->bl_tree;
039400000000    s->bl_desc.stat_desc = &static_bl_desc;
039500000000
039600000000    s->bi_buf = 0;
039700000000    s->bi_valid = 0;
039800000000    s->last_eob_len = 8; /* enough lookahead for inflate */
039900000000#ifdef DEBUG
040000000000    s->compressed_len = 0L;
040100000000    s->bits_sent = 0L;
040200000000#endif
040300000000
040400000000    /* Initialize the first block of the first file: */
040500000000    init_block(s);
040600000000}
040700000000
040800000000/* ===========================================================================
040900000000 * Initialize a new block.
041000000000 */
041100000000local void init_block(s)
041200000000    deflate_state *s;
041300000000{
041400000000    int n; /* iterates over tree elements */
041500000000
041600000000    /* Initialize the trees. */
041700000000    for (n = 0; n < L_CODES;  n++) s->dyn_ltree[n].Freq = 0;
041800000000    for (n = 0; n < D_CODES;  n++) s->dyn_dtree[n].Freq = 0;
041900000000    for (n = 0; n < BL_CODES; n++) s->bl_tree[n].Freq = 0;
042000000000
042100000000    s->dyn_ltree[END_BLOCK].Freq = 1;
042200000000    s->opt_len = s->static_len = 0L;
042300000000    s->last_lit = s->matches = 0;
042400000000}
042500000000
042600000000#define SMALLEST 1
042700000000/* Index within the heap array of least frequent node in the Huffman tree */
042800000000
042900000000
043000000000/* ===========================================================================
043100000000 * Remove the smallest element from the heap and recreate the heap with
043200000000 * one less element. Updates heap and heap_len.
043300000000 */
043400000000#define pqremove(s, tree, top) \
043500000000{\
043600000000    top = s->heap[SMALLEST]; \
043700000000    s->heap[SMALLEST] = s->heap[s->heap_len--]; \
043800000000    pqdownheap(s, tree, SMALLEST); \
043900000000}
044000000000
044100000000/* ===========================================================================
044200000000 * Compares to subtrees, using the tree depth as tie breaker when
044300000000 * the subtrees have equal frequency. This minimizes the worst case length.
044400000000 */
044500000000#define smaller(tree, n, m, depth) \
044600000000   (tree[n].Freq < tree[m].Freq || \
044700000000   (tree[n].Freq == tree[m].Freq && depth[n] <= depth[m]))
044800000000
044900000000/* ===========================================================================
045000000000 * Restore the heap property by moving down the tree starting at node k,
045100000000 * exchanging a node with the smallest of its two sons if necessary, stopping
045200000000 * when the heap property is re-established (each father smaller than its
045300000000 * two sons).
045400000000 */
045500000000local void pqdownheap(s, tree, k)
045600000000    deflate_state *s;
045700000000    ct_data *tree;  /* the tree to restore */
045800000000    int k;               /* node to move down */
045900000000{
046000000000    int v = s->heap[k];
046100000000    int j = k << 1;  /* left son of k */
046200000000    while (j <= s->heap_len) {
046300000000        /* Set j to the smallest of the two sons: */
046400000000        if (j < s->heap_len &&
046500000000            smaller(tree, s->heap[j+1], s->heap[j], s->depth)) {
046600000000            j++;
046700000000        }
046800000000        /* Exit if v is smaller than both sons */
046900000000        if (smaller(tree, v, s->heap[j], s->depth)) break;
047000000000
047100000000        /* Exchange v with the smallest son */
047200000000        s->heap[k] = s->heap[j];  k = j;
047300000000
047400000000        /* And continue down the tree, setting j to the left son of k */
047500000000        j <<= 1;
047600000000    }
047700000000    s->heap[k] = v;
047800000000}
047900000000
048000000000/* ===========================================================================
048100000000 * Compute the optimal bit lengths for a tree and update the total bit length
048200000000 * for the current block.
048300000000 * IN assertion: the fields freq and dad are set, heap[heap_max] and
048400000000 *    above are the tree nodes sorted by increasing frequency.
048500000000 * OUT assertions: the field len is set to the optimal bit length, the
048600000000 *     array bl_count contains the frequencies for each bit length.
048700000000 *     The length opt_len is updated; static_len is also updated if stree is
048800000000 *     not null.
048900000000 */
049000000000local void gen_bitlen(s, desc)
049100000000    deflate_state *s;
049200000000    tree_desc *desc;    /* the tree descriptor */
049300000000{
049400000000    ct_data *tree        = desc->dyn_tree;
049500000000    int max_code         = desc->max_code;
049600000000    const ct_data *stree = desc->stat_desc->static_tree;
049700000000    const intf *extra    = desc->stat_desc->extra_bits;
049800000000    int base             = desc->stat_desc->extra_base;
049900000000    int max_length       = desc->stat_desc->max_length;
050000000000    int h;              /* heap index */
050100000000    int n, m;           /* iterate over the tree elements */
050200000000    int bits;           /* bit length */
050300000000    int xbits;          /* extra bits */
050400000000    ush f;              /* frequency */
050500000000    int overflow = 0;   /* number of elements with bit length too large */
050600000000
050700000000    for (bits = 0; bits <= MAX_BITS; bits++) s->bl_count[bits] = 0;
050800000000
050900000000    /* In a first pass, compute the optimal bit lengths (which may
051000000000     * overflow in the case of the bit length tree).
051100000000     */
051200000000    tree[s->heap[s->heap_max]].Len = 0; /* root of the heap */
051300000000
051400000000    for (h = s->heap_max+1; h < HEAP_SIZE; h++) {
051500000000        n = s->heap[h];
051600000000        bits = tree[tree[n].Dad].Len + 1;
051700000000        if (bits > max_length) bits = max_length, overflow++;
051800000000        tree[n].Len = (ush)bits;
051900000000        /* We overwrite tree[n].Dad which is no longer needed */
052000000000
052100000000        if (n > max_code) continue; /* not a leaf node */
052200000000
052300000000        s->bl_count[bits]++;
052400000000        xbits = 0;
052500000000        if (n >= base) xbits = extra[n-base];
052600000000        f = tree[n].Freq;
052700000000        s->opt_len += (ulg)f * (bits + xbits);
052800000000        if (stree) s->static_len += (ulg)f * (stree[n].Len + xbits);
052900000000    }
053000000000    if (overflow == 0) return;
053100000000
053200000000    Trace((stderr,"\nbit length overflow\n"));
053300000000    /* This happens for example on obj2 and pic of the Calgary corpus */
053400000000
053500000000    /* Find the first bit length which could increase: */
053600000000    do {
053700000000        bits = max_length-1;
053800000000        while (s->bl_count[bits] == 0) bits--;
053900000000        s->bl_count[bits]--;      /* move one leaf down the tree */
054000000000        s->bl_count[bits+1] += 2; /* move one overflow item as its brother */
054100000000        s->bl_count[max_length]--;
054200000000        /* The brother of the overflow item also moves one step up,
054300000000         * but this does not affect bl_count[max_length]
054400000000         */
054500000000        overflow -= 2;
054600000000    } while (overflow > 0);
054700000000
054800000000    /* Now recompute all bit lengths, scanning in increasing frequency.
054900000000     * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
055000000000     * lengths instead of fixing only the wrong ones. This idea is taken
055100000000     * from 'ar' written by Haruhiko Okumura.)
055200000000     */
055300000000    for (bits = max_length; bits != 0; bits--) {
055400000000        n = s->bl_count[bits];
055500000000        while (n != 0) {
055600000000            m = s->heap[--h];
055700000000            if (m > max_code) continue;
055800000000            if ((unsigned) tree[m].Len != (unsigned) bits) {
055900000000                Trace((stderr,"code %d bits %d->%d\n", m, tree[m].Len, bits));
056000000000                s->opt_len += ((long)bits - (long)tree[m].Len)
056100000000                              *(long)tree[m].Freq;
056200000000                tree[m].Len = (ush)bits;
056300000000            }
056400000000            n--;
056500000000        }
056600000000    }
056700000000}
056800000000
056900000000/* ===========================================================================
057000000000 * Generate the codes for a given tree and bit counts (which need not be
057100000000 * optimal).
057200000000 * IN assertion: the array bl_count contains the bit length statistics for
057300000000 * the given tree and the field len is set for all tree elements.
057400000000 * OUT assertion: the field code is set for all tree elements of non
057500000000 *     zero code length.
057600000000 */
057700000000local void gen_codes (tree, max_code, bl_count)
057800000000    ct_data *tree;             /* the tree to decorate */
057900000000    int max_code;              /* largest code with non zero frequency */
058000000000    ushf *bl_count;            /* number of codes at each bit length */
058100000000{
058200000000    ush next_code[MAX_BITS+1]; /* next code value for each bit length */
058300000000    ush code = 0;              /* running code value */
058400000000    int bits;                  /* bit index */
058500000000    int n;                     /* code index */
058600000000
058700000000    /* The distribution counts are first used to generate the code values
058800000000     * without bit reversal.
058900000000     */
059000000000    for (bits = 1; bits <= MAX_BITS; bits++) {
059100000000        next_code[bits] = code = (code + bl_count[bits-1]) << 1;
059200000000    }
059300000000    /* Check that the bit counts in bl_count are consistent. The last code
059400000000     * must be all ones.
059500000000     */
059600000000    Assert (code + bl_count[MAX_BITS]-1 == (1<<MAX_BITS)-1,
059700000000            "inconsistent bit counts");
059800000000    Tracev((stderr,"\ngen_codes: max_code %d ", max_code));
059900000000
060000000000    for (n = 0;  n <= max_code; n++) {
060100000000        int len = tree[n].Len;
060200000000        if (len == 0) continue;
060300000000        /* Now reverse the bits */
060400000000        tree[n].Code = bi_reverse(next_code[len]++, len);
060500000000
060600000000        Tracecv(tree != static_ltree, (stderr,"\nn %3d %c l %2d c %4x (%x) ",
060700000000             n, (isgraph(n) ? n : ' '), len, tree[n].Code, next_code[len]-1));
060800000000    }
060900000000}
061000000000
061100000000/* ===========================================================================
061200000000 * Construct one Huffman tree and assigns the code bit strings and lengths.
061300000000 * Update the total bit length for the current block.
061400000000 * IN assertion: the field freq is set for all tree elements.
061500000000 * OUT assertions: the fields len and code are set to the optimal bit length
061600000000 *     and corresponding code. The length opt_len is updated; static_len is
061700000000 *     also updated if stree is not null. The field max_code is set.
061800000000 */
061900000000local void build_tree(s, desc)
062000000000    deflate_state *s;
062100000000    tree_desc *desc; /* the tree descriptor */
062200000000{
062300000000    ct_data *tree         = desc->dyn_tree;
062400000000    const ct_data *stree  = desc->stat_desc->static_tree;
062500000000    int elems             = desc->stat_desc->elems;
062600000000    int n, m;          /* iterate over heap elements */
062700000000    int max_code = -1; /* largest code with non zero frequency */
062800000000    int node;          /* new node being created */
062900000000
063000000000    /* Construct the initial heap, with least frequent element in
063100000000     * heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
063200000000     * heap[0] is not used.
063300000000     */
063400000000    s->heap_len = 0, s->heap_max = HEAP_SIZE;
063500000000
063600000000    for (n = 0; n < elems; n++) {
063700000000        if (tree[n].Freq != 0) {
063800000000            s->heap[++(s->heap_len)] = max_code = n;
063900000000            s->depth[n] = 0;
064000000000        } else {
064100000000            tree[n].Len = 0;
064200000000        }
064300000000    }
064400000000
064500000000    /* The pkzip format requires that at least one distance code exists,
064600000000     * and that at least one bit should be sent even if there is only one
064700000000     * possible code. So to avoid special checks later on we force at least
064800000000     * two codes of non zero frequency.
064900000000     */
065000000000    while (s->heap_len < 2) {
065100000000        node = s->heap[++(s->heap_len)] = (max_code < 2 ? ++max_code : 0);
065200000000        tree[node].Freq = 1;
065300000000        s->depth[node] = 0;
065400000000        s->opt_len--; if (stree) s->static_len -= stree[node].Len;
065500000000        /* node is 0 or 1 so it does not have extra bits */
065600000000    }
065700000000    desc->max_code = max_code;
065800000000
065900000000    /* The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
066000000000     * establish sub-heaps of increasing lengths:
066100000000     */
066200000000    for (n = s->heap_len/2; n >= 1; n--) pqdownheap(s, tree, n);
066300000000
066400000000    /* Construct the Huffman tree by repeatedly combining the least two
066500000000     * frequent nodes.
066600000000     */
066700000000    node = elems;              /* next internal node of the tree */
066800000000    do {
066900000000        pqremove(s, tree, n);  /* n = node of least frequency */
067000000000        m = s->heap[SMALLEST]; /* m = node of next least frequency */
067100000000
067200000000        s->heap[--(s->heap_max)] = n; /* keep the nodes sorted by frequency */
067300000000        s->heap[--(s->heap_max)] = m;
067400000000
067500000000        /* Create a new node father of n and m */
067600000000        tree[node].Freq = tree[n].Freq + tree[m].Freq;
067700000000        s->depth[node] = (uch)((s->depth[n] >= s->depth[m] ?
067800000000                                s->depth[n] : s->depth[m]) + 1);
067900000000        tree[n].Dad = tree[m].Dad = (ush)node;
068000000000#ifdef DUMP_BL_TREE
068100000000        if (tree == s->bl_tree) {
068200000000            fprintf(stderr,"\nnode %d(%d), sons %d(%d) %d(%d)",
068300000000                    node, tree[node].Freq, n, tree[n].Freq, m, tree[m].Freq);
068400000000        }
068500000000#endif
068600000000        /* and insert the new node in the heap */
068700000000        s->heap[SMALLEST] = node++;
068800000000        pqdownheap(s, tree, SMALLEST);
068900000000
069000000000    } while (s->heap_len >= 2);
069100000000
069200000000    s->heap[--(s->heap_max)] = s->heap[SMALLEST];
069300000000
069400000000    /* At this point, the fields freq and dad are set. We can now
069500000000     * generate the bit lengths.
069600000000     */
069700000000    gen_bitlen(s, (tree_desc *)desc);
069800000000
069900000000    /* The field len is now set, we can generate the bit codes */
070000000000    gen_codes ((ct_data *)tree, max_code, s->bl_count);
070100000000}
070200000000
070300000000/* ===========================================================================
070400000000 * Scan a literal or distance tree to determine the frequencies of the codes
070500000000 * in the bit length tree.
070600000000 */
070700000000local void scan_tree (s, tree, max_code)
070800000000    deflate_state *s;
070900000000    ct_data *tree;   /* the tree to be scanned */
071000000000    int max_code;    /* and its largest code of non zero frequency */
071100000000{
071200000000    int n;                     /* iterates over all tree elements */
071300000000    int prevlen = -1;          /* last emitted length */
071400000000    int curlen;                /* length of current code */
071500000000    int nextlen = tree[0].Len; /* length of next code */
071600000000    int count = 0;             /* repeat count of the current code */
071700000000    int max_count = 7;         /* max repeat count */
071800000000    int min_count = 4;         /* min repeat count */
071900000000
072000000000    if (nextlen == 0) max_count = 138, min_count = 3;
072100000000    tree[max_code+1].Len = (ush)0xffff; /* guard */
072200000000
072300000000    for (n = 0; n <= max_code; n++) {
072400000000        curlen = nextlen; nextlen = tree[n+1].Len;
072500000000        if (++count < max_count && curlen == nextlen) {
072600000000            continue;
072700000000        } else if (count < min_count) {
072800000000            s->bl_tree[curlen].Freq += count;
072900000000        } else if (curlen != 0) {
073000000000            if (curlen != prevlen) s->bl_tree[curlen].Freq++;
073100000000            s->bl_tree[REP_3_6].Freq++;
073200000000        } else if (count <= 10) {
073300000000            s->bl_tree[REPZ_3_10].Freq++;
073400000000        } else {
073500000000            s->bl_tree[REPZ_11_138].Freq++;
073600000000        }
073700000000        count = 0; prevlen = curlen;
073800000000        if (nextlen == 0) {
073900000000            max_count = 138, min_count = 3;
074000000000        } else if (curlen == nextlen) {
074100000000            max_count = 6, min_count = 3;
074200000000        } else {
074300000000            max_count = 7, min_count = 4;
074400000000        }
074500000000    }
074600000000}
074700000000
074800000000/* ===========================================================================
074900000000 * Send a literal or distance tree in compressed form, using the codes in
075000000000 * bl_tree.
075100000000 */
075200000000local void send_tree (s, tree, max_code)
075300000000    deflate_state *s;
075400000000    ct_data *tree; /* the tree to be scanned */
075500000000    int max_code;       /* and its largest code of non zero frequency */
075600000000{
075700000000    int n;                     /* iterates over all tree elements */
075800000000    int prevlen = -1;          /* last emitted length */
075900000000    int curlen;                /* length of current code */
076000000000    int nextlen = tree[0].Len; /* length of next code */
076100000000    int count = 0;             /* repeat count of the current code */
076200000000    int max_count = 7;         /* max repeat count */
076300000000    int min_count = 4;         /* min repeat count */
076400000000
076500000000    /* tree[max_code+1].Len = -1; */  /* guard already set */
076600000000    if (nextlen == 0) max_count = 138, min_count = 3;
076700000000
076800000000    for (n = 0; n <= max_code; n++) {
076900000000        curlen = nextlen; nextlen = tree[n+1].Len;
077000000000        if (++count < max_count && curlen == nextlen) {
077100000000            continue;
077200000000        } else if (count < min_count) {
077300000000            do { send_code(s, curlen, s->bl_tree); } while (--count != 0);
077400000000
077500000000        } else if (curlen != 0) {
077600000000            if (curlen != prevlen) {
077700000000                send_code(s, curlen, s->bl_tree); count--;
077800000000            }
077900000000            Assert(count >= 3 && count <= 6, " 3_6?");
078000000000            send_code(s, REP_3_6, s->bl_tree); send_bits(s, count-3, 2);
078100000000
078200000000        } else if (count <= 10) {
078300000000            send_code(s, REPZ_3_10, s->bl_tree); send_bits(s, count-3, 3);
078400000000
078500000000        } else {
078600000000            send_code(s, REPZ_11_138, s->bl_tree); send_bits(s, count-11, 7);
078700000000        }
078800000000        count = 0; prevlen = curlen;
078900000000        if (nextlen == 0) {
079000000000            max_count = 138, min_count = 3;
079100000000        } else if (curlen == nextlen) {
079200000000            max_count = 6, min_count = 3;
079300000000        } else {
079400000000            max_count = 7, min_count = 4;
079500000000        }
079600000000    }
079700000000}
079800000000
079900000000/* ===========================================================================
080000000000 * Construct the Huffman tree for the bit lengths and return the index in
080100000000 * bl_order of the last bit length code to send.
080200000000 */
080300000000local int build_bl_tree(s)
080400000000    deflate_state *s;
080500000000{
080600000000    int max_blindex;  /* index of last bit length code of non zero freq */
080700000000
080800000000    /* Determine the bit length frequencies for literal and distance trees */
080900000000    scan_tree(s, (ct_data *)s->dyn_ltree, s->l_desc.max_code);
081000000000    scan_tree(s, (ct_data *)s->dyn_dtree, s->d_desc.max_code);
081100000000
081200000000    /* Build the bit length tree: */
081300000000    build_tree(s, (tree_desc *)(&(s->bl_desc)));
081400000000    /* opt_len now includes the length of the tree representations, except
081500000000     * the lengths of the bit lengths codes and the 5+5+4 bits for the counts.
081600000000     */
081700000000
081800000000    /* Determine the number of bit length codes to send. The pkzip format
081900000000     * requires that at least 4 bit length codes be sent. (appnote.txt says
082000000000     * 3 but the actual value used is 4.)
082100000000     */
082200000000    for (max_blindex = BL_CODES-1; max_blindex >= 3; max_blindex--) {
082300000000        if (s->bl_tree[bl_order[max_blindex]].Len != 0) break;
082400000000    }
082500000000    /* Update opt_len to include the bit length tree and counts */
082600000000    s->opt_len += 3*(max_blindex+1) + 5+5+4;
082700000000    Tracev((stderr, "\ndyn trees: dyn %ld, stat %ld",
082800000000            s->opt_len, s->static_len));
082900000000
083000000000    return max_blindex;
083100000000}
083200000000
083300000000/* ===========================================================================
083400000000 * Send the header for a block using dynamic Huffman trees: the counts, the
083500000000 * lengths of the bit length codes, the literal tree and the distance tree.
083600000000 * IN assertion: lcodes >= 257, dcodes >= 1, blcodes >= 4.
083700000000 */
083800000000local void send_all_trees(s, lcodes, dcodes, blcodes)
083900000000    deflate_state *s;
084000000000    int lcodes, dcodes, blcodes; /* number of codes for each tree */
084100000000{
084200000000    int rank;                    /* index in bl_order */
084300000000
084400000000    Assert (lcodes >= 257 && dcodes >= 1 && blcodes >= 4, "not enough codes");
084500000000    Assert (lcodes <= L_CODES && dcodes <= D_CODES && blcodes <= BL_CODES,
084600000000            "too many codes");
084700000000    Tracev((stderr, "\nbl counts: "));
084800000000    send_bits(s, lcodes-257, 5); /* not +255 as stated in appnote.txt */
084900000000    send_bits(s, dcodes-1,   5);
085000000000    send_bits(s, blcodes-4,  4); /* not -3 as stated in appnote.txt */
085100000000    for (rank = 0; rank < blcodes; rank++) {
085200000000        Tracev((stderr, "\nbl code %2d ", bl_order[rank]));
085300000000        send_bits(s, s->bl_tree[bl_order[rank]].Len, 3);
085400000000    }
085500000000    Tracev((stderr, "\nbl tree: sent %ld", s->bits_sent));
085600000000
085700000000    send_tree(s, (ct_data *)s->dyn_ltree, lcodes-1); /* literal tree */
085800000000    Tracev((stderr, "\nlit tree: sent %ld", s->bits_sent));
085900000000
086000000000    send_tree(s, (ct_data *)s->dyn_dtree, dcodes-1); /* distance tree */
086100000000    Tracev((stderr, "\ndist tree: sent %ld", s->bits_sent));
086200000000}
086300000000
086400000000/* ===========================================================================
086500000000 * Send a stored block
086600000000 */
086700000000void _tr_stored_block(s, buf, stored_len, eof)
086800000000    deflate_state *s;
086900000000    charf *buf;       /* input block */
087000000000    ulg stored_len;   /* length of input block */
087100000000    int eof;          /* true if this is the last block for a file */
087200000000{
087300000000    send_bits(s, (STORED_BLOCK<<1)+eof, 3);  /* send block type */
087400000000#ifdef DEBUG
087500000000    s->compressed_len = (s->compressed_len + 3 + 7) & (ulg)~7L;
087600000000    s->compressed_len += (stored_len + 4) << 3;
087700000000#endif
087800000000    copy_block(s, buf, (unsigned)stored_len, 1); /* with header */
087900000000}
088000000000
088100000000/* ===========================================================================
088200000000 * Send one empty static block to give enough lookahead for inflate.
088300000000 * This takes 10 bits, of which 7 may remain in the bit buffer.
088400000000 * The current inflate code requires 9 bits of lookahead. If the
088500000000 * last two codes for the previous block (real code plus EOB) were coded
088600000000 * on 5 bits or less, inflate may have only 5+3 bits of lookahead to decode
088700000000 * the last real code. In this case we send two empty static blocks instead
088800000000 * of one. (There are no problems if the previous block is stored or fixed.)
088900000000 * To simplify the code, we assume the worst case of last real code encoded
089000000000 * on one bit only.
089100000000 */
089200000000void _tr_align(s)
089300000000    deflate_state *s;
089400000000{
089500000000    send_bits(s, STATIC_TREES<<1, 3);
089600000000    send_code(s, END_BLOCK, static_ltree);
089700000000#ifdef DEBUG
089800000000    s->compressed_len += 10L; /* 3 for block type, 7 for EOB */
089900000000#endif
090000000000    bi_flush(s);
090100000000    /* Of the 10 bits for the empty block, we have already sent
090200000000     * (10 - bi_valid) bits. The lookahead for the last real code (before
090300000000     * the EOB of the previous block) was thus at least one plus the length
090400000000     * of the EOB plus what we have just sent of the empty static block.
090500000000     */
090600000000    if (1 + s->last_eob_len + 10 - s->bi_valid < 9) {
090700000000        send_bits(s, STATIC_TREES<<1, 3);
090800000000        send_code(s, END_BLOCK, static_ltree);
090900000000#ifdef DEBUG
091000000000        s->compressed_len += 10L;
091100000000#endif
091200000000        bi_flush(s);
091300000000    }
091400000000    s->last_eob_len = 7;
091500000000}
091600000000
091700000000/* ===========================================================================
091800000000 * Determine the best encoding for the current block: dynamic trees, static
091900000000 * trees or store, and output the encoded block to the zip file.
092000000000 */
092100000000void _tr_flush_block(s, buf, stored_len, eof)
092200000000    deflate_state *s;
092300000000    charf *buf;       /* input block, or NULL if too old */
092400000000    ulg stored_len;   /* length of input block */
092500000000    int eof;          /* true if this is the last block for a file */
092600000000{
092700000000    ulg opt_lenb, static_lenb; /* opt_len and static_len in bytes */
092800000000    int max_blindex = 0;  /* index of last bit length code of non zero freq */
092900000000
093000000000    /* Build the Huffman trees unless a stored block is forced */
093100000000    if (s->level > 0) {
093200000000
093300000000        /* Check if the file is binary or text */
093400000000        if (stored_len > 0 && s->strm->data_type == Z_UNKNOWN)
093500000000            set_data_type(s);
093600000000
093700000000        /* Construct the literal and distance trees */
093800000000        build_tree(s, (tree_desc *)(&(s->l_desc)));
093900000000        Tracev((stderr, "\nlit data: dyn %ld, stat %ld", s->opt_len,
094000000000                s->static_len));
094100000000
094200000000        build_tree(s, (tree_desc *)(&(s->d_desc)));
094300000000        Tracev((stderr, "\ndist data: dyn %ld, stat %ld", s->opt_len,
094400000000                s->static_len));
094500000000        /* At this point, opt_len and static_len are the total bit lengths of
094600000000         * the compressed block data, excluding the tree representations.
094700000000         */
094800000000
094900000000        /* Build the bit length tree for the above two trees, and get the index
095000000000         * in bl_order of the last bit length code to send.
095100000000         */
095200000000        max_blindex = build_bl_tree(s);
095300000000
095400000000        /* Determine the best encoding. Compute the block lengths in bytes. */
095500000000        opt_lenb = (s->opt_len+3+7)>>3;
095600000000        static_lenb = (s->static_len+3+7)>>3;
095700000000
095800000000        Tracev((stderr, "\nopt %lu(%lu) stat %lu(%lu) stored %lu lit %u ",
095900000000                opt_lenb, s->opt_len, static_lenb, s->static_len, stored_len,
096000000000                s->last_lit));
096100000000
096200000000        if (static_lenb <= opt_lenb) opt_lenb = static_lenb;
096300000000
096400000000    } else {
096500000000        Assert(buf != (char*)0, "lost buf");
096600000000        opt_lenb = static_lenb = stored_len + 5; /* force a stored block */
096700000000    }
096800000000
096900000000#ifdef FORCE_STORED
097000000000    if (buf != (char*)0) { /* force stored block */
097100000000#else
097200000000    if (stored_len+4 <= opt_lenb && buf != (char*)0) {
097300000000                       /* 4: two words for the lengths */
097400000000#endif
097500000000        /* The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
097600000000         * Otherwise we can't have processed more than WSIZE input bytes since
097700000000         * the last block flush, because compression would have been
097800000000         * successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
097900000000         * transform a block into a stored block.
098000000000         */
098100000000        _tr_stored_block(s, buf, stored_len, eof);
098200000000
098300000000#ifdef FORCE_STATIC
098400000000    } else if (static_lenb >= 0) { /* force static trees */
098500000000#else
098600000000    } else if (s->strategy == Z_FIXED || static_lenb == opt_lenb) {
098700000000#endif
098800000000        send_bits(s, (STATIC_TREES<<1)+eof, 3);
098900000000        compress_block(s, (ct_data *)static_ltree, (ct_data *)static_dtree);
099000000000#ifdef DEBUG
099100000000        s->compressed_len += 3 + s->static_len;
099200000000#endif
099300000000    } else {
099400000000        send_bits(s, (DYN_TREES<<1)+eof, 3);
099500000000        send_all_trees(s, s->l_desc.max_code+1, s->d_desc.max_code+1,
099600000000                       max_blindex+1);
099700000000        compress_block(s, (ct_data *)s->dyn_ltree, (ct_data *)s->dyn_dtree);
099800000000#ifdef DEBUG
099900000000        s->compressed_len += 3 + s->opt_len;
100000000000#endif
100100000000    }
100200000000    Assert (s->compressed_len == s->bits_sent, "bad compressed size");
100300000000    /* The above check is made mod 2^32, for files larger than 512 MB
100400000000     * and uLong implemented on 32 bits.
100500000000     */
100600000000    init_block(s);
100700000000
100800000000    if (eof) {
100900000000        bi_windup(s);
101000000000#ifdef DEBUG
101100000000        s->compressed_len += 7;  /* align on byte boundary */
101200000000#endif
101300000000    }
101400000000    Tracev((stderr,"\ncomprlen %lu(%lu) ", s->compressed_len>>3,
101500000000           s->compressed_len-7*eof));
101600000000}
101700000000
101800000000/* ===========================================================================
101900000000 * Save the match info and tally the frequency counts. Return true if
102000000000 * the current block must be flushed.
102100000000 */
102200000000int _tr_tally (s, dist, lc)
102300000000    deflate_state *s;
102400000000    unsigned dist;  /* distance of matched string */
102500000000    unsigned lc;    /* match length-MIN_MATCH or unmatched char (if dist==0) */
102600000000{
102700000000    s->d_buf[s->last_lit] = (ush)dist;
102800000000    s->l_buf[s->last_lit++] = (uch)lc;
102900000000    if (dist == 0) {
103000000000        /* lc is the unmatched char */
103100000000        s->dyn_ltree[lc].Freq++;
103200000000    } else {
103300000000        s->matches++;
103400000000        /* Here, lc is the match length - MIN_MATCH */
103500000000        dist--;             /* dist = match distance - 1 */
103600000000        Assert((ush)dist < (ush)MAX_DIST(s) &&
103700000000               (ush)lc <= (ush)(MAX_MATCH-MIN_MATCH) &&
103800000000               (ush)d_code(dist) < (ush)D_CODES,  "_tr_tally: bad match");
103900000000
104000000000        s->dyn_ltree[_length_code[lc]+LITERALS+1].Freq++;
104100000000        s->dyn_dtree[d_code(dist)].Freq++;
104200000000    }
104300000000
104400000000#ifdef TRUNCATE_BLOCK
104500000000    /* Try to guess if it is profitable to stop the current block here */
104600000000    if ((s->last_lit & 0x1fff) == 0 && s->level > 2) {
104700000000        /* Compute an upper bound for the compressed length */
104800000000        ulg out_length = (ulg)s->last_lit*8L;
104900000000        ulg in_length = (ulg)((long)s->strstart - s->block_start);
105000000000        int dcode;
105100000000        for (dcode = 0; dcode < D_CODES; dcode++) {
105200000000            out_length += (ulg)s->dyn_dtree[dcode].Freq *
105300000000                (5L+extra_dbits[dcode]);
105400000000        }
105500000000        out_length >>= 3;
105600000000        Tracev((stderr,"\nlast_lit %u, in %ld, out ~%ld(%ld%%) ",
105700000000               s->last_lit, in_length, out_length,
105800000000               100L - out_length*100L/in_length));
105900000000        if (s->matches < s->last_lit/2 && out_length < in_length/2) return 1;
106000000000    }
106100000000#endif
106200000000    return (s->last_lit == s->lit_bufsize-1);
106300000000    /* We avoid equality with lit_bufsize because of wraparound at 64K
106400000000     * on 16 bit machines and because stored blocks are restricted to
106500000000     * 64K-1 bytes.
106600000000     */
106700000000}
106800000000
106900000000/* ===========================================================================
107000000000 * Send the block data compressed using the given Huffman trees
107100000000 */
107200000000local void compress_block(s, ltree, dtree)
107300000000    deflate_state *s;
107400000000    ct_data *ltree; /* literal tree */
107500000000    ct_data *dtree; /* distance tree */
107600000000{
107700000000    unsigned dist;      /* distance of matched string */
107800000000    int lc;             /* match length or unmatched char (if dist == 0) */
107900000000    unsigned lx = 0;    /* running index in l_buf */
108000000000    unsigned code;      /* the code to send */
108100000000    int extra;          /* number of extra bits to send */
108200000000
108300000000    if (s->last_lit != 0) do {
108400000000        dist = s->d_buf[lx];
108500000000        lc = s->l_buf[lx++];
108600000000        if (dist == 0) {
108700000000            send_code(s, lc, ltree); /* send a literal byte */
108800000000            Tracecv(isgraph(lc), (stderr," '%c' ", lc));
108900000000        } else {
109000000000            /* Here, lc is the match length - MIN_MATCH */
109100000000            code = _length_code[lc];
109200000000            send_code(s, code+LITERALS+1, ltree); /* send the length code */
109300000000            extra = extra_lbits[code];
109400000000            if (extra != 0) {
109500000000                lc -= base_length[code];
109600000000                send_bits(s, lc, extra);       /* send the extra length bits */
109700000000            }
109800000000            dist--; /* dist is now the match distance - 1 */
109900000000            code = d_code(dist);
110000000000            Assert (code < D_CODES, "bad d_code");
110100000000
110200000000            send_code(s, code, dtree);       /* send the distance code */
110300000000            extra = extra_dbits[code];
110400000000            if (extra != 0) {
110500000000                dist -= base_dist[code];
110600000000                send_bits(s, dist, extra);   /* send the extra distance bits */
110700000000            }
110800000000        } /* literal or match pair ? */
110900000000
111000000000        /* Check that the overlay between pending_buf and d_buf+l_buf is ok: */
111100000000        Assert((uInt)(s->pending) < s->lit_bufsize + 2*lx,
111200000000               "pendingBuf overflow");
111300000000
111400000000    } while (lx < s->last_lit);
111500000000
111600000000    send_code(s, END_BLOCK, ltree);
111700000000    s->last_eob_len = ltree[END_BLOCK].Len;
111800000000}
111900000000
112000000000/* ===========================================================================
112100000000 * Set the data type to BINARY or TEXT, using a crude approximation:
112200000000 * set it to Z_TEXT if all symbols are either printable characters (33 to 255)
112300000000 * or white spaces (9 to 13, or 32); or set it to Z_BINARY otherwise.
112400000000 * IN assertion: the fields Freq of dyn_ltree are set.
112500000000 */
112600000000local void set_data_type(s)
112700000000    deflate_state *s;
112800000000{
112900000000    int n;
113000000000
113100000000    for (n = 0; n < 9; n++)
113200000000        if (s->dyn_ltree[n].Freq != 0)
113300000000            break;
113400000000    if (n == 9)
113500000000        for (n = 14; n < 32; n++)
113600000000            if (s->dyn_ltree[n].Freq != 0)
113700000000                break;
113800000000    s->strm->data_type = (n == 32) ? Z_TEXT : Z_BINARY;
113900000000}
114000000000
114100000000/* ===========================================================================
114200000000 * Reverse the first len bits of a code, using straightforward code (a faster
114300000000 * method would use a table)
114400000000 * IN assertion: 1 <= len <= 15
114500000000 */
114600000000local unsigned bi_reverse(code, len)
114700000000    unsigned code; /* the value to invert */
114800000000    int len;       /* its bit length */
114900000000{
115000000000    register unsigned res = 0;
115100000000    do {
115200000000        res |= code & 1;
115300000000        code >>= 1, res <<= 1;
115400000000    } while (--len > 0);
115500000000    return res >> 1;
115600000000}
115700000000
115800000000/* ===========================================================================
115900000000 * Flush the bit buffer, keeping at most 7 bits in it.
116000000000 */
116100000000local void bi_flush(s)
116200000000    deflate_state *s;
116300000000{
116400000000    if (s->bi_valid == 16) {
116500000000        put_short(s, s->bi_buf);
116600000000        s->bi_buf = 0;
116700000000        s->bi_valid = 0;
116800000000    } else if (s->bi_valid >= 8) {
116900000000        put_byte(s, (Byte)s->bi_buf);
117000000000        s->bi_buf >>= 8;
117100000000        s->bi_valid -= 8;
117200000000    }
117300000000}
117400000000
117500000000/* ===========================================================================
117600000000 * Flush the bit buffer and align the output on a byte boundary
117700000000 */
117800000000local void bi_windup(s)
117900000000    deflate_state *s;
118000000000{
118100000000    if (s->bi_valid > 8) {
118200000000        put_short(s, s->bi_buf);
118300000000    } else if (s->bi_valid > 0) {
118400000000        put_byte(s, (Byte)s->bi_buf);
118500000000    }
118600000000    s->bi_buf = 0;
118700000000    s->bi_valid = 0;
118800000000#ifdef DEBUG
118900000000    s->bits_sent = (s->bits_sent+7) & ~7;
119000000000#endif
119100000000}
119200000000
119300000000/* ===========================================================================
119400000000 * Copy a stored block, storing first the length and its
119500000000 * one's complement if requested.
119600000000 */
119700000000local void copy_block(s, buf, len, header)
119800000000    deflate_state *s;
119900000000    charf    *buf;    /* the input data */
120000000000    unsigned len;     /* its length */
120100000000    int      header;  /* true if block header must be written */
120200000000{
120300000000    bi_windup(s);        /* align on byte boundary */
120400000000    s->last_eob_len = 8; /* enough lookahead for inflate */
120500000000
120600000000    if (header) {
120700000000        put_short(s, (ush)len);
120800000000        put_short(s, (ush)~len);
120900000000#ifdef DEBUG
121000000000        s->bits_sent += 2*16;
121100000000#endif
121200000000    }
121300000000#ifdef DEBUG
121400000000    s->bits_sent += (ulg)len<<3;
121500000000#endif
121600000000    while (len--) {
121700000000        put_byte(s, *buf++);
121800000000    }
121900000000}
