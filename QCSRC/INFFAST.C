000100000000/* inffast.c -- fast decoding
000200000000 * Copyright (C) 1995-2004 Mark Adler
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000#include "zutil.h"
000700000000#include "inftrees.h"
000800000000#include "inflate.h"
000900000000#include "inffast.h"
001000000000
001100000000#ifndef ASMINF
001200000000
001300000000/* Allow machine dependent optimization for post-increment or pre-increment.
001400000000   Based on testing to date,
001500000000   Pre-increment preferred for:
001600000000   - PowerPC G3 (Adler)
001700000000   - MIPS R5000 (Randers-Pehrson)
001800000000   Post-increment preferred for:
001900000000   - none
002000000000   No measurable difference:
002100000000   - Pentium III (Anderson)
002200000000   - M68060 (Nikl)
002300000000 */
002400000000#ifdef POSTINC
002500000000#  define OFF 0
002600000000#  define PUP(a) *(a)++
002700000000#else
002800000000#  define OFF 1
002900000000#  define PUP(a) *++(a)
003000000000#endif
003100000000
003200000000/*
003300000000   Decode literal, length, and distance codes and write out the resulting
003400000000   literal and match bytes until either not enough input or output is
003500000000   available, an end-of-block is encountered, or a data error is encountered.
003600000000   When large enough input and output buffers are supplied to inflate(), for
003700000000   example, a 16K input buffer and a 64K output buffer, more than 95% of the
003800000000   inflate execution time is spent in this routine.
003900000000
004000000000   Entry assumptions:
004100000000
004200000000        state->mode == LEN
004300000000        strm->avail_in >= 6
004400000000        strm->avail_out >= 258
004500000000        start >= strm->avail_out
004600000000        state->bits < 8
004700000000
004800000000   On return, state->mode is one of:
004900000000
005000000000        LEN -- ran out of enough output space or enough available input
005100000000        TYPE -- reached end of block code, inflate() to interpret next block
005200000000        BAD -- error in block data
005300000000
005400000000   Notes:
005500000000
005600000000    - The maximum input bits used by a length/distance pair is 15 bits for the
005700000000      length code, 5 bits for the length extra, 15 bits for the distance code,
005800000000      and 13 bits for the distance extra.  This totals 48 bits, or six bytes.
005900000000      Therefore if strm->avail_in >= 6, then there is enough input to avoid
006000000000      checking for available input while decoding.
006100000000
006200000000    - The maximum bytes that a single length/distance pair can output is 258
006300000000      bytes, which is the maximum length that can be coded.  inflate_fast()
006400000000      requires strm->avail_out >= 258 for each loop to avoid checking for
006500000000      output space.
006600000000 */
006700000000void inflate_fast(strm, start)
006800000000z_streamp strm;
006900000000unsigned start;         /* inflate()'s starting value for strm->avail_out */
007000000000{
007100000000    struct inflate_state FAR *state;
007200000000    unsigned char FAR *in;      /* local strm->next_in */
007300000000    unsigned char FAR *last;    /* while in < last, enough input available */
007400000000    unsigned char FAR *out;     /* local strm->next_out */
007500000000    unsigned char FAR *beg;     /* inflate()'s initial strm->next_out */
007600000000    unsigned char FAR *end;     /* while out < end, enough space available */
007700000000#ifdef INFLATE_STRICT
007800000000    unsigned dmax;              /* maximum distance from zlib header */
007900000000#endif
008000000000    unsigned wsize;             /* window size or zero if not using window */
008100000000    unsigned whave;             /* valid bytes in the window */
008200000000    unsigned write;             /* window write index */
008300000000    unsigned char FAR *window;  /* allocated sliding window, if wsize != 0 */
008400000000    unsigned long hold;         /* local strm->hold */
008500000000    unsigned bits;              /* local strm->bits */
008600000000    code const FAR *lcode;      /* local strm->lencode */
008700000000    code const FAR *dcode;      /* local strm->distcode */
008800000000    unsigned lmask;             /* mask for first level of length codes */
008900000000    unsigned dmask;             /* mask for first level of distance codes */
009000000000    code this;                  /* retrieved table entry */
009100000000    unsigned op;                /* code bits, operation, extra bits, or */
009200000000                                /*  window position, window bytes to copy */
009300000000    unsigned len;               /* match length, unused bytes */
009400000000    unsigned dist;              /* match distance */
009500000000    unsigned char FAR *from;    /* where to copy match from */
009600000000
009700000000    /* copy state to local variables */
009800000000    state = (struct inflate_state FAR *)strm->state;
009900000000    in = strm->next_in - OFF;
010000000000    last = in + (strm->avail_in - 5);
010100000000    out = strm->next_out - OFF;
010200000000    beg = out - (start - strm->avail_out);
010300000000    end = out + (strm->avail_out - 257);
010400000000#ifdef INFLATE_STRICT
010500000000    dmax = state->dmax;
010600000000#endif
010700000000    wsize = state->wsize;
010800000000    whave = state->whave;
010900000000    write = state->write;
011000000000    window = state->window;
011100000000    hold = state->hold;
011200000000    bits = state->bits;
011300000000    lcode = state->lencode;
011400000000    dcode = state->distcode;
011500000000    lmask = (1U << state->lenbits) - 1;
011600000000    dmask = (1U << state->distbits) - 1;
011700000000
011800000000    /* decode literals and length/distances until end-of-block or not enough
011900000000       input data or output space */
012000000000    do {
012100000000        if (bits < 15) {
012200000000            hold += (unsigned long)(PUP(in)) << bits;
012300000000            bits += 8;
012400000000            hold += (unsigned long)(PUP(in)) << bits;
012500000000            bits += 8;
012600000000        }
012700000000        this = lcode[hold & lmask];
012800000000      dolen:
012900000000        op = (unsigned)(this.bits);
013000000000        hold >>= op;
013100000000        bits -= op;
013200000000        op = (unsigned)(this.op);
013300000000        if (op == 0) {                          /* literal */
013400000000            Tracevv((stderr, this.val >= 0x20 && this.val < 0x7f ?
013500000000                    "inflate:         literal '%c'\n" :
013600000000                    "inflate:         literal 0x%02x\n", this.val));
013700000000            PUP(out) = (unsigned char)(this.val);
013800000000        }
013900000000        else if (op & 16) {                     /* length base */
014000000000            len = (unsigned)(this.val);
014100000000            op &= 15;                           /* number of extra bits */
014200000000            if (op) {
014300000000                if (bits < op) {
014400000000                    hold += (unsigned long)(PUP(in)) << bits;
014500000000                    bits += 8;
014600000000                }
014700000000                len += (unsigned)hold & ((1U << op) - 1);
014800000000                hold >>= op;
014900000000                bits -= op;
015000000000            }
015100000000            Tracevv((stderr, "inflate:         length %u\n", len));
015200000000            if (bits < 15) {
015300000000                hold += (unsigned long)(PUP(in)) << bits;
015400000000                bits += 8;
015500000000                hold += (unsigned long)(PUP(in)) << bits;
015600000000                bits += 8;
015700000000            }
015800000000            this = dcode[hold & dmask];
015900000000          dodist:
016000000000            op = (unsigned)(this.bits);
016100000000            hold >>= op;
016200000000            bits -= op;
016300000000            op = (unsigned)(this.op);
016400000000            if (op & 16) {                      /* distance base */
016500000000                dist = (unsigned)(this.val);
016600000000                op &= 15;                       /* number of extra bits */
016700000000                if (bits < op) {
016800000000                    hold += (unsigned long)(PUP(in)) << bits;
016900000000                    bits += 8;
017000000000                    if (bits < op) {
017100000000                        hold += (unsigned long)(PUP(in)) << bits;
017200000000                        bits += 8;
017300000000                    }
017400000000                }
017500000000                dist += (unsigned)hold & ((1U << op) - 1);
017600000000#ifdef INFLATE_STRICT
017700000000                if (dist > dmax) {
017800000000                    strm->msg = (char *)"invalid distance too far back";
017900000000                    state->mode = BAD;
018000000000                    break;
018100000000                }
018200000000#endif
018300000000                hold >>= op;
018400000000                bits -= op;
018500000000                Tracevv((stderr, "inflate:         distance %u\n", dist));
018600000000                op = (unsigned)(out - beg);     /* max distance in output */
018700000000                if (dist > op) {                /* see if copy from window */
018800000000                    op = dist - op;             /* distance back in window */
018900000000                    if (op > whave) {
019000000000                        strm->msg = (char *)"invalid distance too far back";
019100000000                        state->mode = BAD;
019200000000                        break;
019300000000                    }
019400000000                    from = window - OFF;
019500000000                    if (write == 0) {           /* very common case */
019600000000                        from += wsize - op;
019700000000                        if (op < len) {         /* some from window */
019800000000                            len -= op;
019900000000                            do {
020000000000                                PUP(out) = PUP(from);
020100000000                            } while (--op);
020200000000                            from = out - dist;  /* rest from output */
020300000000                        }
020400000000                    }
020500000000                    else if (write < op) {      /* wrap around window */
020600000000                        from += wsize + write - op;
020700000000                        op -= write;
020800000000                        if (op < len) {         /* some from end of window */
020900000000                            len -= op;
021000000000                            do {
021100000000                                PUP(out) = PUP(from);
021200000000                            } while (--op);
021300000000                            from = window - OFF;
021400000000                            if (write < len) {  /* some from start of window */
021500000000                                op = write;
021600000000                                len -= op;
021700000000                                do {
021800000000                                    PUP(out) = PUP(from);
021900000000                                } while (--op);
022000000000                                from = out - dist;      /* rest from output */
022100000000                            }
022200000000                        }
022300000000                    }
022400000000                    else {                      /* contiguous in window */
022500000000                        from += write - op;
022600000000                        if (op < len) {         /* some from window */
022700000000                            len -= op;
022800000000                            do {
022900000000                                PUP(out) = PUP(from);
023000000000                            } while (--op);
023100000000                            from = out - dist;  /* rest from output */
023200000000                        }
023300000000                    }
023400000000                    while (len > 2) {
023500000000                        PUP(out) = PUP(from);
023600000000                        PUP(out) = PUP(from);
023700000000                        PUP(out) = PUP(from);
023800000000                        len -= 3;
023900000000                    }
024000000000                    if (len) {
024100000000                        PUP(out) = PUP(from);
024200000000                        if (len > 1)
024300000000                            PUP(out) = PUP(from);
024400000000                    }
024500000000                }
024600000000                else {
024700000000                    from = out - dist;          /* copy direct from output */
024800000000                    do {                        /* minimum length is three */
024900000000                        PUP(out) = PUP(from);
025000000000                        PUP(out) = PUP(from);
025100000000                        PUP(out) = PUP(from);
025200000000                        len -= 3;
025300000000                    } while (len > 2);
025400000000                    if (len) {
025500000000                        PUP(out) = PUP(from);
025600000000                        if (len > 1)
025700000000                            PUP(out) = PUP(from);
025800000000                    }
025900000000                }
026000000000            }
026100000000            else if ((op & 64) == 0) {          /* 2nd level distance code */
026200000000                this = dcode[this.val + (hold & ((1U << op) - 1))];
026300000000                goto dodist;
026400000000            }
026500000000            else {
026600000000                strm->msg = (char *)"invalid distance code";
026700000000                state->mode = BAD;
026800000000                break;
026900000000            }
027000000000        }
027100000000        else if ((op & 64) == 0) {              /* 2nd level length code */
027200000000            this = lcode[this.val + (hold & ((1U << op) - 1))];
027300000000            goto dolen;
027400000000        }
027500000000        else if (op & 32) {                     /* end-of-block */
027600000000            Tracevv((stderr, "inflate:         end of block\n"));
027700000000            state->mode = TYPE;
027800000000            break;
027900000000        }
028000000000        else {
028100000000            strm->msg = (char *)"invalid literal/length code";
028200000000            state->mode = BAD;
028300000000            break;
028400000000        }
028500000000    } while (in < last && out < end);
028600000000
028700000000    /* return unused bytes (on entry, bits < 8, so in won't go too far back) */
028800000000    len = bits >> 3;
028900000000    in -= len;
029000000000    bits -= len << 3;
029100000000    hold &= (1U << bits) - 1;
029200000000
029300000000    /* update state and return */
029400000000    strm->next_in = in + OFF;
029500000000    strm->next_out = out + OFF;
029600000000    strm->avail_in = (unsigned)(in < last ? 5 + (last - in) : 5 - (in - last));
029700000000    strm->avail_out = (unsigned)(out < end ?
029800000000                                 257 + (end - out) : 257 - (out - end));
029900000000    state->hold = hold;
030000000000    state->bits = bits;
030100000000    return;
030200000000}
030300000000
030400000000/*
030500000000   inflate_fast() speedups that turned out slower (on a PowerPC G3 750CXe):
030600000000   - Using bit fields for code structure
030700000000   - Different op definition to avoid & for extra bits (do & for table bits)
030800000000   - Three separate decoding do-loops for direct, window, and write == 0
030900000000   - Special case for distance > 1 copies to do overlapped load and store copy
031000000000   - Explicit branch predictions (based on measured branch probabilities)
031100000000   - Deferring match copy and interspersed it with decoding subsequent codes
031200000000   - Swapping literal/length else
031300000000   - Swapping window/direct else
031400000000   - Larger unrolled copy loops (three is about right)
031500000000   - Moving len -= 3 statement into middle of loop
031600000000 */
031700000000
031800000000#endif /* !ASMINF */
