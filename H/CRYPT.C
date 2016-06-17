000100000000/* crypt.h -- base code for crypt/uncrypt ZIPfile
000200000000
000300000000
000400000000   Version 1.01e, February 12th, 2005
000500000000
000600000000   Copyright (C) 1998-2005 Gilles Vollant
000700000000
000800000000   This code is a modified version of crypting code in Infozip distribution
000900000000
001000000000   The encryption/decryption parts of this source code (as opposed to the
001100000000   non-echoing password parts) were originally written in Europe.  The
001200000000   whole source package can be freely distributed, including from the USA.
001300000000   (Prior to January 2000, re-export from the US was a violation of US law.)
001400000000
001500000000   This encryption code is a direct transcription of the algorithm from
001600000000   Roger Schlafly, described by Phil Katz in the file appnote.txt.  This
001700000000   file (appnote.txt) is distributed with the PKZIP program (even in the
001800000000   version without encryption capabilities).
001900000000
002000000000   If you don't need crypting in your application, just define symbols
002100000000   NOCRYPT and NOUNCRYPT.
002200000000
002300000000   This code support the "Traditional PKWARE Encryption".
002400000000
002500000000   The new AES encryption added on Zip format by Winzip (see the page
002600000000   http://www.winzip.com/aes_info.htm ) and PKWare PKZip 5.x Strong
002700000000   Encryption is not supported.
002800000000*/
002900000000
003000000000#define CRC32(c, b) ((*(pcrc_32_tab+(((int)(c) ^ (b)) & 0xff))) ^ ((c) >> 8))
003100000000
003200000000/***********************************************************************
003300000000 * Return the next byte in the pseudo-random sequence
003400000000 */
003500000000static int decrypt_byte(unsigned long* pkeys, const unsigned long* pcrc_32_tab)
003600000000{
003700000000    unsigned temp;  /* POTENTIAL BUG:  temp*(temp^1) may overflow in an
003800000000                     * unpredictable manner on 16-bit systems; not a problem
003900000000                     * with any known compiler so far, though */
004000000000
004100000000    temp = ((unsigned)(*(pkeys+2)) & 0xffff) | 2;
004200000000    return (int)(((temp * (temp ^ 1)) >> 8) & 0xff);
004300000000}
004400000000
004500000000/***********************************************************************
004600000000 * Update the encryption keys with the next byte of plain text
004700000000 */
004800000000static int update_keys(unsigned long* pkeys,const unsigned long* pcrc_32_tab,int c)
004900000000{
005000000000    (*(pkeys+0)) = CRC32((*(pkeys+0)), c);
005100000000    (*(pkeys+1)) += (*(pkeys+0)) & 0xff;
005200000000    (*(pkeys+1)) = (*(pkeys+1)) * 134775813L + 1;
005300000000    {
005400000000      register int keyshift = (int)((*(pkeys+1)) >> 24);
005500000000      (*(pkeys+2)) = CRC32((*(pkeys+2)), keyshift);
005600000000    }
005700000000    return c;
005800000000}
005900000000
006000000000
006100000000/***********************************************************************
006200000000 * Initialize the encryption keys and the random header according to
006300000000 * the given password.
006400000000 */
006500000000static void init_keys(const char* passwd,unsigned long* pkeys,const unsigned long* pcrc_32_tab)
006600000000{
006700000000    *(pkeys+0) = 305419896L;
006800000000    *(pkeys+1) = 591751049L;
006900000000    *(pkeys+2) = 878082192L;
007000000000    while (*passwd != '\0') {
007100000000        update_keys(pkeys,pcrc_32_tab,(int)*passwd);
007200000000        passwd++;
007300000000    }
007400000000}
007500000000
007600000000#define zdecode(pkeys,pcrc_32_tab,c) \
007700000000    (update_keys(pkeys,pcrc_32_tab,c ^= decrypt_byte(pkeys,pcrc_32_tab)))
007800000000
007900000000#define zencode(pkeys,pcrc_32_tab,c,t) \
008000000000    (t=decrypt_byte(pkeys,pcrc_32_tab), update_keys(pkeys,pcrc_32_tab,c), t^(c))
008100000000
008200000000#ifdef INCLUDECRYPTINGCODE_IFCRYPTALLOWED
008300000000
008400000000#define RAND_HEAD_LEN  12
008500000000   /* "last resort" source for second part of crypt seed pattern */
008600000000#  ifndef ZCR_SEED2
008700000000#    define ZCR_SEED2 3141592654UL     /* use PI as default pattern */
008800000000#  endif
008900000000
009000000000static int crypthead(passwd, buf, bufSize, pkeys, pcrc_32_tab, crcForCrypting)
009100000000    const char *passwd;         /* password string */
009200000000    unsigned char *buf;         /* where to write header */
009300000000    int bufSize;
009400000000    unsigned long* pkeys;
009500000000    const unsigned long* pcrc_32_tab;
009600000000    unsigned long crcForCrypting;
009700000000{
009800000000    int n;                       /* index in random header */
009900000000    int t;                       /* temporary */
010000000000    int c;                       /* random byte */
010100000000    unsigned char header[RAND_HEAD_LEN-2]; /* random header */
010200000000    static unsigned calls = 0;   /* ensure different random header each time */
010300000000
010400000000    if (bufSize<RAND_HEAD_LEN)
010500000000      return 0;
010600000000
010700000000    /* First generate RAND_HEAD_LEN-2 random bytes. We encrypt the
010800000000     * output of rand() to get less predictability, since rand() is
010900000000     * often poorly implemented.
011000000000     */
011100000000    if (++calls == 1)
011200000000    {
011300000000        srand((unsigned)(time(NULL) ^ ZCR_SEED2));
011400000000    }
011500000000    init_keys(passwd, pkeys, pcrc_32_tab);
011600000000    for (n = 0; n < RAND_HEAD_LEN-2; n++)
011700000000    {
011800000000        c = (rand() >> 7) & 0xff;
011900000000        header[n] = (unsigned char)zencode(pkeys, pcrc_32_tab, c, t);
012000000000    }
012100000000    /* Encrypt random header (last two bytes is high word of crc) */
012200000000    init_keys(passwd, pkeys, pcrc_32_tab);
012300000000    for (n = 0; n < RAND_HEAD_LEN-2; n++)
012400000000    {
012500000000        buf[n] = (unsigned char)zencode(pkeys, pcrc_32_tab, header[n], t);
012600000000    }
012700000000    buf[n++] = zencode(pkeys, pcrc_32_tab, (int)(crcForCrypting >> 16) & 0xff, t);
012800000000    buf[n++] = zencode(pkeys, pcrc_32_tab, (int)(crcForCrypting >> 24) & 0xff, t);
012900000000    return n;
013000000000}
013100000000
013200000000#endif
