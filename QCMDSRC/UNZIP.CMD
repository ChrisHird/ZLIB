000100040201             CMD        PROMPT('Unpack Zipped File')
000200040201
000300040201             /* REQUIRED PARAMETERS */
000400040201
000500040201             PARM       KWD(ZIPFILE) TYPE(*PNAME) LEN(128) MIN(1) +
000600040201                          EXPR(*YES) CASE(*MIXED) PROMPT('ZIP file +
000700040201                          name')
000800040201
000900040201             /* OPTIONAL PARAMETERS */
001000040201
001100040201             PARM       KWD(FILE) TYPE(*PNAME) LEN(128) DFT(*ALL) +
001200040201                          SPCVAL((*ALL)) EXPR(*YES) CASE(*MIXED) +
001300040201                          PROMPT('File to extract from ZIP file')
001400040201
001500040201             PARM       KWD(ADDPATH) TYPE(*CHAR) LEN(4) RSTD(*YES) +
001600040201                          DFT(*NO) VALUES(*YES *NO) EXPR(*YES) +
001700040201                          PROMPT('Extract with pathname')
001800040201
001900040201             PARM       KWD(REPLACE) TYPE(*CHAR) LEN(7) RSTD(*YES) +
002000040201                          DFT(*YES) VALUES(*YES *PROMPT) EXPR(*YES) +
002100040201                          PROMPT('Overwrite existing file')
002200040201
002300040201             /* PMTRQS PARAMETERS */
002400040201
002500040201             PARM       KWD(PASSWORD) TYPE(*CHAR) LEN(32) DFT(' ') +
002600040201                          EXPR(*YES) CASE(*MIXED) DSPINPUT(*NO) +
002700040201                          PMTCTL(*PMTRQS) PROMPT('Password for ZIP +
002800040201                          file')
002900040201
