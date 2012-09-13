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
#include "inc/Client.h"
#include "inc/FileName.h"
#include "inc/Flag_.h"
#include "inc/InpKind_.h"
#include "inc/Job.h"
#include "inc/LogLevel_.h"
#include "inc/Status_.h"
#include "inc/Str.h"

tp_Client FirstClient = NIL;
tp_Client CurrentClient = NIL;
tp_Client LocalClient = NIL;

typedef struct _tps_Pending {
   tp_FilHdr FilHdr;
   tp_InpKind InpKind;
   tp_Date ModDate, ElmModDate, ElmNameModDate;
   tp_Pending Next;
} tps_Pending;

static tp_Pending FreePendingS = NIL;
static int num_PendingS = 0;

static tp_Pending FirstPending = NIL;
static tp_Pending LastPending = NIL;

static int UseCount = 0;

static tp_Client FreeClient = NIL;
static int num_ClientS = 0;

static tp_Job FreeJob = NIL;
static int num_JobS = 0;

typedef struct _tps_FHLst {
   tp_FilHdr FilHdr;
   tp_FHLst Next;
} tps_FHLst;

static tp_FHLst FreeFHLstS = NIL;
int num_FHLstS = 0;

void Push_Pending(tp_FilHdr FilHdr, tp_InpKind InpKind)
{
   tp_Pending Pending;

   if (FilHdr_Flag(FilHdr, FLAG_Pending)) {
      Ret_FilHdr(FilHdr);
      return;
   }
   Set_Flag(FilHdr, FLAG_Pending);
   Do_Log("Queuing delayed processing for", FilHdr, LOGLEVEL_Queue);
   {
      if (FreePendingS == NIL) {
         Pending = (tp_Pending) malloc(sizeof(tps_Pending));
         num_PendingS += 1;
      } else {
         Pending = FreePendingS;
         FreePendingS = FreePendingS->Next;
      }
   }
   Pending->FilHdr = FilHdr;
   Pending->InpKind = InpKind;
   Pending->ModDate = FilHdr_ModDate(FilHdr);
   Pending->ElmModDate = FilHdr_ElmModDate(FilHdr);
   Pending->ElmNameModDate = FilHdr_ElmNameModDate(FilHdr);
   Pending->Next = NIL;
   {
      if (FirstPending == NIL) {
         FirstPending = Pending;
      } else {
         LastPending->Next = Pending;
      }
   }
   LastPending = Pending;
}

static boolean IsAllPendingUpToDate(void)
{
   tp_Pending Pending;

   for (Pending = FirstPending; Pending != NIL; Pending = Pending->Next) {
      if (!IsAllUpToDate(Pending->FilHdr, Pending->InpKind)) {
         return FALSE;
      }
   }
   return TRUE;
}

static void GetAllPending(void)
{
   tp_Pending Pending;
   tp_Status Status;

   for (Pending = FirstPending; Pending != NIL; Pending = Pending->Next) {
      CurrentClient->LastToDo = NIL;
      Do_Log("Starting delayed processing for", Pending->FilHdr,
             LOGLEVEL_Queue);
      GetAllReqs(Pending->FilHdr, Pending->InpKind);
      Status = FilHdr_MinStatus(Pending->FilHdr, Pending->InpKind);
      {
         if (Pending->ModDate != FilHdr_ModDate(Pending->FilHdr)
             || Pending->ElmModDate != FilHdr_ElmModDate(Pending->FilHdr)
             || (Pending->ElmNameModDate
                 != FilHdr_ElmNameModDate(Pending->FilHdr))) {
            Pending->ModDate = FilHdr_ModDate(Pending->FilHdr);
            Pending->ElmModDate = FilHdr_ElmModDate(Pending->FilHdr);
            Pending->ElmNameModDate =
                FilHdr_ElmNameModDate(Pending->FilHdr);
            Do_Log("Changed cycle input:", Pending->FilHdr,
                   LOGLEVEL_Circular);
            Broadcast(Pending->FilHdr, STAT_Unknown);
         } else if (Status != STAT_Unknown && Status <= STAT_TgtValError
                    && !Is_PRB_Status(Status)) {
            Broadcast(Pending->FilHdr, STAT_TgtValError);
         }
      }
   }
}

static void Clr_Pending(boolean * AllDonePtr)
{
   tp_FilHdr FilHdr;
   tp_Pending Pending, LastPending;

   *AllDonePtr = TRUE;
   if (FirstPending == NIL) {
      return;
   }
   for (Pending = FirstPending; Pending != NIL; Pending = Pending->Next) {
      LastPending = Pending;
      FilHdr = Pending->FilHdr;
      {
         if (!IsAllDone(FilHdr, Pending->InpKind)) {
            *AllDonePtr = FALSE;
            Broadcast(FilHdr, STAT_Pending);
         } else if (Is_TgtValErrStatus(FilHdr)) {
            Broadcast(FilHdr, STAT_TgtValError);
         }
      }
      Clr_Flag(FilHdr, FLAG_Pending);
      Ret_FilHdr(FilHdr);
   }
   LastPending->Next = FreePendingS;
   FreePendingS = FirstPending;
   FirstPending = NIL;
}

static tp_FHLst New_FHLst(tp_FilHdr FilHdr)
{
   tp_FHLst NewFHLst;

   FORBIDDEN(FilHdr == ERROR);
   {
      if (FreeFHLstS == NIL) {
         NewFHLst = (tp_FHLst) malloc(sizeof(tps_FHLst));
         num_FHLstS += 1;
      } else {
         NewFHLst = FreeFHLstS;
         FreeFHLstS = FreeFHLstS->Next;
      }
   }
   NewFHLst->FilHdr = FilHdr;
   NewFHLst->Next = NIL;
   return NewFHLst;
}

static void Ret_FHLst(tp_FHLst FHLst)
{
   tp_FHLst TailFHLst;

   if (FHLst == NIL) {
      return;
   }
   TailFHLst = FHLst;
   while (TRUE) {
      Ret_FilHdr(TailFHLst->FilHdr);
      TailFHLst->FilHdr = NIL;
      if (TailFHLst->Next == NIL) {
         TailFHLst->Next = FreeFHLstS;
         FreeFHLstS = FHLst;
         return;
      }
      TailFHLst = TailFHLst->Next;
   }
}

void Ret_ToDo(void)
{
   Ret_FHLst(CurrentClient->ToDo);
   CurrentClient->ToDo = NIL;
   CurrentClient->LastToDo = NIL;
}

tp_Client New_Client(tp_ClientID ClientID)
{
   tp_Client Client;

   ; {
      if (FreeClient == NIL) {
         Client = (tp_Client) malloc(sizeof(tps_Client));
         num_ClientS += 1;
         Client->InUse = FALSE;
      } else {
         Client = FreeClient;
         FreeClient = FreeClient->Next;
      }
   }

   Client->ClientID = ClientID;
   Client->KeepGoing = FALSE;
   Client->ErrLevel = 0;
   Client->WarnLevel = 0;
   Client->LogLevel = 0;
   Client->HelpLevel = 0;
   Client->CWDFilHdrS = NIL;
   Client->FilHdr = NIL;
   Client->ToDo = NIL;
   Client->LastToDo = NIL;
   Client->NumJobs = 0;
   Client->MaxJobs = 0;
   Client->Job = NIL;
   Client->Interrupted = FALSE;
   Client->Next = FirstClient;
   FirstClient = Client;
   FORBIDDEN(Client->InUse);
   Client->InUse = TRUE;
   return Client;
}

void Activate_Client(tp_Client Client)
{
   tp_Client OldCurrentClient;

   OldCurrentClient = CurrentClient;
   CurrentClient = Client;
   Push_ContextFilHdr(Copy_FilHdr(OdinDirFilHdr));
   CurrentClient = OldCurrentClient;
   UseCount += 1;
}

void Ret_Client(tp_Client Client)
{
   FORBIDDEN(!Client->InUse);
   FORBIDDEN(Is_LocalClient(Client) && UseCount > 1);
   Client->InUse = FALSE;
}

void Purge_Clients(void)
{
   boolean Changed;
   tp_Client PrevClient, Client, NextClient, OldCurrentClient;
   tp_Job Job, NextJob;

   Changed = FALSE;
   PrevClient = NIL;
   Client = FirstClient;
   while (Client != NIL) {
      NextClient = Client->Next;
      {
         if (Client->InUse) {
            PrevClient = Client;
         } else {
            Changed = TRUE;
            Client->Next = FreeClient;
            FreeClient = Client;
            {
               if (PrevClient == NIL) {
                  FORBIDDEN(FirstClient != Client);
                  FirstClient = NextClient;
               } else {
                  PrevClient->Next = NextClient;
               }
            }
            IPC_Close(Client->ClientID);
            Client->ClientID = -1;
            UseCount -= 1;
            Set_Client_FilHdr(Client, (tp_FilHdr) NIL, FALSE);

            OldCurrentClient = CurrentClient;
            CurrentClient = Client;
            Job = CurrentClient->Job;
            while (Job != NIL) {
               NextJob = Job->Next;
               Local_Job_Done(Job->JobID, TRUE);
               Job = NextJob;
            }
            FORBIDDEN(CurrentClient->Job != NIL);
            Ret_ToDo();
            while (CurrentClient->CWDFilHdrS != NIL) {
               Pop_ContextFilHdr();
            }
            CurrentClient = OldCurrentClient;
         }
      }
      Client = NextClient;
   }
   if (Changed) {
      if (!IsClient && UseCount == 1) {
         Exit(0);
      }
   }
}

boolean Is_ActiveClient(tp_Client Client)
{
   return (Client->InUse);
}

int Client_FD(tp_Client Client)
{
   return (int) Client->ClientID;
}

boolean Client_Interrupted(tp_Client Client)
{
   return Client->Interrupted;
}

void Set_Client_Interrupted(tp_Client Client, boolean Flag)
{
   FORBIDDEN(Client == NIL);
   Client->Interrupted = Flag;
}

boolean Client_KeepGoing(tp_Client Client)
{
   return Client->KeepGoing;
}

int Client_ErrLevel(tp_Client Client)
{
   return Client->ErrLevel;
}

int Client_WarnLevel(tp_Client Client)
{
   return Client->WarnLevel;
}

tp_LogLevel Client_LogLevel(tp_Client Client)
{
   return Client->LogLevel;
}

tp_FilHdr Client_FilHdr(tp_Client Client)
{
   return Copy_FilHdr(Client->FilHdr);
}

void
Set_Client_FilHdr(tp_Client Client, tp_FilHdr FilHdr, boolean NeedsData)
{
   if (Client->FilHdr != NIL) {
      Ret_FilHdr(Client->FilHdr);
   }
   Client->FilHdr = Copy_FilHdr(FilHdr);
   Client->NeedsData = NeedsData;
}

boolean Client_NeedsData(tp_Client Client)
{
   return Client->NeedsData;
}

void Push_AllReqs(boolean * AllDonePtr)
{
   CurrentDate += 1;
   PendingDate = CurrentDate;
   while (!(IsAllUpToDate(CurrentClient->FilHdr, IK_Trans)
            && IsAllPendingUpToDate())) {
      GetAllReqs(CurrentClient->FilHdr, IK_Trans);
      GetAllPending();
   }
   Clr_Pending(AllDonePtr);
   *AllDonePtr = *AllDonePtr && IsAllDone(CurrentClient->FilHdr, IK_Trans);
   CurrentClient->LastToDo = CurrentClient->ToDo;
}

tp_FHLst Client_ToDo(tp_Client Client)
{
   return Client->ToDo;
}

void Push_ToDo(tp_FilHdr FilHdr)
{
   tp_FHLst FHLst;

   Do_Log("Queuing", FilHdr, LOGLEVEL_Queue);
   FHLst = New_FHLst(FilHdr);
   {
      if (CurrentClient->LastToDo == NIL) {
         FHLst->Next = CurrentClient->ToDo;
         CurrentClient->ToDo = FHLst;
      } else {
         FHLst->Next = CurrentClient->LastToDo->Next;
         CurrentClient->LastToDo->Next = FHLst;
      }
   }
   CurrentClient->LastToDo = FHLst;
}

static tp_FHLst ToBroadcast = NIL;

void Push_ToBroadcast(tp_FilHdr FilHdr)
{
   tp_FHLst FHLst;

   Do_Log("Queuing for broadcast", FilHdr, LOGLEVEL_Queue);
   FHLst = New_FHLst(FilHdr);
   FHLst->Next = ToBroadcast;
   ToBroadcast = FHLst;
}

void Do_ToBroadcast(void)
{
   tp_FHLst FHLst;
   tp_FilHdr FilHdr;

   for (FHLst = ToBroadcast; FHLst != NIL; FHLst = FHLst->Next) {
      FilHdr = FHLst->FilHdr;
      Broadcast(FilHdr, STAT_Unknown);
   }
   Ret_FHLst(ToBroadcast);
   ToBroadcast = NIL;
}

tp_Job Client_Job(tp_Client Client)
{
   return Client->Job;
}

tp_Client Client_Next(tp_Client Client)
{
   return Client->Next;
}

boolean Is_LocalClient(tp_Client Client)
{
   return (Client == LocalClient);
}

boolean Is_ServerAction(void)
{
   return (IsServer && CurrentClient->FilHdr != NIL);
}

tp_Job New_Job(void)
{
   tp_Job Job;
   tps_Str Str;
   boolean Abort;

   {
      if (FreeJob == NIL) {
         Job = (tp_Job) malloc(sizeof(tps_Job));
         num_JobS += 1;
         Job->JobID = num_JobS;
         Job->InUse = FALSE;
         (void) sprintf(Str, "%s/JOB%d", JobsDirName, Job->JobID);
         Job->JobDirName = Malloc_Str(Str);
         MakeDirFile(&Abort, Job->JobDirName);
         FORBIDDEN(Abort);
         JobID_LogFileName(Str, Job->JobID);
         MakePlnFile(&Abort, Str);
         FORBIDDEN(Abort);
         (void) sprintf(Str, "%s/WARNINGS", Job->JobDirName);
         Job->WarningFN = Malloc_Str(Str);
         (void) sprintf(Str, "%s/ERRORS", Job->JobDirName);
         Job->ErrorFN = Malloc_Str(Str);
      } else {
         Job = FreeJob;
         FreeJob = FreeJob->Next;
      }
   }

   Job->Canceled = FALSE;
   Job->Next = NIL;
   FORBIDDEN(Job->InUse);
   Job->InUse = TRUE;
   return Job;
}

tp_Job Get_Job(tp_JobID JobID)
{
   tp_Job Job;

   for (Job = Client_Job(CurrentClient); Job != NIL; Job = Job->Next) {
      if (Job->JobID == JobID) {
         return Job;
      }
   }
   return NIL;
}

void Ret_Job(tp_Job Job)
{
   FORBIDDEN(Job == ERROR);
   FORBIDDEN(!Job->InUse);
   Job->InUse = FALSE;
   Job->Next = FreeJob;
   FreeJob = Job;
}

tp_FilHdr Job_FilHdr(tp_Job Job)
{
   return Copy_FilHdr(Job->FilHdr);
}

tp_Job Add_Job(tp_FilHdr FilHdr)
{
   tp_Job Job;

   Job = New_Job();
   Job->FilHdr = Copy_FilHdr(FilHdr);
   Job->Next = CurrentClient->Job;
   CurrentClient->Job = Job;
   CurrentClient->NumJobs += 1;
   FORBIDDEN(CurrentClient->NumJobs > CurrentClient->MaxJobs);
   return Job;
}

void Del_Job(tp_Job Job)
{
   tp_Job PrevJob;

   FORBIDDEN(Job == NIL);
   FORBIDDEN(Is_PRB_Status(FilHdr_Status(Job->FilHdr)) && !Job->Canceled);
   Ret_FilHdr(Job->FilHdr);
   Job->FilHdr = NIL;
   {
      if (CurrentClient->Job == Job) {
         CurrentClient->Job = Job->Next;
      } else {
         PrevJob = CurrentClient->Job;
         while (PrevJob->Next != Job) {
            PrevJob = PrevJob->Next;
            FORBIDDEN(PrevJob == NIL);
         }
         PrevJob->Next = Job->Next;
      }
   }
   CurrentClient->NumJobs -= 1;
   Ret_Job(Job);
}

void Clr_Status(tp_FilHdr FilHdr)
{
   tp_Client OldCurrentClient;
   tp_Job Job;

   if (FilHdr_Status(FilHdr) != STAT_Busy) {
      Do_Log("Clearing status of", FilHdr, LOGLEVEL_Status);
      Set_Status(FilHdr, STAT_Unknown);
      return;
   }
   OldCurrentClient = CurrentClient;
   FOREACH_CLIENT(CurrentClient) {
      for (Job = Client_Job(CurrentClient); Job != NIL; Job = Job->Next) {
         if (Job->FilHdr == FilHdr) {
            if (!Job->Canceled) {
               Do_Log("Canceling", FilHdr, LOGLEVEL_Cancel);
               Job->Canceled = TRUE;
            }
            CurrentClient = OldCurrentClient;
            return;
         }
      }
   }
   FATALERROR("Could not find canceled job");
}

static void Do_1ToDo(boolean * DonePtr)
{
   tp_FHLst FHLst;
   tp_FilHdr FilHdr;

   for (*DonePtr = FALSE;
        CurrentClient->LastToDo != NIL;
        CurrentClient->LastToDo = CurrentClient->LastToDo->Next) {
      if (CurrentClient->NumJobs >= CurrentClient->MaxJobs) {
         return;
      }
      FilHdr = CurrentClient->LastToDo->FilHdr;
      Do_Log("Processing", FilHdr, LOGLEVEL_Process);
      if (FilHdr_Status(FilHdr) == STAT_Ready) {
         Exec(FilHdr);
         if (CurrentClient->Interrupted) {
            return;
         }
      }
   }
   for (FHLst = CurrentClient->ToDo; FHLst != NIL; FHLst = FHLst->Next) {
      if (FilHdr_Status(FHLst->FilHdr) == STAT_Busy) {
         return;
      }
   }
   *DonePtr = TRUE;
}

static void Do_ToDo(boolean * DonePtr)
{
   boolean AllDone;

   Do_1ToDo(DonePtr);
   if (!*DonePtr) {
      return;
   }
   for (Push_AllReqs(&AllDone); !AllDone; Push_AllReqs(&AllDone)) {
      Do_1ToDo(DonePtr);
      if (!*DonePtr) {
         return;
      }
   }
   if (CurrentClient->Job != NIL) {
      *DonePtr = FALSE;
      return;
   }
   Ret_ToDo();
}

boolean Is_TgtValErrStatus(tp_FilHdr FilHdr)
{
   FORBIDDEN(!IsAllDone(FilHdr, IK_Simple));
   return (FilHdr_Status(FilHdr) <= STAT_TgtValError
           && (FilHdr_Status(FilHdr) > STAT_SysAbort || IsCopy(FilHdr)));
}

tp_FilHdr FilHdr_TgtValFilHdr(tp_FilHdr FilHdr)
{
   tp_FilElm FilElm;
   tp_FilHdr TgtValFilHdr;

   FilElm = LocElm_FilElm(FilHdr_TgtValLocElm(FilHdr));
   FORBIDDEN(FilElm != NIL && FilElm_Next(FilElm) != NIL);
   TgtValFilHdr = FilElm_FilHdr(FilElm);
   Ret_FilHdr(FilHdr);
   Ret_FilElm(FilElm);
   return TgtValFilHdr;
}

void ServerAction(void)
{
   boolean Done;

   if (CurrentClient->FilHdr == NIL
       || CurrentClient->NumJobs >= CurrentClient->MaxJobs) {
      return;
   }
   if (CurrentClient->Interrupted) {
      if (CurrentClient->NumJobs == 0) {
         Ret_ToDo();
         End_Get_OdinFile();
      }
      return;
   }
   Do_ToDo(&Done);
   if (Done) {
      End_Get_OdinFile();
   }
}

void Local_Do_Interrupt(boolean InterruptAll)
{
   tp_FilHdr FilHdr;
   tp_Job Job;

   if (CurrentClient->Interrupted) {
      return;
   }
   FilHdr = Client_FilHdr(CurrentClient);
   if (FilHdr == NIL) {
      return;
   }
   IsAny_ReadyServerAction = TRUE;
   CurrentClient->Interrupted = TRUE;
   Ret_FilHdr(FilHdr);
   if (InterruptAll) {
      Job = CurrentClient->Job;
      while (Job != NIL) {
         Abort_Build(Job->JobID);
         Job = Job->Next;
      }
   }
}

boolean IsAny_ServerAction(void)
{
   tp_Client Client;

   FOREACH_CLIENT(Client) {
      if (Client->FilHdr != NIL || Client->Job != NIL) {
         return TRUE;
      }
   }
   return FALSE;
}

tp_FilHdr Top_CWDFilHdr(void)
{
   tp_FilHdr DirFilHdr;

   FORBIDDEN(CurrentClient->CWDFilHdrS == NIL);
   DirFilHdr =
       FilHdr_SrcFilHdr(Copy_FilHdr(CurrentClient->CWDFilHdrS->FilHdr));
   if (!IsDir(DirFilHdr))
      DirFilHdr = FilHdr_DirFilHdr(DirFilHdr);
   return DirFilHdr;
}

tp_FilHdr Top_ContextFilHdr(void)
{
   FORBIDDEN(CurrentClient->CWDFilHdrS == NIL);
   return Copy_FilHdr(CurrentClient->CWDFilHdrS->FilHdr);
}

void Push_ContextFilHdr(tp_FilHdr CWDFilHdr)
{
   tp_FHLst NewCWDFilHdrS;

   FORBIDDEN(CWDFilHdr == ERROR);
   NewCWDFilHdrS = New_FHLst(Deref(CWDFilHdr));
   NewCWDFilHdrS->Next = CurrentClient->CWDFilHdrS;
   CurrentClient->CWDFilHdrS = NewCWDFilHdrS;
}

void Pop_ContextFilHdr(void)
{
   tp_FHLst FHLst;

   FHLst = CurrentClient->CWDFilHdrS;
   FORBIDDEN(FHLst == NIL);
   CurrentClient->CWDFilHdrS = FHLst->Next;
   FHLst->Next = NIL;
   Ret_FHLst(FHLst);
}

void Local_Set_CWD(tp_FileName FileName)
{
   tp_FilHdr FilHdr;

   FilHdr = HostFN_FilHdr(FileName);
   Pop_ContextFilHdr();
   Push_ContextFilHdr(FilHdr);
}

void Local_Push_Context(tp_FileName DirName, tp_FileName FileName)
{
   tp_FilHdr FilHdr;

   FilHdr = DataFileName_FilHdr(FileName);
   FORBIDDEN(FilHdr == ERROR);
   Push_ContextFilHdr(FilHdr);
   FilHdr = Top_CWDFilHdr();
   FilHdr_HostFN(DirName, FilHdr, FALSE);
   Ret_FilHdr(FilHdr);
}

void Local_Pop_Context(tp_FileName DirName)
{
   tp_FilHdr FilHdr;

   Pop_ContextFilHdr();
   FilHdr = Top_CWDFilHdr();
   FilHdr_HostFN(DirName, FilHdr, FALSE);
   Ret_FilHdr(FilHdr);
}

void Local_Set_KeepGoing(boolean Flag)
{
   CurrentClient->KeepGoing = Flag;
}

void Local_Set_ErrLevel(int ErrLevel)
{
   CurrentClient->ErrLevel = ErrLevel;
}

void Local_Set_WarnLevel(int WarnLevel)
{
   CurrentClient->WarnLevel = WarnLevel;
}

void Local_Set_LogLevel(tp_LogLevel LogLevel)
{
   CurrentClient->LogLevel = LogLevel;
}

void Local_Set_HelpLevel(int HelpLevel)
{
   CurrentClient->HelpLevel = HelpLevel;
}

void Local_Set_MaxJobs(int MaxJobs)
{
   CurrentClient->MaxJobs = MaxJobs;
}

void Local_Get_UseCount(int *CountPtr)
{
   *CountPtr = (IsClient ? UseCount : (UseCount - 1));
}
