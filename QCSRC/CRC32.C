000100000000/* crc32.c -- compute the CRC-32 of a data stream
000200000000 * Copyright (C) 1995-2005 Mark Adler
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 *
000500000000 * Thanks to Rodney Brown <rbrown64@csc.com.au> for his contribution of faster
000600000000 * CRC methods: exclusive-oring 32 bits of data at a time, and pre-computing
000700000000 * tables for updating the shift register in one step with three exclusive-ors
000800000000 * instead of four steps with four exclusive-ors.  This results in about a
000900000000 * factor of two increase in speed on a Power PC G4 (PPC7455) using gcc -O3.
001000000000 */
001100000000
001200000000/* @(#) $Id$ */
001300000000
001400000000/*
001500000000  Note on the use of DYNAMIC_CRC_TABLE: there is no mutex or semaphore
001600000000  protection on the static variables used to control the first-use generation
001700000000  of the crc tables.  Therefore, if you #define DYNAMIC_CRC_TABLE, you should
001800000000  first call get_crc_table() to initialize the tables before allowing more than
001900000000  one thread to use crc32().
002000000000 */
002100000000
002200000000#ifdef MAKECRCH
002300000000#  include <stdio.h>
002400000000#  ifndef DYNAMIC_CRC_TABLE
002500000000#    define DYNAMIC_CRC_TABLE
002600000000#  endif /* !DYNAMIC_CRC_TABLE */
002700000000#endif /* MAKECRCH */
002800000000
002900000000#include "zutil.h"      /* for STDC and FAR definitions */
003000000000
003100000000#define local static
003200000000
003300000000/* Find a four-byte integer type for crc32_little() and crc32_big(). */
003400000000#ifndef NOBYFOUR
003500000000#  ifdef STDC           /* need ANSI C limits.h to determine sizes */
003600000000#    include <limits.h>
003700000000#    define BYFOUR
003800000000#    if (UINT_MAX == 0xffffffffUL)
003900000000       typedef unsigned int u4;
004000000000#    else
004100000000#      if (ULONG_MAX == 0xffffffffUL)
004200000000         typedef unsigned long u4;
004300000000#      else
004400000000#        if (USHRT_MAX == 0xffffffffUL)
004500000000           typedef unsigned short u4;
004600000000#        else
004700000000#          undef BYFOUR     /* can't find a four-byte integer type! */
004800000000#        endif
004900000000#      endif
005000000000#    endif
005100000000#  endif /* STDC */
005200000000#endif /* !NOBYFOUR */
005300000000
005400000000/* Definitions for doing the crc four data bytes at a time. */
005500000000#ifdef BYFOUR
005600000000#  define REV(w) (((w)>>24)+(((w)>>8)&0xff00)+ \
005700000000                (((w)&0xff00)<<8)+(((w)&0xff)<<24))
005800000000   local unsigned long crc32_little OF((unsigned long,
005900000000                        const unsigned char FAR *, unsigned));
006000000000   local unsigned long crc32_big OF((unsigned long,
006100000000                        const unsigned char FAR *, unsigned));
006200000000#  define TBLS 8
006300000000#else
006400000000#  define TBLS 1
006500000000#endif /* BYFOUR */
006600000000
006700000000/* Local functions for crc concatenation */
006800000000local unsigned long gf2_matrix_times OF((unsigned long *mat,
006900000000                                         unsigned long vec));
007000000000local void gf2_matrix_square OF((unsigned long *square, unsigned long *mat));
007100000000
007200000000#ifdef DYNAMIC_CRC_TABLE
007300000000
007400000000local volatile int crc_table_empty = 1;
007500000000local unsigned long FAR crc_table[TBLS][256];
007600000000local void make_crc_table OF((void));
007700000000#ifdef MAKECRCH
007800000000   local void write_table OF((FILE *, const unsigned long FAR *));
007900000000#endif /* MAKECRCH */
008000000000/*
008100000000  Generate tables for a byte-wise 32-bit CRC calculation on the polynomial:
008200000000  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.
008300000000
008400000000  Polynomials over GF(2) are represented in binary, one bit per coefficient,
008500000000  with the lowest powers in the most significant bit.  Then adding polynomials
008600000000  is just exclusive-or, and multiplying a polynomial by x is a right shift by
008700000000  one.  If we call the above polynomial p, and represent a byte as the
008800000000  polynomial q, also with the lowest power in the most significant bit (so the
008900000000  byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
009000000000  where a mod b means the remainder after dividing a by b.
009100000000
009200000000  This calculation is done using the shift-register method of multiplying and
009300000000  taking the remainder.  The register is initialized to zero, and for each
009400000000  incoming bit, x^32 is added mod p to the register if the bit is a one (where
009500000000  x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
009600000000  x (which is shifting right by one and adding x^32 mod p if the bit shifted
009700000000  out is a one).  We start with the highest power (least significant bit) of
009800000000  q and repeat for all eight bits of q.
009900000000
010000000000  The first table is simply the CRC of all possible eight bit values.  This is
010100000000  all the information needed to generate CRCs on data a byte at a time for all
010200000000  combinations of CRC register values and incoming bytes.  The remaining tables
010300000000  allow for word-at-a-time CRC calculation for both big-endian and little-
010400000000  endian machines, where a word is four bytes.
010500000000*/
010600000000local void make_crc_table()
010700000000{
010800000000    unsigned long c;
010900000000    int n, k;
011000000000    unsigned long poly;                 /* polynomial exclusive-or pattern */
011100000000    /* terms of polynomial defining this crc (except x^32): */
011200000000    static volatile int first = 1;      /* flag to limit concurrent making */
011300000000    static const unsigned char p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};
011400000000
011500000000    /* See if another task is already doing this (not thread-safe, but better
011600000000       than nothing -- significantly reduces duration of vulnerability in
011700000000       case the advice about DYNAMIC_CRC_TABLE is ignored) */
011800000000    if (first) {
011900000000        first = 0;
012000000000
012100000000        /* make exclusive-or pattern from polynomial (0xedb88320UL) */
012200000000        poly = 0UL;
012300000000        for (n = 0; n < sizeof(p)/sizeof(unsigned char); n++)
012400000000            poly |= 1UL << (31 - p[n]);
012500000000
012600000000        /* generate a crc for every 8-bit value */
012700000000        for (n = 0; n < 256; n++) {
012800000000            c = (unsigned long)n;
012900000000            for (k = 0; k < 8; k++)
013000000000                c = c & 1 ? poly ^ (c >> 1) : c >> 1;
013100000000            crc_table[0][n] = c;
013200000000        }
013300000000
013400000000#ifdef BYFOUR
013500000000        /* generate crc for each value followed by one, two, and three zeros,
013600000000           and then the byte reversal of those as well as the first table */
013700000000        for (n = 0; n < 256; n++) {
013800000000            c = crc_table[0][n];
013900000000            crc_table[4][n] = REV(c);
014000000000            for (k = 1; k < 4; k++) {
014100000000                c = crc_table[0][c & 0xff] ^ (c >> 8);
014200000000                crc_table[k][n] = c;
014300000000                crc_table[k + 4][n] = REV(c);
014400000000            }
014500000000        }
014600000000#endif /* BYFOUR */
014700000000
014800000000        crc_table_empty = 0;
014900000000    }
015000000000    else {      /* not first */
015100000000        /* wait for the other guy to finish (not efficient, but rare) */
015200000000        while (crc_table_empty)
015300000000            ;
015400000000    }
015500000000
015600000000#ifdef MAKECRCH
015700000000    /* write out CRC tables to crc32.h */
015800000000    {
015900000000        FILE *out;
016000000000
016100000000        out = fopen("crc32.h", "w");
016200000000        if (out == NULL) return;
016300000000        fprintf(out, "/* crc32.h -- tables for rapid CRC calculation\n");
016400000000        fprintf(out, " * Generated automatically by crc32.c\n */\n\n");
016500000000        fprintf(out, "local const unsigned long FAR ");
016600000000        fprintf(out, "crc_table[TBLS][256] =\n{\n  {\n");
016700000000        write_table(out, crc_table[0]);
016800000000#  ifdef BYFOUR
016900000000        fprintf(out, "#ifdef BYFOUR\n");
017000000000        for (k = 1; k < 8; k++) {
017100000000            fprintf(out, "  },\n  {\n");
017200000000            write_table(out, crc_table[k]);
017300000000        }
017400000000        fprintf(out, "#endif\n");
017500000000#  endif /* BYFOUR */
017600000000        fprintf(out, "  }\n};\n");
017700000000        fclose(out);
017800000000    }
017900000000#endif /* MAKECRCH */
018000000000}
018100000000
018200000000#ifdef MAKECRCH
018300000000local void write_table(out, table)
018400000000    FILE *out;
018500000000    const unsigned long FAR *table;
018600000000{
018700000000    int n;
018800000000
018900000000    for (n = 0; n < 256; n++)
019000000000        fprintf(out, "%s0x%08lxUL%s", n % 5 ? "" : "    ", table[n],
019100000000                n == 255 ? "\n" : (n % 5 == 4 ? ",\n" : ", "));
019200000000}
019300000000#endif /* MAKECRCH */
019400000000
019500000000#else /* !DYNAMIC_CRC_TABLE */
019600000000/* ========================================================================
019700000000 * Tables of CRC-32s of all single-byte values, made by make_crc_table().
019800000000 */
019900000000#include "crc32.h"
020000000000#endif /* DYNAMIC_CRC_TABLE */
020100000000
020200000000/* =========================================================================
020300000000 * This function can be used by asm versions of crc32()
020400000000 */
020500000000const unsigned long FAR * ZEXPORT get_crc_table()
020600000000{
020700000000#ifdef DYNAMIC_CRC_TABLE
020800000000    if (crc_table_empty)
020900000000        make_crc_table();
021000000000#endif /* DYNAMIC_CRC_TABLE */
021100000000    return (const unsigned long FAR *)crc_table;
021200000000}
021300000000
021400000000/* ========================================================================= */
021500000000#define DO1 crc = crc_table[0][((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8)
021600000000#define DO8 DO1; DO1; DO1; DO1; DO1; DO1; DO1; DO1
021700000000
021800000000/* ========================================================================= */
021900000000unsigned long ZEXPORT crc32(crc, buf, len)
022000000000    unsigned long crc;
022100000000    const unsigned char FAR *buf;
022200000000    unsigned len;
022300000000{
022400000000    if (buf == Z_NULL) return 0UL;
022500000000
022600000000#ifdef DYNAMIC_CRC_TABLE
022700000000    if (crc_table_empty)
022800000000        make_crc_table();
022900000000#endif /* DYNAMIC_CRC_TABLE */
023000000000
023100000000#ifdef BYFOUR
023200000000    if (sizeof(void *) == sizeof(ptrdiff_t)) {
023300000000        u4 endian;
023400000000
023500000000        endian = 1;
023600000000        if (*((unsigned char *)(&endian)))
023700000000            return crc32_little(crc, buf, len);
023800000000        else
023900000000            return crc32_big(crc, buf, len);
024000000000    }
024100000000#endif /* BYFOUR */
024200000000    crc = crc ^ 0xffffffffUL;
024300000000    while (len >= 8) {
024400000000        DO8;
024500000000        len -= 8;
024600000000    }
024700000000    if (len) do {
024800000000        DO1;
024900000000    } while (--len);
025000000000    return crc ^ 0xffffffffUL;
025100000000}
025200000000
025300000000#ifdef BYFOUR
025400000000
025500000000/* ========================================================================= */
025600000000#define DOLIT4 c ^= *buf4++; \
025700000000        c = crc_table[3][c & 0xff] ^ crc_table[2][(c >> 8) & 0xff] ^ \
025800000000            crc_table[1][(c >> 16) & 0xff] ^ crc_table[0][c >> 24]
025900000000#define DOLIT32 DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4; DOLIT4
026000000000
026100000000/* ========================================================================= */
026200000000local unsigned long crc32_little(crc, buf, len)
026300000000    unsigned long crc;
026400000000    const unsigned char FAR *buf;
026500000000    unsigned len;
026600000000{
026700000000    register u4 c;
026800000000    register const u4 FAR *buf4;
026900000000
027000000000    c = (u4)crc;
027100000000    c = ~c;
027200000000    while (len && ((ptrdiff_t)buf & 3)) {
027300000000        c = crc_table[0][(c ^ *buf++) & 0xff] ^ (c >> 8);
027400000000        len--;
027500000000    }
027600000000
027700000000    buf4 = (const u4 FAR *)(const void FAR *)buf;
027800000000    while (len >= 32) {
027900000000        DOLIT32;
028000000000        len -= 32;
028100000000    }
028200000000    while (len >= 4) {
028300000000        DOLIT4;
028400000000        len -= 4;
028500000000    }
028600000000    buf = (const unsigned char FAR *)buf4;
028700000000
028800000000    if (len) do {
028900000000        c = crc_table[0][(c ^ *buf++) & 0xff] ^ (c >> 8);
029000000000    } while (--len);
029100000000    c = ~c;
029200000000    return (unsigned long)c;
029300000000}
029400000000
029500000000/* ========================================================================= */
029600000000#define DOBIG4 c ^= *++buf4; \
029700000000        c = crc_table[4][c & 0xff] ^ crc_table[5][(c >> 8) & 0xff] ^ \
029800000000            crc_table[6][(c >> 16) & 0xff] ^ crc_table[7][c >> 24]
029900000000#define DOBIG32 DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4; DOBIG4
030000000000
030100000000/* ========================================================================= */
030200000000local unsigned long crc32_big(crc, buf, len)
030300000000    unsigned long crc;
030400000000    const unsigned char FAR *buf;
030500000000    unsigned len;
030600000000{
030700000000    register u4 c;
030800000000    register const u4 FAR *buf4;
030900000000
031000000000    c = REV((u4)crc);
031100000000    c = ~c;
031200000000    while (len && ((ptrdiff_t)buf & 3)) {
031300000000        c = crc_table[4][(c >> 24) ^ *buf++] ^ (c << 8);
031400000000        len--;
031500000000    }
031600000000
031700000000    buf4 = (const u4 FAR *)(const void FAR *)buf;
031800000000    buf4--;
031900000000    while (len >= 32) {
032000000000        DOBIG32;
032100000000        len -= 32;
032200000000    }
032300000000    while (len >= 4) {
032400000000        DOBIG4;
032500000000        len -= 4;
032600000000    }
032700000000    buf4++;
032800000000    buf = (const unsigned char FAR *)buf4;
032900000000
033000000000    if (len) do {
033100000000        c = crc_table[4][(c >> 24) ^ *buf++] ^ (c << 8);
033200000000    } while (--len);
033300000000    c = ~c;
033400000000    return (unsigned long)(REV(c));
033500000000}
033600000000
033700000000#endif /* BYFOUR */
033800000000
033900000000#define GF2_DIM 32      /* dimension of GF(2) vectors (length of CRC) */
034000000000
034100000000/* ========================================================================= */
034200000000local unsigned long gf2_matrix_times(mat, vec)
034300000000    unsigned long *mat;
034400000000    unsigned long vec;
034500000000{
034600000000    unsigned long sum;
034700000000
034800000000    sum = 0;
034900000000    while (vec) {
035000000000        if (vec & 1)
035100000000            sum ^= *mat;
035200000000        vec >>= 1;
035300000000        mat++;
035400000000    }
035500000000    return sum;
035600000000}
035700000000
035800000000/* ========================================================================= */
035900000000local void gf2_matrix_square(square, mat)
036000000000    unsigned long *square;
036100000000    unsigned long *mat;
036200000000{
036300000000    int n;
036400000000
036500000000    for (n = 0; n < GF2_DIM; n++)
036600000000        square[n] = gf2_matrix_times(mat, mat[n]);
036700000000}
036800000000
036900000000/* ========================================================================= */
037000000000uLong ZEXPORT crc32_combine(crc1, crc2, len2)
037100000000    uLong crc1;
037200000000    uLong crc2;
037300000000    z_off_t len2;
037400000000{
037500000000    int n;
037600000000    unsigned long row;
037700000000    unsigned long even[GF2_DIM];    /* even-power-of-two zeros operator */
037800000000    unsigned long odd[GF2_DIM];     /* odd-power-of-two zeros operator */
037900000000
038000000000    /* degenerate case */
038100000000    if (len2 == 0)
038200000000        return crc1;
038300000000
038400000000    /* put operator for one zero bit in odd */
038500000000    odd[0] = 0xedb88320L;           /* CRC-32 polynomial */
038600000000    row = 1;
038700000000    for (n = 1; n < GF2_DIM; n++) {
038800000000        odd[n] = row;
038900000000        row <<= 1;
039000000000    }
039100000000
039200000000    /* put operator for two zero bits in even */
039300000000    gf2_matrix_square(even, odd);
039400000000
039500000000    /* put operator for four zero bits in odd */
039600000000    gf2_matrix_square(odd, even);
039700000000
039800000000    /* apply len2 zeros to crc1 (first square will put the operator for one
039900000000       zero byte, eight zero bits, in even) */
040000000000    do {
040100000000        /* apply zeros operator for this bit of len2 */
040200000000        gf2_matrix_square(even, odd);
040300000000        if (len2 & 1)
040400000000            crc1 = gf2_matrix_times(even, crc1);
040500000000        len2 >>= 1;
040600000000
040700000000        /* if no more bits set, then done */
040800000000        if (len2 == 0)
040900000000            break;
041000000000
041100000000        /* another iteration of the loop with odd and even swapped */
041200000000        gf2_matrix_square(odd, even);
041300000000        if (len2 & 1)
041400000000            crc1 = gf2_matrix_times(odd, crc1);
041500000000        len2 >>= 1;
041600000000
041700000000        /* if no more bits set, then done */
041800000000    } while (len2 != 0);
041900000000
042000000000    /* return combined crc */
042100000000    crc1 ^= crc2;
042200000000    return crc1;
042300000000}
