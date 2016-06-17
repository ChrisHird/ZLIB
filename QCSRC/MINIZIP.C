000100000000/*
000200000000   minizip.c
000300000000   Version 1.01e, February 12th, 2005
000400000000
000500000000   Copyright (C) 1998-2005 Gilles Vollant
000600000000*/
000700000000
000800000000#include <stdio.h>
000900000000#include <stdlib.h>
001000000000#include <string.h>
001100000000#include <time.h>
001200000000#include <errno.h>
001300000000#include <fcntl.h>
001400000000
001500000000#ifdef unix
001600000000# include <unistd.h>
001700000000# include <utime.h>
001800000000# include <sys/types.h>
001900000000# include <sys/stat.h>
002000000000#else
002100000000# include <direct.h>
002200000000# include <io.h>
002300000000#endif
002400000000
002500000000#include "zip.h"
002600000000
002700000000#ifdef WIN32
002800000000#define USEWIN32IOAPI
002900000000#include "iowin32.h"
003000000000#endif
003100000000
003200060218#ifndef AS400
003300060218#define CASESENSITIVITY (0)
003400060218#else
003500060218#define CASESENSITIVITY (2)
003600060218#endif
003700060218
003800060218#ifdef FPRINT2SNDMSG
003900060218#include <qusec.h>
004000060218#include <qmhsndpm.h>
004100060218void snd_OS400_msg OF((char []));
004200060218char printtext[256];
004300060218Qus_EC_t error_code;
004400060218char message_key[4];
004500060218#endif
004600060218
004700060218#ifdef AS400
004800060218char err_str[24];
004900060218char *err_str2;
005000060218int rc;
005100060218#endif
005200000000
005300000000#define WRITEBUFFERSIZE (16384)
005400000000#define MAXFILENAME (256)
005500000000
005600000000#ifdef WIN32
005700000000uLong filetime(f, tmzip, dt)
005800000000    char *f;                /* name of file to get info on */
005900000000    tm_zip *tmzip;             /* return value: access, modific. and creation times */
006000000000    uLong *dt;             /* dostime */
006100000000{
006200000000  int ret = 0;
006300000000  {
006400000000      FILETIME ftLocal;
006500000000      HANDLE hFind;
006600000000      WIN32_FIND_DATA  ff32;
006700000000
006800000000      hFind = FindFirstFile(f,&ff32);
006900000000      if (hFind != INVALID_HANDLE_VALUE)
007000000000      {
007100000000        FileTimeToLocalFileTime(&(ff32.ftLastWriteTime),&ftLocal);
007200000000        FileTimeToDosDateTime(&ftLocal,((LPWORD)dt)+1,((LPWORD)dt)+0);
007300000000        FindClose(hFind);
007400000000        ret = 1;
007500000000      }
007600000000  }
007700000000  return ret;
007800000000}
007900000000#else
008000000000#ifdef unix
008100000000uLong filetime(f, tmzip, dt)
008200000000    char *f;               /* name of file to get info on */
008300000000    tm_zip *tmzip;         /* return value: access, modific. and creation times */
008400000000    uLong *dt;             /* dostime */
008500000000{
008600000000  int ret=0;
008700000000  struct stat s;        /* results of stat() */
008800000000  struct tm* filedate;
008900000000  time_t tm_t=0;
009000000000
009100000000  if (strcmp(f,"-")!=0)
009200000000  {
009300000000    char name[MAXFILENAME+1];
009400000000    int len = strlen(f);
009500000000    if (len > MAXFILENAME)
009600000000      len = MAXFILENAME;
009700000000
009800000000    strncpy(name, f,MAXFILENAME-1);
009900000000    /* strncpy doesnt append the trailing NULL, of the string is too long. */
010000000000    name[ MAXFILENAME ] = '\0';
010100000000
010200000000    if (name[len - 1] == '/')
010300000000      name[len - 1] = '\0';
010400000000    /* not all systems allow stat'ing a file with / appended */
010500000000    if (stat(name,&s)==0)
010600000000    {
010700000000      tm_t = s.st_mtime;
010800000000      ret = 1;
010900000000    }
011000000000  }
011100000000  filedate = localtime(&tm_t);
011200000000
011300000000  tmzip->tm_sec  = filedate->tm_sec;
011400000000  tmzip->tm_min  = filedate->tm_min;
011500000000  tmzip->tm_hour = filedate->tm_hour;
011600000000  tmzip->tm_mday = filedate->tm_mday;
011700000000  tmzip->tm_mon  = filedate->tm_mon ;
011800000000  tmzip->tm_year = filedate->tm_year;
011900000000
012000000000  return ret;
012100000000}
012200000000#else
012300000000uLong filetime(f, tmzip, dt)
012400000000    char *f;                /* name of file to get info on */
012500000000    tm_zip *tmzip;             /* return value: access, modific. and creation times */
012600000000    uLong *dt;             /* dostime */
012700000000{
012800000000    return 0;
012900000000}
013000000000#endif
013100000000#endif
013200000000
013300000000
013400000000
013500000000
013600000000int check_exist_file(filename)
013700000000    const char* filename;
013800000000{
013900000000    FILE* ftestexist;
014000000000    int ret = 1;
014100000000    ftestexist = fopen(filename,"rb");
014200000000    if (ftestexist==NULL)
014300000000        ret = 0;
014400000000    else
014500000000        fclose(ftestexist);
014600000000    return ret;
014700000000}
014800000000
014900000000void do_banner()
015000000000{
015100060218// AS400  printf("MiniZip 1.01b, demo of zLib + Zip package written by Gilles Vollant\n");
015200060218    printf("MiniZip 1.01e, demo of zLib + Zip package written by Gilles Vollant\n");
015300000000    printf("more info at http://www.winimage.com/zLibDll/unzip.html\n\n");
015400000000}
015500000000
015600000000void do_help()
015700000000{
015800000000    printf("Usage : minizip [-o] [-a] [-0 to -9] [-p password] file.zip [files_to_add]\n\n" \
015900000000           "  -o  Overwrite existing file.zip\n" \
016000000000           "  -a  Append to existing file.zip\n" \
016100000000           "  -0  Store only\n" \
016200000000           "  -1  Compress faster\n" \
016300000000           "  -9  Compress better\n\n");
016400000000}
016500000000
016600000000/* calculate the CRC32 of a file,
016700000000   because to encrypt a file, we need known the CRC32 of the file before */
016800000000int getFileCrc(const char* filenameinzip,void*buf,unsigned long size_buf,unsigned long* result_crc)
016900000000{
017000000000   unsigned long calculate_crc=0;
017100000000   int err=ZIP_OK;
017200000000   FILE * fin = fopen(filenameinzip,"rb");
017300000000   unsigned long size_read = 0;
017400000000   unsigned long total_read = 0;
017500000000   if (fin==NULL)
017600000000   {
017700000000       err = ZIP_ERRNO;
017800000000   }
017900000000
018000000000    if (err == ZIP_OK)
018100000000        do
018200000000        {
018300000000            err = ZIP_OK;
018400000000            size_read = (int)fread(buf,1,size_buf,fin);
018500000000            if (size_read < size_buf)
018600000000                if (feof(fin)==0)
018700000000            {
018800060218#ifndef FPRINT2SNDMSG
018900000000                printf("error in reading %s\n",filenameinzip);
019000060218#else
019100060218                sprintf(printtext,"error in reading %s\n",filenameinzip);
019200060218                snd_OS400_msg(printtext);
019300060218#endif
019400000000                err = ZIP_ERRNO;
019500000000            }
019600000000
019700000000            if (size_read>0)
019800000000                calculate_crc = crc32(calculate_crc,buf,size_read);
019900000000            total_read += size_read;
020000000000
020100000000        } while ((err == ZIP_OK) && (size_read>0));
020200000000
020300000000    if (fin)
020400000000        fclose(fin);
020500000000
020600000000    *result_crc=calculate_crc;
020700060218#ifndef FPRINT2SNDMSG
020800000000    printf("file %s crc %x\n",filenameinzip,calculate_crc);
020900060218#else
021000060218    sprintf(printtext,"file %s crc %x\n",filenameinzip,calculate_crc);
021100060218    snd_OS400_msg(printtext);
021200060218#endif
021300000000    return err;
021400000000}
021500060218
021600060218#ifdef FPRINT2SNDMSG
021700060218void snd_OS400_msg(OS400msg)
021800060218    char OS400msg[256];
021900060218{
022000060218    error_code.Bytes_Provided = 16;
022100060218
022200060218        QMHSNDPM("CPF9898", "QCPFMSG   *LIBL     ", OS400msg, strlen((char *)OS400msg + 1),
022300060218             "*DIAG     ", "*         ", 0, message_key, &error_code);
022400060218}
022500060218#endif
022600060218
022700000000
022800000000int main(argc,argv)
022900000000    int argc;
023000000000    char *argv[];
023100000000{
023200000000    int i;
023300000000    int opt_overwrite=0;
023400000000    int opt_compress_level=Z_DEFAULT_COMPRESSION;
023500000000    int zipfilenamearg = 0;
023600000000    char filename_try[MAXFILENAME+16];
023700000000    int zipok;
023800000000    int err=0;
023900000000    int size_buf=0;
024000000000    void* buf=NULL;
024100000000    const char* password=NULL;
024200060218#ifdef AS400
024300060218    const char* password_ascii=NULL;
024400060218#endif
024500000000
024600060218#ifndef NODOBANNER
024700000000    do_banner();
024800060218#endif
024900000000    if (argc==1)
025000000000    {
025100000000        do_help();
025200000000        return 0;
025300000000    }
025400000000    else
025500000000    {
025600000000        for (i=1;i<argc;i++)
025700000000        {
025800000000            if ((*argv[i])=='-')
025900000000            {
026000000000                const char *p=argv[i]+1;
026100000000
026200000000                while ((*p)!='\0')
026300000000                {
026400000000                    char c=*(p++);;
026500000000                    if ((c=='o') || (c=='O'))
026600000000                        opt_overwrite = 1;
026700000000                    if ((c=='a') || (c=='A'))
026800000000                        opt_overwrite = 2;
026900000000                    if ((c>='0') && (c<='9'))
027000000000                        opt_compress_level = c-'0';
027100000000
027200000000                    if (((c=='p') || (c=='P')) && (i+1<argc))
027300000000                    {
027400000000                        password=argv[i+1];
027500060218#ifdef AS400
027600060218                        password_ascii=argv[i+1];
027700060218                        e2a(password_ascii);
027800060218#endif
027900000000                        i++;
028000000000                    }
028100000000                }
028200000000            }
028300000000            else
028400000000                if (zipfilenamearg == 0)
028500000000                    zipfilenamearg = i ;
028600000000        }
028700000000    }
028800000000
028900000000    size_buf = WRITEBUFFERSIZE;
029000000000    buf = (void*)malloc(size_buf);
029100000000    if (buf==NULL)
029200000000    {
029300060218#ifndef FPRINT2SNDMSG
029400000000        printf("Error allocating memory\n");
029500060218#else
029600060218        sprintf(printtext,"Error allocating memory\n");
029700060218        snd_OS400_msg(printtext);
029800060218#endif
029900000000        return ZIP_INTERNALERROR;
030000000000    }
030100000000
030200000000    if (zipfilenamearg==0)
030300000000        zipok=0;
030400000000    else
030500000000    {
030600000000        int i,len;
030700000000        int dot_found=0;
030800000000
030900000000        zipok = 1 ;
031000000000        strncpy(filename_try, argv[zipfilenamearg],MAXFILENAME-1);
031100000000        /* strncpy doesnt append the trailing NULL, of the string is too long. */
031200000000        filename_try[ MAXFILENAME ] = '\0';
031300000000
031400000000        len=(int)strlen(filename_try);
031500000000        for (i=0;i<len;i++)
031600000000            if (filename_try[i]=='.')
031700000000                dot_found=1;
031800000000
031900000000        if (dot_found==0)
032000000000            strcat(filename_try,".zip");
032100000000
032200000000        if (opt_overwrite==2)
032300000000        {
032400000000            /* if the file don't exist, we not append file */
032500000000            if (check_exist_file(filename_try)==0)
032600000000                opt_overwrite=1;
032700000000        }
032800000000        else
032900000000        if (opt_overwrite==0)
033000000000            if (check_exist_file(filename_try)!=0)
033100000000            {
033200000000                char rep=0;
033300000000                do
033400000000                {
033500000000                    char answer[128];
033600000000                    int ret;
033700060218#ifndef AS400
033800000000                    printf("The file %s exists. Overwrite ? [y]es, [n]o, [a]ppend : ",filename_try);
033900060218#else
034000060218                    printf("The file %s exist. Overwrite ? [y]es, [n]o, [a]ppend : \n",filename_try);
034100060218#endif
034200000000                    ret = scanf("%1s",answer);
034300000000                    if (ret != 1)
034400000000                    {
034500000000                       exit(EXIT_FAILURE);
034600000000                    }
034700000000                    rep = answer[0] ;
034800000000                    if ((rep>='a') && (rep<='z'))
034900060218#ifndef AS400
035000060218                        rep -= 0x20;
035100060218#else
035200060218                        rep += 0x40;
035300060218#endif
035400000000                }
035500000000                while ((rep!='Y') && (rep!='N') && (rep!='A'));
035600000000                if (rep=='N')
035700000000                    zipok = 0;
035800000000                if (rep=='A')
035900000000                    opt_overwrite = 2;
036000000000            }
036100000000    }
036200000000
036300000000    if (zipok==1)
036400000000    {
036500000000        zipFile zf;
036600000000        int errclose;
036700000000#        ifdef USEWIN32IOAPI
036800000000        zlib_filefunc_def ffunc;
036900000000        fill_win32_filefunc(&ffunc);
037000000000        zf = zipOpen2(filename_try,(opt_overwrite==2) ? 2 : 0,NULL,&ffunc);
037100000000#        else
037200000000        zf = zipOpen(filename_try,(opt_overwrite==2) ? 2 : 0);
037300000000#        endif
037400000000
037500000000        if (zf == NULL)
037600000000        {
037700060218#ifndef FPRINT2SNDMSG
037800000000            printf("error opening %s\n",filename_try);
037900060218#else
038000060218            sprintf(printtext,"error opening %s\n",filename_try);
038100060218            snd_OS400_msg(printtext);
038200060218#endif
038300000000            err= ZIP_ERRNO;
038400000000        }
038500000000        else
038600060218#ifndef FPRINT2SNDMSG
038700000000            printf("creating %s\n",filename_try);
038800060218#else
038900060218          { sprintf(printtext,"creating %s\n",filename_try);
039000060218            snd_OS400_msg(printtext); }
039100060218#endif
039200000000
039300000000        for (i=zipfilenamearg+1;(i<argc) && (err==ZIP_OK);i++)
039400000000        {
039500060218#ifndef AS400
039600000000            if (!((((*(argv[i]))=='-') || ((*(argv[i]))=='/')) &&
039700060218#else /* allow separator (/) */
039800060218            if (!((((*(argv[i]))=='-')) &&
039900060218#endif
040000000000                  ((argv[i][1]=='o') || (argv[i][1]=='O') ||
040100000000                   (argv[i][1]=='a') || (argv[i][1]=='A') ||
040200000000                   (argv[i][1]=='p') || (argv[i][1]=='P') ||
040300000000                   ((argv[i][1]>='0') || (argv[i][1]<='9'))) &&
040400000000                  (strlen(argv[i]) == 2)))
040500000000            {
040600000000                FILE * fin;
040700000000                int size_read;
040800000000                const char* filenameinzip = argv[i];
040900060218#ifdef AS400
041000060218                const char* filename_ascii = argv[i];
041100060218#endif
041200000000                zip_fileinfo zi;
041300000000                unsigned long crcFile=0;
041400000000
041500000000                zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour =
041600000000                zi.tmz_date.tm_mday = zi.tmz_date.tm_mon = zi.tmz_date.tm_year = 0;
041700000000                zi.dosDate = 0;
041800000000                zi.internal_fa = 0;
041900000000                zi.external_fa = 0;
042000000000                filetime(filenameinzip,&zi.tmz_date,&zi.dosDate);
042100000000
042200000000/*
042300000000                err = zipOpenNewFileInZip(zf,filenameinzip,&zi,
042400000000                                 NULL,0,NULL,0,NULL / * comment * /,
042500000000                                 (opt_compress_level != 0) ? Z_DEFLATED : 0,
042600000000                                 opt_compress_level);
042700000000*/
042800000000                if ((password != NULL) && (err==ZIP_OK))
042900000000                    err = getFileCrc(filenameinzip,buf,size_buf,&crcFile);
043000000000
043100060218#ifndef AS400
043200000000                err = zipOpenNewFileInZip3(zf,filenameinzip,&zi,
043300000000                                 NULL,0,NULL,0,NULL /* comment*/,
043400000000                                 (opt_compress_level != 0) ? Z_DEFLATED : 0,
043500000000                                 opt_compress_level,0,
043600000000                                 /* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
043700000000                                 -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
043800000000                                 password,crcFile);
043900060218#else  /* Convert EBCDIC to ASCII*/
044000060218                if (filenameinzip != NULL) e2a(filename_ascii);
044100060218                err = zipOpenNewFileInZip3(zf,filename_ascii,&zi,
044200060218                                 NULL,0,NULL,0,NULL /* comment*/,
044300060218                                 (opt_compress_level != 0) ? Z_DEFLATED : 0,
044400060218                                 opt_compress_level,0,
044500060218                                 /* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
044600060218                                 -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
044700060218                                 password_ascii,crcFile);
044800060218                a2e(filenameinzip);
044900060218#endif
045000000000
045100000000                if (err != ZIP_OK)
045200060218#ifndef FPRINT2SNDMSG
045300000000                    printf("error in opening %s in zipfile\n",filenameinzip);
045400060218#else
045500060218                {   sprintf(printtext,"error in opening %s in zipfile\n",filenameinzip);
045600060218                    snd_OS400_msg(printtext); }
045700060218#endif
045800000000                else
045900000000                {
046000000000                    fin = fopen(filenameinzip,"rb");
046100000000                    if (fin==NULL)
046200000000                    {
046300000000                        err=ZIP_ERRNO;
046400060218#ifndef FPRINT2SNDMSG
046500000000                        printf("error in opening %s for reading\n",filenameinzip);
046600060218#else
046700060218                        sprintf(printtext,"error in opening %s for reading\n",filenameinzip);
046800060218                        snd_OS400_msg(printtext);
046900060218#endif
047000000000                    }
047100000000                }
047200000000
047300000000                if (err == ZIP_OK)
047400000000                    do
047500000000                    {
047600000000                        err = ZIP_OK;
047700000000                        size_read = (int)fread(buf,1,size_buf,fin);
047800000000                        if (size_read < size_buf)
047900000000                            if (feof(fin)==0)
048000000000                        {
048100060218#ifndef FPRINT2SNDMSG
048200000000                            printf("error in reading %s\n",filenameinzip);
048300060218#else
048400060218                            sprintf(printtext,"error in reading %s\n",filenameinzip);
048500060218                            snd_OS400_msg(printtext);
048600060218#endif
048700000000                            err = ZIP_ERRNO;
048800000000                        }
048900000000
049000000000                        if (size_read>0)
049100000000                        {
049200000000                            err = zipWriteInFileInZip (zf,buf,size_read);
049300000000                            if (err<0)
049400000000                            {
049500060218#ifndef FPRINT2SNDMSG
049600000000                                printf("error in writing %s in the zipfile\n",
049700000000                                                 filenameinzip);
049800060218#else
049900060218                                sprintf(printtext,"error in writing %s in the zipfile\n",
050000060218                                                 filenameinzip);
050100060218                                snd_OS400_msg(printtext);
050200060218#endif
050300000000                            }
050400000000
050500000000                        }
050600000000                    } while ((err == ZIP_OK) && (size_read>0));
050700000000
050800000000                if (fin)
050900000000                    fclose(fin);
051000000000
051100000000                if (err<0)
051200000000                    err=ZIP_ERRNO;
051300000000                else
051400000000                {
051500000000                    err = zipCloseFileInZip(zf);
051600000000                    if (err!=ZIP_OK)
051700060218#ifndef FPRINT2SNDMSG
051800000000                        printf("error in closing %s in the zipfile\n",
051900000000                                    filenameinzip);
052000060218#else
052100060218                      { sprintf(printtext,"error in closing %s in the zipfile\n",
052200060218                                    filenameinzip);
052300060218                        snd_OS400_msg(printtext); }
052400060218#endif
052500000000                }
052600000000            }
052700000000        }
052800000000        errclose = zipClose(zf,NULL);
052900000000        if (errclose != ZIP_OK)
053000060218#ifndef FPRINT2SNDMSG
053100000000            printf("error in closing %s\n",filename_try);
053200060218#else
053300060218          { sprintf(printtext,"error in closing %s\n",filename_try);
053400060218            snd_OS400_msg(printtext);
053500060218            err = errclose; }
053600060218#endif
053700000000    }
053800000000    else
053900000000    {
054000000000       do_help();
054100000000    }
054200000000
054300000000    free(buf);
054400060218#ifdef FPRINT2SNDMSG
054500060218    sprintf(printtext,"Program minizip completed with return code %d\n",err);
054600060218    snd_OS400_msg(printtext);
054700060218#endif
054800060218#ifdef AS400
054900060218    sprintf(err_str, "%d", err);
055000060218    err_str2 = strcat("MINIZIP_RTNCDE=", err_str);
055100060218    rc = putenv(err_str2);
055200060218#endif
055300000000    return 0;
055400000000}
