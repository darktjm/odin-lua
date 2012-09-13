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
#include "inc/Flag_.h"
#include "inc/InpKind_.h"
#include "inc/Status_.h"
#include "inc/Str.h"

static void Get_ElmReport(tp_FilDsc, tp_FilHdr, boolean, tp_Status);

static void
Do_Report(tp_FilDsc ReportFD,
          tp_FilHdr FilHdr, tp_Status Status, tp_Status ReportStatus)
{
   boolean MsgFlag;
   tps_FileName WarningFileName, ErrorFileName;
   tp_FilDsc FilDsc;

   if (Status == STAT_SysAbort) {
      return;
   }

   MsgFlag = FALSE;
   if (ReportStatus >= STAT_TgtValError
       && FilHdr_HasErrStatus(FilHdr, STAT_Error)) {
      MsgFlag = TRUE;
      Write(ReportFD, "--- <");
      Print_FilHdr(ReportFD, (tp_Str) NIL, FilHdr);
      Writeln(ReportFD, "> generated errors ---");
      FilHdr_ErrorFileName(ErrorFileName, FilHdr);
      FilDsc = FileName_RFilDsc(ErrorFileName, TRUE);
      FileCopy(ReportFD, FilDsc);
      Close(FilDsc);
   }

   if (ReportStatus >= STAT_Warning
       && FilHdr_HasErrStatus(FilHdr, STAT_Warning)) {
      MsgFlag = TRUE;
      Write(ReportFD, "--- <");
      Print_FilHdr(ReportFD, (tp_Str) NIL, FilHdr);
      Writeln(ReportFD, "> generated warnings ---");
      FilHdr_WarningFileName(WarningFileName, FilHdr);
      FilDsc = FileName_RFilDsc(WarningFileName, TRUE);
      FileCopy(ReportFD, FilDsc);
      Close(FilDsc);
      return;
   }

   if (MsgFlag || ReportStatus < Status) {
      return;
   }

   switch (Status) {
   case STAT_Unknown:{
         Write(ReportFD, "--- <");
         Print_FilHdr(ReportFD, (tp_Str) NIL, FilHdr);
         Writeln(ReportFD, "> has unknown status ---");
         break;
      }
   case STAT_Pending:{
         Write(ReportFD, "--- <");
         Print_FilHdr(ReportFD, (tp_Str) NIL, FilHdr);
         Writeln(ReportFD, "> has pending status ---");
         break;
      }
   case STAT_Ready:{
         Write(ReportFD, "--- <");
         Print_FilHdr(ReportFD, (tp_Str) NIL, FilHdr);
         Writeln(ReportFD, "> has ready status ---");
         break;
      }
   case STAT_Busy:{
         Write(ReportFD, "--- <");
         Print_FilHdr(ReportFD, (tp_Str) NIL, FilHdr);
         Writeln(ReportFD, "> has busy status ---");
         break;
      }
   case STAT_SysAbort:{
         Write(ReportFD, "--- <");
         Print_FilHdr(ReportFD, (tp_Str) NIL, FilHdr);
         Writeln(ReportFD, "> has system abort status ---");
         break;
      }
   case STAT_NoFile:{
         Write(ReportFD, "--- <");
         Print_FilHdr(ReportFD, (tp_Str) NIL, FilHdr);
         Writeln(ReportFD, "> does not exist ---");
         break;
      }
   case STAT_Circular:{
         Write(ReportFD, "--- <");
         Print_FilHdr(ReportFD, (tp_Str) NIL, FilHdr);
         Writeln(ReportFD, "> depends on itself ---");
         break;
      }
   case STAT_ElmCircular:{
         Write(ReportFD, "--- An element of <");
         Print_FilHdr(ReportFD, (tp_Str) NIL, FilHdr);
         Writeln(ReportFD, "> depends on itself ---");
         break;
      }
   case STAT_Error:
   case STAT_TgtValError:
   case STAT_Warning:{
         break;
      }
   default:{
         FATALERROR("Bad status");
      }
   }
}

static void
Set_ElmVisit(boolean * DoneFlagPtr,
             boolean * DataFlagPtr, tp_FilHdr FilHdr, tp_InpKind InpKind)
{
   *DoneFlagPtr = FALSE;
   *DataFlagPtr = NeedsElmData(FilHdr, InpKind);
   if (!(*DataFlagPtr || NeedsElmNameData(FilHdr, InpKind))) {
      *DoneFlagPtr = TRUE;
      return;
   }
   {
      if (*DataFlagPtr) {
         if (FilHdr_Flag(FilHdr, FLAG_ElmVisit)) {
            *DoneFlagPtr = TRUE;
            return;
         }
         Set_Flag(FilHdr, FLAG_ElmVisit);
         if (!FilHdr_Flag(FilHdr, FLAG_ElmNameVisit)) {
            Set_Flag(FilHdr, FLAG_ElmNameVisit);
         }
      } else {
         if (FilHdr_Flag(FilHdr, FLAG_ElmNameVisit)) {
            *DoneFlagPtr = TRUE;
            return;
         }
         Set_Flag(FilHdr, FLAG_ElmNameVisit);
      }
   }
}

static void
Get_Report(tp_FilDsc ReportFD,
           tp_FilHdr FilHdr, tp_InpKind InpKind, tp_Status ReportStatus)
{
   tp_Status Status, InpStatus;
   tp_FilHdr TgtValFilHdr;
   tp_FilInp FilInp;
   tp_FilHdr InpFilHdr;
   boolean DataFlag, DoneFlag;

   FORBIDDEN(FilHdr == ERROR);

   if (!NeedsData(FilHdr, InpKind)) {
      return;
   }

   if (!FilHdr_Flag(FilHdr, FLAG_Visit)) {
      Set_Flag(FilHdr, FLAG_Visit);
      Status = FilHdr_Status(FilHdr);
      if (IsSource(FilHdr)) {
         if (FilHdr_TgtValStatus(FilHdr) <= ReportStatus) {
            TgtValFilHdr = FilHdr_TgtValFilHdr(Copy_FilHdr(FilHdr));
            if (FilHdr_Status(TgtValFilHdr) > STAT_Busy) {
               Get_Report(ReportFD, TgtValFilHdr, IK_Simple, ReportStatus);
            }
            Ret_FilHdr(TgtValFilHdr);
         }
         if (FilHdr_TgtValStatus(FilHdr) >= STAT_Warning
             && Status <= ReportStatus) {
            Do_Report(ReportFD, FilHdr, Status, ReportStatus);
         }
         return;
      }

      if (Status <= ReportStatus) {
         for (FilInp = LocInp_FilInp(FilHdr_LocInp(FilHdr));
              FilInp != NIL; FilInp = FilInp_NextFilInp(FilInp)) {
            InpFilHdr = FilInp_FilHdr(FilInp);
            Get_Report(ReportFD, InpFilHdr, FilInp_InpKind(FilInp),
                       ReportStatus);
            InpStatus =
                FilHdr_MinStatus(InpFilHdr, FilInp_InpKind(FilInp));
            Ret_FilHdr(InpFilHdr);
         }
      }

      if (Status <= ReportStatus && !(IsStructMem(FilHdr)
                                      &&
                                      ((FilHdr_MinErrStatus(FilHdr) ==
                                        STAT_OK && Status == STAT_Circular)
                                       || InpStatus == STAT_Error))) {
         Do_Report(ReportFD, FilHdr, Status, ReportStatus);
      }
   }

   Set_ElmVisit(&DoneFlag, &DataFlag, FilHdr, InpKind);
   if (DoneFlag) {
      return;
   }
   {
      if (DataFlag) {
         if (FilHdr_ElmStatus(FilHdr) == STAT_ElmCircular) {
            Do_Report(ReportFD, FilHdr, STAT_ElmCircular, ReportStatus);
         }
         if (FilHdr_ElmStatus(FilHdr) <= ReportStatus) {
            Get_ElmReport(ReportFD, FilHdr, TRUE, ReportStatus);
         }
      } else {
         if (FilHdr_ElmNameStatus(FilHdr) == STAT_ElmCircular) {
            Do_Report(ReportFD, FilHdr, STAT_ElmCircular, ReportStatus);
         }
         if (FilHdr_ElmNameStatus(FilHdr) <= ReportStatus) {
            Get_ElmReport(ReportFD, FilHdr, FALSE, ReportStatus);
         }
      }
   }

}

static void
Get_ElmReport(tp_FilDsc ReportFD,
              tp_FilHdr FilHdr, boolean DataFlag, tp_Status ReportStatus)
{
   boolean ViewSpecFlag;
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr, TgtValFilHdr;
   tp_InpKind InpKind;

   if (FilHdr_Status(FilHdr) <= STAT_Error) {
      return;
   }

   ViewSpecFlag = IsViewSpec(FilHdr);
   for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      FORBIDDEN(ElmFilHdr == ERROR);
      if (DataFlag || IsRef(ElmFilHdr)) {
         {
            if (ViewSpecFlag && FilHdr_ElmStatus(ElmFilHdr) == STAT_NoFile) {
               if (FilHdr_TgtValStatus(ElmFilHdr) != STAT_OK) {
                  TgtValFilHdr =
                      FilHdr_TgtValFilHdr(Copy_FilHdr(ElmFilHdr));
                  Get_Report(ReportFD, TgtValFilHdr, IK_Simple,
                             ReportStatus);
                  Ret_FilHdr(TgtValFilHdr);
               }
            } else {
               InpKind = (DataFlag ? IK_Trans : IK_TransName);
               Get_Report(ReportFD, ElmFilHdr, InpKind, ReportStatus);
            }
         }
      }
      if (ViewSpecFlag && FilHdr_ElmStatus(ElmFilHdr) != STAT_NoFile) {
         FilElm = FilElm_NextStrFilElm(FilElm);
      }
      Ret_FilHdr(ElmFilHdr);
   }
}

static void Clr_VisitFlags(tp_FilHdr FilHdr)
{
   tp_FilInp FilInp;
   tp_FilElm FilElm;
   tp_FilHdr TgtValFilHdr, InpFilHdr, ElmFilHdr;

   if (!FilHdr_Flag(FilHdr, FLAG_Visit)) {
      return;
   }
   Clr_Flag(FilHdr, FLAG_Visit);
   if (FilHdr_Flag(FilHdr, FLAG_ElmVisit)) {
      Clr_Flag(FilHdr, FLAG_ElmVisit);
   }
   if (FilHdr_Flag(FilHdr, FLAG_ElmNameVisit)) {
      Clr_Flag(FilHdr, FLAG_ElmNameVisit);
   }
   if (IsSource(FilHdr)) {
      TgtValFilHdr = FilHdr_TgtValFilHdr(Copy_FilHdr(FilHdr));
      if (TgtValFilHdr != NIL) {
         Clr_VisitFlags(TgtValFilHdr);
         Ret_FilHdr(TgtValFilHdr);
      }
   }
   for (FilInp = LocInp_FilInp(FilHdr_LocInp(FilHdr));
        FilInp != NIL; FilInp = FilInp_NextFilInp(FilInp)) {
      InpFilHdr = FilInp_FilHdr(FilInp);
      Clr_VisitFlags(InpFilHdr);
      Ret_FilHdr(InpFilHdr);
   }
   for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      Clr_VisitFlags(ElmFilHdr);
      TgtValFilHdr = FilHdr_TgtValFilHdr(Copy_FilHdr(ElmFilHdr));
      if (TgtValFilHdr != NIL) {
         Clr_VisitFlags(TgtValFilHdr);
         Ret_FilHdr(TgtValFilHdr);
      }
      Ret_FilHdr(ElmFilHdr);
   }
}

void
WriteReport(tp_FilDsc StatusFD, tp_FilHdr FilHdr, tp_Status ReportStatus)
{
   if (FilHdr_MinStatus(FilHdr, IK_Trans) > ReportStatus) {
      return;
   }

   Write(StatusFD, "** Summary of ");
   switch (ReportStatus) {
   case STAT_Warning:{
         Write(StatusFD, "warning and error");
         break;
      }
   case STAT_TgtValError:{
         Write(StatusFD, "error");
         break;
      }
   default:{
         FATALERROR("Unexpected status");
      }
   }
   Write(StatusFD, " messages for ");
   Print_FilHdr(StatusFD, (tp_Str) NIL, FilHdr);
   Writeln(StatusFD, "");

   Get_Report(StatusFD, FilHdr, IK_Trans, ReportStatus);
   Clr_VisitFlags(FilHdr);
}

static void
GetDepend1(tp_LocElm * FirstLEPtr,
           tp_LocElm * LastLEPtr,
           tp_FilHdr FilHdr, tp_InpKind InpKind, tp_FilHdr ListFilHdr)
{
   tp_LocElm LocElm;
   tp_FilInp FilInp;
   tp_FilElm FilElm;
   tp_FilHdr InpFilHdr, ElmFilHdr, TgtValFilHdr;
   boolean DoneFlag, DataFlag, ViewSpecFlag;

   FORBIDDEN(FilHdr == ERROR);

   if (!NeedsData(FilHdr, InpKind)) {
      return;
   }

   if (!FilHdr_Flag(FilHdr, FLAG_Visit)) {
      Set_Flag(FilHdr, FLAG_Visit);
      if (IsSource(FilHdr)) {
         TgtValFilHdr = FilHdr_TgtValFilHdr(Copy_FilHdr(FilHdr));
         if (TgtValFilHdr != NIL) {
            if (!IsDfltTgtVal(TgtValFilHdr)) {
               GetDepend1(FirstLEPtr, LastLEPtr, TgtValFilHdr, IK_Simple,
                          ListFilHdr);
            }
            Ret_FilHdr(TgtValFilHdr);
         }
         LocElm = Make_LocElm(FilHdr, RootFilPrm, ListFilHdr);
         Chain_LocElms(FirstLEPtr, LastLEPtr, LocElm);
         return;
      }

      for (FilInp = LocInp_FilInp(FilHdr_LocInp(FilHdr));
           FilInp != NIL; FilInp = FilInp_NextFilInp(FilInp)) {
         InpFilHdr = FilInp_FilHdr(FilInp);
         GetDepend1(FirstLEPtr, LastLEPtr, InpFilHdr,
                    FilInp_InpKind(FilInp), ListFilHdr);
         Ret_FilHdr(InpFilHdr);
      }
   }

   Set_ElmVisit(&DoneFlag, &DataFlag, FilHdr, InpKind);
   if (DoneFlag) {
      return;
   }
   ViewSpecFlag = IsViewSpec(FilHdr);
   for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      FORBIDDEN(ElmFilHdr == ERROR);
      if ((DataFlag || IsRef(ElmFilHdr))
          && !(ViewSpecFlag && FilHdr_ElmStatus(ElmFilHdr) == STAT_NoFile)) {
         GetDepend1(FirstLEPtr, LastLEPtr, ElmFilHdr,
                    (DataFlag ? IK_Trans : IK_TransName), ListFilHdr);
      }
      if (ViewSpecFlag && FilHdr_ElmStatus(ElmFilHdr) != STAT_NoFile) {
         FilElm = FilElm_NextStrFilElm(FilElm);
      }
      Ret_FilHdr(ElmFilHdr);
   }
}

void
GetDepend(tp_LocElm * FirstLEPtr,
          tp_LocElm * LastLEPtr, tp_FilHdr FilHdr, tp_FilHdr ListFilHdr)
{
   GetDepend1(FirstLEPtr, LastLEPtr, FilHdr, IK_Trans, ListFilHdr);
   Clr_VisitFlags(FilHdr);
}

static void
Get_DPath1(boolean * FoundPtr,
           tp_FilHdr FilHdr,
           tp_InpKind InpKind, tp_FilHdr DepFilHdr, boolean First)
{
   tp_Str Message;
   tp_FilInp FilInp;
   tp_FilHdr InpFilHdr, ElmFilHdr, TgtValFilHdr;
   boolean DoneFlag, DataFlag, ViewSpecFlag;
   tp_FilElm FilElm;

   FORBIDDEN(FilHdr == ERROR);

   *FoundPtr = FALSE;
   Message = NIL;

   if (!NeedsData(FilHdr, InpKind)) {
      return;
   }

   {
      if (FilHdr_Flag(FilHdr, FLAG_Visit)) {
         if (FilHdr == DepFilHdr) {
            *FoundPtr = TRUE;
            goto found;
         }
      } else {
         Set_Flag(FilHdr, FLAG_Visit);

         if (FilHdr == DepFilHdr && !First) {
            *FoundPtr = TRUE;
            goto found;
         }

         if (IsSource(FilHdr)) {
            TgtValFilHdr = FilHdr_TgtValFilHdr(Copy_FilHdr(FilHdr));
            if (TgtValFilHdr != NIL) {
               if (!IsDfltTgtVal(TgtValFilHdr)) {
                  Get_DPath1(FoundPtr, TgtValFilHdr, IK_Simple, DepFilHdr,
                             FALSE);
               }
               Ret_FilHdr(TgtValFilHdr);
               if (*FoundPtr) {
                  Message = "   is the bound value of:";
                  goto found;
               }
            }
            return;
         }

         for (FilInp = LocInp_FilInp(FilHdr_LocInp(FilHdr));
              FilInp != NIL; FilInp = FilInp_NextFilInp(FilInp)) {
            InpFilHdr = FilInp_FilHdr(FilInp);
            Get_DPath1(FoundPtr, InpFilHdr,
                       FilInp_InpKind(FilInp), DepFilHdr, FALSE);
            Ret_FilHdr(InpFilHdr);
            if (*FoundPtr) {
               Ret_FilInp(FilInp);
               Message = "   is an input of:";
               goto found;
            }
         }
      }
   }

   Set_ElmVisit(&DoneFlag, &DataFlag, FilHdr, InpKind);
   if (DoneFlag) {
      return;
   }
   ViewSpecFlag = IsViewSpec(FilHdr);
   for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      FORBIDDEN(ElmFilHdr == ERROR);
      if ((DataFlag || IsRef(ElmFilHdr))
          && !(ViewSpecFlag && FilHdr_ElmStatus(ElmFilHdr) == STAT_NoFile
               && FilHdr_TgtValStatus(ElmFilHdr) == STAT_OK)) {
         Get_DPath1(FoundPtr, ElmFilHdr,
                    (DataFlag ? IK_Trans : IK_TransName), DepFilHdr,
                    FALSE);
      }
      if (ViewSpecFlag && FilHdr_ElmStatus(ElmFilHdr) != STAT_NoFile) {
         FilElm = FilElm_NextStrFilElm(FilElm);
      }
      Ret_FilHdr(ElmFilHdr);
      if (*FoundPtr) {
         Ret_FilElm(FilElm);
         Message = "   is an element of:";
         goto found;
      }
   }
   return;

 found:;
   if (Message != NIL)
      LogMessage(Message);
   Print_OdinExpr(FilHdr_LocHdr(FilHdr), RootFilPrm);
}

void Local_Get_DPath(tp_Str OdinExpr)
{
   tp_Nod Root;
   tp_PrmFHdr PrmFHdr;
   tp_FilHdr FilHdr, DepFilHdr;
   tp_FilPrm FilPrm;
   boolean Found;

   Root = YY_Parser(OdinExpr, (tp_FileName) NIL, (int *) NIL);
   if (Root == ERROR) {
      return;
   }
   PrmFHdr = Nod_PrmFHdr(Root);
   Ret_Nod(Root);
   Use_PrmFHdr(&FilHdr, &FilPrm, PrmFHdr);
   if (FilHdr == ERROR) {
      return;
   }

   DepFilHdr =
       Deref(LocHdr_FilHdr(FilPVal_LocHdr(FilPrm_FilPVal(FilPrm))));
   if (DepFilHdr == ERROR) {
      SystemError("+depend parameter required.\n");
      Ret_FilHdr(FilHdr);
      return;
   }

   Get_DPath1(&Found, FilHdr, IK_Trans, DepFilHdr, TRUE);
   Clr_VisitFlags(FilHdr);
   Ret_FilHdr(FilHdr);
   Ret_FilHdr(DepFilHdr);
}
