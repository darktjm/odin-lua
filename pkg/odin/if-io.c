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

#include "inc/System.hh"
/* fcntl.h used for open */
#include <fcntl.h>

#include "inc/GMC.h"
#include "inc/Str.h"

tp_FilDsc StdInFD;
tp_FilDsc StdOutFD;
tp_FilDsc StdErrFD;

void Init_IO(void)
{
   StdInFD = (tp_FilDsc) stdin;
   StdOutFD = (tp_FilDsc) stdout;
   StdErrFD = (tp_FilDsc) stderr;
}

boolean GetIsTTY(void)
{
   return (isatty(0) == 1);
}

tp_FilDsc FileName_CFilDsc(tp_FileName FileName)
{
   int fd;
   FILE *FilDscFILE;

   if (FileName == ERROR)
      return ERROR;
   fd = open(FileName, O_CREAT | O_WRONLY | O_EXCL, 0644);
   if (fd < 0) {
      return ERROR;
   }
   FilDscFILE = fdopen(fd, "w");
   if (FilDscFILE == NULL) {
      return ERROR;
   }
   return (tp_FilDsc) FilDscFILE;
}

static tp_FilDsc
FileName_FilDsc(tp_FileName FileName, char *Mode, boolean RetryFlag)
{
   FILE *FilDscFILE;

   if (FileName == ERROR)
      return ERROR;
   FilDscFILE = fopen(FileName, Mode);
   if (FilDscFILE == NULL) {
      if (!RetryFlag) {
         return ERROR;
      }
      if (Mode[0] == 'w') {
         if (Exists(FileName)) {
            Remove(FileName);
            FilDscFILE = fopen(FileName, Mode);
            if (FilDscFILE != NULL) {
               return (tp_FilDsc) FilDscFILE;
            }
         }
      }
      SysCallError(StdOutFD, "Retrying fopen(FileName_FilDsc)");
      while (FilDscFILE == NULL) {
         sleep(1);
         FilDscFILE = fopen(FileName, Mode);
      }
      Writeln(StdOutFD, "fopen(FileName_FilDsc) succeded.");
   }
   return (tp_FilDsc) FilDscFILE;
}

tp_FilDsc FileName_WFilDsc(tp_FileName FileName, boolean RetryFlag)
{
   return FileName_FilDsc(FileName, "w", RetryFlag);
}

tp_FilDsc FileName_WBFilDsc(tp_FileName FileName, boolean RetryFlag)
{
   return FileName_FilDsc(FileName, "wb", RetryFlag);
}

tp_FilDsc FileName_AFilDsc(tp_FileName FileName, boolean RetryFlag)
{
   return FileName_FilDsc(FileName, "a", RetryFlag);
}

tp_FilDsc FileName_RFilDsc(tp_FileName FileName, boolean RetryFlag)
{
   return FileName_FilDsc(FileName, "r", RetryFlag);
}

tp_FilDsc FileName_RWFilDsc(tp_FileName FileName, boolean RetryFlag)
{
   return FileName_FilDsc
       (FileName, (Exists(FileName) ? "r+" : "w+"), RetryFlag);
}

tp_FilDsc FileName_RWBFilDsc(tp_FileName FileName, boolean RetryFlag)
{
   return FileName_FilDsc
       (FileName, (Exists(FileName) ? "r+b" : "w+b"), RetryFlag);
}

void Flush(tp_FilDsc FilDsc)
{
   int status;

   status = fflush((FILE *) FilDsc);
   if (status == EOF)
      SysCallError(StdOutFD, "fflush(Flush)");
}

void Rewind(tp_FilDsc FilDsc)
{
   FORBIDDEN(FilDsc == ERROR);
   rewind((FILE *) FilDsc);
}

void Unwind(tp_FilDsc FilDsc)
{
   int status;

   FORBIDDEN(FilDsc == ERROR);
   status = fseek((FILE *) FilDsc, (long) 0, 2);
   if (status == -1)
      SysCallError(StdOutFD, "fseek(Unwind)");
}

void Close(tp_FilDsc FilDsc)
{
   int status;

   FORBIDDEN(FilDsc == ERROR);
   status = fclose((FILE *) FilDsc);
   if (status == EOF)
      SysCallError(StdOutFD, "fclose(Close)");
}

boolean EndOfFile(tp_FilDsc FilDsc)
{
   FORBIDDEN(FilDsc == ERROR);
   return feof((FILE *) FilDsc);
}

void Write(tp_FilDsc FilDsc, tp_Str Str)
{
   int status;

   FORBIDDEN(FilDsc == ERROR || Str == ERROR);
   status = fputs(Str, (FILE *) FilDsc);
   if (status == EOF)
      SysCallError(StdOutFD, "fputs(Write)");
}

void Writech(tp_FilDsc FilDsc, char ch)
{
   int status;

   FORBIDDEN(FilDsc == ERROR);
   status = putc(ch, (FILE *) FilDsc);
   if (status == EOF)
      SysCallError(StdOutFD, "putc(Writech)");
}

void WriteInt(tp_FilDsc FilDsc, int Int)
{
   int status;

   FORBIDDEN(FilDsc == ERROR);
   status = fprintf((FILE *) FilDsc, "%d", Int);
   if (status == EOF)
      SysCallError(StdOutFD, "fprintf(WriteInt)");
}

void Writeln(tp_FilDsc FilDsc, const char *Str)
{
   int status;

   FORBIDDEN(FilDsc == ERROR || Str == ERROR);
   status = fputs(Str, (FILE *) FilDsc);
   if (status == EOF)
      SysCallError(StdOutFD, "fputs(Writeln)");
   status = putc('\n', (FILE *) FilDsc);
   if (status == EOF)
      SysCallError(StdOutFD, "putc(Writeln)");
}

void WriteLine(tp_FilDsc FilDsc, tp_Str Str)
{
   int status;
   tp_Str StrPtr;

   FORBIDDEN(FilDsc == ERROR || Str == ERROR);
   for (StrPtr = Str; *StrPtr != 0; StrPtr++) {
      if (*StrPtr == '\n') {
         status = putc('\\', (FILE *) FilDsc);
         if (status == EOF)
            SysCallError(StdOutFD, "putc(WriteLine)");
      };
      status = putc(*StrPtr, (FILE *) FilDsc);
      if (status == EOF)
         SysCallError(StdOutFD, "putc(WriteLine)");
   };
   status = putc('\n', (FILE *) FilDsc);
   if (status == EOF)
      SysCallError(StdOutFD, "putc(WriteLine)");
}

int Readch(tp_FilDsc FilDsc)
{
   return getc((FILE *) FilDsc);
}

tp_Str Readln(tp_Str StrBuf, tp_FilDsc FilDsc)
{
   tp_Str OutStr;
   int Length;

   if (FilDsc == ERROR)
      return ERROR;
   OutStr = fgets(StrBuf, MAX_Str, (FILE *) FilDsc);
   if (OutStr == NULL) {
      return ERROR;
   }
   Length = strlen(StrBuf);
   if (Length > 0 && StrBuf[Length - 1] == '\n') {
      StrBuf[Length - 1] = 0;
   }
   if (Length == (MAX_Str - 1)) {
      StrBuf[100] = 0;
      SystemError("Line too long, truncated:\n%s\n", StrBuf);
   }
   return StrBuf;
}

tp_Str ReadLine(tp_Str StrBuf, tp_FilDsc FilDsc)
{
   tp_Str OutStr;
   int Length;

   if (FilDsc == ERROR)
      return ERROR;
   OutStr = fgets(StrBuf, MAX_Str, (FILE *) FilDsc);
   if (OutStr == NULL) {
      return ERROR;
   }
   Length = strlen(StrBuf);
   while (Length > 1 && Length < (MAX_Str - 1)
          && StrBuf[Length - 2] == '\\' && StrBuf[Length - 1] == '\n') {
      StrBuf[Length - 2] = '\n';
      OutStr =
          fgets(&StrBuf[Length - 1], MAX_Str - Length, (FILE *) FilDsc);
      if (OutStr == NULL) {
         return ERROR;
      }
      Length = strlen(StrBuf);
   }
   if (Length > 0 && StrBuf[Length - 1] == '\n') {
      StrBuf[Length - 1] = 0;
   }
   if (Length == (MAX_Str - 1)) {
      StrBuf[100] = 0;
      SystemError("Line too long, truncated:\n%s\n", StrBuf);
   }
   return StrBuf;
}

boolean Equal(tp_FilDsc FilDsc1, tp_FilDsc FilDsc2)
{
   int ichar1, ichar2;

   FORBIDDEN(FilDsc1 == ERROR || FilDsc2 == ERROR);
   ichar1 = getc((FILE *) FilDsc1);
   ichar2 = getc((FILE *) FilDsc2);
   while (ichar1 == ichar2 && ichar1 != EOF) {
      ichar1 = getc((FILE *) FilDsc1);
      ichar2 = getc((FILE *) FilDsc2);
   }
   return (ichar1 == ichar2);
}

void FileCopy(tp_FilDsc OutFD, tp_FilDsc InFD)
{
   int ichar, status;

   FORBIDDEN(OutFD == ERROR || InFD == ERROR);
   ichar = getc((FILE *) InFD);
   while (ichar != EOF) {
      status = putc((char) ichar, (FILE *) OutFD);
      if (status == EOF)
         SysCallError(StdOutFD, "putc(FileCopy)");
      ichar = getc((FILE *) InFD);
   }
}
