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
#include "inc/FilHdr.h"
#include "inc/Flag_.h"
#include "inc/LogLevel_.h"
#include "inc/Status_.h"

boolean Is_PRB_Status(tp_Status Status)
{
   return (Status == STAT_Pending || Status == STAT_Ready
           || Status == STAT_Busy);
}

void Clr_ErrStatus(tp_FilHdr FilHdr)
{
   tps_FileName FileName;

   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(IsSource(FilHdr));
   if (FilHdr->HdrInf.ErrStatusWord == 0) {
      return;
   }
   if (FilHdr_HasErrStatus(FilHdr, STAT_Warning)) {
      FilHdr_WarningFileName(FileName, FilHdr);
      Remove(FileName);
   }
   if (FilHdr_HasErrStatus(FilHdr, STAT_Error)) {
      FilHdr_ErrorFileName(FileName, FilHdr);
      Remove(FileName);
   }
   FilHdr->HdrInf.ErrStatusWord = 0;
   SetModified(FilHdr);
}

void Add_ErrStatus(tp_FilHdr FilHdr, tp_Status Status)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(IsSource(FilHdr));
   switch (Status) {
   case STAT_Warning:{
         Do_Log("Warnings generated for", FilHdr, LOGLEVEL_Warnings);
         break;
      }
   case STAT_TgtValError:{
         break;
      }
   case STAT_Error:{
         Do_Log("Errors generated for", FilHdr, LOGLEVEL_Errors);
         break;
      }
   case STAT_NoFile:{
         Do_Log("No file generated for", FilHdr, LOGLEVEL_NoFile);
         break;
      }
   default:{
         FATALERROR("bad Status");
      }
   }
   if ((FilHdr->HdrInf.ErrStatusWord & (1 << Status)) == 0) {
      FilHdr->HdrInf.ErrStatusWord
          = (FilHdr->HdrInf.ErrStatusWord | (1 << Status));
      SetModified(FilHdr);
   }
}

boolean FilHdr_HasErrStatus(tp_FilHdr FilHdr, tp_Status Status)
{
   FORBIDDEN(FilHdr == ERROR);
   if (IsSource(FilHdr)) {
      return FALSE;
   }
   return ((FilHdr->HdrInf.ErrStatusWord & (1 << Status)) != 0);
}

tp_Status FilHdr_MinErrStatus(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr_HasErrStatus(FilHdr, STAT_NoFile)) {
      return STAT_NoFile;
   }
   if (FilHdr_HasErrStatus(FilHdr, STAT_Error)) {
      return STAT_Error;
   }
   if (FilHdr_HasErrStatus(FilHdr, STAT_TgtValError)) {
      return STAT_TgtValError;
   }
   if (FilHdr_HasErrStatus(FilHdr, STAT_Warning)) {
      return STAT_Warning;
   }
   return STAT_OK;
}

void
Add_StatusFile(tp_FilHdr FilHdr, tp_Status Status, tp_FileName FileName)
{
   tps_FileName ErrFileName;
   boolean Abort;

   FORBIDDEN(FilHdr == ERROR || FileName == ERROR);
   Add_ErrStatus(FilHdr, Status);
   switch (Status) {
   case STAT_Warning:{
         FilHdr_WarningFileName(ErrFileName, FilHdr);
         break;
      }
   case STAT_Error:{
         FilHdr_ErrorFileName(ErrFileName, FilHdr);
         break;
      }
   default:{
         FATALERROR("bad Status");
      }
   }
   Rename(&Abort, FileName, ErrFileName);
   FORBIDDEN(Abort);
   MakeReadOnly(&Abort, ErrFileName);
   if (Abort)
      Do_MakeReadOnly(ErrFileName);
   switch (Status) {
   case STAT_Warning:{
         if (IsIncremental_MsgLevel(Client_WarnLevel(CurrentClient))) {
            FileErrMessage(ErrFileName);
         }
         break;
      }
   case STAT_Error:{
         if (IsIncremental_MsgLevel(Client_ErrLevel(CurrentClient))) {
            FileErrMessage(ErrFileName);
            if (!Client_KeepGoing(CurrentClient)) {
               Local_Do_Interrupt(FALSE);
            }
         }
         break;
      }
   default:{
         FATALERROR("bad status");
      }
   }
}

void Set_DepStatus(tp_FilHdr FilHdr, tp_Status Status)
{
   FORBIDDEN(Status == ERROR || FilHdr == ERROR);
   FilHdr->DepStatus = Status;
}

tp_Status FilHdr_DepStatus(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   return FilHdr->DepStatus;
}

void Set_DepModDate(tp_FilHdr FilHdr, tp_Date ModDate)
{
   FORBIDDEN(FilHdr == ERROR);
   FilHdr->DepModDate = ModDate;
}

tp_Date FilHdr_DepModDate(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   return FilHdr->DepModDate;
}

void Set_Status(tp_FilHdr FilHdr, tp_Status Status)
{
   FORBIDDEN(Status == ERROR || FilHdr == ERROR);

   if (Status != STAT_Ready && Status != STAT_Busy) {
      FilHdr->DepStatus = STAT_Unknown;
   }

   if (Status == STAT_Unknown) {
      if (FilHdr->HdrInf.Status != STAT_Unknown) {
         FilHdr->HdrInf.Status = STAT_Unknown;
         SetModified(FilHdr);
         Set_ElmNameStatus(FilHdr, STAT_Unknown);
      }
      return;
   }
   if (Status != FilHdr->HdrInf.Status) {
      FilHdr->HdrInf.Status = Status;
      if (Status > STAT_Busy && IsATgt(FilHdr)) {
         {
            if (Status > STAT_Error) {
               Install_ActTgt(FilHdr);
            } else {
               Uninstall_ActTgt(FilHdr);
            }
         }
      }
      SetModified(FilHdr);
   }
   if (FilHdr->HdrInf.VerifyDate < CurrentDate) {
      FilHdr->HdrInf.VerifyDate = CurrentDate;
      SetModified(FilHdr);
   }
}

tp_Status FilHdr_Status(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   return FilHdr->HdrInf.Status;
}

void Set_ElmNameStatus(tp_FilHdr FilHdr, tp_Status Status)
{
   FORBIDDEN(Status == ERROR || FilHdr == ERROR);

   if (Status == STAT_Unknown) {
      if (FilHdr->HdrInf.ElmNameStatus != STAT_Unknown) {
         FilHdr->HdrInf.ElmNameStatus = STAT_Unknown;
         SetModified(FilHdr);
         Set_ElmStatus(FilHdr, STAT_Unknown);
      }
      return;
   }

   if (Status != FilHdr->HdrInf.ElmNameStatus) {
      FORBIDDEN(!IsRef(FilHdr));
      FORBIDDEN(FilHdr->HdrInf.Status == STAT_Unknown);
      FilHdr->HdrInf.ElmNameStatus = Status;
      SetModified(FilHdr);
   }
   if (FilHdr->HdrInf.ElmNameVerifyDate < CurrentDate) {
      FilHdr->HdrInf.ElmNameVerifyDate = CurrentDate;
      SetModified(FilHdr);
   }
}

tp_Status FilHdr_ElmNameStatus(tp_FilHdr FilHdr)
{
   tp_Status Status;

   if (FilHdr == ERROR)
      return ERROR;
   Status = FilHdr->HdrInf.Status;
   if (Status <= STAT_Error || !IsRef(FilHdr)) {
      return Status;
   }
   return FilHdr->HdrInf.ElmNameStatus;
}

void Set_ElmStatus(tp_FilHdr FilHdr, tp_Status Status)
{
   FORBIDDEN(Status == ERROR || FilHdr == ERROR);

   FORBIDDEN(IsSource(FilHdr));
   if (Status == STAT_Unknown) {
      if (FilHdr->HdrInf.ElmStatus != STAT_Unknown) {
         FilHdr->HdrInf.ElmStatus = STAT_Unknown;
         SetModified(FilHdr);
      }
      return;
   }

   if (Status != FilHdr->HdrInf.ElmStatus) {
      FORBIDDEN(!IsRef(FilHdr));
      FORBIDDEN(FilHdr->HdrInf.Status == STAT_Unknown);
      FORBIDDEN(FilHdr->HdrInf.ElmNameStatus == STAT_Unknown);
      FilHdr->HdrInf.ElmStatus = Status;
      SetModified(FilHdr);
   }
   if (FilHdr->HdrInf.ElmVerifyDate < CurrentDate) {
      FilHdr->HdrInf.ElmVerifyDate = CurrentDate;
      SetModified(FilHdr);
   }
}

tp_Status FilHdr_ElmStatus(tp_FilHdr FilHdr)
{
   tp_Status Status;

   if (FilHdr == ERROR)
      return ERROR;
   Status = FilHdr->HdrInf.Status;
   if (Status <= STAT_Error || !IsRef(FilHdr)) {
      return Status;
   }
   return FilHdr->HdrInf.ElmStatus;
}

void Set_TgtValStatus(tp_FilHdr FilHdr, tp_Status Status)
{
   FORBIDDEN(Status == ERROR || FilHdr == ERROR);
   FORBIDDEN(!IsSource(FilHdr));
   if (Status == STAT_Unknown) {
      if (FilHdr->HdrInf.ElmStatus != STAT_Unknown) {
         FilHdr->HdrInf.ElmStatus = STAT_Unknown;
         SetModified(FilHdr);
      }
      return;
   }

   if (Status != FilHdr->HdrInf.ElmStatus) {
      FilHdr->HdrInf.ElmStatus = Status;
      SetModified(FilHdr);
   }
   if (FilHdr->HdrInf.ElmVerifyDate < CurrentDate) {
      FilHdr->HdrInf.ElmVerifyDate = CurrentDate;
      SetModified(FilHdr);
   }
}

tp_Status FilHdr_TgtValStatus(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   if (!IsSource(FilHdr)) {
      return STAT_OK;
   }
   return FilHdr->HdrInf.ElmStatus;
}

tp_Status FilHdr_TgtValMinStatus(tp_FilHdr FilHdr)
{
   tp_Status Status, TgtValStatus;

   if (FilHdr == ERROR)
      return ERROR;
   Status = FilHdr_Status(FilHdr);
   TgtValStatus = FilHdr_TgtValStatus(FilHdr);
   return ((TgtValStatus < Status) ? TgtValStatus : Status);
}

tp_Status FilHdr_MinStatus(tp_FilHdr FilHdr, tp_InpKind InpKind)
{
   tp_Status Status;

   if (FilHdr == ERROR) {
      return ERROR;
   }
   if (!NeedsData(FilHdr, InpKind)) {
      return STAT_OK;
   }
   Status = FilHdr_TgtValMinStatus(FilHdr);
   {
      if (NeedsElmData(FilHdr, InpKind)) {
         if (FilHdr_ElmStatus(FilHdr) < Status) {
            Status = FilHdr_ElmStatus(FilHdr);
         }
      } else if (NeedsElmNameData(FilHdr, InpKind)) {
         if (FilHdr_ElmNameStatus(FilHdr) < Status) {
            Status = FilHdr_ElmNameStatus(FilHdr);
         }
      }
   }
   return Status;
}

void Set_ModDate(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   CurrentDate += 1;
   FORBIDDEN(CurrentDate <= FilHdr->HdrInf.ModDate);
   FilHdr->HdrInf.ModDate = CurrentDate;
   FilHdr->HdrInf.OrigModDate = 0;
   SetModified(FilHdr);
}

tp_Date FilHdr_ModDate(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   return FilHdr->HdrInf.ModDate;
}

void Set_ConfirmDate(tp_FilHdr FilHdr, tp_Date Date)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.ConfirmDate < Date) {
      FilHdr->HdrInf.ConfirmDate = Date;
      SetModified(FilHdr);
   }
}

void Clr_ConfirmDate(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.ConfirmDate != 0) {
      FilHdr->HdrInf.ConfirmDate = 0;
      SetModified(FilHdr);
   }
}

tp_Date FilHdr_ConfirmDate(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   return FilHdr->HdrInf.ConfirmDate;
}

void Set_ElmNameConfirmDate(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.ElmNameConfirmDate < FilHdr->HdrInf.ElmNameModDate) {
      FilHdr->HdrInf.ElmNameConfirmDate = FilHdr->HdrInf.ElmNameModDate;
      SetModified(FilHdr);
   }
}

void Set_ElmConfirmDate(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.ElmConfirmDate < FilHdr->HdrInf.ElmModDate) {
      FilHdr->HdrInf.ElmConfirmDate = FilHdr->HdrInf.ElmModDate;
      SetModified(FilHdr);
   }
}

void Set_ElmModDate(tp_FilHdr FilHdr, tp_Date ElmModDate)
{
   FORBIDDEN(ElmModDate == ERROR || FilHdr == ERROR);
   if (ElmModDate > FilHdr->HdrInf.ElmModDate) {
      FilHdr->HdrInf.ElmModDate = ElmModDate;
      SetModified(FilHdr);
   }
}

tp_Date FilHdr_ElmModDate(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   return FilHdr->HdrInf.ElmModDate;
}

void Set_ElmNameModDate(tp_FilHdr FilHdr, tp_Date ElmNameModDate)
{
   FORBIDDEN(ElmNameModDate == ERROR || FilHdr == ERROR);
   if (ElmNameModDate > FilHdr->HdrInf.ElmNameModDate) {
      FilHdr->HdrInf.ElmNameModDate = ElmNameModDate;
      SetModified(FilHdr);
   }
}

tp_Date FilHdr_ElmNameModDate(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   return FilHdr->HdrInf.ElmNameModDate;
}

void Set_Flag(tp_FilHdr FilHdr, tp_Flag Flag)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN((FilHdr->Flag & 1 << Flag) != 0);
   FilHdr->Flag |= 1 << Flag;
}

void Clr_Flag(tp_FilHdr FilHdr, tp_Flag Flag)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN((FilHdr->Flag & (1 << Flag)) == 0);
   FilHdr->Flag &= ~(1 << Flag);
}

boolean FilHdr_Flag(tp_FilHdr FilHdr, tp_Flag Flag)
{
   if (FilHdr == ERROR)
      return ERROR;
   return ((FilHdr->Flag & (1 << Flag)) != 0);
}

void Set_AnyOKDepth(tp_FilHdr FilHdr, int Depth)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(FilHdr->AnyOKDepth == Depth);
   FilHdr->AnyOKDepth = Depth;
}

int FilHdr_AnyOKDepth(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->AnyOKDepth;
}

void Set_ElmDepth(tp_FilHdr FilHdr, int Depth)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(FilHdr->ElmDepth == Depth);
   FilHdr->ElmDepth = Depth;
}

int FilHdr_ElmDepth(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->ElmDepth;
}

void Set_ElmTag(tp_FilHdr FilHdr, int ElmTag)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(FilHdr->ElmTag == ElmTag);
   FilHdr->ElmTag = ElmTag;
}

int FilHdr_ElmTag(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->ElmTag;
}

void Set_SCC(tp_FilHdr FilHdr, tp_FilHdr SCC)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(FilHdr->SCC == SCC);
   Ret_FilHdr(FilHdr->SCC);
   FilHdr->SCC = Copy_FilHdr(SCC);
}

tp_FilHdr FilHdr_SCC(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(FilHdr->SCC == NIL);
   while (FilHdr->SCC != FilHdr)
      FilHdr = FilHdr->SCC;
   return Copy_FilHdr(FilHdr);
}

/* PndFlag's used to distinguish between dependent on cycle,
 * vs. Dependent on something that has been modified and must be rescanned */

void Set_ListPndFlag(tp_FilHdr FilHdr, boolean PndFlag)
{
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr;

   Set_PndFlag(FilHdr, PndFlag);
   if (IsKeyList(FilHdr)) {
      for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
           FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
         ElmFilHdr = FilElm_FilHdr(FilElm);
         FORBIDDEN(ElmFilHdr == ERROR);
         FORBIDDEN(!IsKeyListElm(ElmFilHdr));
         Set_PndFlag(ElmFilHdr, PndFlag);
         Ret_FilHdr(ElmFilHdr);
      }
   }
}

void Set_PndFlag(tp_FilHdr FilHdr, boolean PndFlag)
{
   FORBIDDEN(FilHdr == ERROR);
   FilHdr->PndFlag = PndFlag;
}

boolean FilHdr_PndFlag(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->PndFlag;
}

void Set_ElmNamePndFlag(tp_FilHdr FilHdr, boolean ElmNamePndFlag)
{
   FORBIDDEN(FilHdr == ERROR);
   if (ElmNamePndFlag)
      Set_ElmPndFlag(FilHdr, TRUE);
   FilHdr->ElmNamePndFlag = ElmNamePndFlag;
}

boolean FilHdr_ElmNamePndFlag(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->ElmNamePndFlag;
}

void Set_ElmPndFlag(tp_FilHdr FilHdr, boolean ElmPndFlag)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(IsSource(FilHdr));
   FilHdr->ElmPndFlag = ElmPndFlag;
}

boolean FilHdr_ElmPndFlag(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (IsSource(FilHdr)) {
      return FALSE;
   }
   return FilHdr->ElmPndFlag;
}

void Set_TgtValPndFlag(tp_FilHdr FilHdr, boolean TgtValPndFlag)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(!IsSource(FilHdr));
   FilHdr->ElmPndFlag = TgtValPndFlag;
}

boolean FilHdr_TgtValPndFlag(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (!IsSource(FilHdr)) {
      return FALSE;
   }
   return FilHdr->ElmPndFlag;
}

tp_LocInp FilHdr_LocInp(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->HdrInf.LocInp;
}

void Set_LocElm(tp_FilHdr FilHdr, tp_LocElm LocElm)
{

   FORBIDDEN(FilHdr == ERROR);
   if (LocElm == FilHdr->HdrInf.LocElm) {
      FORBIDDEN(LocElm != NIL);
      return;
   }

   if (DebugLocElm)
      Validate_LocElm(FilHdr, LocElm);
   if (FilHdr->HdrInf.LocElm != FilHdr->HdrInf.OldLocElm) {
      DeAlloc_ElmInf(FilHdr->HdrInf.LocElm);
   }
   FilHdr->HdrInf.LocElm = LocElm;
   SetModified(FilHdr);
}

tp_LocElm FilHdr_LocElm(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->HdrInf.LocElm;
}

void Set_OldLocElm(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.OldLocElm == FilHdr->HdrInf.LocElm) {
      return;
   }
   DeAlloc_ElmInf(FilHdr->HdrInf.OldLocElm);
   FilHdr->HdrInf.OldLocElm = FilHdr->HdrInf.LocElm;
   SetModified(FilHdr);
}

tp_LocElm FilHdr_OldLocElm(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->HdrInf.OldLocElm;
}

void Set_TgtValLocElm(tp_FilHdr FilHdr, tp_LocElm LocElm)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(!IsSource(FilHdr));
   if (FilHdr->HdrInf.TgtValLocElm == LocElm) {
      return;
   }
   DeAlloc_ElmInf(FilHdr->HdrInf.TgtValLocElm);
   FilHdr->HdrInf.TgtValLocElm = LocElm;
   SetModified(FilHdr);
}

void Set_DfltTgtValLocElm(tp_FilHdr FilHdr)
{
   tp_FilHdr TgtValFilHdr;

   TgtValFilHdr = Do_Deriv(FilHdr_Father(Copy_FilHdr(FilHdr)),
                           RootFilPrm, RootFilPrm, ActTargetsFilTyp);
   FORBIDDEN(TgtValFilHdr == NIL);
   Set_TgtValLocElm(FilHdr, Make_LocElm(TgtValFilHdr, RootFilPrm, FilHdr));
   Set_OrigLocHdr(FilHdr, (tp_LocHdr) NIL);
   Ret_FilHdr(TgtValFilHdr);
}

tp_LocElm FilHdr_TgtValLocElm(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->HdrInf.TgtValLocElm;
}

boolean FilHdr_ActTgtInstalled(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.OrigLocHdr != 0);
}

void Set_ActTgtInstalled(tp_FilHdr FilHdr, boolean Flag)
{
   tp_LocHdr LocHdr;

   FORBIDDEN(FilHdr == ERROR);
   LocHdr = (Flag ? 1 : 0);
   FORBIDDEN(LocHdr == FilHdr->HdrInf.OrigLocHdr);
   FilHdr->HdrInf.OrigLocHdr = LocHdr;
   SetModified(FilHdr);
}

void Set_InpLink(tp_FilHdr FilHdr, tp_LocInp LocInp)
{
   FORBIDDEN(FilHdr == ERROR);
   if (LocInp != FilHdr->HdrInf.InpLink) {
      FilHdr->HdrInf.InpLink = LocInp;
      SetModified(FilHdr);
   }
}

tp_LocInp FilHdr_InpLink(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->HdrInf.InpLink;
}

void Set_ElmLink(tp_FilHdr FilHdr, tp_LocElm LocElm)
{
   FORBIDDEN(FilHdr == ERROR);
   if (LocElm != FilHdr->HdrInf.ElmLink) {
      FilHdr->HdrInf.ElmLink = LocElm;
      SetModified(FilHdr);
   }
}

tp_LocElm FilHdr_ElmLink(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->HdrInf.ElmLink;
}

int FilHdr_Size(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.Size == -1) {
      return 0;
   }
   return FilHdr->HdrInf.Size;
}

void Set_Size(tp_FilHdr FilHdr, int Size)
{
   int OldSize;

   FORBIDDEN(FilHdr == ERROR || Size < -1);
   if (Size != FilHdr->HdrInf.Size) {
      OldSize = FilHdr_Size(FilHdr);
      FilHdr->HdrInf.Size = Size;
      SetModified(FilHdr);
      CurSize += FilHdr_Size(FilHdr) - OldSize;
   }
}

boolean Data_Exists(tp_FilHdr FilHdr)
{
   return (FilHdr->HdrInf.Size != -1);
}

void Local_Get_CurSize(int *SizePtr)
{
   *SizePtr = CurSize;
}

void Set_OrigLocHdr(tp_FilHdr FilHdr, tp_LocHdr LocHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(!IsSource(FilHdr));
   if (FilHdr->HdrInf.OrigLocHdr != LocHdr) {
      FilHdr->HdrInf.OrigLocHdr = LocHdr;
      FilHdr->HdrInf.OrigModDate = 0;
      SetModified(FilHdr);
   }
}

tp_LocHdr FilHdr_OrigLocHdr(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->HdrInf.OrigLocHdr;
}

void Set_OrigModDate(tp_FilHdr FilHdr, tp_Date ModDate)
{
   FORBIDDEN(FilHdr == ERROR || ModDate == ERROR);
   if (FilHdr->HdrInf.OrigModDate != ModDate) {
      FilHdr->HdrInf.OrigModDate = ModDate;
      SetModified(FilHdr);
   }
}

tp_Date FilHdr_OrigModDate(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->HdrInf.OrigModDate;
}
