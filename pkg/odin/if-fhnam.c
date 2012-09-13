/*
Copyright (C) 1991 Geoffrey M. Clemm

This file is part of the Odin system.

The Odin system is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation (see the file COPYING).

The Odin system is distributed WITHOUT ANY WARRANTY, without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

geoff@boulder.colorado.edu
*/

#include "inc/GMC.h"
#include "inc/FKind_.h"
#include "inc/FileName.h"
#include "inc/FilHdr.h"
#include "inc/Str.h"

void FilHdr_DataFileName(tp_FileName FileName, tp_FilHdr FilHdr)
{
   tps_Str StrBuf;
   tp_Str ExtName;
   boolean New_Flag, Abort;
   int Num, i;
   tps_FileName tmp;
   size_t sz;

   FORBIDDEN(FileName == ERROR || FilHdr == ERROR);
   {
      if (IsSource(FilHdr)) {
         FilHdr_HostFN(FileName, FilHdr, FALSE);
      } else if (IsStr(FilHdr)) {
         (void) strcpy(StrBuf, FilHdr_Ident(FilHdr));
         sz = snprintf(FileName, MAX_FileName, "%s", StrBuf);
         if (sz >= MAX_FileName) {
            (void) fprintf(stderr,
                           "File name too long (MAX_FileName=%d): %s\n",
                           MAX_FileName, StrBuf);
            exit(1);
         }
      } else {
         New_Flag = FALSE;
         if (FilHdr->HdrInf.DataNum == 0) {
            DataNum += 1;
            FilHdr->HdrInf.DataNum = DataNum;
            New_Flag = TRUE;
         }
         (void) strcpy(FileName, CacheDirName);
         Num = (FilHdr->HdrInf.DataNum - 1) / 34;
         if (Num > 0) {
            (void) strcpy(StrBuf, FileName);
            Append_DataNum(Tail(StrBuf), Num - 1);
            sz = snprintf(FileName, MAX_FileName, "%s", StrBuf);
            if (sz >= MAX_FileName) {
               (void) fprintf(stderr,
                              "File name too long (MAX_FileName=%d): %s\n",
                              MAX_FileName, StrBuf);
               exit(1);
            }
            if (New_Flag) {
               MakeDirFile(&Abort, FileName);
               if (Abort)
                  FATALERROR("cannot make cache sub-directory");
            };
         }
         (void) strcpy(tmp, FileName);
         {
            if (ShortCacheNameFlag) {
               (void) strcpy(StrBuf, ".");
               (void) strcat(StrBuf,
                             FilTyp_ShortFTName(FilHdr_FilTyp(FilHdr)));
               ExtName = StrBuf;
               for (i = 0; StrBuf[i] != '\0'; i += 1) {
                  if (StrBuf[i] == '.') {
                     ExtName = &StrBuf[i];
                  }
               }
               if ((int) strlen(ExtName) > 4)
                  ExtName = "";
               Num = (int) FilHdr_LocHdr(FilHdr);
               sz = snprintf(FileName, MAX_FileName, "%s/%d%s", tmp, Num,
                             ExtName);
               if (sz >= MAX_FileName) {
                  (void) fprintf(stderr,
                                 "File name too long (MAX_FileName=%d): %s/%d%s\n",
                                 MAX_FileName, tmp, Num, ExtName);
                  exit(1);
               }
            } else {
               ExtName = FilHdr_Label(StrBuf, FilHdr, TRUE);
               sz = snprintf(FileName, MAX_FileName, "%s/%s", tmp,
                             ExtName);
               if (sz >= MAX_FileName) {
                  (void) fprintf(stderr,
                                 "File name too long (MAX_FileName=%d): %s/%s\n",
                                 MAX_FileName, tmp, ExtName);
                  exit(1);
               }
            }
         }
      }
   }
}

void FilHdr_ErrorFileName(tp_FileName FileName, tp_FilHdr FilHdr)
{
   tps_FileName buf;
   size_t sz;

   FORBIDDEN(FileName == ERROR || FilHdr == ERROR);
   FORBIDDEN(IsSource(FilHdr));
   FilHdr_DataFileName((tp_FileName) strcpy(buf, FileName), FilHdr);
   sz = snprintf(FileName, MAX_FileName, "%sE", buf);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr, "File name too long (MAX_FileName=%d): %sE\n",
                     MAX_FileName, buf);
      exit(1);
   }
}

void FilHdr_WarningFileName(tp_FileName FileName, tp_FilHdr FilHdr)
{
   tps_FileName buf;
   size_t sz;

   FORBIDDEN(FileName == ERROR || FilHdr == ERROR);
   FORBIDDEN(IsSource(FilHdr));
   FilHdr_DataFileName((tp_FileName) strcpy(buf, FileName), FilHdr);
   sz = snprintf(FileName, MAX_FileName, "%sW", buf);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr, "File name too long (MAX_FileName=%d): %sW\n",
                     MAX_FileName, buf);
      exit(1);
   }
}

static void
FilHdr_HostFN1(tp_FileName FileName,
               tp_FilHdr FilHdr, boolean QuoteFlag, boolean AliasFlag)
{
   int i, j, middle;
   tp_FilHdr RestFilHdr;
   char ch;
   tps_Str Str, KeyBuf;
   tp_Str Key;
   size_t sz;

   FORBIDDEN(FileName == ERROR || FilHdr == ERROR);
   if (FilHdr == RootFilHdr) {
      (void) strcpy(FileName, "/");
      return;
   }

   for (i = 0, RestFilHdr = Copy_FilHdr(FilHdr);
        RestFilHdr != RootFilHdr; RestFilHdr = FilHdr_Father(RestFilHdr)) {
      if (AliasFlag) {
         RestFilHdr = FilHdr_AliasFilHdr(RestFilHdr);
      }
      Str[i] = '/';
      i += 1;
      Key = FilHdr_Key(KeyBuf, RestFilHdr);
      for (j = strlen(Key) - 1; j >= 0; j--, i++) {
         Str[i] = Key[j];
         if (QuoteFlag && !IsWordChr(Key[j])) {
            i += 1;
            Str[i] = '\\';
         }
      }
   }
   Ret_FilHdr(RestFilHdr);

   Str[i] = '/';
   middle = (i + 1) / 2;
   for (j = 0; j < middle; j++) {
      ch = Str[j];
      Str[j] = Str[i - j];
      Str[i - j] = ch;
   }
   Str[i] = 0;
   sz = snprintf(FileName, MAX_FileName, "%s", Str);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr, "File name too long (MAX_FileName=%d): %s\n",
                     MAX_FileName, Str);
      exit(1);
   }
}

void Local_Do_Alias(tp_FileName FileName, boolean ForceFlag)
{
   tp_FilHdr FilHdr, SymLinkFilHdr;

   FilHdr = HostFN_FilHdr(FileName);
   if (!IsSource(FilHdr)) {
      if (!IsDrvDir(FilHdr)) {
         FilHdr_Error("<%s> expected to be a source file.\n", FilHdr);
      }
      Ret_FilHdr(FilHdr);
      return;
   }
   if (!IsSrcUpToDate(FilHdr)) {
      Update_SrcFilHdr(FilHdr, FALSE);
   }
   while (FilHdr != RootFilHdr) {
      if (ForceFlag)
         Set_AliasLocHdr(FilHdr, (tp_LocHdr) NIL);
      if (IsSymLink(FilHdr)) {
         SymLinkFilHdr = Deref_SymLink(Copy_FilHdr(FilHdr));
         if (ForceFlag || FilHdr_AliasLocHdr(SymLinkFilHdr) == NIL) {
            Set_AliasLocHdr(SymLinkFilHdr, FilHdr_LocHdr(FilHdr));
         }
         Ret_FilHdr(SymLinkFilHdr);
      }
      FilHdr = FilHdr_Father(FilHdr);
   }
   Ret_FilHdr(FilHdr);
}

void Local_Get_Alias(tp_FileName OutFileName, tp_FileName FileName)
{
   tp_FilHdr FilHdr;

   FilHdr = HostFN_FilHdr(FileName);
   if (!IsSource(FilHdr)) {
      if (!IsDrvDir(FilHdr)) {
         FilHdr_Error("<%s> expected to be a source file.\n", FilHdr);
      }
      (void) strcpy(OutFileName, FileName);
      Ret_FilHdr(FilHdr);
      return;
   }
   FilHdr_HostFN1(OutFileName, FilHdr, FALSE, TRUE);
   Ret_FilHdr(FilHdr);
}

void
FilHdr_HostFN(tp_FileName FileName, tp_FilHdr FilHdr, boolean QuoteFlag)
{
   if (QuoteFlag && FilHdr == EmptyFilHdr) {
      (void) strcpy(FileName, "()");
      return;
   }
   FilHdr_HostFN1(FileName, FilHdr, QuoteFlag, FALSE);
}

tp_FilHdr HostFN_FilHdr(tp_FileName FileName)
{
   tps_Str Key;
   tp_FilHdr FilHdr;
   int i;

   if (FileName == ERROR)
      return ERROR;
   i = 0;
   {
      if (FileName[i] == '/') {
         FilHdr =
             Do_Keys(Copy_FilHdr(RootFilHdr), strcpy(Key, &FileName[1]));
      } else {
         FilHdr = Do_Keys(Top_CWDFilHdr(), strcpy(Key, FileName));
      }
   }
   return FilHdr;
}

tp_FilHdr Do_Keys(tp_FilHdr FilHdr, tp_Key Key)
{
   tps_Str SubKey;
   int i, j;

   for (i = 0; Key[i] != 0; i += 1) {
      for (j = 0; Key[i] != 0 && Key[i] != '/'; i += 1, j += 1) {
         SubKey[j] = Key[i];
      }
      SubKey[j] = 0;
      FilHdr = Do_Key(FilHdr, SubKey);
      if (Key[i] == 0)
         i -= 1;
   };
   return FilHdr;
}

tp_FilHdr CacheFileName_FilHdr(tp_FileName CacheFileName)
{
   tp_FilHdr FilHdr;

   FilHdr = LocHdr_FilHdr(CacheFileName_LocHdr(CacheFileName));
   if (FilHdr == ERROR) {
      SystemError("Bad cache filename: <%s>.\n", CacheFileName);
   }
   return FilHdr;
}

tp_FilHdr DataFileName_FilHdr(tp_FileName DataFileName)
{
   if (DataFileName == ERROR)
      return ERROR;
   if (strncmp(CacheDirName, DataFileName, strlen(CacheDirName)) == 0) {
      return CacheFileName_FilHdr(DataFileName);
   }
   return HostFN_FilHdr(DataFileName);
}
