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
#include "inc/Build.h"
#include "inc/LogLevel_.h"
#include "inc/OC_NodTyp_.h"
#include "inc/Str.h"

static tp_Build FirstBuild = NIL;
int MaxBuilds;

static int ChildWaitPID = 0;

#define		MAX_BUILDARGV 100
static int Num_BuildArgV = 0;
static char *BuildArgV[MAX_BUILDARGV];

static tp_Build BuildID_Build(tp_BuildID BuildID)
{
   tp_Build Build;

   for (Build = FirstBuild; Build != NIL; Build = Build->Next) {
      if (BuildID == Build->BuildID) {
         return Build;
      }
   }
   return NIL;
}

tp_Build JobID_Build(tp_JobID JobID)
{
   tp_Build Build;

   for (Build = FirstBuild; Build != NIL; Build = Build->Next) {
      if (JobID == Build->JobID) {
         return Build;
      }
   }
   return NIL;
}

void Extend_Builds(int NumBuilds)
{
   int i;
   tp_Build PrevBuild, Build;

   PrevBuild = NIL;
   Build = FirstBuild;
   for (i = 0; i < NumBuilds; i += 1) {
      if (Build == NIL) {
         Build = (tp_Build) malloc(sizeof(tps_Build));
         Build->JobID = 0;
         Build->BuildID = 0;
         Build->Host = NIL;
         Build->Next = NIL;
         {
            if (PrevBuild == NIL) {
               FirstBuild = Build;
            } else {
               PrevBuild->Next = Build;
            }
         }
      }
      PrevBuild = Build;
      Build = Build->Next;
   }
}

void Set_BuildHosts(boolean * AbortPtr, tp_Str Str)
{
   tp_Nod Nod, Son, HostNod;
   tp_Build Build;
   boolean IsHelp, IsHandled;

   Nod = OC_Parser(Str, (tp_FileName) NIL, (int *) NIL);
   if (Nod == ERROR) {
      *AbortPtr = TRUE;
      return;
   }
   Do_Help(AbortPtr, &IsHelp, &IsHandled, Nod);
   if (*AbortPtr) {
      return;
   }
   if (IsHelp) {
      Writeln(StdOutFD, "?*? A colon separated list of hosts.\n");
      return;
   }
   for (Son = Nod_Son(1, Nod); Son != NIL; Son = Nod_Brother(Son)) {
      HostNod = (Nod_NodTyp(Son) == DRVOPR ? Nod_Son(1, Son) : Son);
      if (Nod_NodTyp(HostNod) != WORD) {
         SystemError
             ("BuildHosts must be a colon separated list of hosts.\n");
         Ret_Nod(Nod);
      }
   }
   Extend_Builds(Nod_NumSons(Nod));
   Son = Nod_Son(1, Nod);
   for (Build = FirstBuild; Build != NIL; Build = Build->Next) {
      {
         if (Son == NIL) {
            Build->Host = NIL;
         } else {
            HostNod = (Nod_NodTyp(Son) == DRVOPR ? Nod_Son(1, Son) : Son);
            Build->Host = Lookup_Host(Sym_Str(Nod_Sym(HostNod)));
            Son = Nod_Brother(Son);
         }
      }
   }
   Ret_Nod(Nod);
}

void Write_BuildHosts(tp_FilDsc FilDsc)
{
   boolean IsFirst;
   tp_Build Build;
   int i;

   IsFirst = TRUE;
   Build = FirstBuild;
   for (i = 0; i < MaxBuilds; i++) {
      FORBIDDEN(Build == NIL);
      if (!IsFirst)
         Write(FilDsc, " : ");
      IsFirst = FALSE;
      Write(FilDsc,
            Build->Host == NIL ? "LOCAL" : Host_HostName(Build->Host));
      Build = Build->Next;
   }
   Writeln(FilDsc, "");
}

void Local_Add_BuildArg(tp_FileName Arg)
{
   FORBIDDEN((Num_BuildArgV + 2) > MAX_BUILDARGV);
   if (BuildArgV[Num_BuildArgV] != NIL)
      free(BuildArgV[Num_BuildArgV]);
   BuildArgV[Num_BuildArgV] = Malloc_Str(Arg);
   Num_BuildArgV += 1;
}

void
Local_Do_Build(tp_JobID JobID,
               tp_FileName JobDirName, tp_FileName LogFileName)
{
   tp_Build Build;
   int i;
   tps_Str OldCWD;
   boolean Abort;
   tp_FileName FileName;

   if (BuildArgV[Num_BuildArgV] != NIL)
      free(BuildArgV[Num_BuildArgV]);
   BuildArgV[Num_BuildArgV] = 0;
   Num_BuildArgV = 0;

   i = 0;
   for (Build = FirstBuild; Build->JobID != NIL; Build = Build->Next) {
      i += 1;
      FORBIDDEN(i > MaxBuilds);
   }
   FORBIDDEN(Build->BuildID != NIL);
   Build->JobID = JobID;
   if (LogLevel >= LOGLEVEL_ExecLine) {
      (void) fprintf((FILE *) StdOutFD, "** Executing :");
      for (i = 0; BuildArgV[i] != NIL; i += 1) {
         (void) fprintf((FILE *) StdOutFD, " '%s'", BuildArgV[i]);
      }
      (void) fprintf((FILE *) StdOutFD, "\n");
   }
   if (Build->Host != NIL) {
      RBS_Do_Build(Build->Host, JobID, JobDirName, LogFileName, BuildArgV);
      return;
   }
   (void) strcpy(OldCWD, CWDirName);
   ChangeDir(&Abort, JobDirName);
   FORBIDDEN(Abort);
   ClearDir(JobDirName);
   FileName = (MaxBuilds > 1 ? LogFileName : NIL);
   Build->BuildID = SystemExec(BuildArgV[0], BuildArgV, FileName);
   ChangeDir(&Abort, OldCWD);
   FORBIDDEN(Abort);
}

void Local_Abort_Build(tp_JobID JobID)
{
   tp_Build Build;

   Build = JobID_Build(JobID);
   if (Build == NIL) {
      if (LogLevel >= LOGLEVEL_AbortDone) {
         Writeln(StdOutFD, "Job to be aborted already completed.");
      }
      return;
   }
   if (Build->Host == NIL) {
      SystemInterrupt(Build->BuildID);
      return;
   }
   RBS_Abort_Build(Build->Host, JobID);
}

static boolean Is_ActiveBuild(void)
{
   tp_Build Build;

   for (Build = FirstBuild; Build != NIL; Build = Build->Next) {
      if (Build->BuildID != 0) {
         return TRUE;
      }
   }
   return FALSE;
}

void
SystemExecCmdWait(boolean * AbortPtr, const char *Cmd, boolean Interactive)
{
   FORBIDDEN(ChildWaitPID != 0);
   ChildWaitPID = SystemExecCmd(Cmd, Interactive);
   if (ChildWaitPID <= 0) {
      ChildWaitPID = 0;
      *AbortPtr = TRUE;
      return;
   }
   IPC_Get_Commands(AbortPtr, (char *) NIL);
}

void ChildAction(boolean * AbortPtr, boolean * DonePtr)
{
   tp_BuildID BuildID;
   boolean Abort;

   *AbortPtr = FALSE;
   *DonePtr = FALSE;
   while (Is_ActiveBuild() || ChildWaitPID != 0) {
      SystemWait(&BuildID, &Abort);
      if (BuildID == 0) {
         return;
      }
      {
         if (BuildID == ChildWaitPID) {
            ChildWaitPID = 0;
            *AbortPtr = Abort;
            *DonePtr = TRUE;
         } else if (IsServerPId(BuildID)) {
            DeadServerExit();
         } else if (PId_Host(BuildID) != NIL) {
            RBS_Done(PId_Host(BuildID));
         } else {
            Build_Done(BuildID_Build(BuildID), Abort);
         }
      }
   }
}

void Cancel_Builds(tp_Host Host)
{
   tp_Build Build;

   for (Build = FirstBuild; Build != NIL; Build = Build->Next) {
      if (Build->Host == Host && Build->JobID != 0) {
         Build_Done(Build, TRUE);
      }
   }
}

void Build_Done(tp_Build Build, boolean Abort)
{
   tps_Str LogFileName;
   tp_FilDsc LogFD;

   FORBIDDEN(Build == NIL || Build->JobID == 0);
   if (MaxBuilds > 1 || Build->Host != NIL) {
      JobID_LogFileName(LogFileName, Build->JobID);
      LogFD = FileName_RFilDsc(LogFileName, TRUE);
      FileCopy(StdOutFD, LogFD);
      Flush(StdOutFD);
      Close(LogFD);
   }
   Job_Done(Build->JobID, Abort);
   Build->BuildID = 0;
   Build->JobID = 0;
}

void Local_Do_MakeReadOnly(tp_FileName FileName)
{
   boolean Abort;

   MakeReadOnly(&Abort, FileName);
   if (Abort) {
      SystemError("\"%s\": could not remove write permission.\n",
                  FileName);
   }
}
