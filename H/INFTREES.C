000100000000/* inftrees.h -- header to use inftrees.c
000200000000 * Copyright (C) 1995-2005 Mark Adler
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/* WARNING: this file should *not* be used by applications. It is
000700000000   part of the implementation of the compression library and is
000800000000   subject to change. Applications should only use zlib.h.
000900000000 */
001000000000
001100000000/* Structure for decoding tables.  Each entry provides either the
001200000000   information needed to do the operation requested by the code that
001300000000   indexed that table entry, or it provides a pointer to another
001400000000   table that indexes more bits of the code.  op indicates whether
001500000000   the entry is a pointer to another table, a literal, a length or
001600000000   distance, an end-of-block, or an invalid code.  For a table
001700000000   pointer, the low four bits of op is the number of index bits of
001800000000   that table.  For a length or distance, the low four bits of op
001900000000   is the number of extra bits to get after the code.  bits is
002000000000   the number of bits in this code or part of the code to drop off
002100000000   of the bit buffer.  val is the actual byte to output in the case
002200000000   of a literal, the base length or distance, or the offset from
002300000000   the current table to the next table.  Each entry is four bytes. */
002400000000typedef struct {
002500000000    unsigned char op;           /* operation, extra bits, table bits */
002600000000    unsigned char bits;         /* bits in this part of the code */
002700000000    unsigned short val;         /* offset in table or code value */
002800000000} code;
002900000000
003000000000/* op values as set by inflate_table():
003100000000    00000000 - literal
003200000000    0000tttt - table link, tttt != 0 is the number of table index bits
003300000000    0001eeee - length or distance, eeee is the number of extra bits
003400000000    01100000 - end of block
003500000000    01000000 - invalid code
003600000000 */
003700000000
003800000000/* Maximum size of dynamic tree.  The maximum found in a long but non-
003900000000   exhaustive search was 1444 code structures (852 for length/literals
004000000000   and 592 for distances, the latter actually the result of an
004100000000   exhaustive search).  The true maximum is not known, but the value
004200000000   below is more than safe. */
004300000000#define ENOUGH 2048
004400000000#define MAXD 592
004500000000
004600000000/* Type of code to build for inftable() */
004700000000typedef enum {
004800000000    CODES,
004900000000    LENS,
005000000000    DISTS
005100000000} codetype;
005200000000
005300000000extern int inflate_table OF((codetype type, unsigned short FAR *lens,
005400000000                             unsigned codes, code FAR * FAR *table,
005500000000                             unsigned FAR *bits, unsigned short FAR *work));
