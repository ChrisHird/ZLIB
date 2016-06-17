000100000000/* inflate.h -- internal inflate state definition
000200000000 * Copyright (C) 1995-2004 Mark Adler
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/* WARNING: this file should *not* be used by applications. It is
000700000000   part of the implementation of the compression library and is
000800000000   subject to change. Applications should only use zlib.h.
000900000000 */
001000000000
001100000000/* define NO_GZIP when compiling if you want to disable gzip header and
001200000000   trailer decoding by inflate().  NO_GZIP would be used to avoid linking in
001300000000   the crc code when it is not needed.  For shared libraries, gzip decoding
001400000000   should be left enabled. */
001500000000#ifndef NO_GZIP
001600000000#  define GUNZIP
001700000000#endif
001800000000
001900000000/* Possible inflate modes between inflate() calls */
002000000000typedef enum {
002100000000    HEAD,       /* i: waiting for magic header */
002200000000    FLAGS,      /* i: waiting for method and flags (gzip) */
002300000000    TIME,       /* i: waiting for modification time (gzip) */
002400000000    OS,         /* i: waiting for extra flags and operating system (gzip) */
002500000000    EXLEN,      /* i: waiting for extra length (gzip) */
002600000000    EXTRA,      /* i: waiting for extra bytes (gzip) */
002700000000    NAME,       /* i: waiting for end of file name (gzip) */
002800000000    COMMENT,    /* i: waiting for end of comment (gzip) */
002900000000    HCRC,       /* i: waiting for header crc (gzip) */
003000000000    DICTID,     /* i: waiting for dictionary check value */
003100000000    DICT,       /* waiting for inflateSetDictionary() call */
003200000000        TYPE,       /* i: waiting for type bits, including last-flag bit */
003300000000        TYPEDO,     /* i: same, but skip check to exit inflate on new block */
003400000000        STORED,     /* i: waiting for stored size (length and complement) */
003500000000        COPY,       /* i/o: waiting for input or output to copy stored block */
003600000000        TABLE,      /* i: waiting for dynamic block table lengths */
003700000000        LENLENS,    /* i: waiting for code length code lengths */
003800000000        CODELENS,   /* i: waiting for length/lit and distance code lengths */
003900000000            LEN,        /* i: waiting for length/lit code */
004000000000            LENEXT,     /* i: waiting for length extra bits */
004100000000            DIST,       /* i: waiting for distance code */
004200000000            DISTEXT,    /* i: waiting for distance extra bits */
004300000000            MATCH,      /* o: waiting for output space to copy string */
004400000000            LIT,        /* o: waiting for output space to write literal */
004500000000    CHECK,      /* i: waiting for 32-bit check value */
004600000000    LENGTH,     /* i: waiting for 32-bit length (gzip) */
004700000000    DONE,       /* finished check, done -- remain here until reset */
004800000000    BAD,        /* got a data error -- remain here until reset */
004900000000    MEM,        /* got an inflate() memory error -- remain here until reset */
005000000000    SYNC        /* looking for synchronization bytes to restart inflate() */
005100000000} inflate_mode;
005200000000
005300000000/*
005400000000    State transitions between above modes -
005500000000
005600000000    (most modes can go to the BAD or MEM mode -- not shown for clarity)
005700000000
005800000000    Process header:
005900000000        HEAD -> (gzip) or (zlib)
006000000000        (gzip) -> FLAGS -> TIME -> OS -> EXLEN -> EXTRA -> NAME
006100000000        NAME -> COMMENT -> HCRC -> TYPE
006200000000        (zlib) -> DICTID or TYPE
006300000000        DICTID -> DICT -> TYPE
006400000000    Read deflate blocks:
006500000000            TYPE -> STORED or TABLE or LEN or CHECK
006600000000            STORED -> COPY -> TYPE
006700000000            TABLE -> LENLENS -> CODELENS -> LEN
006800000000    Read deflate codes:
006900000000                LEN -> LENEXT or LIT or TYPE
007000000000                LENEXT -> DIST -> DISTEXT -> MATCH -> LEN
007100000000                LIT -> LEN
007200000000    Process trailer:
007300000000        CHECK -> LENGTH -> DONE
007400000000 */
007500000000
007600000000/* state maintained between inflate() calls.  Approximately 7K bytes. */
007700000000struct inflate_state {
007800000000    inflate_mode mode;          /* current inflate mode */
007900000000    int last;                   /* true if processing last block */
008000000000    int wrap;                   /* bit 0 true for zlib, bit 1 true for gzip */
008100000000    int havedict;               /* true if dictionary provided */
008200000000    int flags;                  /* gzip header method and flags (0 if zlib) */
008300000000    unsigned dmax;              /* zlib header max distance (INFLATE_STRICT) */
008400000000    unsigned long check;        /* protected copy of check value */
008500000000    unsigned long total;        /* protected copy of output count */
008600000000    gz_headerp head;            /* where to save gzip header information */
008700000000        /* sliding window */
008800000000    unsigned wbits;             /* log base 2 of requested window size */
008900000000    unsigned wsize;             /* window size or zero if not using window */
009000000000    unsigned whave;             /* valid bytes in the window */
009100000000    unsigned write;             /* window write index */
009200000000    unsigned char FAR *window;  /* allocated sliding window, if needed */
009300000000        /* bit accumulator */
009400000000    unsigned long hold;         /* input bit accumulator */
009500000000    unsigned bits;              /* number of bits in "in" */
009600000000        /* for string and stored block copying */
009700000000    unsigned length;            /* literal or length of data to copy */
009800000000    unsigned offset;            /* distance back to copy string from */
009900000000        /* for table and code decoding */
010000000000    unsigned extra;             /* extra bits needed */
010100000000        /* fixed and dynamic code tables */
010200000000    code const FAR *lencode;    /* starting table for length/literal codes */
010300000000    code const FAR *distcode;   /* starting table for distance codes */
010400000000    unsigned lenbits;           /* index bits for lencode */
010500000000    unsigned distbits;          /* index bits for distcode */
010600000000        /* dynamic table building */
010700000000    unsigned ncode;             /* number of code length code lengths */
010800000000    unsigned nlen;              /* number of length code lengths */
010900000000    unsigned ndist;             /* number of distance code lengths */
011000000000    unsigned have;              /* number of code lengths in lens[] */
011100000000    code FAR *next;             /* next available space in codes[] */
011200000000    unsigned short lens[320];   /* temporary storage for code lengths */
011300000000    unsigned short work[288];   /* work area for code table building */
011400000000    code codes[ENOUGH];         /* space for code tables */
011500000000};
