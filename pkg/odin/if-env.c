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
#include <sys/types.h>
#include <sys/stat.h>

#include "inc/GMC.h"
#include "inc/EnvVar.h"
#include "inc/FileName.h"
#include "inc/Job.h"
#include "inc/Str.h"

extern int num_EnvVarS;
extern tp_EnvVar EnvVarS;

tp_Str RBS_Cmd;
boolean ShortCacheNameFlag;
boolean LocalIPCFlag;

tp_FileName OdinDirName;
tp_FileName CacheDirName;
tp_FileName JobsDirName;

static tp_Str *EnvVarDefS;

void Get_SocketFileName(tp_FileName FileName)
{
   size_t sz;

   sz = snprintf(FileName, MAX_FileName, "%s/SOCKET", OdinDirName);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/SOCKET\n",
                     MAX_FileName, OdinDirName);
      exit(1);
   }
}

void Get_DGFileName(tp_FileName FileName)
{
   size_t sz;

   sz = snprintf(FileName, MAX_FileName, "%s/DG", OdinDirName);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/DG\n",
                     MAX_FileName, OdinDirName);
      exit(1);
   }
}

void Get_PkgDirName(tp_FileName FileName, tp_Package Package)
{
   size_t sz;

   sz = snprintf(FileName, MAX_FileName, "%s/PKGS/%s", OdinDirName,
                 Package);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/PKGS/%s\n",
                     MAX_FileName, OdinDirName, Package);
      exit(1);
   }
}

void Get_InfoFileName(tp_FileName FileName)
{
   size_t sz;

   sz = snprintf(FileName, MAX_FileName, "%s/INFO", OdinDirName);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/INFO\n",
                     MAX_FileName, OdinDirName);
      exit(1);
   }
}

void Get_DebugFileName(tp_FileName FileName)
{
   size_t sz;

   sz = snprintf(FileName, MAX_FileName, "%s/DEBUG", OdinDirName);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/DEBUG\n",
                     MAX_FileName, OdinDirName);
      exit(1);
   }
}

void
Get_WorkFileName(tp_FileName WorkFileName, tp_Job Job, tp_FilHdr FilHdr)
{
   size_t sz;
   tp_FilHdr OutFilHdr;
   tp_FTName FTName;

   OutFilHdr = Copy_FilHdr(FilHdr);
   if (IsInstance(OutFilHdr))
      OutFilHdr = FilHdr_Father(OutFilHdr);
   FTName = FilTyp_ShortFTName(FilHdr_FilTyp(OutFilHdr));
   sz = snprintf(WorkFileName, MAX_FileName, "%s/%s", Job->JobDirName,
                 FTName);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/%s\n",
                     MAX_FileName, Job->JobDirName, FTName);
      exit(11);
   }
   Ret_FilHdr(OutFilHdr);
}

void JobID_LogFileName(tp_FileName LogFileName, int JobID)
{
   size_t sz;

   sz = snprintf(LogFileName, MAX_FileName, "%s/LOG%d", JobsDirName,
                 JobID);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/LOG/%d\n",
                     MAX_FileName, JobsDirName, JobID);
      exit(1);
   }
}

void Local_ShutDown(void)
{
   CurrentClient = LocalClient;
   Exit(0);
}

static void Read_Env(tp_FileName FileName)
{
   tp_FilDsc FilDsc;
   tp_Str Str;
   tps_Str StrBuf;
   int count, i, status;

   FilDsc = FileName_RFilDsc(FileName, FALSE);
   if (FilDsc == ERROR) {
      Writeln(StdOutFD, "Using bootstrap derivation graph.");
      EnvVarDefS =
          (tp_Str *) malloc((unsigned) (num_EnvVarS * sizeof(tp_Str)));
      for (i = 0; i < num_EnvVarS; i += 1) {
         Str = GetEnv(EnvVarS[i].Name);
         if (Str == NIL)
            Str = EnvVarS[i].Default;
         (void) sprintf(StrBuf, "%s=%s", EnvVarS[i].Name, Str);
         EnvVarDefS[i] = Malloc_Str(StrBuf);
         status = putenv(EnvVarDefS[i]);
         FORBIDDEN(status != 0);
      }
      return;
   }
   count = fscanf((FILE *) FilDsc, "%d\n", &num_EnvVarS);
   FORBIDDEN(count != 1);
   EnvVarDefS =
       (tp_Str *) malloc((unsigned) (num_EnvVarS * sizeof(tp_Str)));
   for (i = 0; i < num_EnvVarS; i += 1) {
      count = fscanf((FILE *) FilDsc, "%[^\1]\1\n", StrBuf);
      FORBIDDEN(count != 1);
      EnvVarDefS[i] = Malloc_Str(StrBuf);
      status = putenv(EnvVarDefS[i]);
      FORBIDDEN(status != 0);
   }
   Close(FilDsc);
}

void Init_Env(void)
{
   tps_FileName FileName;
   boolean Abort;
   size_t sz;

   OdinDirName = GetEnv("ODINCACHE");
   FORBIDDEN(OdinDirName == NIL);
   if (!IsDirectory_FileName(OdinDirName)) {
      SystemError("Odin root <%s> does not exist.\n", OdinDirName);
      FATALERROR("");
   }
   if (OdinDirName[0] != '/') {
      SystemError("Odin cache pathname <%s> must be absolute.\n",
                  OdinDirName);
      FATALERROR("");
   }
   Set_ModeMask(OdinDirName);

   sz = snprintf(FileName, MAX_FileName, "%s/FILES", OdinDirName);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/FILES\n",
                     MAX_FileName, OdinDirName);
      exit(1);
   }
   CacheDirName = Malloc_Str(FileName);
   MakeDirFile(&Abort, CacheDirName);
   if (Abort)
      FATALERROR("cannot create odin FILES directory");

   sz = snprintf(FileName, MAX_FileName, "%s/JOBS", OdinDirName);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/JOBS\n",
                     MAX_FileName, OdinDirName);
      exit(1);
   }
   JobsDirName = Malloc_Str(FileName);
   MakeDirFile(&Abort, JobsDirName);
   if (Abort)
      FATALERROR("cannot create odin JOBS directory");

   sz = snprintf(FileName, MAX_FileName, "%s/ENV", OdinDirName);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/ENV\n",
                     MAX_FileName, OdinDirName);
      exit(1);
   }
   Read_Env(FileName);

   DumpCore = (GetEnv("DUMPCORE") != NIL);
   RBS_Cmd = GetEnv("ODIN_RBSCMD");
   FORBIDDEN(RBS_Cmd == NIL);
   ShortCacheNameFlag = (GetEnv("ODIN_SHORTNAMES") != NIL);
   LocalIPCFlag = (GetEnv("ODIN_LOCALIPC") != NIL);
}

void Write_ENV2(void)
{
   tps_FileName FileName;
   tp_FilDsc FilDsc;
   tp_Str Str;
   tps_Str StrBuf;
   tp_EnvVar EnvVar;
   int i;
   tp_FilHdr FilHdr;
   size_t sz;

   sz = snprintf(FileName, MAX_FileName, "%s/ENV2", OdinDirName);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/ENV2\n",
                     MAX_FileName, OdinDirName);
      exit(1);
   }
   FilDsc = FileName_WFilDsc(FileName, FALSE);
   if (FilDsc == ERROR) {
      SystemError("Cannot open ENV2 file.\n");
      exit(1);
   }
   for (i = 0; i < num_EnvVarS; i += 1) {
      EnvVar = &EnvVarS[i];
      if (EnvVar->IsFile) {
         Str = GetEnv(EnvVar->Name);
         FORBIDDEN(Str == NIL);
         FilHdr = OdinExpr_FilHdr(Str);
         if (FilHdr == ERROR) {
            SystemError("Value of $%s is not a legal Odin expression.\n",
                        EnvVar->Name);
            exit(1);
         }
         FilHdr_DataFileName(StrBuf, FilHdr);
         Ret_FilHdr(FilHdr);
         (void) fprintf((FILE *) FilDsc, "%s=%s\1\n", EnvVar->Name,
                        StrBuf);
      }
   }
   Close(FilDsc);
}

void Read_ENV2(void)
{
   tps_FileName FileName;
   tp_FilDsc FilDsc;
   tps_Str StrBuf;
   tp_Str Str;
   int count, status;
   size_t sz;

   sz = snprintf(FileName, MAX_FileName, "%s/ENV2", OdinDirName);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/ENV2\n",
                     MAX_FileName, OdinDirName);
      exit(1);
   }
   FilDsc = FileName_RFilDsc(FileName, FALSE);
   FORBIDDEN(FilDsc == ERROR);
   for (count = fscanf((FILE *) FilDsc, "%[^\1]\1\n", StrBuf);
        count == 1;
        count = fscanf((FILE *) FilDsc, "%[^\1]\1\n", StrBuf)) {
      Str = Malloc_Str(StrBuf);
      status = putenv(Str);
      FORBIDDEN(status != 0);
   }
   FORBIDDEN(!EndOfFile(FilDsc));
   Close(FilDsc);
}

boolean IsDef_EnvVar(tp_Str Name)
{
   int Len, i;

   Len = strlen(Name);
   for (i = 0; i < num_EnvVarS; i += 1) {
      if (strncmp(Name, EnvVarDefS[i], Len) == 0
          && EnvVarDefS[i][Len] == '=') {
         return TRUE;
      }
   }
   return FALSE;
}

static tps_FileName _CWDirName;
tp_FileName CWDirName = _CWDirName;

void Init_CWD(void)
{
   tp_Str Home, PWD;
   boolean Abort;
   tps_Str RawCWDirName;

   GetWorkingDir(&Abort, RawCWDirName);
   if (Abort) {
      SystemError("Current working directory name too long.\n");
      Exit(1);
   }

   Do_Alias(OdinDirName, FALSE);

   Home = GetHome("");
   if (Home == NIL)
      Home = GetEnv("HOME");
   if (Home != NIL)
      Do_Alias(Home, FALSE);

   PWD = GetEnv("PWD");
   if (PWD == NIL)
      PWD = GetEnv("cwd");
   if (PWD != NIL && strncmp(PWD, RawCWDirName, strlen(PWD)) != 0) {
      Do_Alias(PWD, FALSE);
   }

   Get_Alias(CWDirName, RawCWDirName);
   Set_CWD(CWDirName);
}

void DeadServerExit(void)
{
   tps_FileName FileName;
   tp_FilDsc FilDsc;
   size_t sz;

   sz = snprintf(FileName, MAX_FileName, "%s/ERR", OdinDirName);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr,
                     "File name too long (MAX_FileName=%d): %s/ERR\n",
                     MAX_FileName, OdinDirName);
      exit(1);
   }
   FilDsc = FileName_RFilDsc(FileName, FALSE);
   {
      if (FilDsc != ERROR) {
         FileCopy(StdErrFD, FilDsc);
         Close(FilDsc);
      } else {
         SystemError("Odin server shut down.\n");
      }
   }
   Exit(1);
}

void Exit(int Status)
{
   tp_Client Client;

   if (IsServer) {
      Do_ToBroadcast();
      IPC_Finish();
      CleanUp();
      FOREACH_CLIENT(Client) {
         if (!Is_LocalClient(Client)) {
            Ret_Client(Client);
         }
      }
      Purge_Clients();
      FORBIDDEN(!Is_LocalClient(CurrentClient));
      Ret_Client(CurrentClient);
      Purge_Clients();
      Ret_FilHdr(RootFilHdr);
      Ret_FilHdr(NetRootFilHdr);
      Ret_FilHdr(OdinDirFilHdr);
      Ret_FilHdr(CacheDirFilHdr);
      Ret_FilHdr(PrmDataDirFilHdr);
      Ret_FilHdr(StrDirFilHdr);
      Ret_FilHdr(NilStrFilHdr);
      Ret_FilHdr(FlagPrmFilHdr);
      Ret_FilHdr(EmptyFilHdr);
      Update_Info();
      Free_FilHdrs();
      Free_FilInps();
      Free_FilElms();
      Close_Info();
      FORBIDDEN(PrmFHdrs_InUse());
      FORBIDDEN(FilHdrs_InUse());
      FORBIDDEN(FilInps_InUse());
      FORBIDDEN(FilElms_InUse());
   }
   exit(Status);
}
