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
#include <errno.h>
#include <string.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "inc/GMC.h"
#include "inc/Str.h"

extern char *strerror(int);

extern tp_Str Author;

int num_Errors = 0;

boolean DumpCore;

boolean Is_IPC_Err = FALSE;

static tp_FilDsc ErrFD;
static tp_FileName ErrFileName;

void Init_Err(void)
{
   ErrFileName = NIL;
   ErrFD = StdErrFD;
   Reset_Err();
}

void Set_IPC_Err(boolean Flag)
{
   Is_IPC_Err = Flag;
}

void Set_ErrFile(tp_FileName FileName, boolean IsIPC, tp_FilDsc FilDsc)
{
   FORBIDDEN(FileName != NIL && IsIPC);
   if (FileName == ErrFileName && IsIPC == Is_IPC_Err && FilDsc == ErrFD) {
      return;
   }
   if (ErrFD != NIL && ErrFD != StdOutFD && ErrFD != StdErrFD)
      Close(ErrFD);
   ErrFileName = FileName;
   Is_IPC_Err = IsIPC;
   ErrFD = FilDsc;
}

void
Save_ErrFile(tp_FileName * FileNamePtr,
             boolean * IsIPC_Ptr, tp_FilDsc * FilDscPtr)
{
   *FileNamePtr = ErrFileName;
   *IsIPC_Ptr = Is_IPC_Err;
   *FilDscPtr = ErrFD;
}

boolean IsErr(void)
{
   FORBIDDEN(ErrFD == StdOutFD || ErrFD == StdErrFD);
   return (ErrFD != NIL);
}

void Reset_Err(void)
{
   num_Errors = 0;
}

void Increment_Errors(void)
{
   num_Errors += 1;
}

int Num_Errors(void)
{
   return num_Errors;
}

#if NO_STRERROR
extern char *sys_errlist[];
char *strerror(int num)
{
   return sys_errlist[num];
}
#endif

void SysCallError(tp_FilDsc FilDsc, char *Message)
{
   (void) fprintf((FILE *) FilDsc, "%s: %s.\n", Message, strerror(errno));
   (void) fflush((FILE *) FilDsc);
}

void FatalError(char *Message, char *FileName, int LineNum)
{
   tps_Str Msg;

   (void) sprintf(Msg, "\"%s\", line %d: %s", FileName, LineNum, Message);
   fatal_err(Msg);
}

#ifdef __STDC__

void SystemError(char *Fmt, ...)
/*VARARGS 1*/
{
   va_list Args;
   tps_Str Message;

   va_start(Args, Fmt);
   Increment_Errors();
   (void) vsprintf(Message, Fmt, Args);
   {
      if (Is_IPC_Err) {
         ErrMessage(Message);
      } else {
         Local_ErrMessage(Message);
      }
   }
   va_end(Args);
}

#else

void SystemError(va_alist)
/*VARARGS 1*/
va_dcl
{
   va_list Args;
   tp_Str Fmt;
   tps_Str Message;

   va_start(Args);
   Fmt = va_arg(Args, char *);
   Increment_Errors();
   (void) vsprintf(Message, Fmt, Args);
   {
      if (Is_IPC_Err) {
         ErrMessage(Message);
      } else {
         Local_ErrMessage(Message);
      }
   }
   va_end(Args);
}

#endif

#ifdef NO_IPC
void ErrMessage(tp_Str Message)
{
   Local_ErrMessage(Message);
}
#endif

void Local_ErrMessage(tp_Str Message)
{
   int status;

   if (ErrFD == NIL) {
      FORBIDDEN(ErrFileName == NIL);
      ErrFD = FileName_WFilDsc(ErrFileName, FALSE);
      if (ErrFD == ERROR) {
         status =
             fputs("!! Could not open error file !!", (FILE *) StdErrFD);
         if (status == EOF)
            SysCallError(StdOutFD, "fputs(Local_ErrMessage)");
         ErrFD = StdErrFD;
      }
   }
   status = fputs(Message, (FILE *) ErrFD);
   if (status == EOF)
      SysCallError(StdOutFD, "fputs(Local_ErrMessage)");
   (void) fflush((FILE *) ErrFD);
}

void fatal_err(char *Message)
{
   (void) fprintf(stderr, "%s\n", Message);
   (void) fprintf(stderr, "Anomalous Internal State Detected\n");
   (void) fprintf(stderr, "please mail description to %s\n", Author);
   if (DumpCore) {
      (void) fprintf(stderr,
                     "'illegal instruction' issued to generate core for analysis\n");
      abort();
   }
   exit(1);
}
