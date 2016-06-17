000100000000/* adler32.c -- compute the Adler-32 checksum of a data stream
000200000000 * Copyright (C) 1995-2004 Mark Adler
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/* @(#) $Id$ */
000700000000
000800000000#define ZLIB_INTERNAL
000900000000#include "zlib.h"
001000000000
001100000000#define BASE 65521UL    /* largest prime smaller than 65536 */
001200000000#define NMAX 5552
001300000000/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */
001400000000
001500000000#define DO1(buf,i)  {adler += (buf)[i]; sum2 += adler;}
001600000000#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
001700000000#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
001800000000#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
001900000000#define DO16(buf)   DO8(buf,0); DO8(buf,8);
002000000000
002100000000/* use NO_DIVIDE if your processor does not do division in hardware */
002200000000#ifdef NO_DIVIDE
002300000000#  define MOD(a) \
002400000000    do { \
002500000000        if (a >= (BASE << 16)) a -= (BASE << 16); \
002600000000        if (a >= (BASE << 15)) a -= (BASE << 15); \
002700000000        if (a >= (BASE << 14)) a -= (BASE << 14); \
002800000000        if (a >= (BASE << 13)) a -= (BASE << 13); \
002900000000        if (a >= (BASE << 12)) a -= (BASE << 12); \
003000000000        if (a >= (BASE << 11)) a -= (BASE << 11); \
003100000000        if (a >= (BASE << 10)) a -= (BASE << 10); \
003200000000        if (a >= (BASE << 9)) a -= (BASE << 9); \
003300000000        if (a >= (BASE << 8)) a -= (BASE << 8); \
003400000000        if (a >= (BASE << 7)) a -= (BASE << 7); \
003500000000        if (a >= (BASE << 6)) a -= (BASE << 6); \
003600000000        if (a >= (BASE << 5)) a -= (BASE << 5); \
003700000000        if (a >= (BASE << 4)) a -= (BASE << 4); \
003800000000        if (a >= (BASE << 3)) a -= (BASE << 3); \
003900000000        if (a >= (BASE << 2)) a -= (BASE << 2); \
004000000000        if (a >= (BASE << 1)) a -= (BASE << 1); \
004100000000        if (a >= BASE) a -= BASE; \
004200000000    } while (0)
004300000000#  define MOD4(a) \
004400000000    do { \
004500000000        if (a >= (BASE << 4)) a -= (BASE << 4); \
004600000000        if (a >= (BASE << 3)) a -= (BASE << 3); \
004700000000        if (a >= (BASE << 2)) a -= (BASE << 2); \
004800000000        if (a >= (BASE << 1)) a -= (BASE << 1); \
004900000000        if (a >= BASE) a -= BASE; \
005000000000    } while (0)
005100000000#else
005200000000#  define MOD(a) a %= BASE
005300000000#  define MOD4(a) a %= BASE
005400000000#endif
005500000000
005600000000/* ========================================================================= */
005700000000uLong ZEXPORT adler32(adler, buf, len)
005800000000    uLong adler;
005900000000    const Bytef *buf;
006000000000    uInt len;
006100000000{
006200000000    unsigned long sum2;
006300000000    unsigned n;
006400000000
006500000000    /* split Adler-32 into component sums */
006600000000    sum2 = (adler >> 16) & 0xffff;
006700000000    adler &= 0xffff;
006800000000
006900000000    /* in case user likes doing a byte at a time, keep it fast */
007000000000    if (len == 1) {
007100000000        adler += buf[0];
007200000000        if (adler >= BASE)
007300000000            adler -= BASE;
007400000000        sum2 += adler;
007500000000        if (sum2 >= BASE)
007600000000            sum2 -= BASE;
007700000000        return adler | (sum2 << 16);
007800000000    }
007900000000
008000000000    /* initial Adler-32 value (deferred check for len == 1 speed) */
008100000000    if (buf == Z_NULL)
008200000000        return 1L;
008300000000
008400000000    /* in case short lengths are provided, keep it somewhat fast */
008500000000    if (len < 16) {
008600000000        while (len--) {
008700000000            adler += *buf++;
008800000000            sum2 += adler;
008900000000        }
009000000000        if (adler >= BASE)
009100000000            adler -= BASE;
009200000000        MOD4(sum2);             /* only added so many BASE's */
009300000000        return adler | (sum2 << 16);
009400000000    }
009500000000
009600000000    /* do length NMAX blocks -- requires just one modulo operation */
009700000000    while (len >= NMAX) {
009800000000        len -= NMAX;
009900000000        n = NMAX / 16;          /* NMAX is divisible by 16 */
010000000000        do {
010100000000            DO16(buf);          /* 16 sums unrolled */
010200000000            buf += 16;
010300000000        } while (--n);
010400000000        MOD(adler);
010500000000        MOD(sum2);
010600000000    }
010700000000
010800000000    /* do remaining bytes (less than NMAX, still just one modulo) */
010900000000    if (len) {                  /* avoid modulos if none remaining */
011000000000        while (len >= 16) {
011100000000            len -= 16;
011200000000            DO16(buf);
011300000000            buf += 16;
011400000000        }
011500000000        while (len--) {
011600000000            adler += *buf++;
011700000000            sum2 += adler;
011800000000        }
011900000000        MOD(adler);
012000000000        MOD(sum2);
012100000000    }
012200000000
012300000000    /* return recombined sums */
012400000000    return adler | (sum2 << 16);
012500000000}
012600000000
012700000000/* ========================================================================= */
012800000000uLong ZEXPORT adler32_combine(adler1, adler2, len2)
012900000000    uLong adler1;
013000000000    uLong adler2;
013100000000    z_off_t len2;
013200000000{
013300000000    unsigned long sum1;
013400000000    unsigned long sum2;
013500000000    unsigned rem;
013600000000
013700000000    /* the derivation of this formula is left as an exercise for the reader */
013800000000    rem = (unsigned)(len2 % BASE);
013900000000    sum1 = adler1 & 0xffff;
014000000000    sum2 = rem * sum1;
014100000000    MOD(sum2);
014200000000    sum1 += (adler2 & 0xffff) + BASE - 1;
014300000000    sum2 += ((adler1 >> 16) & 0xffff) + ((adler2 >> 16) & 0xffff) + BASE - rem;
014400000000    if (sum1 > BASE) sum1 -= BASE;
014500000000    if (sum1 > BASE) sum1 -= BASE;
014600000000    if (sum2 > (BASE << 1)) sum2 -= (BASE << 1);
014700000000    if (sum2 > BASE) sum2 -= BASE;
014800000000    return sum1 | (sum2 << 16);
014900000000}
