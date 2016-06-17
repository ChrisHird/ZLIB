000100000000/* inflate.c -- zlib decompression
000200000000 * Copyright (C) 1995-2005 Mark Adler
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/*
000700000000 * Change history:
000800000000 *
000900000000 * 1.2.beta0    24 Nov 2002
001000000000 * - First version -- complete rewrite of inflate to simplify code, avoid
001100000000 *   creation of window when not needed, minimize use of window when it is
001200000000 *   needed, make inffast.c even faster, implement gzip decoding, and to
001300000000 *   improve code readability and style over the previous zlib inflate code
001400000000 *
001500000000 * 1.2.beta1    25 Nov 2002
001600000000 * - Use pointers for available input and output checking in inffast.c
001700000000 * - Remove input and output counters in inffast.c
001800000000 * - Change inffast.c entry and loop from avail_in >= 7 to >= 6
001900000000 * - Remove unnecessary second byte pull from length extra in inffast.c
002000000000 * - Unroll direct copy to three copies per loop in inffast.c
002100000000 *
002200000000 * 1.2.beta2    4 Dec 2002
002300000000 * - Change external routine names to reduce potential conflicts
002400000000 * - Correct filename to inffixed.h for fixed tables in inflate.c
002500000000 * - Make hbuf[] unsigned char to match parameter type in inflate.c
002600000000 * - Change strm->next_out[-state->offset] to *(strm->next_out - state->offset)
002700000000 *   to avoid negation problem on Alphas (64 bit) in inflate.c
002800000000 *
002900000000 * 1.2.beta3    22 Dec 2002
003000000000 * - Add comments on state->bits assertion in inffast.c
003100000000 * - Add comments on op field in inftrees.h
003200000000 * - Fix bug in reuse of allocated window after inflateReset()
003300000000 * - Remove bit fields--back to byte structure for speed
003400000000 * - Remove distance extra == 0 check in inflate_fast()--only helps for lengths
003500000000 * - Change post-increments to pre-increments in inflate_fast(), PPC biased?
003600000000 * - Add compile time option, POSTINC, to use post-increments instead (Intel?)
003700000000 * - Make MATCH copy in inflate() much faster for when inflate_fast() not used
003800000000 * - Use local copies of stream next and avail values, as well as local bit
003900000000 *   buffer and bit count in inflate()--for speed when inflate_fast() not used
004000000000 *
004100000000 * 1.2.beta4    1 Jan 2003
004200000000 * - Split ptr - 257 statements in inflate_table() to avoid compiler warnings
004300000000 * - Move a comment on output buffer sizes from inffast.c to inflate.c
004400000000 * - Add comments in inffast.c to introduce the inflate_fast() routine
004500000000 * - Rearrange window copies in inflate_fast() for speed and simplification
004600000000 * - Unroll last copy for window match in inflate_fast()
004700000000 * - Use local copies of window variables in inflate_fast() for speed
004800000000 * - Pull out common write == 0 case for speed in inflate_fast()
004900000000 * - Make op and len in inflate_fast() unsigned for consistency
005000000000 * - Add FAR to lcode and dcode declarations in inflate_fast()
005100000000 * - Simplified bad distance check in inflate_fast()
005200000000 * - Added inflateBackInit(), inflateBack(), and inflateBackEnd() in new
005300000000 *   source file infback.c to provide a call-back interface to inflate for
005400000000 *   programs like gzip and unzip -- uses window as output buffer to avoid
005500000000 *   window copying
005600000000 *
005700000000 * 1.2.beta5    1 Jan 2003
005800000000 * - Improved inflateBack() interface to allow the caller to provide initial
005900000000 *   input in strm.
006000000000 * - Fixed stored blocks bug in inflateBack()
006100000000 *
006200000000 * 1.2.beta6    4 Jan 2003
006300000000 * - Added comments in inffast.c on effectiveness of POSTINC
006400000000 * - Typecasting all around to reduce compiler warnings
006500000000 * - Changed loops from while (1) or do {} while (1) to for (;;), again to
006600000000 *   make compilers happy
006700000000 * - Changed type of window in inflateBackInit() to unsigned char *
006800000000 *
006900000000 * 1.2.beta7    27 Jan 2003
007000000000 * - Changed many types to unsigned or unsigned short to avoid warnings
007100000000 * - Added inflateCopy() function
007200000000 *
007300000000 * 1.2.0        9 Mar 2003
007400000000 * - Changed inflateBack() interface to provide separate opaque descriptors
007500000000 *   for the in() and out() functions
007600000000 * - Changed inflateBack() argument and in_func typedef to swap the length
007700000000 *   and buffer address return values for the input function
007800000000 * - Check next_in and next_out for Z_NULL on entry to inflate()
007900000000 *
008000000000 * The history for versions after 1.2.0 are in ChangeLog in zlib distribution.
008100000000 */
008200000000
008300000000#include "zutil.h"
008400000000#include "inftrees.h"
008500000000#include "inflate.h"
008600000000#include "inffast.h"
008700000000
008800000000#ifdef MAKEFIXED
008900000000#  ifndef BUILDFIXED
009000000000#    define BUILDFIXED
009100000000#  endif
009200000000#endif
009300000000
009400000000/* function prototypes */
009500000000local void fixedtables OF((struct inflate_state FAR *state));
009600000000local int updatewindow OF((z_streamp strm, unsigned out));
009700000000#ifdef BUILDFIXED
009800000000   void makefixed OF((void));
009900000000#endif
010000000000local unsigned syncsearch OF((unsigned FAR *have, unsigned char FAR *buf,
010100000000                              unsigned len));
010200000000
010300000000int ZEXPORT inflateReset(strm)
010400000000z_streamp strm;
010500000000{
010600000000    struct inflate_state FAR *state;
010700000000
010800000000    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
010900000000    state = (struct inflate_state FAR *)strm->state;
011000000000    strm->total_in = strm->total_out = state->total = 0;
011100000000    strm->msg = Z_NULL;
011200000000    strm->adler = 1;        /* to support ill-conceived Java test suite */
011300000000    state->mode = HEAD;
011400000000    state->last = 0;
011500000000    state->havedict = 0;
011600000000    state->dmax = 32768U;
011700000000    state->head = Z_NULL;
011800000000    state->wsize = 0;
011900000000    state->whave = 0;
012000000000    state->write = 0;
012100000000    state->hold = 0;
012200000000    state->bits = 0;
012300000000    state->lencode = state->distcode = state->next = state->codes;
012400000000    Tracev((stderr, "inflate: reset\n"));
012500000000    return Z_OK;
012600000000}
012700000000
012800000000int ZEXPORT inflatePrime(strm, bits, value)
012900000000z_streamp strm;
013000000000int bits;
013100000000int value;
013200000000{
013300000000    struct inflate_state FAR *state;
013400000000
013500000000    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
013600000000    state = (struct inflate_state FAR *)strm->state;
013700000000    if (bits > 16 || state->bits + bits > 32) return Z_STREAM_ERROR;
013800000000    value &= (1L << bits) - 1;
013900000000    state->hold += value << state->bits;
014000000000    state->bits += bits;
014100000000    return Z_OK;
014200000000}
014300000000
014400000000int ZEXPORT inflateInit2_(strm, windowBits, version, stream_size)
014500000000z_streamp strm;
014600000000int windowBits;
014700000000const char *version;
014800000000int stream_size;
014900000000{
015000000000    struct inflate_state FAR *state;
015100000000
015200000000    if (version == Z_NULL || version[0] != ZLIB_VERSION[0] ||
015300000000        stream_size != (int)(sizeof(z_stream)))
015400000000        return Z_VERSION_ERROR;
015500000000    if (strm == Z_NULL) return Z_STREAM_ERROR;
015600000000    strm->msg = Z_NULL;                 /* in case we return an error */
015700000000    if (strm->zalloc == (alloc_func)0) {
015800000000        strm->zalloc = zcalloc;
015900000000        strm->opaque = (voidpf)0;
016000000000    }
016100000000    if (strm->zfree == (free_func)0) strm->zfree = zcfree;
016200000000    state = (struct inflate_state FAR *)
016300000000            ZALLOC(strm, 1, sizeof(struct inflate_state));
016400000000    if (state == Z_NULL) return Z_MEM_ERROR;
016500000000    Tracev((stderr, "inflate: allocated\n"));
016600000000    strm->state = (struct internal_state FAR *)state;
016700000000    if (windowBits < 0) {
016800000000        state->wrap = 0;
016900000000        windowBits = -windowBits;
017000000000    }
017100000000    else {
017200000000        state->wrap = (windowBits >> 4) + 1;
017300000000#ifdef GUNZIP
017400000000        if (windowBits < 48) windowBits &= 15;
017500000000#endif
017600000000    }
017700000000    if (windowBits < 8 || windowBits > 15) {
017800000000        ZFREE(strm, state);
017900000000        strm->state = Z_NULL;
018000000000        return Z_STREAM_ERROR;
018100000000    }
018200000000    state->wbits = (unsigned)windowBits;
018300000000    state->window = Z_NULL;
018400000000    return inflateReset(strm);
018500000000}
018600000000
018700000000int ZEXPORT inflateInit_(strm, version, stream_size)
018800000000z_streamp strm;
018900000000const char *version;
019000000000int stream_size;
019100000000{
019200000000    return inflateInit2_(strm, DEF_WBITS, version, stream_size);
019300000000}
019400000000
019500000000/*
019600000000   Return state with length and distance decoding tables and index sizes set to
019700000000   fixed code decoding.  Normally this returns fixed tables from inffixed.h.
019800000000   If BUILDFIXED is defined, then instead this routine builds the tables the
019900000000   first time it's called, and returns those tables the first time and
020000000000   thereafter.  This reduces the size of the code by about 2K bytes, in
020100000000   exchange for a little execution time.  However, BUILDFIXED should not be
020200000000   used for threaded applications, since the rewriting of the tables and virgin
020300000000   may not be thread-safe.
020400000000 */
020500000000local void fixedtables(state)
020600000000struct inflate_state FAR *state;
020700000000{
020800000000#ifdef BUILDFIXED
020900000000    static int virgin = 1;
021000000000    static code *lenfix, *distfix;
021100000000    static code fixed[544];
021200000000
021300000000    /* build fixed huffman tables if first call (may not be thread safe) */
021400000000    if (virgin) {
021500000000        unsigned sym, bits;
021600000000        static code *next;
021700000000
021800000000        /* literal/length table */
021900000000        sym = 0;
022000000000        while (sym < 144) state->lens[sym++] = 8;
022100000000        while (sym < 256) state->lens[sym++] = 9;
022200000000        while (sym < 280) state->lens[sym++] = 7;
022300000000        while (sym < 288) state->lens[sym++] = 8;
022400000000        next = fixed;
022500000000        lenfix = next;
022600000000        bits = 9;
022700000000        inflate_table(LENS, state->lens, 288, &(next), &(bits), state->work);
022800000000
022900000000        /* distance table */
023000000000        sym = 0;
023100000000        while (sym < 32) state->lens[sym++] = 5;
023200000000        distfix = next;
023300000000        bits = 5;
023400000000        inflate_table(DISTS, state->lens, 32, &(next), &(bits), state->work);
023500000000
023600000000        /* do this just once */
023700000000        virgin = 0;
023800000000    }
023900000000#else /* !BUILDFIXED */
024000000000#   include "inffixed.h"
024100000000#endif /* BUILDFIXED */
024200000000    state->lencode = lenfix;
024300000000    state->lenbits = 9;
024400000000    state->distcode = distfix;
024500000000    state->distbits = 5;
024600000000}
024700000000
024800000000#ifdef MAKEFIXED
024900000000#include <stdio.h>
025000000000
025100000000/*
025200000000   Write out the inffixed.h that is #include'd above.  Defining MAKEFIXED also
025300000000   defines BUILDFIXED, so the tables are built on the fly.  makefixed() writes
025400000000   those tables to stdout, which would be piped to inffixed.h.  A small program
025500000000   can simply call makefixed to do this:
025600000000
025700000000    void makefixed(void);
025800000000
025900000000    int main(void)
026000000000    {
026100000000        makefixed();
026200000000        return 0;
026300000000    }
026400000000
026500000000   Then that can be linked with zlib built with MAKEFIXED defined and run:
026600000000
026700000000    a.out > inffixed.h
026800000000 */
026900000000void makefixed()
027000000000{
027100000000    unsigned low, size;
027200000000    struct inflate_state state;
027300000000
027400000000    fixedtables(&state);
027500000000    puts("    /* inffixed.h -- table for decoding fixed codes");
027600000000    puts("     * Generated automatically by makefixed().");
027700000000    puts("     */");
027800000000    puts("");
027900000000    puts("    /* WARNING: this file should *not* be used by applications.");
028000000000    puts("       It is part of the implementation of this library and is");
028100000000    puts("       subject to change. Applications should only use zlib.h.");
028200000000    puts("     */");
028300000000    puts("");
028400000000    size = 1U << 9;
028500000000    printf("    static const code lenfix[%u] = {", size);
028600000000    low = 0;
028700000000    for (;;) {
028800000000        if ((low % 7) == 0) printf("\n        ");
028900000000        printf("{%u,%u,%d}", state.lencode[low].op, state.lencode[low].bits,
029000000000               state.lencode[low].val);
029100000000        if (++low == size) break;
029200000000        putchar(',');
029300000000    }
029400000000    puts("\n    };");
029500000000    size = 1U << 5;
029600000000    printf("\n    static const code distfix[%u] = {", size);
029700000000    low = 0;
029800000000    for (;;) {
029900000000        if ((low % 6) == 0) printf("\n        ");
030000000000        printf("{%u,%u,%d}", state.distcode[low].op, state.distcode[low].bits,
030100000000               state.distcode[low].val);
030200000000        if (++low == size) break;
030300000000        putchar(',');
030400000000    }
030500000000    puts("\n    };");
030600000000}
030700000000#endif /* MAKEFIXED */
030800000000
030900000000/*
031000000000   Update the window with the last wsize (normally 32K) bytes written before
031100000000   returning.  If window does not exist yet, create it.  This is only called
031200000000   when a window is already in use, or when output has been written during this
031300000000   inflate call, but the end of the deflate stream has not been reached yet.
031400000000   It is also called to create a window for dictionary data when a dictionary
031500000000   is loaded.
031600000000
031700000000   Providing output buffers larger than 32K to inflate() should provide a speed
031800000000   advantage, since only the last 32K of output is copied to the sliding window
031900000000   upon return from inflate(), and since all distances after the first 32K of
032000000000   output will fall in the output data, making match copies simpler and faster.
032100000000   The advantage may be dependent on the size of the processor's data caches.
032200000000 */
032300000000local int updatewindow(strm, out)
032400000000z_streamp strm;
032500000000unsigned out;
032600000000{
032700000000    struct inflate_state FAR *state;
032800000000    unsigned copy, dist;
032900000000
033000000000    state = (struct inflate_state FAR *)strm->state;
033100000000
033200000000    /* if it hasn't been done already, allocate space for the window */
033300000000    if (state->window == Z_NULL) {
033400000000        state->window = (unsigned char FAR *)
033500000000                        ZALLOC(strm, 1U << state->wbits,
033600000000                               sizeof(unsigned char));
033700000000        if (state->window == Z_NULL) return 1;
033800000000    }
033900000000
034000000000    /* if window not in use yet, initialize */
034100000000    if (state->wsize == 0) {
034200000000        state->wsize = 1U << state->wbits;
034300000000        state->write = 0;
034400000000        state->whave = 0;
034500000000    }
034600000000
034700000000    /* copy state->wsize or less output bytes into the circular window */
034800000000    copy = out - strm->avail_out;
034900000000    if (copy >= state->wsize) {
035000000000        zmemcpy(state->window, strm->next_out - state->wsize, state->wsize);
035100000000        state->write = 0;
035200000000        state->whave = state->wsize;
035300000000    }
035400000000    else {
035500000000        dist = state->wsize - state->write;
035600000000        if (dist > copy) dist = copy;
035700000000        zmemcpy(state->window + state->write, strm->next_out - copy, dist);
035800000000        copy -= dist;
035900000000        if (copy) {
036000000000            zmemcpy(state->window, strm->next_out - copy, copy);
036100000000            state->write = copy;
036200000000            state->whave = state->wsize;
036300000000        }
036400000000        else {
036500000000            state->write += dist;
036600000000            if (state->write == state->wsize) state->write = 0;
036700000000            if (state->whave < state->wsize) state->whave += dist;
036800000000        }
036900000000    }
037000000000    return 0;
037100000000}
037200000000
037300000000/* Macros for inflate(): */
037400000000
037500000000/* check function to use adler32() for zlib or crc32() for gzip */
037600000000#ifdef GUNZIP
037700000000#  define UPDATE(check, buf, len) \
037800000000    (state->flags ? crc32(check, buf, len) : adler32(check, buf, len))
037900000000#else
038000000000#  define UPDATE(check, buf, len) adler32(check, buf, len)
038100000000#endif
038200000000
038300000000/* check macros for header crc */
038400000000#ifdef GUNZIP
038500000000#  define CRC2(check, word) \
038600000000    do { \
038700000000        hbuf[0] = (unsigned char)(word); \
038800000000        hbuf[1] = (unsigned char)((word) >> 8); \
038900000000        check = crc32(check, hbuf, 2); \
039000000000    } while (0)
039100000000
039200000000#  define CRC4(check, word) \
039300000000    do { \
039400000000        hbuf[0] = (unsigned char)(word); \
039500000000        hbuf[1] = (unsigned char)((word) >> 8); \
039600000000        hbuf[2] = (unsigned char)((word) >> 16); \
039700000000        hbuf[3] = (unsigned char)((word) >> 24); \
039800000000        check = crc32(check, hbuf, 4); \
039900000000    } while (0)
040000000000#endif
040100000000
040200000000/* Load registers with state in inflate() for speed */
040300000000#define LOAD() \
040400000000    do { \
040500000000        put = strm->next_out; \
040600000000        left = strm->avail_out; \
040700000000        next = strm->next_in; \
040800000000        have = strm->avail_in; \
040900000000        hold = state->hold; \
041000000000        bits = state->bits; \
041100000000    } while (0)
041200000000
041300000000/* Restore state from registers in inflate() */
041400000000#define RESTORE() \
041500000000    do { \
041600000000        strm->next_out = put; \
041700000000        strm->avail_out = left; \
041800000000        strm->next_in = next; \
041900000000        strm->avail_in = have; \
042000000000        state->hold = hold; \
042100000000        state->bits = bits; \
042200000000    } while (0)
042300000000
042400000000/* Clear the input bit accumulator */
042500000000#define INITBITS() \
042600000000    do { \
042700000000        hold = 0; \
042800000000        bits = 0; \
042900000000    } while (0)
043000000000
043100000000/* Get a byte of input into the bit accumulator, or return from inflate()
043200000000   if there is no input available. */
043300000000#define PULLBYTE() \
043400000000    do { \
043500000000        if (have == 0) goto inf_leave; \
043600000000        have--; \
043700000000        hold += (unsigned long)(*next++) << bits; \
043800000000        bits += 8; \
043900000000    } while (0)
044000000000
044100000000/* Assure that there are at least n bits in the bit accumulator.  If there is
044200000000   not enough available input to do that, then return from inflate(). */
044300000000#define NEEDBITS(n) \
044400000000    do { \
044500000000        while (bits < (unsigned)(n)) \
044600000000            PULLBYTE(); \
044700000000    } while (0)
044800000000
044900000000/* Return the low n bits of the bit accumulator (n < 16) */
045000000000#define BITS(n) \
045100000000    ((unsigned)hold & ((1U << (n)) - 1))
045200000000
045300000000/* Remove n bits from the bit accumulator */
045400000000#define DROPBITS(n) \
045500000000    do { \
045600000000        hold >>= (n); \
045700000000        bits -= (unsigned)(n); \
045800000000    } while (0)
045900000000
046000000000/* Remove zero to seven bits as needed to go to a byte boundary */
046100000000#define BYTEBITS() \
046200000000    do { \
046300000000        hold >>= bits & 7; \
046400000000        bits -= bits & 7; \
046500000000    } while (0)
046600000000
046700000000/* Reverse the bytes in a 32-bit value */
046800000000#define REVERSE(q) \
046900000000    ((((q) >> 24) & 0xff) + (((q) >> 8) & 0xff00) + \
047000000000     (((q) & 0xff00) << 8) + (((q) & 0xff) << 24))
047100000000
047200000000/*
047300000000   inflate() uses a state machine to process as much input data and generate as
047400000000   much output data as possible before returning.  The state machine is
047500000000   structured roughly as follows:
047600000000
047700000000    for (;;) switch (state) {
047800000000    ...
047900000000    case STATEn:
048000000000        if (not enough input data or output space to make progress)
048100000000            return;
048200000000        ... make progress ...
048300000000        state = STATEm;
048400000000        break;
048500000000    ...
048600000000    }
048700000000
048800000000   so when inflate() is called again, the same case is attempted again, and
048900000000   if the appropriate resources are provided, the machine proceeds to the
049000000000   next state.  The NEEDBITS() macro is usually the way the state evaluates
049100000000   whether it can proceed or should return.  NEEDBITS() does the return if
049200000000   the requested bits are not available.  The typical use of the BITS macros
049300000000   is:
049400000000
049500000000        NEEDBITS(n);
049600000000        ... do something with BITS(n) ...
049700000000        DROPBITS(n);
049800000000
049900000000   where NEEDBITS(n) either returns from inflate() if there isn't enough
050000000000   input left to load n bits into the accumulator, or it continues.  BITS(n)
050100000000   gives the low n bits in the accumulator.  When done, DROPBITS(n) drops
050200000000   the low n bits off the accumulator.  INITBITS() clears the accumulator
050300000000   and sets the number of available bits to zero.  BYTEBITS() discards just
050400000000   enough bits to put the accumulator on a byte boundary.  After BYTEBITS()
050500000000   and a NEEDBITS(8), then BITS(8) would return the next byte in the stream.
050600000000
050700000000   NEEDBITS(n) uses PULLBYTE() to get an available byte of input, or to return
050800000000   if there is no input available.  The decoding of variable length codes uses
050900000000   PULLBYTE() directly in order to pull just enough bytes to decode the next
051000000000   code, and no more.
051100000000
051200000000   Some states loop until they get enough input, making sure that enough
051300000000   state information is maintained to continue the loop where it left off
051400000000   if NEEDBITS() returns in the loop.  For example, want, need, and keep
051500000000   would all have to actually be part of the saved state in case NEEDBITS()
051600000000   returns:
051700000000
051800000000    case STATEw:
051900000000        while (want < need) {
052000000000            NEEDBITS(n);
052100000000            keep[want++] = BITS(n);
052200000000            DROPBITS(n);
052300000000        }
052400000000        state = STATEx;
052500000000    case STATEx:
052600000000
052700000000   As shown above, if the next state is also the next case, then the break
052800000000   is omitted.
052900000000
053000000000   A state may also return if there is not enough output space available to
053100000000   complete that state.  Those states are copying stored data, writing a
053200000000   literal byte, and copying a matching string.
053300000000
053400000000   When returning, a "goto inf_leave" is used to update the total counters,
053500000000   update the check value, and determine whether any progress has been made
053600000000   during that inflate() call in order to return the proper return code.
053700000000   Progress is defined as a change in either strm->avail_in or strm->avail_out.
053800000000   When there is a window, goto inf_leave will update the window with the last
053900000000   output written.  If a goto inf_leave occurs in the middle of decompression
054000000000   and there is no window currently, goto inf_leave will create one and copy
054100000000   output to the window for the next call of inflate().
054200000000
054300000000   In this implementation, the flush parameter of inflate() only affects the
054400000000   return code (per zlib.h).  inflate() always writes as much as possible to
054500000000   strm->next_out, given the space available and the provided input--the effect
054600000000   documented in zlib.h of Z_SYNC_FLUSH.  Furthermore, inflate() always defers
054700000000   the allocation of and copying into a sliding window until necessary, which
054800000000   provides the effect documented in zlib.h for Z_FINISH when the entire input
054900000000   stream available.  So the only thing the flush parameter actually does is:
055000000000   when flush is set to Z_FINISH, inflate() cannot return Z_OK.  Instead it
055100000000   will return Z_BUF_ERROR if it has not reached the end of the stream.
055200000000 */
055300000000
055400000000int ZEXPORT inflate(strm, flush)
055500000000z_streamp strm;
055600000000int flush;
055700000000{
055800000000    struct inflate_state FAR *state;
055900000000    unsigned char FAR *next;    /* next input */
056000000000    unsigned char FAR *put;     /* next output */
056100000000    unsigned have, left;        /* available input and output */
056200000000    unsigned long hold;         /* bit buffer */
056300000000    unsigned bits;              /* bits in bit buffer */
056400000000    unsigned in, out;           /* save starting available input and output */
056500000000    unsigned copy;              /* number of stored or match bytes to copy */
056600000000    unsigned char FAR *from;    /* where to copy match bytes from */
056700000000    code this;                  /* current decoding table entry */
056800000000    code last;                  /* parent table entry */
056900000000    unsigned len;               /* length to copy for repeats, bits to drop */
057000000000    int ret;                    /* return code */
057100000000#ifdef GUNZIP
057200000000    unsigned char hbuf[4];      /* buffer for gzip header crc calculation */
057300000000#endif
057400000000    static const unsigned short order[19] = /* permutation of code lengths */
057500000000        {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
057600000000
057700000000    if (strm == Z_NULL || strm->state == Z_NULL || strm->next_out == Z_NULL ||
057800000000        (strm->next_in == Z_NULL && strm->avail_in != 0))
057900000000        return Z_STREAM_ERROR;
058000000000
058100000000    state = (struct inflate_state FAR *)strm->state;
058200000000    if (state->mode == TYPE) state->mode = TYPEDO;      /* skip check */
058300000000    LOAD();
058400000000    in = have;
058500000000    out = left;
058600000000    ret = Z_OK;
058700000000    for (;;)
058800000000        switch (state->mode) {
058900000000        case HEAD:
059000000000            if (state->wrap == 0) {
059100000000                state->mode = TYPEDO;
059200000000                break;
059300000000            }
059400000000            NEEDBITS(16);
059500000000#ifdef GUNZIP
059600000000            if ((state->wrap & 2) && hold == 0x8b1f) {  /* gzip header */
059700000000                state->check = crc32(0L, Z_NULL, 0);
059800000000                CRC2(state->check, hold);
059900000000                INITBITS();
060000000000                state->mode = FLAGS;
060100000000                break;
060200000000            }
060300000000            state->flags = 0;           /* expect zlib header */
060400000000            if (state->head != Z_NULL)
060500000000                state->head->done = -1;
060600000000            if (!(state->wrap & 1) ||   /* check if zlib header allowed */
060700000000#else
060800000000            if (
060900000000#endif
061000000000                ((BITS(8) << 8) + (hold >> 8)) % 31) {
061100000000                strm->msg = (char *)"incorrect header check";
061200000000                state->mode = BAD;
061300000000                break;
061400000000            }
061500000000            if (BITS(4) != Z_DEFLATED) {
061600000000                strm->msg = (char *)"unknown compression method";
061700000000                state->mode = BAD;
061800000000                break;
061900000000            }
062000000000            DROPBITS(4);
062100000000            len = BITS(4) + 8;
062200000000            if (len > state->wbits) {
062300000000                strm->msg = (char *)"invalid window size";
062400000000                state->mode = BAD;
062500000000                break;
062600000000            }
062700000000            state->dmax = 1U << len;
062800000000            Tracev((stderr, "inflate:   zlib header ok\n"));
062900000000            strm->adler = state->check = adler32(0L, Z_NULL, 0);
063000000000            state->mode = hold & 0x200 ? DICTID : TYPE;
063100000000            INITBITS();
063200000000            break;
063300000000#ifdef GUNZIP
063400000000        case FLAGS:
063500000000            NEEDBITS(16);
063600000000            state->flags = (int)(hold);
063700000000            if ((state->flags & 0xff) != Z_DEFLATED) {
063800000000                strm->msg = (char *)"unknown compression method";
063900000000                state->mode = BAD;
064000000000                break;
064100000000            }
064200000000            if (state->flags & 0xe000) {
064300000000                strm->msg = (char *)"unknown header flags set";
064400000000                state->mode = BAD;
064500000000                break;
064600000000            }
064700000000            if (state->head != Z_NULL)
064800000000                state->head->text = (int)((hold >> 8) & 1);
064900000000            if (state->flags & 0x0200) CRC2(state->check, hold);
065000000000            INITBITS();
065100000000            state->mode = TIME;
065200000000        case TIME:
065300000000            NEEDBITS(32);
065400000000            if (state->head != Z_NULL)
065500000000                state->head->time = hold;
065600000000            if (state->flags & 0x0200) CRC4(state->check, hold);
065700000000            INITBITS();
065800000000            state->mode = OS;
065900000000        case OS:
066000000000            NEEDBITS(16);
066100000000            if (state->head != Z_NULL) {
066200000000                state->head->xflags = (int)(hold & 0xff);
066300000000                state->head->os = (int)(hold >> 8);
066400000000            }
066500000000            if (state->flags & 0x0200) CRC2(state->check, hold);
066600000000            INITBITS();
066700000000            state->mode = EXLEN;
066800000000        case EXLEN:
066900000000            if (state->flags & 0x0400) {
067000000000                NEEDBITS(16);
067100000000                state->length = (unsigned)(hold);
067200000000                if (state->head != Z_NULL)
067300000000                    state->head->extra_len = (unsigned)hold;
067400000000                if (state->flags & 0x0200) CRC2(state->check, hold);
067500000000                INITBITS();
067600000000            }
067700000000            else if (state->head != Z_NULL)
067800000000                state->head->extra = Z_NULL;
067900000000            state->mode = EXTRA;
068000000000        case EXTRA:
068100000000            if (state->flags & 0x0400) {
068200000000                copy = state->length;
068300000000                if (copy > have) copy = have;
068400000000                if (copy) {
068500000000                    if (state->head != Z_NULL &&
068600000000                        state->head->extra != Z_NULL) {
068700000000                        len = state->head->extra_len - state->length;
068800000000                        zmemcpy(state->head->extra + len, next,
068900000000                                len + copy > state->head->extra_max ?
069000000000                                state->head->extra_max - len : copy);
069100000000                    }
069200000000                    if (state->flags & 0x0200)
069300000000                        state->check = crc32(state->check, next, copy);
069400000000                    have -= copy;
069500000000                    next += copy;
069600000000                    state->length -= copy;
069700000000                }
069800000000                if (state->length) goto inf_leave;
069900000000            }
070000000000            state->length = 0;
070100000000            state->mode = NAME;
070200000000        case NAME:
070300000000            if (state->flags & 0x0800) {
070400000000                if (have == 0) goto inf_leave;
070500000000                copy = 0;
070600000000                do {
070700000000                    len = (unsigned)(next[copy++]);
070800000000                    if (state->head != Z_NULL &&
070900000000                            state->head->name != Z_NULL &&
071000000000                            state->length < state->head->name_max)
071100000000                        state->head->name[state->length++] = len;
071200000000                } while (len && copy < have);
071300000000                if (state->flags & 0x0200)
071400000000                    state->check = crc32(state->check, next, copy);
071500000000                have -= copy;
071600000000                next += copy;
071700000000                if (len) goto inf_leave;
071800000000            }
071900000000            else if (state->head != Z_NULL)
072000000000                state->head->name = Z_NULL;
072100000000            state->length = 0;
072200000000            state->mode = COMMENT;
072300000000        case COMMENT:
072400000000            if (state->flags & 0x1000) {
072500000000                if (have == 0) goto inf_leave;
072600000000                copy = 0;
072700000000                do {
072800000000                    len = (unsigned)(next[copy++]);
072900000000                    if (state->head != Z_NULL &&
073000000000                            state->head->comment != Z_NULL &&
073100000000                            state->length < state->head->comm_max)
073200000000                        state->head->comment[state->length++] = len;
073300000000                } while (len && copy < have);
073400000000                if (state->flags & 0x0200)
073500000000                    state->check = crc32(state->check, next, copy);
073600000000                have -= copy;
073700000000                next += copy;
073800000000                if (len) goto inf_leave;
073900000000            }
074000000000            else if (state->head != Z_NULL)
074100000000                state->head->comment = Z_NULL;
074200000000            state->mode = HCRC;
074300000000        case HCRC:
074400000000            if (state->flags & 0x0200) {
074500000000                NEEDBITS(16);
074600000000                if (hold != (state->check & 0xffff)) {
074700000000                    strm->msg = (char *)"header crc mismatch";
074800000000                    state->mode = BAD;
074900000000                    break;
075000000000                }
075100000000                INITBITS();
075200000000            }
075300000000            if (state->head != Z_NULL) {
075400000000                state->head->hcrc = (int)((state->flags >> 9) & 1);
075500000000                state->head->done = 1;
075600000000            }
075700000000            strm->adler = state->check = crc32(0L, Z_NULL, 0);
075800000000            state->mode = TYPE;
075900000000            break;
076000000000#endif
076100000000        case DICTID:
076200000000            NEEDBITS(32);
076300000000            strm->adler = state->check = REVERSE(hold);
076400000000            INITBITS();
076500000000            state->mode = DICT;
076600000000        case DICT:
076700000000            if (state->havedict == 0) {
076800000000                RESTORE();
076900000000                return Z_NEED_DICT;
077000000000            }
077100000000            strm->adler = state->check = adler32(0L, Z_NULL, 0);
077200000000            state->mode = TYPE;
077300000000        case TYPE:
077400000000            if (flush == Z_BLOCK) goto inf_leave;
077500000000        case TYPEDO:
077600000000            if (state->last) {
077700000000                BYTEBITS();
077800000000                state->mode = CHECK;
077900000000                break;
078000000000            }
078100000000            NEEDBITS(3);
078200000000            state->last = BITS(1);
078300000000            DROPBITS(1);
078400000000            switch (BITS(2)) {
078500000000            case 0:                             /* stored block */
078600000000                Tracev((stderr, "inflate:     stored block%s\n",
078700000000                        state->last ? " (last)" : ""));
078800000000                state->mode = STORED;
078900000000                break;
079000000000            case 1:                             /* fixed block */
079100000000                fixedtables(state);
079200000000                Tracev((stderr, "inflate:     fixed codes block%s\n",
079300000000                        state->last ? " (last)" : ""));
079400000000                state->mode = LEN;              /* decode codes */
079500000000                break;
079600000000            case 2:                             /* dynamic block */
079700000000                Tracev((stderr, "inflate:     dynamic codes block%s\n",
079800000000                        state->last ? " (last)" : ""));
079900000000                state->mode = TABLE;
080000000000                break;
080100000000            case 3:
080200000000                strm->msg = (char *)"invalid block type";
080300000000                state->mode = BAD;
080400000000            }
080500000000            DROPBITS(2);
080600000000            break;
080700000000        case STORED:
080800000000            BYTEBITS();                         /* go to byte boundary */
080900000000            NEEDBITS(32);
081000000000            if ((hold & 0xffff) != ((hold >> 16) ^ 0xffff)) {
081100000000                strm->msg = (char *)"invalid stored block lengths";
081200000000                state->mode = BAD;
081300000000                break;
081400000000            }
081500000000            state->length = (unsigned)hold & 0xffff;
081600000000            Tracev((stderr, "inflate:       stored length %u\n",
081700000000                    state->length));
081800000000            INITBITS();
081900000000            state->mode = COPY;
082000000000        case COPY:
082100000000            copy = state->length;
082200000000            if (copy) {
082300000000                if (copy > have) copy = have;
082400000000                if (copy > left) copy = left;
082500000000                if (copy == 0) goto inf_leave;
082600000000                zmemcpy(put, next, copy);
082700000000                have -= copy;
082800000000                next += copy;
082900000000                left -= copy;
083000000000                put += copy;
083100000000                state->length -= copy;
083200000000                break;
083300000000            }
083400000000            Tracev((stderr, "inflate:       stored end\n"));
083500000000            state->mode = TYPE;
083600000000            break;
083700000000        case TABLE:
083800000000            NEEDBITS(14);
083900000000            state->nlen = BITS(5) + 257;
084000000000            DROPBITS(5);
084100000000            state->ndist = BITS(5) + 1;
084200000000            DROPBITS(5);
084300000000            state->ncode = BITS(4) + 4;
084400000000            DROPBITS(4);
084500000000#ifndef PKZIP_BUG_WORKAROUND
084600000000            if (state->nlen > 286 || state->ndist > 30) {
084700000000                strm->msg = (char *)"too many length or distance symbols";
084800000000                state->mode = BAD;
084900000000                break;
085000000000            }
085100000000#endif
085200000000            Tracev((stderr, "inflate:       table sizes ok\n"));
085300000000            state->have = 0;
085400000000            state->mode = LENLENS;
085500000000        case LENLENS:
085600000000            while (state->have < state->ncode) {
085700000000                NEEDBITS(3);
085800000000                state->lens[order[state->have++]] = (unsigned short)BITS(3);
085900000000                DROPBITS(3);
086000000000            }
086100000000            while (state->have < 19)
086200000000                state->lens[order[state->have++]] = 0;
086300000000            state->next = state->codes;
086400000000            state->lencode = (code const FAR *)(state->next);
086500000000            state->lenbits = 7;
086600000000            ret = inflate_table(CODES, state->lens, 19, &(state->next),
086700000000                                &(state->lenbits), state->work);
086800000000            if (ret) {
086900000000                strm->msg = (char *)"invalid code lengths set";
087000000000                state->mode = BAD;
087100000000                break;
087200000000            }
087300000000            Tracev((stderr, "inflate:       code lengths ok\n"));
087400000000            state->have = 0;
087500000000            state->mode = CODELENS;
087600000000        case CODELENS:
087700000000            while (state->have < state->nlen + state->ndist) {
087800000000                for (;;) {
087900000000                    this = state->lencode[BITS(state->lenbits)];
088000000000                    if ((unsigned)(this.bits) <= bits) break;
088100000000                    PULLBYTE();
088200000000                }
088300000000                if (this.val < 16) {
088400000000                    NEEDBITS(this.bits);
088500000000                    DROPBITS(this.bits);
088600000000                    state->lens[state->have++] = this.val;
088700000000                }
088800000000                else {
088900000000                    if (this.val == 16) {
089000000000                        NEEDBITS(this.bits + 2);
089100000000                        DROPBITS(this.bits);
089200000000                        if (state->have == 0) {
089300000000                            strm->msg = (char *)"invalid bit length repeat";
089400000000                            state->mode = BAD;
089500000000                            break;
089600000000                        }
089700000000                        len = state->lens[state->have - 1];
089800000000                        copy = 3 + BITS(2);
089900000000                        DROPBITS(2);
090000000000                    }
090100000000                    else if (this.val == 17) {
090200000000                        NEEDBITS(this.bits + 3);
090300000000                        DROPBITS(this.bits);
090400000000                        len = 0;
090500000000                        copy = 3 + BITS(3);
090600000000                        DROPBITS(3);
090700000000                    }
090800000000                    else {
090900000000                        NEEDBITS(this.bits + 7);
091000000000                        DROPBITS(this.bits);
091100000000                        len = 0;
091200000000                        copy = 11 + BITS(7);
091300000000                        DROPBITS(7);
091400000000                    }
091500000000                    if (state->have + copy > state->nlen + state->ndist) {
091600000000                        strm->msg = (char *)"invalid bit length repeat";
091700000000                        state->mode = BAD;
091800000000                        break;
091900000000                    }
092000000000                    while (copy--)
092100000000                        state->lens[state->have++] = (unsigned short)len;
092200000000                }
092300000000            }
092400000000
092500000000            /* handle error breaks in while */
092600000000            if (state->mode == BAD) break;
092700000000
092800000000            /* build code tables */
092900000000            state->next = state->codes;
093000000000            state->lencode = (code const FAR *)(state->next);
093100000000            state->lenbits = 9;
093200000000            ret = inflate_table(LENS, state->lens, state->nlen, &(state->next),
093300000000                                &(state->lenbits), state->work);
093400000000            if (ret) {
093500000000                strm->msg = (char *)"invalid literal/lengths set";
093600000000                state->mode = BAD;
093700000000                break;
093800000000            }
093900000000            state->distcode = (code const FAR *)(state->next);
094000000000            state->distbits = 6;
094100000000            ret = inflate_table(DISTS, state->lens + state->nlen, state->ndist,
094200000000                            &(state->next), &(state->distbits), state->work);
094300000000            if (ret) {
094400000000                strm->msg = (char *)"invalid distances set";
094500000000                state->mode = BAD;
094600000000                break;
094700000000            }
094800000000            Tracev((stderr, "inflate:       codes ok\n"));
094900000000            state->mode = LEN;
095000000000        case LEN:
095100000000            if (have >= 6 && left >= 258) {
095200000000                RESTORE();
095300000000                inflate_fast(strm, out);
095400000000                LOAD();
095500000000                break;
095600000000            }
095700000000            for (;;) {
095800000000                this = state->lencode[BITS(state->lenbits)];
095900000000                if ((unsigned)(this.bits) <= bits) break;
096000000000                PULLBYTE();
096100000000            }
096200000000            if (this.op && (this.op & 0xf0) == 0) {
096300000000                last = this;
096400000000                for (;;) {
096500000000                    this = state->lencode[last.val +
096600000000                            (BITS(last.bits + last.op) >> last.bits)];
096700000000                    if ((unsigned)(last.bits + this.bits) <= bits) break;
096800000000                    PULLBYTE();
096900000000                }
097000000000                DROPBITS(last.bits);
097100000000            }
097200000000            DROPBITS(this.bits);
097300000000            state->length = (unsigned)this.val;
097400000000            if ((int)(this.op) == 0) {
097500000000                Tracevv((stderr, this.val >= 0x20 && this.val < 0x7f ?
097600000000                        "inflate:         literal '%c'\n" :
097700000000                        "inflate:         literal 0x%02x\n", this.val));
097800000000                state->mode = LIT;
097900000000                break;
098000000000            }
098100000000            if (this.op & 32) {
098200000000                Tracevv((stderr, "inflate:         end of block\n"));
098300000000                state->mode = TYPE;
098400000000                break;
098500000000            }
098600000000            if (this.op & 64) {
098700000000                strm->msg = (char *)"invalid literal/length code";
098800000000                state->mode = BAD;
098900000000                break;
099000000000            }
099100000000            state->extra = (unsigned)(this.op) & 15;
099200000000            state->mode = LENEXT;
099300000000        case LENEXT:
099400000000            if (state->extra) {
099500000000                NEEDBITS(state->extra);
099600000000                state->length += BITS(state->extra);
099700000000                DROPBITS(state->extra);
099800000000            }
099900000000            Tracevv((stderr, "inflate:         length %u\n", state->length));
100000000000            state->mode = DIST;
100100000000        case DIST:
100200000000            for (;;) {
100300000000                this = state->distcode[BITS(state->distbits)];
100400000000                if ((unsigned)(this.bits) <= bits) break;
100500000000                PULLBYTE();
100600000000            }
100700000000            if ((this.op & 0xf0) == 0) {
100800000000                last = this;
100900000000                for (;;) {
101000000000                    this = state->distcode[last.val +
101100000000                            (BITS(last.bits + last.op) >> last.bits)];
101200000000                    if ((unsigned)(last.bits + this.bits) <= bits) break;
101300000000                    PULLBYTE();
101400000000                }
101500000000                DROPBITS(last.bits);
101600000000            }
101700000000            DROPBITS(this.bits);
101800000000            if (this.op & 64) {
101900000000                strm->msg = (char *)"invalid distance code";
102000000000                state->mode = BAD;
102100000000                break;
102200000000            }
102300000000            state->offset = (unsigned)this.val;
102400000000            state->extra = (unsigned)(this.op) & 15;
102500000000            state->mode = DISTEXT;
102600000000        case DISTEXT:
102700000000            if (state->extra) {
102800000000                NEEDBITS(state->extra);
102900000000                state->offset += BITS(state->extra);
103000000000                DROPBITS(state->extra);
103100000000            }
103200000000#ifdef INFLATE_STRICT
103300000000            if (state->offset > state->dmax) {
103400000000                strm->msg = (char *)"invalid distance too far back";
103500000000                state->mode = BAD;
103600000000                break;
103700000000            }
103800000000#endif
103900000000            if (state->offset > state->whave + out - left) {
104000000000                strm->msg = (char *)"invalid distance too far back";
104100000000                state->mode = BAD;
104200000000                break;
104300000000            }
104400000000            Tracevv((stderr, "inflate:         distance %u\n", state->offset));
104500000000            state->mode = MATCH;
104600000000        case MATCH:
104700000000            if (left == 0) goto inf_leave;
104800000000            copy = out - left;
104900000000            if (state->offset > copy) {         /* copy from window */
105000000000                copy = state->offset - copy;
105100000000                if (copy > state->write) {
105200000000                    copy -= state->write;
105300000000                    from = state->window + (state->wsize - copy);
105400000000                }
105500000000                else
105600000000                    from = state->window + (state->write - copy);
105700000000                if (copy > state->length) copy = state->length;
105800000000            }
105900000000            else {                              /* copy from output */
106000000000                from = put - state->offset;
106100000000                copy = state->length;
106200000000            }
106300000000            if (copy > left) copy = left;
106400000000            left -= copy;
106500000000            state->length -= copy;
106600000000            do {
106700000000                *put++ = *from++;
106800000000            } while (--copy);
106900000000            if (state->length == 0) state->mode = LEN;
107000000000            break;
107100000000        case LIT:
107200000000            if (left == 0) goto inf_leave;
107300000000            *put++ = (unsigned char)(state->length);
107400000000            left--;
107500000000            state->mode = LEN;
107600000000            break;
107700000000        case CHECK:
107800000000            if (state->wrap) {
107900000000                NEEDBITS(32);
108000000000                out -= left;
108100000000                strm->total_out += out;
108200000000                state->total += out;
108300000000                if (out)
108400000000                    strm->adler = state->check =
108500000000                        UPDATE(state->check, put - out, out);
108600000000                out = left;
108700000000                if ((
108800000000#ifdef GUNZIP
108900000000                     state->flags ? hold :
109000000000#endif
109100000000                     REVERSE(hold)) != state->check) {
109200000000                    strm->msg = (char *)"incorrect data check";
109300000000                    state->mode = BAD;
109400000000                    break;
109500000000                }
109600000000                INITBITS();
109700000000                Tracev((stderr, "inflate:   check matches trailer\n"));
109800000000            }
109900000000#ifdef GUNZIP
110000000000            state->mode = LENGTH;
110100000000        case LENGTH:
110200000000            if (state->wrap && state->flags) {
110300000000                NEEDBITS(32);
110400000000                if (hold != (state->total & 0xffffffffUL)) {
110500000000                    strm->msg = (char *)"incorrect length check";
110600000000                    state->mode = BAD;
110700000000                    break;
110800000000                }
110900000000                INITBITS();
111000000000                Tracev((stderr, "inflate:   length matches trailer\n"));
111100000000            }
111200000000#endif
111300000000            state->mode = DONE;
111400000000        case DONE:
111500000000            ret = Z_STREAM_END;
111600000000            goto inf_leave;
111700000000        case BAD:
111800000000            ret = Z_DATA_ERROR;
111900000000            goto inf_leave;
112000000000        case MEM:
112100000000            return Z_MEM_ERROR;
112200000000        case SYNC:
112300000000        default:
112400000000            return Z_STREAM_ERROR;
112500000000        }
112600000000
112700000000    /*
112800000000       Return from inflate(), updating the total counts and the check value.
112900000000       If there was no progress during the inflate() call, return a buffer
113000000000       error.  Call updatewindow() to create and/or update the window state.
113100000000       Note: a memory error from inflate() is non-recoverable.
113200000000     */
113300000000  inf_leave:
113400000000    RESTORE();
113500000000    if (state->wsize || (state->mode < CHECK && out != strm->avail_out))
113600000000        if (updatewindow(strm, out)) {
113700000000            state->mode = MEM;
113800000000            return Z_MEM_ERROR;
113900000000        }
114000000000    in -= strm->avail_in;
114100000000    out -= strm->avail_out;
114200000000    strm->total_in += in;
114300000000    strm->total_out += out;
114400000000    state->total += out;
114500000000    if (state->wrap && out)
114600000000        strm->adler = state->check =
114700000000            UPDATE(state->check, strm->next_out - out, out);
114800000000    strm->data_type = state->bits + (state->last ? 64 : 0) +
114900000000                      (state->mode == TYPE ? 128 : 0);
115000000000    if (((in == 0 && out == 0) || flush == Z_FINISH) && ret == Z_OK)
115100000000        ret = Z_BUF_ERROR;
115200000000    return ret;
115300000000}
115400000000
115500000000int ZEXPORT inflateEnd(strm)
115600000000z_streamp strm;
115700000000{
115800000000    struct inflate_state FAR *state;
115900000000    if (strm == Z_NULL || strm->state == Z_NULL || strm->zfree == (free_func)0)
116000000000        return Z_STREAM_ERROR;
116100000000    state = (struct inflate_state FAR *)strm->state;
116200000000    if (state->window != Z_NULL) ZFREE(strm, state->window);
116300000000    ZFREE(strm, strm->state);
116400000000    strm->state = Z_NULL;
116500000000    Tracev((stderr, "inflate: end\n"));
116600000000    return Z_OK;
116700000000}
116800000000
116900000000int ZEXPORT inflateSetDictionary(strm, dictionary, dictLength)
117000000000z_streamp strm;
117100000000const Bytef *dictionary;
117200000000uInt dictLength;
117300000000{
117400000000    struct inflate_state FAR *state;
117500000000    unsigned long id;
117600000000
117700000000    /* check state */
117800000000    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
117900000000    state = (struct inflate_state FAR *)strm->state;
118000000000    if (state->wrap != 0 && state->mode != DICT)
118100000000        return Z_STREAM_ERROR;
118200000000
118300000000    /* check for correct dictionary id */
118400000000    if (state->mode == DICT) {
118500000000        id = adler32(0L, Z_NULL, 0);
118600000000        id = adler32(id, dictionary, dictLength);
118700000000        if (id != state->check)
118800000000            return Z_DATA_ERROR;
118900000000    }
119000000000
119100000000    /* copy dictionary to window */
119200000000    if (updatewindow(strm, strm->avail_out)) {
119300000000        state->mode = MEM;
119400000000        return Z_MEM_ERROR;
119500000000    }
119600000000    if (dictLength > state->wsize) {
119700000000        zmemcpy(state->window, dictionary + dictLength - state->wsize,
119800000000                state->wsize);
119900000000        state->whave = state->wsize;
120000000000    }
120100000000    else {
120200000000        zmemcpy(state->window + state->wsize - dictLength, dictionary,
120300000000                dictLength);
120400000000        state->whave = dictLength;
120500000000    }
120600000000    state->havedict = 1;
120700000000    Tracev((stderr, "inflate:   dictionary set\n"));
120800000000    return Z_OK;
120900000000}
121000000000
121100000000int ZEXPORT inflateGetHeader(strm, head)
121200000000z_streamp strm;
121300000000gz_headerp head;
121400000000{
121500000000    struct inflate_state FAR *state;
121600000000
121700000000    /* check state */
121800000000    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
121900000000    state = (struct inflate_state FAR *)strm->state;
122000000000    if ((state->wrap & 2) == 0) return Z_STREAM_ERROR;
122100000000
122200000000    /* save header structure */
122300000000    state->head = head;
122400000000    head->done = 0;
122500000000    return Z_OK;
122600000000}
122700000000
122800000000/*
122900000000   Search buf[0..len-1] for the pattern: 0, 0, 0xff, 0xff.  Return when found
123000000000   or when out of input.  When called, *have is the number of pattern bytes
123100000000   found in order so far, in 0..3.  On return *have is updated to the new
123200000000   state.  If on return *have equals four, then the pattern was found and the
123300000000   return value is how many bytes were read including the last byte of the
123400000000   pattern.  If *have is less than four, then the pattern has not been found
123500000000   yet and the return value is len.  In the latter case, syncsearch() can be
123600000000   called again with more data and the *have state.  *have is initialized to
123700000000   zero for the first call.
123800000000 */
123900000000local unsigned syncsearch(have, buf, len)
124000000000unsigned FAR *have;
124100000000unsigned char FAR *buf;
124200000000unsigned len;
124300000000{
124400000000    unsigned got;
124500000000    unsigned next;
124600000000
124700000000    got = *have;
124800000000    next = 0;
124900000000    while (next < len && got < 4) {
125000000000        if ((int)(buf[next]) == (got < 2 ? 0 : 0xff))
125100000000            got++;
125200000000        else if (buf[next])
125300000000            got = 0;
125400000000        else
125500000000            got = 4 - got;
125600000000        next++;
125700000000    }
125800000000    *have = got;
125900000000    return next;
126000000000}
126100000000
126200000000int ZEXPORT inflateSync(strm)
126300000000z_streamp strm;
126400000000{
126500000000    unsigned len;               /* number of bytes to look at or looked at */
126600000000    unsigned long in, out;      /* temporary to save total_in and total_out */
126700000000    unsigned char buf[4];       /* to restore bit buffer to byte string */
126800000000    struct inflate_state FAR *state;
126900000000
127000000000    /* check parameters */
127100000000    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
127200000000    state = (struct inflate_state FAR *)strm->state;
127300000000    if (strm->avail_in == 0 && state->bits < 8) return Z_BUF_ERROR;
127400000000
127500000000    /* if first time, start search in bit buffer */
127600000000    if (state->mode != SYNC) {
127700000000        state->mode = SYNC;
127800000000        state->hold <<= state->bits & 7;
127900000000        state->bits -= state->bits & 7;
128000000000        len = 0;
128100000000        while (state->bits >= 8) {
128200000000            buf[len++] = (unsigned char)(state->hold);
128300000000            state->hold >>= 8;
128400000000            state->bits -= 8;
128500000000        }
128600000000        state->have = 0;
128700000000        syncsearch(&(state->have), buf, len);
128800000000    }
128900000000
129000000000    /* search available input */
129100000000    len = syncsearch(&(state->have), strm->next_in, strm->avail_in);
129200000000    strm->avail_in -= len;
129300000000    strm->next_in += len;
129400000000    strm->total_in += len;
129500000000
129600000000    /* return no joy or set up to restart inflate() on a new block */
129700000000    if (state->have != 4) return Z_DATA_ERROR;
129800000000    in = strm->total_in;  out = strm->total_out;
129900000000    inflateReset(strm);
130000000000    strm->total_in = in;  strm->total_out = out;
130100000000    state->mode = TYPE;
130200000000    return Z_OK;
130300000000}
130400000000
130500000000/*
130600000000   Returns true if inflate is currently at the end of a block generated by
130700000000   Z_SYNC_FLUSH or Z_FULL_FLUSH. This function is used by one PPP
130800000000   implementation to provide an additional safety check. PPP uses
130900000000   Z_SYNC_FLUSH but removes the length bytes of the resulting empty stored
131000000000   block. When decompressing, PPP checks that at the end of input packet,
131100000000   inflate is waiting for these length bytes.
131200000000 */
131300000000int ZEXPORT inflateSyncPoint(strm)
131400000000z_streamp strm;
131500000000{
131600000000    struct inflate_state FAR *state;
131700000000
131800000000    if (strm == Z_NULL || strm->state == Z_NULL) return Z_STREAM_ERROR;
131900000000    state = (struct inflate_state FAR *)strm->state;
132000000000    return state->mode == STORED && state->bits == 0;
132100000000}
132200000000
132300000000int ZEXPORT inflateCopy(dest, source)
132400000000z_streamp dest;
132500000000z_streamp source;
132600000000{
132700000000    struct inflate_state FAR *state;
132800000000    struct inflate_state FAR *copy;
132900000000    unsigned char FAR *window;
133000000000    unsigned wsize;
133100000000
133200000000    /* check input */
133300000000    if (dest == Z_NULL || source == Z_NULL || source->state == Z_NULL ||
133400000000        source->zalloc == (alloc_func)0 || source->zfree == (free_func)0)
133500000000        return Z_STREAM_ERROR;
133600000000    state = (struct inflate_state FAR *)source->state;
133700000000
133800000000    /* allocate space */
133900000000    copy = (struct inflate_state FAR *)
134000000000           ZALLOC(source, 1, sizeof(struct inflate_state));
134100000000    if (copy == Z_NULL) return Z_MEM_ERROR;
134200000000    window = Z_NULL;
134300000000    if (state->window != Z_NULL) {
134400000000        window = (unsigned char FAR *)
134500000000                 ZALLOC(source, 1U << state->wbits, sizeof(unsigned char));
134600000000        if (window == Z_NULL) {
134700000000            ZFREE(source, copy);
134800000000            return Z_MEM_ERROR;
134900000000        }
135000000000    }
135100000000
135200000000    /* copy state */
135300000000    zmemcpy(dest, source, sizeof(z_stream));
135400000000    zmemcpy(copy, state, sizeof(struct inflate_state));
135500000000    if (state->lencode >= state->codes &&
135600000000        state->lencode <= state->codes + ENOUGH - 1) {
135700000000        copy->lencode = copy->codes + (state->lencode - state->codes);
135800000000        copy->distcode = copy->codes + (state->distcode - state->codes);
135900000000    }
136000000000    copy->next = copy->codes + (state->next - state->codes);
136100000000    if (window != Z_NULL) {
136200000000        wsize = 1U << state->wbits;
136300000000        zmemcpy(window, state->window, wsize);
136400000000    }
136500000000    copy->window = window;
136600000000    dest->state = (struct internal_state FAR *)copy;
136700000000    return Z_OK;
136800000000}
