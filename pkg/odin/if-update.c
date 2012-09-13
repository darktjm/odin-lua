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
#include "inc/FileName.h"
#include "inc/FKind_.h"
#include "inc/Job.h"
#include "inc/Status_.h"
#include "inc/Str.h"

static void
Set_UpToDate(tp_FilHdr FilHdr, tp_Status Status, tp_Date DepModDate)
{
   FORBIDDEN(Status <= STAT_Unknown);
   Set_Status(FilHdr, Status);
   if (FilHdr_ModDate(FilHdr) == 0)
      Set_ModDate(FilHdr);
   Set_ConfirmDate(FilHdr, DepModDate);
}

static void
Update_File(tp_FilHdr FilHdr,
            tp_Status Status, tp_Date DepModDate, tp_FileName WorkFileName)
{
   tp_Status NewStatus;
   int NewSize;
   boolean Abort, Changed;
   tps_FileName DataFileName;
   tp_FilDsc WorkFD, DataFD;

   FORBIDDEN(Status < STAT_Busy);
   NewStatus = Status;
   if (IsVoid(FilHdr)) {
      goto done;
   }

   FilHdr_DataFileName(DataFileName, FilHdr);
   FileSize(&Abort, &NewSize, WorkFileName);
   if (Abort) {
      FORBIDDEN(NewStatus < STAT_NoFile);
      NewStatus = STAT_NoFile;
      if (Data_Exists(FilHdr)) {
         Remove(DataFileName);
         Set_Size(FilHdr, -1);
      }
      Add_ErrStatus(FilHdr, STAT_NoFile);
      goto done;
   }

   Changed = (FilHdr_Size(FilHdr) != NewSize || !Data_Exists(FilHdr));
   if (!Changed && NewSize > 0) {
      WorkFD = FileName_RFilDsc(WorkFileName, FALSE);
      FORBIDDEN(WorkFD == ERROR);
      DataFD = FileName_RFilDsc(DataFileName, TRUE);
      FORBIDDEN(DataFD == ERROR);
      Changed = !(Equal(WorkFD, DataFD));
      Close(WorkFD);
      Close(DataFD);
   }

   {
      if (Changed) {
         Rename(&Abort, WorkFileName, DataFileName);
         if (Abort) {
            SystemError("Cannot write to cache file: %s.\n", DataFileName);
            Set_Status(FilHdr, STAT_Unknown);
            Local_Do_Interrupt(FALSE);
            return;
         }
         Set_ModDate(FilHdr);
         Set_Size(FilHdr, NewSize);
         MakeReadOnly(&Abort, DataFileName);
         if (Abort)
            Do_MakeReadOnly(DataFileName);
      } else {
         Remove(WorkFileName);
      }
   }

 done:;
   Set_UpToDate(FilHdr, NewStatus, DepModDate);
}

static void
Update_Struct(tp_FilHdr FilHdr, tp_Status Status, tp_Date DepModDate)
{
   FORBIDDEN(FilHdr == ERROR || !IsStruct(FilHdr));
   Set_ModDate(FilHdr);
   Set_UpToDate(FilHdr, Status, DepModDate);
}

static void
Do_DrvDir(tp_Status * StatusPtr,
          tp_FilHdr FilHdr, tp_Date DepModDate, tp_Job Job)
{
   tps_FileName WorkFileName;
   tp_FilHdr ElmFilHdr;
   tp_LocElm FirstLE, LastLE, LocElm;
   tps_FileName InFileName, DirName, ElmFileName, LinkFileName;
   tps_Str Key;
   tp_FilDsc WorkDirFilDsc;
   boolean End, Abort;
   size_t sz;

   FilHdr_DataFileName(DirName, FilHdr);
   if (!Data_Exists(FilHdr)) {
      MakeDirFile(&Abort, DirName);
      FORBIDDEN(Abort);
      Set_Size(FilHdr, 1);
   }
   ClearDir(DirName);

   Get_WorkFileName(WorkFileName, Job, FilHdr);
   FirstLE = NIL;
   LastLE = NIL;
   WorkDirFilDsc = OpenDir(WorkFileName);
   if (WorkDirFilDsc != ERROR) {
      for (ReadDir(Key, &End, WorkDirFilDsc);
           !End; ReadDir(Key, &End, WorkDirFilDsc)) {
         sz = snprintf(InFileName, MAX_FileName, "%s/%s", WorkFileName,
                       Key);
         if (sz >= MAX_FileName) {
            (void) fprintf(stderr,
                           "File name too long (MAX_FileName=%d): %s/%s\n",
                           MAX_FileName, WorkFileName, Key);
            exit(1);
         }
         ElmFilHdr = Do_Key(Copy_FilHdr(FilHdr), Key);
         Update_File(ElmFilHdr, *StatusPtr, DepModDate, InFileName);
         LocElm = Make_LocElm(ElmFilHdr, RootFilPrm, FilHdr);
         Chain_LocElms(&FirstLE, &LastLE, LocElm);
         FilHdr_DataFileName(ElmFileName, ElmFilHdr);
         sz = snprintf(LinkFileName, MAX_FileName, "%s/%s", DirName, Key);
         if (sz >= MAX_FileName) {
            (void) fprintf(stderr,
                           "File name too long (MAX_FileName=%d): %s/%s\n",
                           MAX_FileName, DirName, Key);
            exit(1);
         }
         SymLink(&Abort, LinkFileName, ElmFileName);
         FORBIDDEN(Abort);
         Ret_FilHdr(ElmFilHdr);
      }
      CloseDir(WorkDirFilDsc);
      RemoveDir(WorkFileName);
   }
   Set_LocElm(FilHdr, FirstLE);
}

void
Do_Update(tp_FilHdr FilHdr,
          tp_OutFilHdrs OutFilHdrs,
          int NumOuts,
          tp_Job Job,
          tp_Status Status, tp_Date DepModDate, boolean IsInternal)
{
   tp_Date NewDepModDate;
   tp_Status NewStatus;
   int i;
   tp_FilHdr OutFilHdr;
   tps_FileName WorkFileName;

   FORBIDDEN(IsStructMem(FilHdr) && IsAtmc(FilHdr));

   NewDepModDate = DepModDate;
   if (IsStruct(FilHdr)) {
      Update_Struct(FilHdr, Status, DepModDate);
      NewDepModDate = FilHdr_ModDate(FilHdr);
   }
   for (i = 0; i < NumOuts; i++) {
      NewStatus = Status;
      OutFilHdr = Copy_FilHdr(OutFilHdrs[i]);
      {
         if (IsVirDir(OutFilHdr)) {
            Set_ModDate(OutFilHdr);
            Set_UpToDate(OutFilHdr, NewStatus, NewDepModDate);
         } else if (IsDrvDir(OutFilHdr)) {
            FORBIDDEN(IsInternal);
            Do_DrvDir(&NewStatus, OutFilHdr, NewDepModDate, Job);
            Update_RefFile(OutFilHdr, NewStatus, NewDepModDate);
            Set_DrvDirConfirm(OutFilHdr, NewStatus);
         } else if (IsAtmc(OutFilHdr)) {
            Get_WorkFileName(WorkFileName, Job, OutFilHdr);
            Update_File(OutFilHdr, NewStatus, NewDepModDate, WorkFileName);
         } else {
            FORBIDDEN(!IsInternal);
            Update_RefFile(OutFilHdr, NewStatus, NewDepModDate);
            Set_DrvDirConfirm(OutFilHdr, NewStatus);
         }
      }
      Ret_FilHdr(OutFilHdr);
   }
}

void Validate_IsPntr(tp_FilHdr FilHdr)
{
   tp_FilElm FilElm;

   FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
   {
      if (FilElm == NIL) {
         FilHdr_Error("Empty pointer file : <%s>.\n", FilHdr);
      } else {
         if (FilElm_Next(FilElm) != NIL) {
            FilHdr_Error
                ("Too many elements in pointer file : <%s>.\n", FilHdr);
         }
         Ret_FilElm(FilElm);
      }
   }
}

void Update_RefFile(tp_FilHdr FilHdr, tp_Status Status, tp_Date DepModDate)
{
   FORBIDDEN(Status < STAT_Busy);
   if (!IsEquiv_LocElms(FilHdr_OldLocElm(FilHdr), FilHdr_LocElm(FilHdr))) {
      Set_ModDate(FilHdr);
      Set_ElmNameStatus(FilHdr, STAT_Unknown);
   }
   Set_OldLocElm(FilHdr);
   Set_UpToDate(FilHdr, Status, DepModDate);
}

void Set_DrvDirConfirm(tp_FilHdr FilHdr, tp_Status Status)
{
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr;
   tp_Date DepModDate;

   if (!IsKeyList(FilHdr) || Status <= STAT_Error) {
      return;
   }

   DepModDate = FilHdr_ModDate(FilHdr);
   for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      FORBIDDEN(ElmFilHdr == ERROR);
      FORBIDDEN(!IsKeyListElm(ElmFilHdr));
      Set_Status(ElmFilHdr, Status);
      Set_ConfirmDate(ElmFilHdr, DepModDate);
      Ret_FilHdr(ElmFilHdr);
   }
}

void Set_ListStatus(tp_FilHdr FilHdr, tp_Status Status)
{
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr;

   Set_Status(FilHdr, Status);
   if (IsKeyList(FilHdr) && Status > STAT_Error) {
      for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
           FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
         ElmFilHdr = FilElm_FilHdr(FilElm);
         FORBIDDEN(ElmFilHdr == ERROR);
         FORBIDDEN(!IsKeyListElm(ElmFilHdr));
         Set_Status(ElmFilHdr, Status);
         Ret_FilHdr(ElmFilHdr);
      }
   }
}
