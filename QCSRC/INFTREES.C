000100000000/* inftrees.c -- generate Huffman trees for efficient decoding
000200000000 * Copyright (C) 1995-2005 Mark Adler
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000#include "zutil.h"
000700000000#include "inftrees.h"
000800000000
000900000000#define MAXBITS 15
001000000000
001100000000const char inflate_copyright[] =
001200000000   " inflate 1.2.3 Copyright 1995-2005 Mark Adler ";
001300000000/*
001400000000  If you use the zlib library in a product, an acknowledgment is welcome
001500000000  in the documentation of your product. If for some reason you cannot
001600000000  include such an acknowledgment, I would appreciate that you keep this
001700000000  copyright string in the executable of your product.
001800000000 */
001900000000
002000000000/*
002100000000   Build a set of tables to decode the provided canonical Huffman code.
002200000000   The code lengths are lens[0..codes-1].  The result starts at *table,
002300000000   whose indices are 0..2^bits-1.  work is a writable array of at least
002400000000   lens shorts, which is used as a work area.  type is the type of code
002500000000   to be generated, CODES, LENS, or DISTS.  On return, zero is success,
002600000000   -1 is an invalid code, and +1 means that ENOUGH isn't enough.  table
002700000000   on return points to the next available entry's address.  bits is the
002800000000   requested root table index bits, and on return it is the actual root
002900000000   table index bits.  It will differ if the request is greater than the
003000000000   longest code or if it is less than the shortest code.
003100000000 */
003200000000int inflate_table(type, lens, codes, table, bits, work)
003300000000codetype type;
003400000000unsigned short FAR *lens;
003500000000unsigned codes;
003600000000code FAR * FAR *table;
003700000000unsigned FAR *bits;
003800000000unsigned short FAR *work;
003900000000{
004000000000    unsigned len;               /* a code's length in bits */
004100000000    unsigned sym;               /* index of code symbols */
004200000000    unsigned min, max;          /* minimum and maximum code lengths */
004300000000    unsigned root;              /* number of index bits for root table */
004400000000    unsigned curr;              /* number of index bits for current table */
004500000000    unsigned drop;              /* code bits to drop for sub-table */
004600000000    int left;                   /* number of prefix codes available */
004700000000    unsigned used;              /* code entries in table used */
004800000000    unsigned huff;              /* Huffman code */
004900000000    unsigned incr;              /* for incrementing code, index */
005000000000    unsigned fill;              /* index for replicating entries */
005100000000    unsigned low;               /* low bits for current root entry */
005200000000    unsigned mask;              /* mask for low root bits */
005300000000    code this;                  /* table entry for duplication */
005400000000    code FAR *next;             /* next available space in table */
005500000000    const unsigned short FAR *base;     /* base value table to use */
005600000000    const unsigned short FAR *extra;    /* extra bits table to use */
005700000000    int end;                    /* use base and extra for symbol > end */
005800000000    unsigned short count[MAXBITS+1];    /* number of codes of each length */
005900000000    unsigned short offs[MAXBITS+1];     /* offsets in table for each length */
006000000000    static const unsigned short lbase[31] = { /* Length codes 257..285 base */
006100000000        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
006200000000        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
006300000000    static const unsigned short lext[31] = { /* Length codes 257..285 extra */
006400000000        16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18,
006500000000        19, 19, 19, 19, 20, 20, 20, 20, 21, 21, 21, 21, 16, 201, 196};
006600000000    static const unsigned short dbase[32] = { /* Distance codes 0..29 base */
006700000000        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
006800000000        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
006900000000        8193, 12289, 16385, 24577, 0, 0};
007000000000    static const unsigned short dext[32] = { /* Distance codes 0..29 extra */
007100000000        16, 16, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
007200000000        23, 23, 24, 24, 25, 25, 26, 26, 27, 27,
007300000000        28, 28, 29, 29, 64, 64};
007400000000
007500000000    /*
007600000000       Process a set of code lengths to create a canonical Huffman code.  The
007700000000       code lengths are lens[0..codes-1].  Each length corresponds to the
007800000000       symbols 0..codes-1.  The Huffman code is generated by first sorting the
007900000000       symbols by length from short to long, and retaining the symbol order
008000000000       for codes with equal lengths.  Then the code starts with all zero bits
008100000000       for the first code of the shortest length, and the codes are integer
008200000000       increments for the same length, and zeros are appended as the length
008300000000       increases.  For the deflate format, these bits are stored backwards
008400000000       from their more natural integer increment ordering, and so when the
008500000000       decoding tables are built in the large loop below, the integer codes
008600000000       are incremented backwards.
008700000000
008800000000       This routine assumes, but does not check, that all of the entries in
008900000000       lens[] are in the range 0..MAXBITS.  The caller must assure this.
009000000000       1..MAXBITS is interpreted as that code length.  zero means that that
009100000000       symbol does not occur in this code.
009200000000
009300000000       The codes are sorted by computing a count of codes for each length,
009400000000       creating from that a table of starting indices for each length in the
009500000000       sorted table, and then entering the symbols in order in the sorted
009600000000       table.  The sorted table is work[], with that space being provided by
009700000000       the caller.
009800000000
009900000000       The length counts are used for other purposes as well, i.e. finding
010000000000       the minimum and maximum length codes, determining if there are any
010100000000       codes at all, checking for a valid set of lengths, and looking ahead
010200000000       at length counts to determine sub-table sizes when building the
010300000000       decoding tables.
010400000000     */
010500000000
010600000000    /* accumulate lengths for codes (assumes lens[] all in 0..MAXBITS) */
010700000000    for (len = 0; len <= MAXBITS; len++)
010800000000        count[len] = 0;
010900000000    for (sym = 0; sym < codes; sym++)
011000000000        count[lens[sym]]++;
011100000000
011200000000    /* bound code lengths, force root to be within code lengths */
011300000000    root = *bits;
011400000000    for (max = MAXBITS; max >= 1; max--)
011500000000        if (count[max] != 0) break;
011600000000    if (root > max) root = max;
011700000000    if (max == 0) {                     /* no symbols to code at all */
011800000000        this.op = (unsigned char)64;    /* invalid code marker */
011900000000        this.bits = (unsigned char)1;
012000000000        this.val = (unsigned short)0;
012100000000        *(*table)++ = this;             /* make a table to force an error */
012200000000        *(*table)++ = this;
012300000000        *bits = 1;
012400000000        return 0;     /* no symbols, but wait for decoding to report error */
012500000000    }
012600000000    for (min = 1; min <= MAXBITS; min++)
012700000000        if (count[min] != 0) break;
012800000000    if (root < min) root = min;
012900000000
013000000000    /* check for an over-subscribed or incomplete set of lengths */
013100000000    left = 1;
013200000000    for (len = 1; len <= MAXBITS; len++) {
013300000000        left <<= 1;
013400000000        left -= count[len];
013500000000        if (left < 0) return -1;        /* over-subscribed */
013600000000    }
013700000000    if (left > 0 && (type == CODES || max != 1))
013800000000        return -1;                      /* incomplete set */
013900000000
014000000000    /* generate offsets into symbol table for each length for sorting */
014100000000    offs[1] = 0;
014200000000    for (len = 1; len < MAXBITS; len++)
014300000000        offs[len + 1] = offs[len] + count[len];
014400000000
014500000000    /* sort symbols by length, by symbol order within each length */
014600000000    for (sym = 0; sym < codes; sym++)
014700000000        if (lens[sym] != 0) work[offs[lens[sym]]++] = (unsigned short)sym;
014800000000
014900000000    /*
015000000000       Create and fill in decoding tables.  In this loop, the table being
015100000000       filled is at next and has curr index bits.  The code being used is huff
015200000000       with length len.  That code is converted to an index by dropping drop
015300000000       bits off of the bottom.  For codes where len is less than drop + curr,
015400000000       those top drop + curr - len bits are incremented through all values to
015500000000       fill the table with replicated entries.
015600000000
015700000000       root is the number of index bits for the root table.  When len exceeds
015800000000       root, sub-tables are created pointed to by the root entry with an index
015900000000       of the low root bits of huff.  This is saved in low to check for when a
016000000000       new sub-table should be started.  drop is zero when the root table is
016100000000       being filled, and drop is root when sub-tables are being filled.
016200000000
016300000000       When a new sub-table is needed, it is necessary to look ahead in the
016400000000       code lengths to determine what size sub-table is needed.  The length
016500000000       counts are used for this, and so count[] is decremented as codes are
016600000000       entered in the tables.
016700000000
016800000000       used keeps track of how many table entries have been allocated from the
016900000000       provided *table space.  It is checked when a LENS table is being made
017000000000       against the space in *table, ENOUGH, minus the maximum space needed by
017100000000       the worst case distance code, MAXD.  This should never happen, but the
017200000000       sufficiency of ENOUGH has not been proven exhaustively, hence the check.
017300000000       This assumes that when type == LENS, bits == 9.
017400000000
017500000000       sym increments through all symbols, and the loop terminates when
017600000000       all codes of length max, i.e. all codes, have been processed.  This
017700000000       routine permits incomplete codes, so another loop after this one fills
017800000000       in the rest of the decoding tables with invalid code markers.
017900000000     */
018000000000
018100000000    /* set up for code type */
018200000000    switch (type) {
018300000000    case CODES:
018400000000        base = extra = work;    /* dummy value--not used */
018500000000        end = 19;
018600000000        break;
018700000000    case LENS:
018800000000        base = lbase;
018900000000        base -= 257;
019000000000        extra = lext;
019100000000        extra -= 257;
019200000000        end = 256;
019300000000        break;
019400000000    default:            /* DISTS */
019500000000        base = dbase;
019600000000        extra = dext;
019700000000        end = -1;
019800000000    }
019900000000
020000000000    /* initialize state for loop */
020100000000    huff = 0;                   /* starting code */
020200000000    sym = 0;                    /* starting code symbol */
020300000000    len = min;                  /* starting code length */
020400000000    next = *table;              /* current table to fill in */
020500000000    curr = root;                /* current table index bits */
020600000000    drop = 0;                   /* current bits to drop from code for index */
020700000000    low = (unsigned)(-1);       /* trigger new sub-table when len > root */
020800000000    used = 1U << root;          /* use root table entries */
020900000000    mask = used - 1;            /* mask for comparing low */
021000000000
021100000000    /* check available table space */
021200000000    if (type == LENS && used >= ENOUGH - MAXD)
021300000000        return 1;
021400000000
021500000000    /* process all codes and make table entries */
021600000000    for (;;) {
021700000000        /* create table entry */
021800000000        this.bits = (unsigned char)(len - drop);
021900000000        if ((int)(work[sym]) < end) {
022000000000            this.op = (unsigned char)0;
022100000000            this.val = work[sym];
022200000000        }
022300000000        else if ((int)(work[sym]) > end) {
022400000000            this.op = (unsigned char)(extra[work[sym]]);
022500000000            this.val = base[work[sym]];
022600000000        }
022700000000        else {
022800000000            this.op = (unsigned char)(32 + 64);         /* end of block */
022900000000            this.val = 0;
023000000000        }
023100000000
023200000000        /* replicate for those indices with low len bits equal to huff */
023300000000        incr = 1U << (len - drop);
023400000000        fill = 1U << curr;
023500000000        min = fill;                 /* save offset to next table */
023600000000        do {
023700000000            fill -= incr;
023800000000            next[(huff >> drop) + fill] = this;
023900000000        } while (fill != 0);
024000000000
024100000000        /* backwards increment the len-bit code huff */
024200000000        incr = 1U << (len - 1);
024300000000        while (huff & incr)
024400000000            incr >>= 1;
024500000000        if (incr != 0) {
024600000000            huff &= incr - 1;
024700000000            huff += incr;
024800000000        }
024900000000        else
025000000000            huff = 0;
025100000000
025200000000        /* go to next symbol, update count, len */
025300000000        sym++;
025400000000        if (--(count[len]) == 0) {
025500000000            if (len == max) break;
025600000000            len = lens[work[sym]];
025700000000        }
025800000000
025900000000        /* create new sub-table if needed */
026000000000        if (len > root && (huff & mask) != low) {
026100000000            /* if first time, transition to sub-tables */
026200000000            if (drop == 0)
026300000000                drop = root;
026400000000
026500000000            /* increment past last table */
026600000000            next += min;            /* here min is 1 << curr */
026700000000
026800000000            /* determine length of next table */
026900000000            curr = len - drop;
027000000000            left = (int)(1 << curr);
027100000000            while (curr + drop < max) {
027200000000                left -= count[curr + drop];
027300000000                if (left <= 0) break;
027400000000                curr++;
027500000000                left <<= 1;
027600000000            }
027700000000
027800000000            /* check for enough space */
027900000000            used += 1U << curr;
028000000000            if (type == LENS && used >= ENOUGH - MAXD)
028100000000                return 1;
028200000000
028300000000            /* point entry in root table to sub-table */
028400000000            low = huff & mask;
028500000000            (*table)[low].op = (unsigned char)curr;
028600000000            (*table)[low].bits = (unsigned char)root;
028700000000            (*table)[low].val = (unsigned short)(next - *table);
028800000000        }
028900000000    }
029000000000
029100000000    /*
029200000000       Fill in rest of table for incomplete codes.  This loop is similar to the
029300000000       loop above in incrementing huff for table indices.  It is assumed that
029400000000       len is equal to curr + drop, so there is no loop needed to increment
029500000000       through high index bits.  When the current sub-table is filled, the loop
029600000000       drops back to the root table to fill in any remaining entries there.
029700000000     */
029800000000    this.op = (unsigned char)64;                /* invalid code marker */
029900000000    this.bits = (unsigned char)(len - drop);
030000000000    this.val = (unsigned short)0;
030100000000    while (huff != 0) {
030200000000        /* when done with sub-table, drop back to root table */
030300000000        if (drop != 0 && (huff & mask) != low) {
030400000000            drop = 0;
030500000000            len = root;
030600000000            next = *table;
030700000000            this.bits = (unsigned char)len;
030800000000        }
030900000000
031000000000        /* put invalid code marker in table */
031100000000        next[huff >> drop] = this;
031200000000
031300000000        /* backwards increment the len-bit code huff */
031400000000        incr = 1U << (len - 1);
031500000000        while (huff & incr)
031600000000            incr >>= 1;
031700000000        if (incr != 0) {
031800000000            huff &= incr - 1;
031900000000            huff += incr;
032000000000        }
032100000000        else
032200000000            huff = 0;
032300000000    }
032400000000
032500000000    /* set return parameters */
032600000000    *table += used;
032700000000    *bits = root;
032800000000    return 0;
032900000000}
