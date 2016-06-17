000100000000ZLIB DATA COMPRESSION LIBRARY
000200000000
000300000000zlib 1.2.3 is a general purpose data compression library.  All the code is
000400000000thread safe.  The data format used by the zlib library is described by RFCs
000500000000(Request for Comments) 1950 to 1952 in the files
000600000000http://www.ietf.org/rfc/rfc1950.txt (zlib format), rfc1951.txt (deflate format)
000700000000and rfc1952.txt (gzip format). These documents are also available in other
000800000000formats from ftp://ftp.uu.net/graphics/png/documents/zlib/zdoc-index.html
000900000000
001000000000All functions of the compression library are documented in the file zlib.h
001100000000(volunteer to write man pages welcome, contact zlib@gzip.org). A usage example
001200000000of the library is given in the file example.c which also tests that the library
001300000000is working correctly. Another example is given in the file minigzip.c. The
001400000000compression library itself is composed of all source files except example.c and
001500000000minigzip.c.
001600000000
001700000000To compile all files and run the test program, follow the instructions given at
001800000000the top of Makefile. In short "make test; make install" should work for most
001900000000machines. For Unix: "./configure; make test; make install". For MSDOS, use one
002000000000of the special makefiles such as Makefile.msc. For VMS, use make_vms.com.
002100000000
002200000000Questions about zlib should be sent to <zlib@gzip.org>, or to Gilles Vollant
002300000000<info@winimage.com> for the Windows DLL version. The zlib home page is
002400000000http://www.zlib.org or http://www.gzip.org/zlib/ Before reporting a problem,
002500000000please check this site to verify that you have the latest version of zlib;
002600000000otherwise get the latest version and check whether the problem still exists or
002700000000not.
002800000000
002900000000PLEASE read the zlib FAQ http://www.gzip.org/zlib/zlib_faq.html before asking
003000000000for help.
003100000000
003200000000Mark Nelson <markn@ieee.org> wrote an article about zlib for the Jan. 1997
003300000000issue of  Dr. Dobb's Journal; a copy of the article is available in
003400000000http://dogma.net/markn/articles/zlibtool/zlibtool.htm
003500000000
003600000000The changes made in version 1.2.3 are documented in the file ChangeLog.
003700000000
003800000000Unsupported third party contributions are provided in directory "contrib".
003900000000
004000000000A Java implementation of zlib is available in the Java Development Kit
004100000000http://java.sun.com/j2se/1.4.2/docs/api/java/util/zip/package-summary.html
004200000000See the zlib home page http://www.zlib.org for details.
004300000000
004400000000A Perl interface to zlib written by Paul Marquess <pmqs@cpan.org> is in the
004500000000CPAN (Comprehensive Perl Archive Network) sites
004600000000http://www.cpan.org/modules/by-module/Compress/
004700000000
004800000000A Python interface to zlib written by A.M. Kuchling <amk@amk.ca> is
004900000000available in Python 1.5 and later versions, see
005000000000http://www.python.org/doc/lib/module-zlib.html
005100000000
005200000000A zlib binding for TCL written by Andreas Kupries <a.kupries@westend.com> is
005300000000availlable at http://www.oche.de/~akupries/soft/trf/trf_zip.html
005400000000
005500000000An experimental package to read and write files in .zip format, written on top
005600000000of zlib by Gilles Vollant <info@winimage.com>, is available in the
005700000000contrib/minizip directory of zlib.
005800000000
005900000000
006000000000Notes for some targets:
006100000000
006200000000- For Windows DLL versions, please see win32/DLL_FAQ.txt
006300000000
006400000000- For 64-bit Irix, deflate.c must be compiled without any optimization. With
006500000000  -O, one libpng test fails. The test works in 32 bit mode (with the -n32
006600000000  compiler flag). The compiler bug has been reported to SGI.
006700000000
006800000000- zlib doesn't work with gcc 2.6.3 on a DEC 3000/300LX under OSF/1 2.1 it works
006900000000  when compiled with cc.
007000000000
007100000000- On Digital Unix 4.0D (formely OSF/1) on AlphaServer, the cc option -std1 is
007200000000  necessary to get gzprintf working correctly. This is done by configure.
007300000000
007400000000- zlib doesn't work on HP-UX 9.05 with some versions of /bin/cc. It works with
007500000000  other compilers. Use "make test" to check your compiler.
007600000000
007700000000- gzdopen is not supported on RISCOS, BEOS and by some Mac compilers.
007800000000
007900000000- For PalmOs, see http://palmzlib.sourceforge.net/
008000000000
008100000000- When building a shared, i.e. dynamic library on Mac OS X, the library must be
008200000000  installed before testing (do "make install" before "make test"), since the
008300000000  library location is specified in the library.
008400000000
008500000000
008600000000Acknowledgments:
008700000000
008800000000  The deflate format used by zlib was defined by Phil Katz. The deflate
008900000000  and zlib specifications were written by L. Peter Deutsch. Thanks to all the
009000000000  people who reported problems and suggested various improvements in zlib;
009100000000  they are too numerous to cite here.
009200000000
009300000000Copyright notice:
009400000000
009500000000 (C) 1995-2004 Jean-loup Gailly and Mark Adler
009600000000
009700000000  This software is provided 'as-is', without any express or implied
009800000000  warranty.  In no event will the authors be held liable for any damages
009900000000  arising from the use of this software.
010000000000
010100000000  Permission is granted to anyone to use this software for any purpose,
010200000000  including commercial applications, and to alter it and redistribute it
010300000000  freely, subject to the following restrictions:
010400000000
010500000000  1. The origin of this software must not be misrepresented; you must not
010600000000     claim that you wrote the original software. If you use this software
010700000000     in a product, an acknowledgment in the product documentation would be
010800000000     appreciated but is not required.
010900000000  2. Altered source versions must be plainly marked as such, and must not be
011000000000     misrepresented as being the original software.
011100000000  3. This notice may not be removed or altered from any source distribution.
011200000000
011300000000  Jean-loup Gailly        Mark Adler
011400000000  jloup@gzip.org          madler@alumni.caltech.edu
011500000000
011600000000If you use the zlib library in a product, we would appreciate *not*
011700000000receiving lengthy legal documents to sign. The sources are provided
011800000000for free but without warranty of any kind.  The library has been
011900000000entirely written by Jean-loup Gailly and Mark Adler; it does not
012000000000include third-party code.
012100000000
012200000000If you redistribute modified sources, we would appreciate that you include
012300000000in the file ChangeLog history information documenting your changes. Please
012400000000read the FAQ for more information on the distribution of modified source
012500000000versions.
