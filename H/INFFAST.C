000100000000/* inffast.h -- header to use inffast.c
000200000000 * Copyright (C) 1995-2003 Mark Adler
000300000000 * For conditions of distribution and use, see copyright notice in zlib.h
000400000000 */
000500000000
000600000000/* WARNING: this file should *not* be used by applications. It is
000700000000   part of the implementation of the compression library and is
000800000000   subject to change. Applications should only use zlib.h.
000900000000 */
001000000000
001100000000void inflate_fast OF((z_streamp strm, unsigned start));
