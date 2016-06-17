000100000000/* infback.c -- inflate using a call-back interface
000200000000 * Copyright (C) 1995-2005 Mark Adler
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/*
000700000000   This code is largely copied from inflate.c.  Normally either infback.o or
000800000000   inflate.o would be linked into an application--not both.  The interface
000900000000   with inffast.c is retained so that optimized assembler-coded versions of
001000000000   inflate_fast() can be used with either inflate.c or infback.c.
001100000000 */
001200000000
001300000000#include "zutil.h"
001400000000#include "inftrees.h"
001500000000#include "inflate.h"
001600000000#include "inffast.h"
001700000000
001800000000/* function prototypes */
001900000000local void fixedtables OF((struct inflate_state FAR *state));
002000000000
002100000000/*
002200000000   strm provides memory allocation functions in zalloc and zfree, or
002300000000   Z_NULL to use the library memory allocation functions.
002400000000
002500000000   windowBits is in the range 8..15, and window is a user-supplied
002600000000   window and output buffer that is 2**windowBits bytes.
002700000000 */
002800000000int ZEXPORT inflateBackInit_(strm, windowBits, window, version, stream_size)
002900000000z_streamp strm;
003000000000int windowBits;
003100000000unsigned char FAR *window;
003200000000const char *version;
003300000000int stream_size;
003400000000{
003500000000    struct inflate_state FAR *state;
003600000000
003700000000    if (version == Z_NULL || version[0] != ZLIB_VERSION[0] ||
003800000000        stream_size != (int)(sizeof(z_stream)))
003900000000        return Z_VERSION_ERROR;
004000000000    if (strm == Z_NULL || window == Z_NULL ||
004100000000        windowBits < 8 || windowBits > 15)
004200000000        return Z_STREAM_ERROR;
004300000000    strm->msg = Z_NULL;                 /* in case we return an error */
004400000000    if (strm->zalloc == (alloc_func)0) {
004500000000        strm->zalloc = zcalloc;
004600000000        strm->opaque = (voidpf)0;
004700000000    }
004800000000    if (strm->zfree == (free_func)0) strm->zfree = zcfree;
004900000000    state = (struct inflate_state FAR *)ZALLOC(strm, 1,
005000000000                                               sizeof(struct inflate_state));
005100000000    if (state == Z_NULL) return Z_MEM_ERROR;
005200000000    Tracev((stderr, "inflate: allocated\n"));
005300000000    strm->state = (struct internal_state FAR *)state;
005400000000    state->dmax = 32768U;
005500000000    state->wbits = windowBits;
005600000000    state->wsize = 1U << windowBits;
005700000000    state->window = window;
005800000000    state->write = 0;
005900000000    state->whave = 0;
006000000000    return Z_OK;
006100000000}
006200000000
006300000000/*
006400000000   Return state with length and distance decoding tables and index sizes set to
006500000000   fixed code decoding.  Normally this returns fixed tables from inffixed.h.
006600000000   If BUILDFIXED is defined, then instead this routine builds the tables the
006700000000   first time it's called, and returns those tables the first time and
006800000000   thereafter.  This reduces the size of the code by about 2K bytes, in
006900000000   exchange for a little execution time.  However, BUILDFIXED should not be
007000000000   used for threaded applications, since the rewriting of the tables and virgin
007100000000   may not be thread-safe.
007200000000 */
007300000000local void fixedtables(state)
007400000000struct inflate_state FAR *state;
007500000000{
007600000000#ifdef BUILDFIXED
007700000000    static int virgin = 1;
007800000000    static code *lenfix, *distfix;
007900000000    static code fixed[544];
008000000000
008100000000    /* build fixed huffman tables if first call (may not be thread safe) */
008200000000    if (virgin) {
008300000000        unsigned sym, bits;
008400000000        static code *next;
008500000000
008600000000        /* literal/length table */
008700000000        sym = 0;
008800000000        while (sym < 144) state->lens[sym++] = 8;
008900000000        while (sym < 256) state->lens[sym++] = 9;
009000000000        while (sym < 280) state->lens[sym++] = 7;
009100000000        while (sym < 288) state->lens[sym++] = 8;
009200000000        next = fixed;
009300000000        lenfix = next;
009400000000        bits = 9;
009500000000        inflate_table(LENS, state->lens, 288, &(next), &(bits), state->work);
009600000000
009700000000        /* distance table */
009800000000        sym = 0;
009900000000        while (sym < 32) state->lens[sym++] = 5;
010000000000        distfix = next;
010100000000        bits = 5;
010200000000        inflate_table(DISTS, state->lens, 32, &(next), &(bits), state->work);
010300000000
010400000000        /* do this just once */
010500000000        virgin = 0;
010600000000    }
010700000000#else /* !BUILDFIXED */
010800000000#   include "inffixed.h"
010900000000#endif /* BUILDFIXED */
011000000000    state->lencode = lenfix;
011100000000    state->lenbits = 9;
011200000000    state->distcode = distfix;
011300000000    state->distbits = 5;
011400000000}
011500000000
011600000000/* Macros for inflateBack(): */
011700000000
011800000000/* Load returned state from inflate_fast() */
011900000000#define LOAD() \
012000000000    do { \
012100000000        put = strm->next_out; \
012200000000        left = strm->avail_out; \
012300000000        next = strm->next_in; \
012400000000        have = strm->avail_in; \
012500000000        hold = state->hold; \
012600000000        bits = state->bits; \
012700000000    } while (0)
012800000000
012900000000/* Set state from registers for inflate_fast() */
013000000000#define RESTORE() \
013100000000    do { \
013200000000        strm->next_out = put; \
013300000000        strm->avail_out = left; \
013400000000        strm->next_in = next; \
013500000000        strm->avail_in = have; \
013600000000        state->hold = hold; \
013700000000        state->bits = bits; \
013800000000    } while (0)
013900000000
014000000000/* Clear the input bit accumulator */
014100000000#define INITBITS() \
014200000000    do { \
014300000000        hold = 0; \
014400000000        bits = 0; \
014500000000    } while (0)
014600000000
014700000000/* Assure that some input is available.  If input is requested, but denied,
014800000000   then return a Z_BUF_ERROR from inflateBack(). */
014900000000#define PULL() \
015000000000    do { \
015100000000        if (have == 0) { \
015200000000            have = in(in_desc, &next); \
015300000000            if (have == 0) { \
015400000000                next = Z_NULL; \
015500000000                ret = Z_BUF_ERROR; \
015600000000                goto inf_leave; \
015700000000            } \
015800000000        } \
015900000000    } while (0)
016000000000
016100000000/* Get a byte of input into the bit accumulator, or return from inflateBack()
016200000000   with an error if there is no input available. */
016300000000#define PULLBYTE() \
016400000000    do { \
016500000000        PULL(); \
016600000000        have--; \
016700000000        hold += (unsigned long)(*next++) << bits; \
016800000000        bits += 8; \
016900000000    } while (0)
017000000000
017100000000/* Assure that there are at least n bits in the bit accumulator.  If there is
017200000000   not enough available input to do that, then return from inflateBack() with
017300000000   an error. */
017400000000#define NEEDBITS(n) \
017500000000    do { \
017600000000        while (bits < (unsigned)(n)) \
017700000000            PULLBYTE(); \
017800000000    } while (0)
017900000000
018000000000/* Return the low n bits of the bit accumulator (n < 16) */
018100000000#define BITS(n) \
018200000000    ((unsigned)hold & ((1U << (n)) - 1))
018300000000
018400000000/* Remove n bits from the bit accumulator */
018500000000#define DROPBITS(n) \
018600000000    do { \
018700000000        hold >>= (n); \
018800000000        bits -= (unsigned)(n); \
018900000000    } while (0)
019000000000
019100000000/* Remove zero to seven bits as needed to go to a byte boundary */
019200000000#define BYTEBITS() \
019300000000    do { \
019400000000        hold >>= bits & 7; \
019500000000        bits -= bits & 7; \
019600000000    } while (0)
019700000000
019800000000/* Assure that some output space is available, by writing out the window
019900000000   if it's full.  If the write fails, return from inflateBack() with a
020000000000   Z_BUF_ERROR. */
020100000000#define ROOM() \
020200000000    do { \
020300000000        if (left == 0) { \
020400000000            put = state->window; \
020500000000            left = state->wsize; \
020600000000            state->whave = left; \
020700000000            if (out(out_desc, put, left)) { \
020800000000                ret = Z_BUF_ERROR; \
020900000000                goto inf_leave; \
021000000000            } \
021100000000        } \
021200000000    } while (0)
021300000000
021400000000/*
021500000000   strm provides the memory allocation functions and window buffer on input,
021600000000   and provides information on the unused input on return.  For Z_DATA_ERROR
021700000000   returns, strm will also provide an error message.
021800000000
021900000000   in() and out() are the call-back input and output functions.  When
022000000000   inflateBack() needs more input, it calls in().  When inflateBack() has
022100000000   filled the window with output, or when it completes with data in the
022200000000   window, it calls out() to write out the data.  The application must not
022300000000   change the provided input until in() is called again or inflateBack()
022400000000   returns.  The application must not change the window/output buffer until
022500000000   inflateBack() returns.
022600000000
022700000000   in() and out() are called with a descriptor parameter provided in the
022800000000   inflateBack() call.  This parameter can be a structure that provides the
022900000000   information required to do the read or write, as well as accumulated
023000000000   information on the input and output such as totals and check values.
023100000000
023200000000   in() should return zero on failure.  out() should return non-zero on
023300000000   failure.  If either in() or out() fails, than inflateBack() returns a
023400000000   Z_BUF_ERROR.  strm->next_in can be checked for Z_NULL to see whether it
023500000000   was in() or out() that caused in the error.  Otherwise,  inflateBack()
023600000000   returns Z_STREAM_END on success, Z_DATA_ERROR for an deflate format
023700000000   error, or Z_MEM_ERROR if it could not allocate memory for the state.
023800000000   inflateBack() can also return Z_STREAM_ERROR if the input parameters
023900000000   are not correct, i.e. strm is Z_NULL or the state was not initialized.
024000000000 */
024100000000int ZEXPORT inflateBack(strm, in, in_desc, out, out_desc)
024200000000z_streamp strm;
024300000000in_func in;
024400000000void FAR *in_desc;
024500000000out_func out;
024600000000void FAR *out_desc;
024700000000{
024800000000    struct inflate_state FAR *state;
024900000000    unsigned char FAR *next;    /* next input */
025000000000    unsigned char FAR *put;     /* next output */
025100000000    unsigned have, left;        /* available input and output */
025200000000    unsigned long hold;         /* bit buffer */
025300000000    unsigned bits;              /* bits in bit buffer */
025400000000    unsigned copy;              /* number of stored or match bytes to copy */
025500000000    unsigned char FAR *from;    /* where to copy match bytes from */
025600000000    code this;                  /* current decoding table entry */
025700000000    code last;                  /* parent table entry */
025800000000    unsigned len;               /* length to copy for repeats, bits to drop */
025900000000    int ret;                    /* return code */
026000000000    static const unsigned short order[19] = /* permutation of code lengths */
026100000000        {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
026200000000
026300000000    /* Check that the strm exists and that the state was initialized */
026400000000    if (strm == Z_NULL || strm->state == Z_NULL)
026500000000        return Z_STREAM_ERROR;
026600000000    state = (struct inflate_state FAR *)strm->state;
026700000000
026800000000    /* Reset the state */
026900000000    strm->msg = Z_NULL;
027000000000    state->mode = TYPE;
027100000000    state->last = 0;
027200000000    state->whave = 0;
027300000000    next = strm->next_in;
027400000000    have = next != Z_NULL ? strm->avail_in : 0;
027500000000    hold = 0;
027600000000    bits = 0;
027700000000    put = state->window;
027800000000    left = state->wsize;
027900000000
028000000000    /* Inflate until end of block marked as last */
028100000000    for (;;)
028200000000        switch (state->mode) {
028300000000        case TYPE:
028400000000            /* determine and dispatch block type */
028500000000            if (state->last) {
028600000000                BYTEBITS();
028700000000                state->mode = DONE;
028800000000                break;
028900000000            }
029000000000            NEEDBITS(3);
029100000000            state->last = BITS(1);
029200000000            DROPBITS(1);
029300000000            switch (BITS(2)) {
029400000000            case 0:                             /* stored block */
029500000000                Tracev((stderr, "inflate:     stored block%s\n",
029600000000                        state->last ? " (last)" : ""));
029700000000                state->mode = STORED;
029800000000                break;
029900000000            case 1:                             /* fixed block */
030000000000                fixedtables(state);
030100000000                Tracev((stderr, "inflate:     fixed codes block%s\n",
030200000000                        state->last ? " (last)" : ""));
030300000000                state->mode = LEN;              /* decode codes */
030400000000                break;
030500000000            case 2:                             /* dynamic block */
030600000000                Tracev((stderr, "inflate:     dynamic codes block%s\n",
030700000000                        state->last ? " (last)" : ""));
030800000000                state->mode = TABLE;
030900000000                break;
031000000000            case 3:
031100000000                strm->msg = (char *)"invalid block type";
031200000000                state->mode = BAD;
031300000000            }
031400000000            DROPBITS(2);
031500000000            break;
031600000000
031700000000        case STORED:
031800000000            /* get and verify stored block length */
031900000000            BYTEBITS();                         /* go to byte boundary */
032000000000            NEEDBITS(32);
032100000000            if ((hold & 0xffff) != ((hold >> 16) ^ 0xffff)) {
032200000000                strm->msg = (char *)"invalid stored block lengths";
032300000000                state->mode = BAD;
032400000000                break;
032500000000            }
032600000000            state->length = (unsigned)hold & 0xffff;
032700000000            Tracev((stderr, "inflate:       stored length %u\n",
032800000000                    state->length));
032900000000            INITBITS();
033000000000
033100000000            /* copy stored block from input to output */
033200000000            while (state->length != 0) {
033300000000                copy = state->length;
033400000000                PULL();
033500000000                ROOM();
033600000000                if (copy > have) copy = have;
033700000000                if (copy > left) copy = left;
033800000000                zmemcpy(put, next, copy);
033900000000                have -= copy;
034000000000                next += copy;
034100000000                left -= copy;
034200000000                put += copy;
034300000000                state->length -= copy;
034400000000            }
034500000000            Tracev((stderr, "inflate:       stored end\n"));
034600000000            state->mode = TYPE;
034700000000            break;
034800000000
034900000000        case TABLE:
035000000000            /* get dynamic table entries descriptor */
035100000000            NEEDBITS(14);
035200000000            state->nlen = BITS(5) + 257;
035300000000            DROPBITS(5);
035400000000            state->ndist = BITS(5) + 1;
035500000000            DROPBITS(5);
035600000000            state->ncode = BITS(4) + 4;
035700000000            DROPBITS(4);
035800000000#ifndef PKZIP_BUG_WORKAROUND
035900000000            if (state->nlen > 286 || state->ndist > 30) {
036000000000                strm->msg = (char *)"too many length or distance symbols";
036100000000                state->mode = BAD;
036200000000                break;
036300000000            }
036400000000#endif
036500000000            Tracev((stderr, "inflate:       table sizes ok\n"));
036600000000
036700000000            /* get code length code lengths (not a typo) */
036800000000            state->have = 0;
036900000000            while (state->have < state->ncode) {
037000000000                NEEDBITS(3);
037100000000                state->lens[order[state->have++]] = (unsigned short)BITS(3);
037200000000                DROPBITS(3);
037300000000            }
037400000000            while (state->have < 19)
037500000000                state->lens[order[state->have++]] = 0;
037600000000            state->next = state->codes;
037700000000            state->lencode = (code const FAR *)(state->next);
037800000000            state->lenbits = 7;
037900000000            ret = inflate_table(CODES, state->lens, 19, &(state->next),
038000000000                                &(state->lenbits), state->work);
038100000000            if (ret) {
038200000000                strm->msg = (char *)"invalid code lengths set";
038300000000                state->mode = BAD;
038400000000                break;
038500000000            }
038600000000            Tracev((stderr, "inflate:       code lengths ok\n"));
038700000000
038800000000            /* get length and distance code code lengths */
038900000000            state->have = 0;
039000000000            while (state->have < state->nlen + state->ndist) {
039100000000                for (;;) {
039200000000                    this = state->lencode[BITS(state->lenbits)];
039300000000                    if ((unsigned)(this.bits) <= bits) break;
039400000000                    PULLBYTE();
039500000000                }
039600000000                if (this.val < 16) {
039700000000                    NEEDBITS(this.bits);
039800000000                    DROPBITS(this.bits);
039900000000                    state->lens[state->have++] = this.val;
040000000000                }
040100000000                else {
040200000000                    if (this.val == 16) {
040300000000                        NEEDBITS(this.bits + 2);
040400000000                        DROPBITS(this.bits);
040500000000                        if (state->have == 0) {
040600000000                            strm->msg = (char *)"invalid bit length repeat";
040700000000                            state->mode = BAD;
040800000000                            break;
040900000000                        }
041000000000                        len = (unsigned)(state->lens[state->have - 1]);
041100000000                        copy = 3 + BITS(2);
041200000000                        DROPBITS(2);
041300000000                    }
041400000000                    else if (this.val == 17) {
041500000000                        NEEDBITS(this.bits + 3);
041600000000                        DROPBITS(this.bits);
041700000000                        len = 0;
041800000000                        copy = 3 + BITS(3);
041900000000                        DROPBITS(3);
042000000000                    }
042100000000                    else {
042200000000                        NEEDBITS(this.bits + 7);
042300000000                        DROPBITS(this.bits);
042400000000                        len = 0;
042500000000                        copy = 11 + BITS(7);
042600000000                        DROPBITS(7);
042700000000                    }
042800000000                    if (state->have + copy > state->nlen + state->ndist) {
042900000000                        strm->msg = (char *)"invalid bit length repeat";
043000000000                        state->mode = BAD;
043100000000                        break;
043200000000                    }
043300000000                    while (copy--)
043400000000                        state->lens[state->have++] = (unsigned short)len;
043500000000                }
043600000000            }
043700000000
043800000000            /* handle error breaks in while */
043900000000            if (state->mode == BAD) break;
044000000000
044100000000            /* build code tables */
044200000000            state->next = state->codes;
044300000000            state->lencode = (code const FAR *)(state->next);
044400000000            state->lenbits = 9;
044500000000            ret = inflate_table(LENS, state->lens, state->nlen, &(state->next),
044600000000                                &(state->lenbits), state->work);
044700000000            if (ret) {
044800000000                strm->msg = (char *)"invalid literal/lengths set";
044900000000                state->mode = BAD;
045000000000                break;
045100000000            }
045200000000            state->distcode = (code const FAR *)(state->next);
045300000000            state->distbits = 6;
045400000000            ret = inflate_table(DISTS, state->lens + state->nlen, state->ndist,
045500000000                            &(state->next), &(state->distbits), state->work);
045600000000            if (ret) {
045700000000                strm->msg = (char *)"invalid distances set";
045800000000                state->mode = BAD;
045900000000                break;
046000000000            }
046100000000            Tracev((stderr, "inflate:       codes ok\n"));
046200000000            state->mode = LEN;
046300000000
046400000000        case LEN:
046500000000            /* use inflate_fast() if we have enough input and output */
046600000000            if (have >= 6 && left >= 258) {
046700000000                RESTORE();
046800000000                if (state->whave < state->wsize)
046900000000                    state->whave = state->wsize - left;
047000000000                inflate_fast(strm, state->wsize);
047100000000                LOAD();
047200000000                break;
047300000000            }
047400000000
047500000000            /* get a literal, length, or end-of-block code */
047600000000            for (;;) {
047700000000                this = state->lencode[BITS(state->lenbits)];
047800000000                if ((unsigned)(this.bits) <= bits) break;
047900000000                PULLBYTE();
048000000000            }
048100000000            if (this.op && (this.op & 0xf0) == 0) {
048200000000                last = this;
048300000000                for (;;) {
048400000000                    this = state->lencode[last.val +
048500000000                            (BITS(last.bits + last.op) >> last.bits)];
048600000000                    if ((unsigned)(last.bits + this.bits) <= bits) break;
048700000000                    PULLBYTE();
048800000000                }
048900000000                DROPBITS(last.bits);
049000000000            }
049100000000            DROPBITS(this.bits);
049200000000            state->length = (unsigned)this.val;
049300000000
049400000000            /* process literal */
049500000000            if (this.op == 0) {
049600000000                Tracevv((stderr, this.val >= 0x20 && this.val < 0x7f ?
049700000000                        "inflate:         literal '%c'\n" :
049800000000                        "inflate:         literal 0x%02x\n", this.val));
049900000000                ROOM();
050000000000                *put++ = (unsigned char)(state->length);
050100000000                left--;
050200000000                state->mode = LEN;
050300000000                break;
050400000000            }
050500000000
050600000000            /* process end of block */
050700000000            if (this.op & 32) {
050800000000                Tracevv((stderr, "inflate:         end of block\n"));
050900000000                state->mode = TYPE;
051000000000                break;
051100000000            }
051200000000
051300000000            /* invalid code */
051400000000            if (this.op & 64) {
051500000000                strm->msg = (char *)"invalid literal/length code";
051600000000                state->mode = BAD;
051700000000                break;
051800000000            }
051900000000
052000000000            /* length code -- get extra bits, if any */
052100000000            state->extra = (unsigned)(this.op) & 15;
052200000000            if (state->extra != 0) {
052300000000                NEEDBITS(state->extra);
052400000000                state->length += BITS(state->extra);
052500000000                DROPBITS(state->extra);
052600000000            }
052700000000            Tracevv((stderr, "inflate:         length %u\n", state->length));
052800000000
052900000000            /* get distance code */
053000000000            for (;;) {
053100000000                this = state->distcode[BITS(state->distbits)];
053200000000                if ((unsigned)(this.bits) <= bits) break;
053300000000                PULLBYTE();
053400000000            }
053500000000            if ((this.op & 0xf0) == 0) {
053600000000                last = this;
053700000000                for (;;) {
053800000000                    this = state->distcode[last.val +
053900000000                            (BITS(last.bits + last.op) >> last.bits)];
054000000000                    if ((unsigned)(last.bits + this.bits) <= bits) break;
054100000000                    PULLBYTE();
054200000000                }
054300000000                DROPBITS(last.bits);
054400000000            }
054500000000            DROPBITS(this.bits);
054600000000            if (this.op & 64) {
054700000000                strm->msg = (char *)"invalid distance code";
054800000000                state->mode = BAD;
054900000000                break;
055000000000            }
055100000000            state->offset = (unsigned)this.val;
055200000000
055300000000            /* get distance extra bits, if any */
055400000000            state->extra = (unsigned)(this.op) & 15;
055500000000            if (state->extra != 0) {
055600000000                NEEDBITS(state->extra);
055700000000                state->offset += BITS(state->extra);
055800000000                DROPBITS(state->extra);
055900000000            }
056000000000            if (state->offset > state->wsize - (state->whave < state->wsize ?
056100000000                                                left : 0)) {
056200000000                strm->msg = (char *)"invalid distance too far back";
056300000000                state->mode = BAD;
056400000000                break;
056500000000            }
056600000000            Tracevv((stderr, "inflate:         distance %u\n", state->offset));
056700000000
056800000000            /* copy match from window to output */
056900000000            do {
057000000000                ROOM();
057100000000                copy = state->wsize - state->offset;
057200000000                if (copy < left) {
057300000000                    from = put + copy;
057400000000                    copy = left - copy;
057500000000                }
057600000000                else {
057700000000                    from = put - state->offset;
057800000000                    copy = left;
057900000000                }
058000000000                if (copy > state->length) copy = state->length;
058100000000                state->length -= copy;
058200000000                left -= copy;
058300000000                do {
058400000000                    *put++ = *from++;
058500000000                } while (--copy);
058600000000            } while (state->length != 0);
058700000000            break;
058800000000
058900000000        case DONE:
059000000000            /* inflate stream terminated properly -- write leftover output */
059100000000            ret = Z_STREAM_END;
059200000000            if (left < state->wsize) {
059300000000                if (out(out_desc, state->window, state->wsize - left))
059400000000                    ret = Z_BUF_ERROR;
059500000000            }
059600000000            goto inf_leave;
059700000000
059800000000        case BAD:
059900000000            ret = Z_DATA_ERROR;
060000000000            goto inf_leave;
060100000000
060200000000        default:                /* can't happen, but makes compilers happy */
060300000000            ret = Z_STREAM_ERROR;
060400000000            goto inf_leave;
060500000000        }
060600000000
060700000000    /* Return unused input */
060800000000  inf_leave:
060900000000    strm->next_in = next;
061000000000    strm->avail_in = have;
061100000000    return ret;
061200000000}
061300000000
061400000000int ZEXPORT inflateBackEnd(strm)
061500000000z_streamp strm;
061600000000{
061700000000    if (strm == Z_NULL || strm->state == Z_NULL || strm->zfree == (free_func)0)
061800000000        return Z_STREAM_ERROR;
061900000000    ZFREE(strm, strm->state);
062000000000    strm->state = Z_NULL;
062100000000    Tracev((stderr, "inflate: end\n"));
062200000000    return Z_OK;
062300000000}
