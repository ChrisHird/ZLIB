000100000000/* zip.c -- IO on .zip files using zlib
000200000000   Version 1.01e, February 12th, 2005
000300000000
000400000000   27 Dec 2004 Rolf Kalbermatter
000500000000   Modification to zipOpen2 to support globalComment retrieval.
000600000000
000700000000   Copyright (C) 1998-2005 Gilles Vollant
000800000000
000900000000   Read zip.h for more info
001000000000*/
001100000000
001200060219#ifdef AS400
001300060219#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
001400060219#endif
001500000000
001600000000#include <stdio.h>
001700000000#include <stdlib.h>
001800000000#include <string.h>
001900000000#include <time.h>
002000000000#include "zlib.h"
002100000000#include "zip.h"
002200000000
002300000000#ifdef STDC
002400000000#  include <stddef.h>
002500000000#  include <string.h>
002600000000#  include <stdlib.h>
002700000000#endif
002800000000#ifdef NO_ERRNO_H
002900000000    extern int errno;
003000000000#else
003100000000#   include <errno.h>
003200000000#endif
003300000000
003400000000
003500000000#ifndef local
003600000000#  define local static
003700000000#endif
003800000000/* compile with -Dlocal if your debugger can't find static symbols */
003900000000
004000000000#ifndef VERSIONMADEBY
004100000000# define VERSIONMADEBY   (0x0) /* platform depedent */
004200000000#endif
004300000000
004400000000#ifndef Z_BUFSIZE
004500000000#define Z_BUFSIZE (16384)
004600000000#endif
004700000000
004800000000#ifndef Z_MAXFILENAMEINZIP
004900000000#define Z_MAXFILENAMEINZIP (256)
005000000000#endif
005100000000
005200000000#ifndef ALLOC
005300000000# define ALLOC(size) (malloc(size))
005400000000#endif
005500000000#ifndef TRYFREE
005600000000# define TRYFREE(p) {if (p) free(p);}
005700000000#endif
005800000000
005900000000/*
006000000000#define SIZECENTRALDIRITEM (0x2e)
006100000000#define SIZEZIPLOCALHEADER (0x1e)
006200000000*/
006300000000
006400000000/* I've found an old Unix (a SunOS 4.1.3_U1) without all SEEK_* defined.... */
006500000000
006600000000#ifndef SEEK_CUR
006700000000#define SEEK_CUR    1
006800000000#endif
006900000000
007000000000#ifndef SEEK_END
007100000000#define SEEK_END    2
007200000000#endif
007300000000
007400000000#ifndef SEEK_SET
007500000000#define SEEK_SET    0
007600000000#endif
007700000000
007800000000#ifndef DEF_MEM_LEVEL
007900000000#if MAX_MEM_LEVEL >= 8
008000000000#  define DEF_MEM_LEVEL 8
008100000000#else
008200000000#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
008300000000#endif
008400000000#endif
008500000000const char zip_copyright[] =
008600000000   " zip 1.01 Copyright 1998-2004 Gilles Vollant - http://www.winimage.com/zLibDll";
008700000000
008800000000
008900000000#define SIZEDATA_INDATABLOCK (4096-(4*4))
009000000000
009100000000#define LOCALHEADERMAGIC    (0x04034b50)
009200000000#define CENTRALHEADERMAGIC  (0x02014b50)
009300000000#define ENDHEADERMAGIC      (0x06054b50)
009400000000
009500000000#define FLAG_LOCALHEADER_OFFSET (0x06)
009600000000#define CRC_LOCALHEADER_OFFSET  (0x0e)
009700000000
009800000000#define SIZECENTRALHEADER (0x2e) /* 46 */
009900000000
010000000000typedef struct linkedlist_datablock_internal_s
010100000000{
010200000000  struct linkedlist_datablock_internal_s* next_datablock;
010300000000  uLong  avail_in_this_block;
010400000000  uLong  filled_in_this_block;
010500000000  uLong  unused; /* for future use and alignement */
010600000000  unsigned char data[SIZEDATA_INDATABLOCK];
010700000000} linkedlist_datablock_internal;
010800000000
010900000000typedef struct linkedlist_data_s
011000000000{
011100000000    linkedlist_datablock_internal* first_block;
011200000000    linkedlist_datablock_internal* last_block;
011300000000} linkedlist_data;
011400000000
011500000000
011600000000typedef struct
011700000000{
011800000000    z_stream stream;            /* zLib stream structure for inflate */
011900000000    int  stream_initialised;    /* 1 is stream is initialised */
012000000000    uInt pos_in_buffered_data;  /* last written byte in buffered_data */
012100000000
012200000000    uLong pos_local_header;     /* offset of the local header of the file
012300000000                                     currenty writing */
012400000000    char* central_header;       /* central header data for the current file */
012500000000    uLong size_centralheader;   /* size of the central header for cur file */
012600000000    uLong flag;                 /* flag of the file currently writing */
012700000000
012800000000    int  method;                /* compression method of file currenty wr.*/
012900000000    int  raw;                   /* 1 for directly writing raw data */
013000000000    Byte buffered_data[Z_BUFSIZE];/* buffer contain compressed data to be writ*/
013100000000    uLong dosDate;
013200000000    uLong crc32;
013300000000    int  encrypt;
013400000000#ifndef NOCRYPT
013500000000    unsigned long keys[3];     /* keys defining the pseudo-random sequence */
013600000000    const unsigned long* pcrc_32_tab;
013700000000    int crypt_header_size;
013800000000#endif
013900000000} curfile_info;
014000000000
014100000000typedef struct
014200000000{
014300000000    zlib_filefunc_def z_filefunc;
014400000000    voidpf filestream;        /* io structore of the zipfile */
014500000000    linkedlist_data central_dir;/* datablock with central dir in construction*/
014600000000    int  in_opened_file_inzip;  /* 1 if a file in the zip is currently writ.*/
014700000000    curfile_info ci;            /* info on the file curretly writing */
014800000000
014900000000    uLong begin_pos;            /* position of the beginning of the zipfile */
015000000000    uLong add_position_when_writting_offset;
015100000000    uLong number_entry;
015200000000#ifndef NO_ADDFILEINEXISTINGZIP
015300000000    char *globalcomment;
015400000000#endif
015500000000} zip_internal;
015600000000
015700000000
015800000000
015900000000#ifndef NOCRYPT
016000000000#define INCLUDECRYPTINGCODE_IFCRYPTALLOWED
016100000000#include "crypt.h"
016200000000#endif
016300000000
016400000000local linkedlist_datablock_internal* allocate_new_datablock()
016500000000{
016600000000    linkedlist_datablock_internal* ldi;
016700000000    ldi = (linkedlist_datablock_internal*)
016800000000                 ALLOC(sizeof(linkedlist_datablock_internal));
016900000000    if (ldi!=NULL)
017000000000    {
017100000000        ldi->next_datablock = NULL ;
017200000000        ldi->filled_in_this_block = 0 ;
017300000000        ldi->avail_in_this_block = SIZEDATA_INDATABLOCK ;
017400000000    }
017500000000    return ldi;
017600000000}
017700000000
017800000000local void free_datablock(ldi)
017900000000    linkedlist_datablock_internal* ldi;
018000000000{
018100000000    while (ldi!=NULL)
018200000000    {
018300000000        linkedlist_datablock_internal* ldinext = ldi->next_datablock;
018400000000        TRYFREE(ldi);
018500000000        ldi = ldinext;
018600000000    }
018700000000}
018800000000
018900000000local void init_linkedlist(ll)
019000000000    linkedlist_data* ll;
019100000000{
019200000000    ll->first_block = ll->last_block = NULL;
019300000000}
019400000000
019500000000local void free_linkedlist(ll)
019600000000    linkedlist_data* ll;
019700000000{
019800000000    free_datablock(ll->first_block);
019900000000    ll->first_block = ll->last_block = NULL;
020000000000}
020100000000
020200000000
020300000000local int add_data_in_datablock(ll,buf,len)
020400000000    linkedlist_data* ll;
020500000000    const void* buf;
020600000000    uLong len;
020700000000{
020800000000    linkedlist_datablock_internal* ldi;
020900000000    const unsigned char* from_copy;
021000000000
021100000000    if (ll==NULL)
021200000000        return ZIP_INTERNALERROR;
021300000000
021400000000    if (ll->last_block == NULL)
021500000000    {
021600000000        ll->first_block = ll->last_block = allocate_new_datablock();
021700000000        if (ll->first_block == NULL)
021800000000            return ZIP_INTERNALERROR;
021900000000    }
022000000000
022100000000    ldi = ll->last_block;
022200000000    from_copy = (unsigned char*)buf;
022300000000
022400000000    while (len>0)
022500000000    {
022600000000        uInt copy_this;
022700000000        uInt i;
022800000000        unsigned char* to_copy;
022900000000
023000000000        if (ldi->avail_in_this_block==0)
023100000000        {
023200000000            ldi->next_datablock = allocate_new_datablock();
023300000000            if (ldi->next_datablock == NULL)
023400000000                return ZIP_INTERNALERROR;
023500000000            ldi = ldi->next_datablock ;
023600000000            ll->last_block = ldi;
023700000000        }
023800000000
023900000000        if (ldi->avail_in_this_block < len)
024000000000            copy_this = (uInt)ldi->avail_in_this_block;
024100000000        else
024200000000            copy_this = (uInt)len;
024300000000
024400000000        to_copy = &(ldi->data[ldi->filled_in_this_block]);
024500000000
024600000000        for (i=0;i<copy_this;i++)
024700000000            *(to_copy+i)=*(from_copy+i);
024800000000
024900000000        ldi->filled_in_this_block += copy_this;
025000000000        ldi->avail_in_this_block -= copy_this;
025100000000        from_copy += copy_this ;
025200000000        len -= copy_this;
025300000000    }
025400000000    return ZIP_OK;
025500000000}
025600000000
025700000000
025800000000
025900000000/****************************************************************************/
026000000000
026100000000#ifndef NO_ADDFILEINEXISTINGZIP
026200000000/* ===========================================================================
026300000000   Inputs a long in LSB order to the given file
026400000000   nbByte == 1, 2 or 4 (byte, short or long)
026500000000*/
026600000000
026700000000local int ziplocal_putValue OF((const zlib_filefunc_def* pzlib_filefunc_def,
026800000000                                voidpf filestream, uLong x, int nbByte));
026900000000local int ziplocal_putValue (pzlib_filefunc_def, filestream, x, nbByte)
027000000000    const zlib_filefunc_def* pzlib_filefunc_def;
027100000000    voidpf filestream;
027200000000    uLong x;
027300000000    int nbByte;
027400000000{
027500000000    unsigned char buf[4];
027600000000    int n;
027700000000    for (n = 0; n < nbByte; n++)
027800000000    {
027900000000        buf[n] = (unsigned char)(x & 0xff);
028000000000        x >>= 8;
028100000000    }
028200000000    if (x != 0)
028300000000      {     /* data overflow - hack for ZIP64 (X Roche) */
028400000000      for (n = 0; n < nbByte; n++)
028500000000        {
028600000000          buf[n] = 0xff;
028700000000        }
028800000000      }
028900000000
029000000000    if (ZWRITE(*pzlib_filefunc_def,filestream,buf,nbByte)!=(uLong)nbByte)
029100000000        return ZIP_ERRNO;
029200000000    else
029300000000        return ZIP_OK;
029400000000}
029500000000
029600000000local void ziplocal_putValue_inmemory OF((void* dest, uLong x, int nbByte));
029700000000local void ziplocal_putValue_inmemory (dest, x, nbByte)
029800000000    void* dest;
029900000000    uLong x;
030000000000    int nbByte;
030100000000{
030200000000    unsigned char* buf=(unsigned char*)dest;
030300000000    int n;
030400000000    for (n = 0; n < nbByte; n++) {
030500000000        buf[n] = (unsigned char)(x & 0xff);
030600000000        x >>= 8;
030700000000    }
030800000000
030900000000    if (x != 0)
031000000000    {     /* data overflow - hack for ZIP64 */
031100000000       for (n = 0; n < nbByte; n++)
031200000000       {
031300000000          buf[n] = 0xff;
031400000000       }
031500000000    }
031600000000}
031700000000
031800000000/****************************************************************************/
031900000000
032000000000
032100000000local uLong ziplocal_TmzDateToDosDate(ptm,dosDate)
032200000000    const tm_zip* ptm;
032300000000    uLong dosDate;
032400000000{
032500000000    uLong year = (uLong)ptm->tm_year;
032600000000    if (year>1980)
032700000000        year-=1980;
032800000000    else if (year>80)
032900000000        year-=80;
033000000000    return
033100000000      (uLong) (((ptm->tm_mday) + (32 * (ptm->tm_mon+1)) + (512 * year)) << 16) |
033200000000        ((ptm->tm_sec/2) + (32* ptm->tm_min) + (2048 * (uLong)ptm->tm_hour));
033300000000}
033400000000
033500000000
033600000000/****************************************************************************/
033700000000
033800000000local int ziplocal_getByte OF((
033900000000    const zlib_filefunc_def* pzlib_filefunc_def,
034000000000    voidpf filestream,
034100000000    int *pi));
034200000000
034300000000local int ziplocal_getByte(pzlib_filefunc_def,filestream,pi)
034400000000    const zlib_filefunc_def* pzlib_filefunc_def;
034500000000    voidpf filestream;
034600000000    int *pi;
034700000000{
034800000000    unsigned char c;
034900000000    int err = (int)ZREAD(*pzlib_filefunc_def,filestream,&c,1);
035000000000    if (err==1)
035100000000    {
035200000000        *pi = (int)c;
035300000000        return ZIP_OK;
035400000000    }
035500000000    else
035600000000    {
035700000000        if (ZERROR(*pzlib_filefunc_def,filestream))
035800000000            return ZIP_ERRNO;
035900000000        else
036000000000            return ZIP_EOF;
036100000000    }
036200000000}
036300000000
036400000000
036500000000/* ===========================================================================
036600000000   Reads a long in LSB order from the given gz_stream. Sets
036700000000*/
036800000000local int ziplocal_getShort OF((
036900000000    const zlib_filefunc_def* pzlib_filefunc_def,
037000000000    voidpf filestream,
037100000000    uLong *pX));
037200000000
037300000000local int ziplocal_getShort (pzlib_filefunc_def,filestream,pX)
037400000000    const zlib_filefunc_def* pzlib_filefunc_def;
037500000000    voidpf filestream;
037600000000    uLong *pX;
037700000000{
037800000000    uLong x ;
037900000000    int i;
038000000000    int err;
038100000000
038200000000    err = ziplocal_getByte(pzlib_filefunc_def,filestream,&i);
038300000000    x = (uLong)i;
038400000000
038500000000    if (err==ZIP_OK)
038600000000        err = ziplocal_getByte(pzlib_filefunc_def,filestream,&i);
038700000000    x += ((uLong)i)<<8;
038800000000
038900000000    if (err==ZIP_OK)
039000000000        *pX = x;
039100000000    else
039200000000        *pX = 0;
039300000000    return err;
039400000000}
039500000000
039600000000local int ziplocal_getLong OF((
039700000000    const zlib_filefunc_def* pzlib_filefunc_def,
039800000000    voidpf filestream,
039900000000    uLong *pX));
040000000000
040100000000local int ziplocal_getLong (pzlib_filefunc_def,filestream,pX)
040200000000    const zlib_filefunc_def* pzlib_filefunc_def;
040300000000    voidpf filestream;
040400000000    uLong *pX;
040500000000{
040600000000    uLong x ;
040700000000    int i;
040800000000    int err;
040900000000
041000000000    err = ziplocal_getByte(pzlib_filefunc_def,filestream,&i);
041100000000    x = (uLong)i;
041200000000
041300000000    if (err==ZIP_OK)
041400000000        err = ziplocal_getByte(pzlib_filefunc_def,filestream,&i);
041500000000    x += ((uLong)i)<<8;
041600000000
041700000000    if (err==ZIP_OK)
041800000000        err = ziplocal_getByte(pzlib_filefunc_def,filestream,&i);
041900000000    x += ((uLong)i)<<16;
042000000000
042100000000    if (err==ZIP_OK)
042200000000        err = ziplocal_getByte(pzlib_filefunc_def,filestream,&i);
042300000000    x += ((uLong)i)<<24;
042400000000
042500000000    if (err==ZIP_OK)
042600000000        *pX = x;
042700000000    else
042800000000        *pX = 0;
042900000000    return err;
043000000000}
043100000000
043200000000#ifndef BUFREADCOMMENT
043300000000#define BUFREADCOMMENT (0x400)
043400000000#endif
043500000000/*
043600000000  Locate the Central directory of a zipfile (at the end, just before
043700000000    the global comment)
043800000000*/
043900000000local uLong ziplocal_SearchCentralDir OF((
044000000000    const zlib_filefunc_def* pzlib_filefunc_def,
044100000000    voidpf filestream));
044200000000
044300000000local uLong ziplocal_SearchCentralDir(pzlib_filefunc_def,filestream)
044400000000    const zlib_filefunc_def* pzlib_filefunc_def;
044500000000    voidpf filestream;
044600000000{
044700000000    unsigned char* buf;
044800000000    uLong uSizeFile;
044900000000    uLong uBackRead;
045000000000    uLong uMaxBack=0xffff; /* maximum size of global comment */
045100000000    uLong uPosFound=0;
045200000000
045300000000    if (ZSEEK(*pzlib_filefunc_def,filestream,0,ZLIB_FILEFUNC_SEEK_END) != 0)
045400000000        return 0;
045500000000
045600000000
045700000000    uSizeFile = ZTELL(*pzlib_filefunc_def,filestream);
045800000000
045900000000    if (uMaxBack>uSizeFile)
046000000000        uMaxBack = uSizeFile;
046100000000
046200000000    buf = (unsigned char*)ALLOC(BUFREADCOMMENT+4);
046300000000    if (buf==NULL)
046400000000        return 0;
046500000000
046600000000    uBackRead = 4;
046700000000    while (uBackRead<uMaxBack)
046800000000    {
046900000000        uLong uReadSize,uReadPos ;
047000000000        int i;
047100000000        if (uBackRead+BUFREADCOMMENT>uMaxBack)
047200000000            uBackRead = uMaxBack;
047300000000        else
047400000000            uBackRead+=BUFREADCOMMENT;
047500000000        uReadPos = uSizeFile-uBackRead ;
047600000000
047700000000        uReadSize = ((BUFREADCOMMENT+4) < (uSizeFile-uReadPos)) ?
047800000000                     (BUFREADCOMMENT+4) : (uSizeFile-uReadPos);
047900000000        if (ZSEEK(*pzlib_filefunc_def,filestream,uReadPos,ZLIB_FILEFUNC_SEEK_SET)!=0)
048000000000            break;
048100000000
048200000000        if (ZREAD(*pzlib_filefunc_def,filestream,buf,uReadSize)!=uReadSize)
048300000000            break;
048400000000
048500000000        for (i=(int)uReadSize-3; (i--)>0;)
048600000000            if (((*(buf+i))==0x50) && ((*(buf+i+1))==0x4b) &&
048700000000                ((*(buf+i+2))==0x05) && ((*(buf+i+3))==0x06))
048800000000            {
048900000000                uPosFound = uReadPos+i;
049000000000                break;
049100000000            }
049200000000
049300000000        if (uPosFound!=0)
049400000000            break;
049500000000    }
049600000000    TRYFREE(buf);
049700000000    return uPosFound;
049800000000}
049900000000#endif /* !NO_ADDFILEINEXISTINGZIP*/
050000000000
050100000000/************************************************************/
050200000000extern zipFile ZEXPORT zipOpen2 (pathname, append, globalcomment, pzlib_filefunc_def)
050300000000    const char *pathname;
050400000000    int append;
050500000000    zipcharpc* globalcomment;
050600000000    zlib_filefunc_def* pzlib_filefunc_def;
050700000000{
050800000000    zip_internal ziinit;
050900000000    zip_internal* zi;
051000000000    int err=ZIP_OK;
051100000000
051200000000
051300000000    if (pzlib_filefunc_def==NULL)
051400000000        fill_fopen_filefunc(&ziinit.z_filefunc);
051500000000    else
051600000000        ziinit.z_filefunc = *pzlib_filefunc_def;
051700000000
051800000000    ziinit.filestream = (*(ziinit.z_filefunc.zopen_file))
051900000000                 (ziinit.z_filefunc.opaque,
052000000000                  pathname,
052100000000                  (append == APPEND_STATUS_CREATE) ?
052200000000                  (ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_CREATE) :
052300000000                    (ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_EXISTING));
052400000000
052500000000    if (ziinit.filestream == NULL)
052600000000        return NULL;
052700000000    ziinit.begin_pos = ZTELL(ziinit.z_filefunc,ziinit.filestream);
052800000000    ziinit.in_opened_file_inzip = 0;
052900000000    ziinit.ci.stream_initialised = 0;
053000000000    ziinit.number_entry = 0;
053100000000    ziinit.add_position_when_writting_offset = 0;
053200000000    init_linkedlist(&(ziinit.central_dir));
053300000000
053400000000
053500000000    zi = (zip_internal*)ALLOC(sizeof(zip_internal));
053600000000    if (zi==NULL)
053700000000    {
053800000000        ZCLOSE(ziinit.z_filefunc,ziinit.filestream);
053900000000        return NULL;
054000000000    }
054100000000
054200000000    /* now we add file in a zipfile */
054300000000#    ifndef NO_ADDFILEINEXISTINGZIP
054400000000    ziinit.globalcomment = NULL;
054500000000    if (append == APPEND_STATUS_ADDINZIP)
054600000000    {
054700000000        uLong byte_before_the_zipfile;/* byte before the zipfile, (>0 for sfx)*/
054800000000
054900000000        uLong size_central_dir;     /* size of the central directory  */
055000000000        uLong offset_central_dir;   /* offset of start of central directory */
055100000000        uLong central_pos,uL;
055200000000
055300000000        uLong number_disk;          /* number of the current dist, used for
055400000000                                    spaning ZIP, unsupported, always 0*/
055500000000        uLong number_disk_with_CD;  /* number the the disk with central dir, used
055600000000                                    for spaning ZIP, unsupported, always 0*/
055700000000        uLong number_entry;
055800000000        uLong number_entry_CD;      /* total number of entries in
055900000000                                    the central dir
056000000000                                    (same than number_entry on nospan) */
056100000000        uLong size_comment;
056200000000
056300000000        central_pos = ziplocal_SearchCentralDir(&ziinit.z_filefunc,ziinit.filestream);
056400000000        if (central_pos==0)
056500000000            err=ZIP_ERRNO;
056600000000
056700000000        if (ZSEEK(ziinit.z_filefunc, ziinit.filestream,
056800000000                                        central_pos,ZLIB_FILEFUNC_SEEK_SET)!=0)
056900000000            err=ZIP_ERRNO;
057000000000
057100000000        /* the signature, already checked */
057200000000        if (ziplocal_getLong(&ziinit.z_filefunc, ziinit.filestream,&uL)!=ZIP_OK)
057300000000            err=ZIP_ERRNO;
057400000000
057500000000        /* number of this disk */
057600000000        if (ziplocal_getShort(&ziinit.z_filefunc, ziinit.filestream,&number_disk)!=ZIP_OK)
057700000000            err=ZIP_ERRNO;
057800000000
057900000000        /* number of the disk with the start of the central directory */
058000000000        if (ziplocal_getShort(&ziinit.z_filefunc, ziinit.filestream,&number_disk_with_CD)!=ZIP_OK)
058100000000            err=ZIP_ERRNO;
058200000000
058300000000        /* total number of entries in the central dir on this disk */
058400000000        if (ziplocal_getShort(&ziinit.z_filefunc, ziinit.filestream,&number_entry)!=ZIP_OK)
058500000000            err=ZIP_ERRNO;
058600000000
058700000000        /* total number of entries in the central dir */
058800000000        if (ziplocal_getShort(&ziinit.z_filefunc, ziinit.filestream,&number_entry_CD)!=ZIP_OK)
058900000000            err=ZIP_ERRNO;
059000000000
059100000000        if ((number_entry_CD!=number_entry) ||
059200000000            (number_disk_with_CD!=0) ||
059300000000            (number_disk!=0))
059400000000            err=ZIP_BADZIPFILE;
059500000000
059600000000        /* size of the central directory */
059700000000        if (ziplocal_getLong(&ziinit.z_filefunc, ziinit.filestream,&size_central_dir)!=ZIP_OK)
059800000000            err=ZIP_ERRNO;
059900000000
060000000000        /* offset of start of central directory with respect to the
060100000000            starting disk number */
060200000000        if (ziplocal_getLong(&ziinit.z_filefunc, ziinit.filestream,&offset_central_dir)!=ZIP_OK)
060300000000            err=ZIP_ERRNO;
060400000000
060500000000        /* zipfile global comment length */
060600000000        if (ziplocal_getShort(&ziinit.z_filefunc, ziinit.filestream,&size_comment)!=ZIP_OK)
060700000000            err=ZIP_ERRNO;
060800000000
060900000000        if ((central_pos<offset_central_dir+size_central_dir) &&
061000000000            (err==ZIP_OK))
061100000000            err=ZIP_BADZIPFILE;
061200000000
061300000000        if (err!=ZIP_OK)
061400000000        {
061500000000            ZCLOSE(ziinit.z_filefunc, ziinit.filestream);
061600000000            return NULL;
061700000000        }
061800000000
061900000000        if (size_comment>0)
062000000000        {
062100000000            ziinit.globalcomment = ALLOC(size_comment+1);
062200000000            if (ziinit.globalcomment)
062300000000            {
062400000000               size_comment = ZREAD(ziinit.z_filefunc, ziinit.filestream,ziinit.globalcomment,size_comment);
062500000000               ziinit.globalcomment[size_comment]=0;
062600000000            }
062700000000        }
062800000000
062900000000        byte_before_the_zipfile = central_pos -
063000000000                                (offset_central_dir+size_central_dir);
063100000000        ziinit.add_position_when_writting_offset = byte_before_the_zipfile;
063200000000
063300000000        {
063400000000            uLong size_central_dir_to_read = size_central_dir;
063500000000            size_t buf_size = SIZEDATA_INDATABLOCK;
063600000000            void* buf_read = (void*)ALLOC(buf_size);
063700000000            if (ZSEEK(ziinit.z_filefunc, ziinit.filestream,
063800000000                  offset_central_dir + byte_before_the_zipfile,
063900000000                  ZLIB_FILEFUNC_SEEK_SET) != 0)
064000000000                  err=ZIP_ERRNO;
064100000000
064200000000            while ((size_central_dir_to_read>0) && (err==ZIP_OK))
064300000000            {
064400000000                uLong read_this = SIZEDATA_INDATABLOCK;
064500000000                if (read_this > size_central_dir_to_read)
064600000000                    read_this = size_central_dir_to_read;
064700000000                if (ZREAD(ziinit.z_filefunc, ziinit.filestream,buf_read,read_this) != read_this)
064800000000                    err=ZIP_ERRNO;
064900000000
065000000000                if (err==ZIP_OK)
065100000000                    err = add_data_in_datablock(&ziinit.central_dir,buf_read,
065200000000                                                (uLong)read_this);
065300000000                size_central_dir_to_read-=read_this;
065400000000            }
065500000000            TRYFREE(buf_read);
065600000000        }
065700000000        ziinit.begin_pos = byte_before_the_zipfile;
065800000000        ziinit.number_entry = number_entry_CD;
065900000000
066000000000        if (ZSEEK(ziinit.z_filefunc, ziinit.filestream,
066100000000                  offset_central_dir+byte_before_the_zipfile,ZLIB_FILEFUNC_SEEK_SET)!=0)
066200000000            err=ZIP_ERRNO;
066300000000    }
066400000000
066500000000    if (globalcomment)
066600000000    {
066700000000      *globalcomment = ziinit.globalcomment;
066800000000    }
066900000000#    endif /* !NO_ADDFILEINEXISTINGZIP*/
067000000000
067100000000    if (err != ZIP_OK)
067200000000    {
067300000000#    ifndef NO_ADDFILEINEXISTINGZIP
067400000000        TRYFREE(ziinit.globalcomment);
067500000000#    endif /* !NO_ADDFILEINEXISTINGZIP*/
067600000000        TRYFREE(zi);
067700000000        return NULL;
067800000000    }
067900000000    else
068000000000    {
068100000000        *zi = ziinit;
068200000000        return (zipFile)zi;
068300000000    }
068400000000}
068500000000
068600000000extern zipFile ZEXPORT zipOpen (pathname, append)
068700000000    const char *pathname;
068800000000    int append;
068900000000{
069000000000    return zipOpen2(pathname,append,NULL,NULL);
069100000000}
069200000000
069300000000extern int ZEXPORT zipOpenNewFileInZip3 (file, filename, zipfi,
069400000000                                         extrafield_local, size_extrafield_local,
069500000000                                         extrafield_global, size_extrafield_global,
069600000000                                         comment, method, level, raw,
069700000000                                         windowBits, memLevel, strategy,
069800000000                                         password, crcForCrypting)
069900000000    zipFile file;
070000000000    const char* filename;
070100000000    const zip_fileinfo* zipfi;
070200000000    const void* extrafield_local;
070300000000    uInt size_extrafield_local;
070400000000    const void* extrafield_global;
070500000000    uInt size_extrafield_global;
070600000000    const char* comment;
070700000000    int method;
070800000000    int level;
070900000000    int raw;
071000000000    int windowBits;
071100000000    int memLevel;
071200000000    int strategy;
071300000000    const char* password;
071400000000    uLong crcForCrypting;
071500000000{
071600000000    zip_internal* zi;
071700000000    uInt size_filename;
071800000000    uInt size_comment;
071900000000    uInt i;
072000000000    int err = ZIP_OK;
072100000000
072200000000#    ifdef NOCRYPT
072300000000    if (password != NULL)
072400000000        return ZIP_PARAMERROR;
072500000000#    endif
072600000000
072700000000    if (file == NULL)
072800000000        return ZIP_PARAMERROR;
072900000000    if ((method!=0) && (method!=Z_DEFLATED))
073000000000        return ZIP_PARAMERROR;
073100000000
073200000000    zi = (zip_internal*)file;
073300000000
073400000000    if (zi->in_opened_file_inzip == 1)
073500000000    {
073600000000        err = zipCloseFileInZip (file);
073700000000        if (err != ZIP_OK)
073800000000            return err;
073900000000    }
074000000000
074100000000
074200000000    if (filename==NULL)
074300000000        filename="-";
074400000000
074500000000    if (comment==NULL)
074600000000        size_comment = 0;
074700000000    else
074800000000        size_comment = (uInt)strlen(comment);
074900000000
075000000000    size_filename = (uInt)strlen(filename);
075100000000
075200000000    if (zipfi == NULL)
075300000000        zi->ci.dosDate = 0;
075400000000    else
075500000000    {
075600000000        if (zipfi->dosDate != 0)
075700000000            zi->ci.dosDate = zipfi->dosDate;
075800000000        else zi->ci.dosDate = ziplocal_TmzDateToDosDate(&zipfi->tmz_date,zipfi->dosDate);
075900000000    }
076000000000
076100000000    zi->ci.flag = 0;
076200000000    if ((level==8) || (level==9))
076300000000      zi->ci.flag |= 2;
076400000000    if ((level==2))
076500000000      zi->ci.flag |= 4;
076600000000    if ((level==1))
076700000000      zi->ci.flag |= 6;
076800000000    if (password != NULL)
076900000000      zi->ci.flag |= 1;
077000000000
077100000000    zi->ci.crc32 = 0;
077200000000    zi->ci.method = method;
077300000000    zi->ci.encrypt = 0;
077400000000    zi->ci.stream_initialised = 0;
077500000000    zi->ci.pos_in_buffered_data = 0;
077600000000    zi->ci.raw = raw;
077700000000    zi->ci.pos_local_header = ZTELL(zi->z_filefunc,zi->filestream) ;
077800000000    zi->ci.size_centralheader = SIZECENTRALHEADER + size_filename +
077900000000                                      size_extrafield_global + size_comment;
078000000000    zi->ci.central_header = (char*)ALLOC((uInt)zi->ci.size_centralheader);
078100000000
078200000000    ziplocal_putValue_inmemory(zi->ci.central_header,(uLong)CENTRALHEADERMAGIC,4);
078300000000    /* version info */
078400000000    ziplocal_putValue_inmemory(zi->ci.central_header+4,(uLong)VERSIONMADEBY,2);
078500000000    ziplocal_putValue_inmemory(zi->ci.central_header+6,(uLong)20,2);
078600000000    ziplocal_putValue_inmemory(zi->ci.central_header+8,(uLong)zi->ci.flag,2);
078700000000    ziplocal_putValue_inmemory(zi->ci.central_header+10,(uLong)zi->ci.method,2);
078800000000    ziplocal_putValue_inmemory(zi->ci.central_header+12,(uLong)zi->ci.dosDate,4);
078900000000    ziplocal_putValue_inmemory(zi->ci.central_header+16,(uLong)0,4); /*crc*/
079000000000    ziplocal_putValue_inmemory(zi->ci.central_header+20,(uLong)0,4); /*compr size*/
079100000000    ziplocal_putValue_inmemory(zi->ci.central_header+24,(uLong)0,4); /*uncompr size*/
079200000000    ziplocal_putValue_inmemory(zi->ci.central_header+28,(uLong)size_filename,2);
079300000000    ziplocal_putValue_inmemory(zi->ci.central_header+30,(uLong)size_extrafield_global,2);
079400000000    ziplocal_putValue_inmemory(zi->ci.central_header+32,(uLong)size_comment,2);
079500000000    ziplocal_putValue_inmemory(zi->ci.central_header+34,(uLong)0,2); /*disk nm start*/
079600000000
079700000000    if (zipfi==NULL)
079800000000        ziplocal_putValue_inmemory(zi->ci.central_header+36,(uLong)0,2);
079900000000    else
080000000000        ziplocal_putValue_inmemory(zi->ci.central_header+36,(uLong)zipfi->internal_fa,2);
080100000000
080200000000    if (zipfi==NULL)
080300000000        ziplocal_putValue_inmemory(zi->ci.central_header+38,(uLong)0,4);
080400000000    else
080500000000        ziplocal_putValue_inmemory(zi->ci.central_header+38,(uLong)zipfi->external_fa,4);
080600000000
080700000000    ziplocal_putValue_inmemory(zi->ci.central_header+42,(uLong)zi->ci.pos_local_header- zi->add_position_when_writting_offset,4);
080800000000
080900000000    for (i=0;i<size_filename;i++)
081000000000        *(zi->ci.central_header+SIZECENTRALHEADER+i) = *(filename+i);
081100000000
081200000000    for (i=0;i<size_extrafield_global;i++)
081300000000        *(zi->ci.central_header+SIZECENTRALHEADER+size_filename+i) =
081400000000              *(((const char*)extrafield_global)+i);
081500000000
081600000000    for (i=0;i<size_comment;i++)
081700000000        *(zi->ci.central_header+SIZECENTRALHEADER+size_filename+
081800000000              size_extrafield_global+i) = *(comment+i);
081900000000    if (zi->ci.central_header == NULL)
082000000000        return ZIP_INTERNALERROR;
082100000000
082200000000    /* write the local header */
082300000000    err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)LOCALHEADERMAGIC,4);
082400000000
082500000000    if (err==ZIP_OK)
082600000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)20,2);/* version needed to extract */
082700000000    if (err==ZIP_OK)
082800000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)zi->ci.flag,2);
082900000000
083000000000    if (err==ZIP_OK)
083100000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)zi->ci.method,2);
083200000000
083300000000    if (err==ZIP_OK)
083400000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)zi->ci.dosDate,4);
083500000000
083600000000    if (err==ZIP_OK)
083700000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)0,4); /* crc 32, unknown */
083800000000    if (err==ZIP_OK)
083900000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)0,4); /* compressed size, unknown */
084000000000    if (err==ZIP_OK)
084100000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)0,4); /* uncompressed size, unknown */
084200000000
084300000000    if (err==ZIP_OK)
084400000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)size_filename,2);
084500000000
084600000000    if (err==ZIP_OK)
084700000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)size_extrafield_local,2);
084800000000
084900000000    if ((err==ZIP_OK) && (size_filename>0))
085000000000        if (ZWRITE(zi->z_filefunc,zi->filestream,filename,size_filename)!=size_filename)
085100000000                err = ZIP_ERRNO;
085200000000
085300000000    if ((err==ZIP_OK) && (size_extrafield_local>0))
085400000000        if (ZWRITE(zi->z_filefunc,zi->filestream,extrafield_local,size_extrafield_local)
085500000000                                                                           !=size_extrafield_local)
085600000000                err = ZIP_ERRNO;
085700000000
085800000000    zi->ci.stream.avail_in = (uInt)0;
085900000000    zi->ci.stream.avail_out = (uInt)Z_BUFSIZE;
086000000000    zi->ci.stream.next_out = zi->ci.buffered_data;
086100000000    zi->ci.stream.total_in = 0;
086200000000    zi->ci.stream.total_out = 0;
086300000000
086400000000    if ((err==ZIP_OK) && (zi->ci.method == Z_DEFLATED) && (!zi->ci.raw))
086500000000    {
086600000000        zi->ci.stream.zalloc = (alloc_func)0;
086700000000        zi->ci.stream.zfree = (free_func)0;
086800000000        zi->ci.stream.opaque = (voidpf)0;
086900000000
087000000000        if (windowBits>0)
087100000000            windowBits = -windowBits;
087200000000
087300000000        err = deflateInit2(&zi->ci.stream, level,
087400000000               Z_DEFLATED, windowBits, memLevel, strategy);
087500000000
087600000000        if (err==Z_OK)
087700000000            zi->ci.stream_initialised = 1;
087800000000    }
087900000000#    ifndef NOCRYPT
088000000000    zi->ci.crypt_header_size = 0;
088100000000    if ((err==Z_OK) && (password != NULL))
088200000000    {
088300000000        unsigned char bufHead[RAND_HEAD_LEN];
088400000000        unsigned int sizeHead;
088500000000        zi->ci.encrypt = 1;
088600000000        zi->ci.pcrc_32_tab = get_crc_table();
088700000000        /*init_keys(password,zi->ci.keys,zi->ci.pcrc_32_tab);*/
088800000000
088900000000        sizeHead=crypthead(password,bufHead,RAND_HEAD_LEN,zi->ci.keys,zi->ci.pcrc_32_tab,crcForCrypting);
089000000000        zi->ci.crypt_header_size = sizeHead;
089100000000
089200000000        if (ZWRITE(zi->z_filefunc,zi->filestream,bufHead,sizeHead) != sizeHead)
089300000000                err = ZIP_ERRNO;
089400000000    }
089500000000#    endif
089600000000
089700000000    if (err==Z_OK)
089800000000        zi->in_opened_file_inzip = 1;
089900000000    return err;
090000000000}
090100000000
090200000000extern int ZEXPORT zipOpenNewFileInZip2(file, filename, zipfi,
090300000000                                        extrafield_local, size_extrafield_local,
090400000000                                        extrafield_global, size_extrafield_global,
090500000000                                        comment, method, level, raw)
090600000000    zipFile file;
090700000000    const char* filename;
090800000000    const zip_fileinfo* zipfi;
090900000000    const void* extrafield_local;
091000000000    uInt size_extrafield_local;
091100000000    const void* extrafield_global;
091200000000    uInt size_extrafield_global;
091300000000    const char* comment;
091400000000    int method;
091500000000    int level;
091600000000    int raw;
091700000000{
091800000000    return zipOpenNewFileInZip3 (file, filename, zipfi,
091900000000                                 extrafield_local, size_extrafield_local,
092000000000                                 extrafield_global, size_extrafield_global,
092100000000                                 comment, method, level, raw,
092200000000                                 -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
092300000000                                 NULL, 0);
092400000000}
092500000000
092600000000extern int ZEXPORT zipOpenNewFileInZip (file, filename, zipfi,
092700000000                                        extrafield_local, size_extrafield_local,
092800000000                                        extrafield_global, size_extrafield_global,
092900000000                                        comment, method, level)
093000000000    zipFile file;
093100000000    const char* filename;
093200000000    const zip_fileinfo* zipfi;
093300000000    const void* extrafield_local;
093400000000    uInt size_extrafield_local;
093500000000    const void* extrafield_global;
093600000000    uInt size_extrafield_global;
093700000000    const char* comment;
093800000000    int method;
093900000000    int level;
094000000000{
094100000000    return zipOpenNewFileInZip2 (file, filename, zipfi,
094200000000                                 extrafield_local, size_extrafield_local,
094300000000                                 extrafield_global, size_extrafield_global,
094400000000                                 comment, method, level, 0);
094500000000}
094600000000
094700000000local int zipFlushWriteBuffer(zi)
094800000000  zip_internal* zi;
094900000000{
095000000000    int err=ZIP_OK;
095100000000
095200000000    if (zi->ci.encrypt != 0)
095300000000    {
095400000000#ifndef NOCRYPT
095500000000        uInt i;
095600000000        int t;
095700000000        for (i=0;i<zi->ci.pos_in_buffered_data;i++)
095800000000            zi->ci.buffered_data[i] = zencode(zi->ci.keys, zi->ci.pcrc_32_tab,
095900000000                                       zi->ci.buffered_data[i],t);
096000000000#endif
096100000000    }
096200000000    if (ZWRITE(zi->z_filefunc,zi->filestream,zi->ci.buffered_data,zi->ci.pos_in_buffered_data)
096300000000                                                                    !=zi->ci.pos_in_buffered_data)
096400000000      err = ZIP_ERRNO;
096500000000    zi->ci.pos_in_buffered_data = 0;
096600000000    return err;
096700000000}
096800000000
096900000000extern int ZEXPORT zipWriteInFileInZip (file, buf, len)
097000000000    zipFile file;
097100000000    const void* buf;
097200000000    unsigned len;
097300000000{
097400000000    zip_internal* zi;
097500000000    int err=ZIP_OK;
097600000000
097700000000    if (file == NULL)
097800000000        return ZIP_PARAMERROR;
097900000000    zi = (zip_internal*)file;
098000000000
098100000000    if (zi->in_opened_file_inzip == 0)
098200000000        return ZIP_PARAMERROR;
098300000000
098400000000    zi->ci.stream.next_in = (void*)buf;
098500000000    zi->ci.stream.avail_in = len;
098600000000    zi->ci.crc32 = crc32(zi->ci.crc32,buf,len);
098700000000
098800000000    while ((err==ZIP_OK) && (zi->ci.stream.avail_in>0))
098900000000    {
099000000000        if (zi->ci.stream.avail_out == 0)
099100000000        {
099200000000            if (zipFlushWriteBuffer(zi) == ZIP_ERRNO)
099300000000                err = ZIP_ERRNO;
099400000000            zi->ci.stream.avail_out = (uInt)Z_BUFSIZE;
099500000000            zi->ci.stream.next_out = zi->ci.buffered_data;
099600000000        }
099700000000
099800000000
099900000000        if(err != ZIP_OK)
100000000000            break;
100100000000
100200000000        if ((zi->ci.method == Z_DEFLATED) && (!zi->ci.raw))
100300000000        {
100400000000            uLong uTotalOutBefore = zi->ci.stream.total_out;
100500000000            err=deflate(&zi->ci.stream,  Z_NO_FLUSH);
100600000000            zi->ci.pos_in_buffered_data += (uInt)(zi->ci.stream.total_out - uTotalOutBefore) ;
100700000000
100800000000        }
100900000000        else
101000000000        {
101100000000            uInt copy_this,i;
101200000000            if (zi->ci.stream.avail_in < zi->ci.stream.avail_out)
101300000000                copy_this = zi->ci.stream.avail_in;
101400000000            else
101500000000                copy_this = zi->ci.stream.avail_out;
101600000000            for (i=0;i<copy_this;i++)
101700000000                *(((char*)zi->ci.stream.next_out)+i) =
101800000000                    *(((const char*)zi->ci.stream.next_in)+i);
101900000000            {
102000000000                zi->ci.stream.avail_in -= copy_this;
102100000000                zi->ci.stream.avail_out-= copy_this;
102200000000                zi->ci.stream.next_in+= copy_this;
102300000000                zi->ci.stream.next_out+= copy_this;
102400000000                zi->ci.stream.total_in+= copy_this;
102500000000                zi->ci.stream.total_out+= copy_this;
102600000000                zi->ci.pos_in_buffered_data += copy_this;
102700000000            }
102800000000        }
102900000000    }
103000000000
103100000000    return err;
103200000000}
103300000000
103400000000extern int ZEXPORT zipCloseFileInZipRaw (file, uncompressed_size, crc32)
103500000000    zipFile file;
103600000000    uLong uncompressed_size;
103700000000    uLong crc32;
103800000000{
103900000000    zip_internal* zi;
104000000000    uLong compressed_size;
104100000000    int err=ZIP_OK;
104200000000
104300000000    if (file == NULL)
104400000000        return ZIP_PARAMERROR;
104500000000    zi = (zip_internal*)file;
104600000000
104700000000    if (zi->in_opened_file_inzip == 0)
104800000000        return ZIP_PARAMERROR;
104900000000    zi->ci.stream.avail_in = 0;
105000000000
105100000000    if ((zi->ci.method == Z_DEFLATED) && (!zi->ci.raw))
105200000000        while (err==ZIP_OK)
105300000000    {
105400000000        uLong uTotalOutBefore;
105500000000        if (zi->ci.stream.avail_out == 0)
105600000000        {
105700000000            if (zipFlushWriteBuffer(zi) == ZIP_ERRNO)
105800000000                err = ZIP_ERRNO;
105900000000            zi->ci.stream.avail_out = (uInt)Z_BUFSIZE;
106000000000            zi->ci.stream.next_out = zi->ci.buffered_data;
106100000000        }
106200000000        uTotalOutBefore = zi->ci.stream.total_out;
106300000000        err=deflate(&zi->ci.stream,  Z_FINISH);
106400000000        zi->ci.pos_in_buffered_data += (uInt)(zi->ci.stream.total_out - uTotalOutBefore) ;
106500000000    }
106600000000
106700000000    if (err==Z_STREAM_END)
106800000000        err=ZIP_OK; /* this is normal */
106900000000
107000000000    if ((zi->ci.pos_in_buffered_data>0) && (err==ZIP_OK))
107100000000        if (zipFlushWriteBuffer(zi)==ZIP_ERRNO)
107200000000            err = ZIP_ERRNO;
107300000000
107400000000    if ((zi->ci.method == Z_DEFLATED) && (!zi->ci.raw))
107500000000    {
107600000000        err=deflateEnd(&zi->ci.stream);
107700000000        zi->ci.stream_initialised = 0;
107800000000    }
107900000000
108000000000    if (!zi->ci.raw)
108100000000    {
108200000000        crc32 = (uLong)zi->ci.crc32;
108300000000        uncompressed_size = (uLong)zi->ci.stream.total_in;
108400000000    }
108500000000    compressed_size = (uLong)zi->ci.stream.total_out;
108600000000#    ifndef NOCRYPT
108700000000    compressed_size += zi->ci.crypt_header_size;
108800000000#    endif
108900000000
109000000000    ziplocal_putValue_inmemory(zi->ci.central_header+16,crc32,4); /*crc*/
109100000000    ziplocal_putValue_inmemory(zi->ci.central_header+20,
109200000000                                compressed_size,4); /*compr size*/
109300000000    if (zi->ci.stream.data_type == Z_ASCII)
109400000000        ziplocal_putValue_inmemory(zi->ci.central_header+36,(uLong)Z_ASCII,2);
109500000000    ziplocal_putValue_inmemory(zi->ci.central_header+24,
109600000000                                uncompressed_size,4); /*uncompr size*/
109700000000
109800000000    if (err==ZIP_OK)
109900000000        err = add_data_in_datablock(&zi->central_dir,zi->ci.central_header,
110000000000                                       (uLong)zi->ci.size_centralheader);
110100000000    free(zi->ci.central_header);
110200000000
110300000000    if (err==ZIP_OK)
110400000000    {
110500000000        long cur_pos_inzip = ZTELL(zi->z_filefunc,zi->filestream);
110600000000        if (ZSEEK(zi->z_filefunc,zi->filestream,
110700000000                  zi->ci.pos_local_header + 14,ZLIB_FILEFUNC_SEEK_SET)!=0)
110800000000            err = ZIP_ERRNO;
110900000000
111000000000        if (err==ZIP_OK)
111100000000            err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,crc32,4); /* crc 32, unknown */
111200000000
111300000000        if (err==ZIP_OK) /* compressed size, unknown */
111400000000            err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,compressed_size,4);
111500000000
111600000000        if (err==ZIP_OK) /* uncompressed size, unknown */
111700000000            err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,uncompressed_size,4);
111800000000
111900000000        if (ZSEEK(zi->z_filefunc,zi->filestream,
112000000000                  cur_pos_inzip,ZLIB_FILEFUNC_SEEK_SET)!=0)
112100000000            err = ZIP_ERRNO;
112200000000    }
112300000000
112400000000    zi->number_entry ++;
112500000000    zi->in_opened_file_inzip = 0;
112600000000
112700000000    return err;
112800000000}
112900000000
113000000000extern int ZEXPORT zipCloseFileInZip (file)
113100000000    zipFile file;
113200000000{
113300000000    return zipCloseFileInZipRaw (file,0,0);
113400000000}
113500000000
113600000000extern int ZEXPORT zipClose (file, global_comment)
113700000000    zipFile file;
113800000000    const char* global_comment;
113900000000{
114000000000    zip_internal* zi;
114100000000    int err = 0;
114200000000    uLong size_centraldir = 0;
114300000000    uLong centraldir_pos_inzip;
114400000000    uInt size_global_comment;
114500000000    if (file == NULL)
114600000000        return ZIP_PARAMERROR;
114700000000    zi = (zip_internal*)file;
114800000000
114900000000    if (zi->in_opened_file_inzip == 1)
115000000000    {
115100000000        err = zipCloseFileInZip (file);
115200000000    }
115300000000
115400000000#ifndef NO_ADDFILEINEXISTINGZIP
115500000000    if (global_comment==NULL)
115600000000        global_comment = zi->globalcomment;
115700000000#endif
115800000000    if (global_comment==NULL)
115900000000        size_global_comment = 0;
116000000000    else
116100000000        size_global_comment = (uInt)strlen(global_comment);
116200000000
116300000000    centraldir_pos_inzip = ZTELL(zi->z_filefunc,zi->filestream);
116400000000    if (err==ZIP_OK)
116500000000    {
116600000000        linkedlist_datablock_internal* ldi = zi->central_dir.first_block ;
116700000000        while (ldi!=NULL)
116800000000        {
116900000000            if ((err==ZIP_OK) && (ldi->filled_in_this_block>0))
117000000000                if (ZWRITE(zi->z_filefunc,zi->filestream,
117100000000                           ldi->data,ldi->filled_in_this_block)
117200000000                              !=ldi->filled_in_this_block )
117300000000                    err = ZIP_ERRNO;
117400000000
117500000000            size_centraldir += ldi->filled_in_this_block;
117600000000            ldi = ldi->next_datablock;
117700000000        }
117800000000    }
117900000000    free_datablock(zi->central_dir.first_block);
118000000000
118100000000    if (err==ZIP_OK) /* Magic End */
118200000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)ENDHEADERMAGIC,4);
118300000000
118400000000    if (err==ZIP_OK) /* number of this disk */
118500000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)0,2);
118600000000
118700000000    if (err==ZIP_OK) /* number of the disk with the start of the central directory */
118800000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)0,2);
118900000000
119000000000    if (err==ZIP_OK) /* total number of entries in the central dir on this disk */
119100000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)zi->number_entry,2);
119200000000
119300000000    if (err==ZIP_OK) /* total number of entries in the central dir */
119400000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)zi->number_entry,2);
119500000000
119600000000    if (err==ZIP_OK) /* size of the central directory */
119700000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)size_centraldir,4);
119800000000
119900000000    if (err==ZIP_OK) /* offset of start of central directory with respect to the
120000000000                            starting disk number */
120100000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,
120200000000                                (uLong)(centraldir_pos_inzip - zi->add_position_when_writting_offset),4);
120300000000
120400000000    if (err==ZIP_OK) /* zipfile comment length */
120500000000        err = ziplocal_putValue(&zi->z_filefunc,zi->filestream,(uLong)size_global_comment,2);
120600000000
120700000000    if ((err==ZIP_OK) && (size_global_comment>0))
120800000000        if (ZWRITE(zi->z_filefunc,zi->filestream,
120900000000                   global_comment,size_global_comment) != size_global_comment)
121000000000                err = ZIP_ERRNO;
121100000000
121200000000    if (ZCLOSE(zi->z_filefunc,zi->filestream) != 0)
121300000000        if (err == ZIP_OK)
121400000000            err = ZIP_ERRNO;
121500000000
121600000000#ifndef NO_ADDFILEINEXISTINGZIP
121700000000    TRYFREE(zi->globalcomment);
121800000000#endif
121900000000    TRYFREE(zi);
122000000000
122100000000    return err;
122200000000}
