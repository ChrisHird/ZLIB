000100040201             CMD        PROMPT('Display Zipped File')
000200040201
000300040201             /* REQUIRED PARAMETERS */
000400040201
000500040201             PARM       KWD(ZIPFILE) TYPE(*PNAME) LEN(128) MIN(1) +
000600040201                          EXPR(*YES) CASE(*MIXED) PROMPT('ZIP file +
000700040201                          name')
000800040201
