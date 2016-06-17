000100040201             CMD        PROMPT('Zip (compress) file')
000200040201
000300040201             /* REQUIRED PARAMETERS */
000400040201
000500040201             PARM       KWD(ZIPFILE) TYPE(*PNAME) LEN(128) MIN(1) +
000600040201                          EXPR(*YES) CASE(*MIXED) PROMPT('ZIP file +
000700040201                          name')
000800040201
000900040201             PARM       KWD(FILES) TYPE(*PNAME) LEN(128) MIN(1) +
001000040201                          MAX(20) EXPR(*YES) CASE(*MIXED) +
001100040201                          PROMPT('Files to add ZIP file')
001200040201
001300040201             /* OPTIONAL PARAMETERS */
001400040201
001500040201             PARM       KWD(REPLACE) TYPE(*CHAR) LEN(8) RSTD(*YES) +
001600040201                          DFT(*REPLACE) VALUES(*REPLACE *APPEND +
001700040201                          *PROMPT) EXPR(*YES) PROMPT('Overwrite +
001800040201                          existing ZIP file')
001900040201
002000040201             PARM       KWD(COMPLVL) TYPE(*CHAR) LEN(1) +
002100040201                          DFT(*DEFAULT) RANGE('0' '9') +
002200040201                          SPCVAL((*DEFAULT '6')) EXPR(*YES) +
002300040201                          PROMPT('Compression level')
002400040201
002500040201             /* PMTRQS PARAMETERS */
002600040201
002700040201             PARM       KWD(PASSWORD) TYPE(*CHAR) LEN(32) DFT(' ') +
002800040201                          EXPR(*YES) CASE(*MIXED) DSPINPUT(*NO) +
002900040201                          PMTCTL(*PMTRQS) PROMPT('Password for ZIP +
003000040201                          file')
003100040201
