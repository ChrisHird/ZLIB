000100000000/*
000200000000   miniunz.c
000300000000   Version 1.01e, February 12th, 2005
000400000000
000500000000   Copyright (C) 1998-2005 Gilles Vollant
000600000000*/
000700000000
000800000000
000900000000#include <stdio.h>
001000000000#include <stdlib.h>
001100000000#include <string.h>
001200000000#include <time.h>
001300000000#include <errno.h>
001400000000#include <fcntl.h>
001500000000
001600000000#ifdef unix
001700000000# include <unistd.h>
001800000000# include <utime.h>
001900000000#else
002000000000# include <direct.h>
002100000000# include <io.h>
002200000000#endif
002300000000
002400000000#include "unzip.h"
002500000000
002600060218#ifndef AS400
002700060218#define CASESENSITIVITY (0)
002800060218#else
002900060218#define CASESENSITIVITY (2)
003000060218#endif
003100060218
003200060218#ifdef FPRINT2SNDMSG
003300060218#include <qusec.h>
003400060218#include <qmhsndpm.h>
003500060218void snd_OS400_msg OF((char []));
003600060218char printtext[256];
003700060218Qus_EC_t error_code;
003800060218char message_key[4];
003900060218#endif
004000060218
004100060218#ifdef AS400
004200060218char err_str[24];
004300060218char *err_str2;
004400060218int rc;
004500060218#endif
004600060218
004700000000#define WRITEBUFFERSIZE (8192)
004800000000#define MAXFILENAME (256)
004900000000
005000000000#ifdef WIN32
005100000000#define USEWIN32IOAPI
005200000000#include "iowin32.h"
005300000000#endif
005400000000/*
005500000000  mini unzip, demo of unzip package
005600000000
005700000000  usage :
005800000000  Usage : miniunz [-exvlo] file.zip [file_to_extract] [-d extractdir]
005900000000
006000000000  list the file in the zipfile, and print the content of FILE_ID.ZIP or README.TXT
006100000000    if it exists
006200000000*/
006300000000
006400000000
006500000000/* change_file_date : change the date/time of a file
006600000000    filename : the filename of the file where date/time must be modified
006700000000    dosdate : the new date at the MSDos format (4 bytes)
006800000000    tmu_date : the SAME new date at the tm_unz format */
006900000000void change_file_date(filename,dosdate,tmu_date)
007000000000    const char *filename;
007100000000    uLong dosdate;
007200000000    tm_unz tmu_date;
007300000000{
007400000000#ifdef WIN32
007500000000  HANDLE hFile;
007600000000  FILETIME ftm,ftLocal,ftCreate,ftLastAcc,ftLastWrite;
007700000000
007800000000  hFile = CreateFile(filename,GENERIC_READ | GENERIC_WRITE,
007900000000                      0,NULL,OPEN_EXISTING,0,NULL);
008000000000  GetFileTime(hFile,&ftCreate,&ftLastAcc,&ftLastWrite);
008100000000  DosDateTimeToFileTime((WORD)(dosdate>>16),(WORD)dosdate,&ftLocal);
008200000000  LocalFileTimeToFileTime(&ftLocal,&ftm);
008300000000  SetFileTime(hFile,&ftm,&ftLastAcc,&ftm);
008400000000  CloseHandle(hFile);
008500000000#else
008600000000#ifdef unix
008700000000  struct utimbuf ut;
008800000000  struct tm newdate;
008900000000  newdate.tm_sec = tmu_date.tm_sec;
009000000000  newdate.tm_min=tmu_date.tm_min;
009100000000  newdate.tm_hour=tmu_date.tm_hour;
009200000000  newdate.tm_mday=tmu_date.tm_mday;
009300000000  newdate.tm_mon=tmu_date.tm_mon;
009400000000  if (tmu_date.tm_year > 1900)
009500000000      newdate.tm_year=tmu_date.tm_year - 1900;
009600000000  else
009700000000      newdate.tm_year=tmu_date.tm_year ;
009800000000  newdate.tm_isdst=-1;
009900000000
010000000000  ut.actime=ut.modtime=mktime(&newdate);
010100000000  utime(filename,&ut);
010200000000#endif
010300000000#endif
010400000000}
010500000000
010600000000
010700000000/* mymkdir and change_file_date are not 100 % portable
010800000000   As I don't know well Unix, I wait feedback for the unix portion */
010900000000
011000000000int mymkdir(dirname)
011100000000    const char* dirname;
011200000000{
011300000000    int ret=0;
011400000000#ifdef WIN32
011500000000    ret = mkdir(dirname);
011600000000#else
011700000000#ifdef unix
011800000000    ret = mkdir (dirname,0775);
011900000000#endif
012000000000#endif
012100000000    return ret;
012200000000}
012300000000
012400000000int makedir (newdir)
012500000000    char *newdir;
012600000000{
012700000000  char *buffer ;
012800000000  char *p;
012900000000  int  len = (int)strlen(newdir);
013000000000
013100000000  if (len <= 0)
013200000000    return 0;
013300000000
013400000000  buffer = (char*)malloc(len+1);
013500000000  strcpy(buffer,newdir);
013600000000
013700000000  if (buffer[len-1] == '/') {
013800000000    buffer[len-1] = '\0';
013900000000  }
014000000000  if (mymkdir(buffer) == 0)
014100000000    {
014200000000      free(buffer);
014300000000      return 1;
014400000000    }
014500000000
014600000000  p = buffer+1;
014700000000  while (1)
014800000000    {
014900000000      char hold;
015000000000
015100000000      while(*p && *p != '\\' && *p != '/')
015200000000        p++;
015300000000      hold = *p;
015400000000      *p = 0;
015500000000      if ((mymkdir(buffer) == -1) && (errno == ENOENT))
015600000000        {
015700000000          printf("couldn't create directory %s\n",buffer);
015800000000          free(buffer);
015900000000          return 0;
016000000000        }
016100000000      if (hold == 0)
016200000000        break;
016300000000      *p++ = hold;
016400000000    }
016500000000  free(buffer);
016600000000  return 1;
016700000000}
016800000000
016900000000void do_banner()
017000000000{
017100060218//AS400  printf("MiniUnz 1.01b, demo of zLib + Unz package written by Gilles Vollant\n");
017200060218    printf("MiniUnz 1.01e, demo of zLib + Unz package written by Gilles Vollant\n");
017300000000    printf("more info at http://www.winimage.com/zLibDll/unzip.html\n\n");
017400000000}
017500000000
017600000000void do_help()
017700000000{
017800000000    printf("Usage : miniunz [-e] [-x] [-v] [-l] [-o] [-p password] file.zip [file_to_extr.] [-d extractdir]\n\n" \
017900000000           "  -e  Extract without pathname (junk paths)\n" \
018000000000           "  -x  Extract with pathname\n" \
018100000000           "  -v  list files\n" \
018200000000           "  -l  list files\n" \
018300000000           "  -d  directory to extract into\n" \
018400000000           "  -o  overwrite files without prompting\n" \
018500000000           "  -p  extract crypted file using password\n\n");
018600000000}
018700000000
018800000000
018900000000int do_list(uf)
019000000000    unzFile uf;
019100000000{
019200000000    uLong i;
019300000000    unz_global_info gi;
019400000000    int err;
019500000000
019600000000    err = unzGetGlobalInfo (uf,&gi);
019700000000    if (err!=UNZ_OK)
019800060218#ifndef FPRINT2SNDMSG
019900000000        printf("error %d with zipfile in unzGetGlobalInfo \n",err);
020000060218#else
020100060218      { sprintf(printtext,"error %d with zipfile in unzGetGlobalInfo\n",err);
020200060218        snd_OS400_msg(printtext); }
020300060218#endif
020400060218#ifdef AS400
020500060218    printf("\n");
020600060218#endif
020700000000    printf(" Length  Method   Size  Ratio   Date    Time   CRC-32     Name\n");
020800000000    printf(" ------  ------   ----  -----   ----    ----   ------     ----\n");
020900000000    for (i=0;i<gi.number_entry;i++)
021000000000    {
021100000000        char filename_inzip[256];
021200060218#ifdef AS400
021300060218        char filename_ebcdic[256];
021400060218#endif
021500000000        unz_file_info file_info;
021600000000        uLong ratio=0;
021700000000        const char *string_method;
021800000000        char charCrypt=' ';
021900000000        err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
022000000000        if (err!=UNZ_OK)
022100000000        {
022200060218#ifndef FPRINT2SNDMSG
022300000000            printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
022400060218#else
022500000000            break;
022600060218            sprintf(printtext,"error %d with zipfile in unzGetCurrentFileInfo\n",err);
022700060218            snd_OS400_msg(printtext);
022800060218#endif
022900000000        }
023000000000        if (file_info.uncompressed_size>0)
023100000000            ratio = (file_info.compressed_size*100)/file_info.uncompressed_size;
023200000000
023300000000        /* display a '*' if the file is crypted */
023400000000        if ((file_info.flag & 1) != 0)
023500000000            charCrypt='*';
023600000000
023700000000        if (file_info.compression_method==0)
023800000000            string_method="Stored";
023900000000        else
024000000000        if (file_info.compression_method==Z_DEFLATED)
024100000000        {
024200000000            uInt iLevel=(uInt)((file_info.flag & 0x6)/2);
024300000000            if (iLevel==0)
024400000000              string_method="Defl:N";
024500000000            else if (iLevel==1)
024600000000              string_method="Defl:X";
024700000000            else if ((iLevel==2) || (iLevel==3))
024800000000              string_method="Defl:F"; /* 2:fast , 3 : extra fast*/
024900000000        }
025000000000        else
025100000000            string_method="Unkn. ";
025200000000
025300060218#ifdef AS400
025400060218        strcpy(filename_ebcdic, filename_inzip);
025500060218        a2e(filename_ebcdic);
025600060218#endif
025700000000        printf("%7lu  %6s%c%7lu %3lu%%  %2.2lu-%2.2lu-%2.2lu  %2.2lu:%2.2lu  %8.8lx   %s\n",
025800000000                file_info.uncompressed_size,string_method,
025900000000                charCrypt,
026000000000                file_info.compressed_size,
026100000000                ratio,
026200000000                (uLong)file_info.tmu_date.tm_mon + 1,
026300000000                (uLong)file_info.tmu_date.tm_mday,
026400000000                (uLong)file_info.tmu_date.tm_year % 100,
026500000000                (uLong)file_info.tmu_date.tm_hour,(uLong)file_info.tmu_date.tm_min,
026600060218#ifndef AS400
026700060218                (uLong)file_info.crc,filename_inzip);
026800060218#else
026900060218                (uLong)file_info.crc,filename_ebcdic);
027000060218#endif
027100000000        if ((i+1)<gi.number_entry)
027200000000        {
027300000000            err = unzGoToNextFile(uf);
027400000000            if (err!=UNZ_OK)
027500000000            {
027600060218#ifndef FPRINT2SNDMSG
027700000000                printf("error %d with zipfile in unzGoToNextFile\n",err);
027800060218#else
027900060218                sprintf(printtext,"error %d with zipfile in unzGoToNextFile\n",err);
028000060218                snd_OS400_msg(printtext);
028100060218#endif
028200000000                break;
028300000000            }
028400000000        }
028500000000    }
028600000000
028700060218#ifndef AS400
028800060218    return 0;
028900060218#else
029000060218    printf("\n");
029100060218    return err;
029200060218#endif
029300000000}
029400000000
029500000000
029600000000int do_extract_currentfile(uf,popt_extract_without_path,popt_overwrite,password)
029700000000    unzFile uf;
029800000000    const int* popt_extract_without_path;
029900000000    int* popt_overwrite;
030000000000    const char* password;
030100000000{
030200000000    char filename_inzip[256];
030300000000    char* filename_withoutpath;
030400000000    char* p;
030500000000    int err=UNZ_OK;
030600000000    FILE *fout=NULL;
030700000000    void* buf;
030800000000    uInt size_buf;
030900000000
031000000000    unz_file_info file_info;
031100000000    uLong ratio=0;
031200000000    err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
031300000000
031400000000    if (err!=UNZ_OK)
031500000000    {
031600060218#ifndef FPRINT2SNDMSG
031700000000        printf("error %d with zipfile in unzGetCurrentFileInfo\n",err);
031800060218#else
031900060218        sprintf(printtext,"error %d with zipfile in unzGetCurrentFileInfo\n",err);
032000060218        snd_OS400_msg(printtext);
032100060218#endif
032200000000        return err;
032300000000    }
032400000000
032500000000    size_buf = WRITEBUFFERSIZE;
032600000000    buf = (void*)malloc(size_buf);
032700000000    if (buf==NULL)
032800000000    {
032900060218#ifndef FPRINT2SNDMSG
033000000000        printf("Error allocating memory\n");
033100060218#else
033200060218        sprintf(printtext,"Error allocating memory\n");
033300060218        snd_OS400_msg(printtext);
033400060218#endif
033500000000        return UNZ_INTERNALERROR;
033600000000    }
033700000000
033800060218#ifdef AS400
033900060218        a2e(filename_inzip);
034000060218#endif
034100060218
034200000000    p = filename_withoutpath = filename_inzip;
034300000000    while ((*p) != '\0')
034400000000    {
034500000000        if (((*p)=='/') || ((*p)=='\\'))
034600000000            filename_withoutpath = p+1;
034700000000        p++;
034800000000    }
034900000000
035000000000    if ((*filename_withoutpath)=='\0')
035100000000    {
035200000000        if ((*popt_extract_without_path)==0)
035300000000        {
035400060218#ifndef FPRINT2SNDMSG
035500000000            printf("creating directory: %s\n",filename_inzip);
035600060218#else
035700060218            sprintf(printtext,"creating directory: %s\n",filename_inzip);
035800060218            snd_OS400_msg(printtext);
035900060218#endif
036000000000            mymkdir(filename_inzip);
036100000000        }
036200000000    }
036300000000    else
036400000000    {
036500000000        const char* write_filename;
036600000000        int skip=0;
036700000000
036800000000        if ((*popt_extract_without_path)==0)
036900000000            write_filename = filename_inzip;
037000000000        else
037100000000            write_filename = filename_withoutpath;
037200000000
037300000000        err = unzOpenCurrentFilePassword(uf,password);
037400000000        if (err!=UNZ_OK)
037500000000        {
037600060218#ifndef FPRINT2SNDMSG
037700000000            printf("error %d with zipfile in unzOpenCurrentFilePassword\n",err);
037800060218#else
037900060218            sprintf(printtext,"error %d with zipfile in unzOpenCurrentFilePassword\n",err);
038000060218            snd_OS400_msg(printtext);
038100060218#endif
038200000000        }
038300000000
038400000000        if (((*popt_overwrite)==0) && (err==UNZ_OK))
038500000000        {
038600000000            char rep=0;
038700000000            FILE* ftestexist;
038800000000            ftestexist = fopen(write_filename,"rb");
038900000000            if (ftestexist!=NULL)
039000000000            {
039100000000                fclose(ftestexist);
039200000000                do
039300000000                {
039400000000                    char answer[128];
039500000000                    int ret;
039600000000
039700060218#ifndef AS400
039800000000                    printf("The file %s exists. Overwrite ? [y]es, [n]o, [A]ll: ",write_filename);
039900060218#else
040000060218                    printf("The file %s exist. Overwrite ? [y]es, [n]o, [A]ll: \n",write_filename);
040100060218#endif
040200000000                    ret = scanf("%1s",answer);
040300000000                    if (ret != 1)
040400000000                    {
040500000000                       exit(EXIT_FAILURE);
040600000000                    }
040700000000                    rep = answer[0] ;
040800000000                    if ((rep>='a') && (rep<='z'))
040900060218#ifndef AS400
041000060218                        rep -= 0x20;
041100060218#else
041200060218                        rep += 0x40;
041300060218#endif
041400000000                }
041500000000                while ((rep!='Y') && (rep!='N') && (rep!='A'));
041600000000            }
041700000000
041800000000            if (rep == 'N')
041900000000                skip = 1;
042000000000
042100000000            if (rep == 'A')
042200000000                *popt_overwrite=1;
042300000000        }
042400000000
042500000000        if ((skip==0) && (err==UNZ_OK))
042600000000        {
042700000000            fout=fopen(write_filename,"wb");
042800000000
042900000000            /* some zipfile don't contain directory alone before file */
043000000000            if ((fout==NULL) && ((*popt_extract_without_path)==0) &&
043100000000                                (filename_withoutpath!=(char*)filename_inzip))
043200000000            {
043300000000                char c=*(filename_withoutpath-1);
043400000000                *(filename_withoutpath-1)='\0';
043500000000                makedir(write_filename);
043600000000                *(filename_withoutpath-1)=c;
043700000000                fout=fopen(write_filename,"wb");
043800000000            }
043900000000
044000000000            if (fout==NULL)
044100000000            {
044200060218#ifndef FPRINT2SNDMSG
044300000000                printf("error opening %s\n",write_filename);
044400060218#else
044500060218                sprintf(printtext,"error opening %s\n",write_filename);
044600060218                snd_OS400_msg(printtext);
044700060218#endif
044800000000            }
044900000000        }
045000000000
045100000000        if (fout!=NULL)
045200000000        {
045300060218#ifndef FPRINT2SNDMSG
045400000000            printf(" extracting: %s\n",write_filename);
045500060218#else
045600060218            sprintf(printtext," extracting: %s\n",write_filename);
045700060218            snd_OS400_msg(printtext);
045800060218#endif
045900000000
046000000000            do
046100000000            {
046200000000                err = unzReadCurrentFile(uf,buf,size_buf);
046300000000                if (err<0)
046400000000                {
046500060218#ifndef FPRINT2SNDMSG
046600000000                    printf("error %d with zipfile in unzReadCurrentFile\n",err);
046700060218#else
046800060218                    sprintf(printtext,"error %d with zipfile in unzReadCurrentFile\n",err);
046900060218                    snd_OS400_msg(printtext);
047000060218#endif
047100000000                    break;
047200000000                }
047300000000                if (err>0)
047400000000                    if (fwrite(buf,err,1,fout)!=1)
047500000000                    {
047600060218#ifndef FPRINT2SNDMSG
047700000000                        printf("error in writing extracted file\n");
047800060218#else
047900060218                        sprintf(printtext,"error in writing extracted file\n");
048000060218                        snd_OS400_msg(printtext);
048100060218#endif
048200000000                        err=UNZ_ERRNO;
048300000000                        break;
048400000000                    }
048500000000            }
048600000000            while (err>0);
048700000000            if (fout)
048800000000                    fclose(fout);
048900000000
049000000000            if (err==0)
049100000000                change_file_date(write_filename,file_info.dosDate,
049200000000                                 file_info.tmu_date);
049300000000        }
049400000000
049500000000        if (err==UNZ_OK)
049600000000        {
049700000000            err = unzCloseCurrentFile (uf);
049800000000            if (err!=UNZ_OK)
049900000000            {
050000060218#ifndef FPRINT2SNDMSG
050100000000                printf("error %d with zipfile in unzCloseCurrentFile\n",err);
050200060218#else
050300060219                sprintf(printtext,"error %d with zipfile in unzCloseCurrentFile\n",err);
050400060218                snd_OS400_msg(printtext);
050500060218#endif
050600000000            }
050700000000        }
050800000000        else
050900000000            unzCloseCurrentFile(uf); /* don't lose the error */
051000000000    }
051100000000
051200000000    free(buf);
051300000000    return err;
051400000000}
051500000000
051600000000
051700000000int do_extract(uf,opt_extract_without_path,opt_overwrite,password)
051800000000    unzFile uf;
051900000000    int opt_extract_without_path;
052000000000    int opt_overwrite;
052100000000    const char* password;
052200000000{
052300000000    uLong i;
052400000000    unz_global_info gi;
052500000000    int err;
052600000000    FILE* fout=NULL;
052700000000
052800000000    err = unzGetGlobalInfo (uf,&gi);
052900000000    if (err!=UNZ_OK)
053000060218#ifndef FPRINT2SNDMSG
053100000000        printf("error %d with zipfile in unzGetGlobalInfo \n",err);
053200060218#else
053300060218      { sprintf(printtext,"error %d with zipfile in unzGetGlobalInfo \n",err);
053400060218        snd_OS400_msg(printtext); }
053500060218#endif
053600000000
053700000000    for (i=0;i<gi.number_entry;i++)
053800000000    {
053900060218#ifndef AS400
054000000000        if (do_extract_currentfile(uf,&opt_extract_without_path,
054100000000                                      &opt_overwrite,
054200000000                                      password) != UNZ_OK)
054300060218#else
054400060218        err = do_extract_currentfile(uf,&opt_extract_without_path,
054500060218                                      &opt_overwrite,
054600060218                                      password);
054700060218        if (err != UNZ_OK)
054800060218#endif
054900000000            break;
055000000000
055100000000        if ((i+1)<gi.number_entry)
055200000000        {
055300000000            err = unzGoToNextFile(uf);
055400000000            if (err!=UNZ_OK)
055500000000            {
055600060218#ifndef FPRINT2SNDMSG
055700000000                printf("error %d with zipfile in unzGoToNextFile\n",err);
055800060218#else
055900060218                sprintf(printtext,"error %d with zipfile in unzGoToNextFile\n",err);
056000060218                snd_OS400_msg(printtext);
056100060218#endif
056200000000                break;
056300000000            }
056400000000        }
056500000000    }
056600000000
056700060218#ifndef AS400
056800060218    return 0;
056900060218#else
057000060218    return err;
057100060218#endif
057200000000}
057300000000
057400000000int do_extract_onefile(uf,filename,opt_extract_without_path,opt_overwrite,password)
057500000000    unzFile uf;
057600000000    const char* filename;
057700000000    int opt_extract_without_path;
057800000000    int opt_overwrite;
057900000000    const char* password;
058000000000{
058100060218#ifdef AS400
058200060218    char filename_ascii[MAXFILENAME];
058300060218#endif
058400000000    int err = UNZ_OK;
058500000000    if (unzLocateFile(uf,filename,CASESENSITIVITY)!=UNZ_OK)
058600000000    {
058700060218#ifndef AS400
058800000000        printf("file %s not found in the zipfile\n",filename);
058900060218#else  /* Convert EBCDIC to ASCII*/
059000060218        strcpy(filename_ascii, filename);
059100060218        a2e(filename_ascii);
059200060218#endif
059300060218#ifndef FPRINT2SNDMSG
059400060218        printf("file %s not found in the zipfile\n",filename);
059500060218#else  /* Convert EBCDIC to ASCII*/
059600060218        sprintf(printtext,"file %s not found in the zipfile\n",filename_ascii);
059700060218        snd_OS400_msg(printtext);
059800060218#endif
059900000000        return 2;
060000000000    }
060100000000
060200000000    if (do_extract_currentfile(uf,&opt_extract_without_path,
060300000000                                      &opt_overwrite,
060400000000                                      password) == UNZ_OK)
060500000000        return 0;
060600000000    else
060700000000        return 1;
060800000000}
060900000000
061000060218#ifdef FPRINT2SNDMSG
061100060218void snd_OS400_msg(OS400msg)
061200060218    char OS400msg[256];
061300060218{
061400060218    error_code.Bytes_Provided = 16;
061500060218
061600060218        QMHSNDPM("CPF9898", "QCPFMSG   *LIBL     ", OS400msg, strlen((char *)OS400msg + 1),
061700060218             "*DIAG     ", "*         ", 0, message_key, &error_code);
061800060218}
061900060218#endif
062000000000
062100000000int main(argc,argv)
062200000000    int argc;
062300000000    char *argv[];
062400000000{
062500000000    const char *zipfilename=NULL;
062600000000    const char *filename_to_extract=NULL;
062700000000    const char *password=NULL;
062800000000    char filename_try[MAXFILENAME+16] = "";
062900000000    int i;
063000000000    int opt_do_list=0;
063100000000    int opt_do_extract=1;
063200000000    int opt_do_extract_withoutpath=0;
063300000000    int opt_overwrite=0;
063400000000    int opt_extractdir=0;
063500000000    const char *dirname=NULL;
063600000000    unzFile uf=NULL;
063700060218#ifdef AS400
063800060218    int err;
063900060218    const char *filename_to_extract_ascii=NULL;
064000060218    const char *password_ascii=NULL;
064100060218#endif
064200000000
064300060218#ifndef NODOBANNER
064400000000    do_banner();
064500060218#endif
064600000000    if (argc==1)
064700000000    {
064800000000        do_help();
064900000000        return 0;
065000000000    }
065100000000    else
065200000000    {
065300000000        for (i=1;i<argc;i++)
065400000000        {
065500000000            if ((*argv[i])=='-')
065600000000            {
065700000000                const char *p=argv[i]+1;
065800000000
065900000000                while ((*p)!='\0')
066000000000                {
066100000000                    char c=*(p++);;
066200000000                    if ((c=='l') || (c=='L'))
066300000000                        opt_do_list = 1;
066400000000                    if ((c=='v') || (c=='V'))
066500000000                        opt_do_list = 1;
066600000000                    if ((c=='x') || (c=='X'))
066700000000                        opt_do_extract = 1;
066800000000                    if ((c=='e') || (c=='E'))
066900000000                        opt_do_extract = opt_do_extract_withoutpath = 1;
067000000000                    if ((c=='o') || (c=='O'))
067100000000                        opt_overwrite=1;
067200000000                    if ((c=='d') || (c=='D'))
067300000000                    {
067400000000                        opt_extractdir=1;
067500000000                        dirname=argv[i+1];
067600000000                    }
067700000000
067800000000                    if (((c=='p') || (c=='P')) && (i+1<argc))
067900000000                    {
068000000000                        password=argv[i+1];
068100060218#ifdef AS400
068200060218                        password_ascii=argv[i+1];
068300060218#endif
068400000000                        i++;
068500000000                    }
068600000000                }
068700000000            }
068800000000            else
068900000000            {
069000000000                if (zipfilename == NULL)
069100000000                    zipfilename = argv[i];
069200000000                else if ((filename_to_extract==NULL) && (!opt_extractdir))
069300000000                        filename_to_extract = argv[i] ;
069400060218#ifdef AS400
069500060218                        filename_to_extract_ascii = argv[i] ;
069600060218#endif
069700000000            }
069800000000        }
069900000000    }
070000000000
070100000000    if (zipfilename!=NULL)
070200000000    {
070300000000
070400000000#        ifdef USEWIN32IOAPI
070500000000        zlib_filefunc_def ffunc;
070600000000#        endif
070700000000
070800000000        strncpy(filename_try, zipfilename,MAXFILENAME-1);
070900000000        /* strncpy doesnt append the trailing NULL, of the string is too long. */
071000000000        filename_try[ MAXFILENAME ] = '\0';
071100000000
071200000000#        ifdef USEWIN32IOAPI
071300000000        fill_win32_filefunc(&ffunc);
071400000000        uf = unzOpen2(zipfilename,&ffunc);
071500000000#        else
071600000000        uf = unzOpen(zipfilename);
071700000000#        endif
071800000000        if (uf==NULL)
071900000000        {
072000000000            strcat(filename_try,".zip");
072100000000#            ifdef USEWIN32IOAPI
072200000000            uf = unzOpen2(filename_try,&ffunc);
072300000000#            else
072400000000            uf = unzOpen(filename_try);
072500000000#            endif
072600000000        }
072700000000    }
072800000000
072900000000    if (uf==NULL)
073000000000    {
073100060218#ifndef FPRINT2SNDMSG
073200000000        printf("Cannot open %s or %s.zip\n",zipfilename,zipfilename);
073300060218#else
073400060218        sprintf(printtext,"Cannot open %s or %s.zip\n",zipfilename,zipfilename);
073500060218        snd_OS400_msg(printtext);
073600060218#endif
073700060218#ifdef AS400
073800060218        sprintf(err_str, "%d", 1);
073900060218        err_str2 = strcat("MINIUNZ_RTNCDE=", err_str);
074000060218        rc = putenv(err_str2);
074100060218#endif
074200000000        return 1;
074300000000    }
074400060218#ifndef FPRINT2SNDMSG
074500000000    printf("%s opened\n",filename_try);
074600060218#else
074700060218    sprintf(printtext,"%s opened\n",filename_try);
074800060218    snd_OS400_msg(printtext);
074900060218#endif
075000000000
075100000000    if (opt_do_list==1)
075200060218#ifndef AS400
075300000000        return do_list(uf);
075400060218#else
075500060218      { err = do_list(uf);
075600060218        sprintf(err_str, "%d", err);
075700060218        err_str2 = strcat("MINIUNZ_RTNCDE=", err_str);
075800060218        rc = putenv(err_str2);
075900060218        return err; }
076000060218#endif
076100000000    else if (opt_do_extract==1)
076200000000    {
076300060218// does not support output dir option for now...
076400000000        if (opt_extractdir && chdir(dirname))
076500000000        {
076600000000          printf("Error changing into %s, aborting\n", dirname);
076700000000          exit(-1);
076800000000        }
076900000000
077000060218#ifndef AS400
077100000000        if (filename_to_extract == NULL)
077200000000            return do_extract(uf,opt_do_extract_withoutpath,opt_overwrite,password);
077300000000        else
077400000000            return do_extract_onefile(uf,filename_to_extract,
077500000000                                      opt_do_extract_withoutpath,opt_overwrite,password);
077600060218#else
077700060218        if (password != NULL) e2a(password_ascii);
077800060218        if (filename_to_extract == NULL)
077900060218            err = do_extract(uf,opt_do_extract_withoutpath,opt_overwrite,password_ascii);
078000060218        else {
078100060218            e2a(filename_to_extract_ascii);
078200060218            err = do_extract_onefile(uf,filename_to_extract_ascii,
078300060218                                      opt_do_extract_withoutpath,opt_overwrite,password_ascii);  }
078400060218        sprintf(err_str, "%d", err);
078500060218        err_str2 = strcat("MINIUNZ_RTNCDE=", err_str);
078600060218        rc = putenv(err_str2);
078700060218        return err;
078800060218#endif
078900000000    }
079000000000    unzCloseCurrentFile(uf);
079100000000
079200000000    return 0;
079300000000}
