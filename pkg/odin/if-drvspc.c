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
#include "inc/DPType_.h"
#include "inc/DrvSpc.h"
#include "inc/FKind_.h"
#include "inc/Str.h"

static tp_DrvSpc FreeDrvSpc;
int num_DrvSpcS;

static tp_DrvSpc New_DrvSpc(void)
{
   tp_DrvSpc DrvSpc;

   {
      if (FreeDrvSpc == NIL) {
         DrvSpc = (tp_DrvSpc) malloc(sizeof(tps_DrvSpc));
         num_DrvSpcS += 1;
         DrvSpc->InUse = FALSE;
      } else {
         DrvSpc = FreeDrvSpc;
         FreeDrvSpc = FreeDrvSpc->Next;
      }
   }

   DrvSpc->FilPrm = NIL;
   DrvSpc->FilTyp = NIL;
   DrvSpc->Key = NIL;
   DrvSpc->FilHdr = NIL;
   DrvSpc->Next = NIL;
   FORBIDDEN(DrvSpc->InUse);
   DrvSpc->InUse = TRUE;
   return DrvSpc;
}

static void Ret_DrvSpc(tp_DrvSpc DrvSpc)
{
   tp_DrvSpc DrvSpcElm, LastDrvSpc;

   if (DrvSpc == NIL) {
      return;
   }
   LastDrvSpc = DrvSpc;
   for (DrvSpcElm = DrvSpc; DrvSpcElm != NIL; DrvSpcElm = DrvSpcElm->Next) {
      FORBIDDEN(!DrvSpcElm->InUse);
      DrvSpcElm->InUse = FALSE;
      Ret_FilHdr(DrvSpcElm->FilHdr);
      LastDrvSpc = DrvSpcElm;
   }

   LastDrvSpc->Next = FreeDrvSpc;
   FreeDrvSpc = DrvSpc;
}

static tp_DrvSpc Last_DrvSpc(tp_DrvSpc DrvSpc)
{
   tp_DrvSpc LastDrvSpc;

   FORBIDDEN(DrvSpc == ERROR);
   LastDrvSpc = DrvSpc;
   while (LastDrvSpc->Next != NIL)
      LastDrvSpc = LastDrvSpc->Next;
   return LastDrvSpc;
}

static void
ShiftLeft_DrvSpc(tp_DrvSpc LeftDrvSpc, tp_DrvSpc * RiteDrvSpcPtr)
{
   tp_DrvSpc LastDrvSpc;

   FORBIDDEN(LeftDrvSpc == NIL || *RiteDrvSpcPtr == NIL);

   LastDrvSpc = Last_DrvSpc(LeftDrvSpc);
   LastDrvSpc->Next = *RiteDrvSpcPtr;

   *RiteDrvSpcPtr = (*RiteDrvSpcPtr)->Next;
   LastDrvSpc->Next->Next = NIL;
}

static void Print_DrvSpc(tp_FilDsc FilDsc, tp_Str Str, tp_DrvSpc DrvSpc)
{
   tp_Str OprStr, Word;

   FORBIDDEN(((FilDsc == NIL) == (Str == NIL)) || DrvSpc == ERROR);

   if (Str != NIL)
      (void) strcpy(Str, "");

   if (DrvSpc->FilTyp == ApplyFilTyp) {
      return;
   }

   {
      if (IsVTgt_FKind(DrvSpc->FKind) || IsVTgtText_FKind(DrvSpc->FKind)) {
         OprStr = "%";
         Word = DrvSpc->Key;
      } else if (HasKey_FKind(DrvSpc->FKind)) {
         OprStr = "/";
         Word = DrvSpc->Key;
      } else {
         OprStr = " :";
         Word = FilTyp_FTName(DrvSpc->FilTyp);
      }
   }
   {
      if (FilDsc != NIL) {
         Write(FilDsc, OprStr);
         Print_Unlex(FilDsc, Word);
      } else {
         (void) strcat(Str, OprStr);
         Unlex(Tail(Str), Word);
      }
   }

   if (IsSecOrd_FilTyp(DrvSpc->FilTyp)) {
      Word = FilTyp_FTName(FilTyp_ArgFilTyp(DrvSpc->FilTyp));
      {
         if (FilDsc != NIL) {
            Write(FilDsc, "=:");
            Print_Unlex(FilDsc, Word);
         } else {
            (void) strcat(Str, "=:");
            Unlex(Tail(Str), Word);
         }
      }
   }
}

static tp_DrvSpc FilHdr_DrvSpc(tp_FilHdr FilHdr)
{
   tp_FilHdr TmpFilHdr;
   tp_DrvSpc DrvSpc, NewDrvSpc;
   tps_Str KeyBuf;

   FORBIDDEN(FilHdr == ERROR);

   DrvSpc = NIL;
   for (TmpFilHdr = Copy_FilHdr(FilHdr);
        !IsSource(TmpFilHdr); TmpFilHdr = FilHdr_Father(TmpFilHdr)) {
      if (!IsInstance(TmpFilHdr)) {
         NewDrvSpc = New_DrvSpc();
         NewDrvSpc->FKind = FilHdr_FKind(TmpFilHdr);
         NewDrvSpc->FilTyp = FilHdr_FilTyp(TmpFilHdr);
         NewDrvSpc->FilPrm = FilHdr_FilPrm(TmpFilHdr);
         NewDrvSpc->Key = Sym_Str(Str_Sym(FilHdr_Key(KeyBuf, TmpFilHdr)));
         NewDrvSpc->FilHdr = Copy_FilHdr(TmpFilHdr);
         NewDrvSpc->Next = DrvSpc;
         DrvSpc = NewDrvSpc;
      }
   }

   Ret_FilHdr(TmpFilHdr);
   return DrvSpc;
}

static boolean
CheckCompact(tp_DrvSpc DrvSpc,
             tp_DrvSpc NextDrvSpc, tp_DrvPth DrvPth, tp_FilPrm FilPrm)
{
   tp_DrvSpc DrvSpcElm;
   tp_DrvPth DrvPthElm, GroupingDrvPthElm;
   tp_PrmTypLst PrmTypLst;
   tp_FilPrm NewFilPrm;

   FORBIDDEN(DrvSpc == ERROR || NextDrvSpc == ERROR || DrvPth == ERROR
             || FilPrm == ERROR);
   DrvSpcElm = DrvSpc;
   GroupingDrvPthElm = Find_GroupingDrvPthElm(DrvPth);
   for (DrvPthElm = DrvPth;
        DrvPthElm != NIL; DrvPthElm = DrvPth_Next(DrvPthElm)) {
      if (DrvPth_DPType(DrvPthElm) == DPT_Drv) {
         if (DrvSpcElm == NIL) {
            DrvSpcElm = NextDrvSpc;
            NextDrvSpc = NIL;
         }
         FORBIDDEN(DrvSpcElm == NIL);
         FORBIDDEN(DrvSpcElm->Key != NIL && NextDrvSpc != NIL);
         if (DrvPth_FilTyp(DrvPthElm) != DrvSpcElm->FilTyp) {
            return FALSE;
         }
         if (DrvPth_FKind(DrvPthElm) != DrvSpcElm->FKind) {
            return FALSE;
         }
         PrmTypLst = DrvPth_PrmTypLst(DrvPthElm);
         NewFilPrm = FilPrm;
         /* if is Grouping, should strip inhfilprm, but don't know it here */
         if (!(IsGroupingInput_FilTyp(DrvSpcElm->FilTyp)
               || DrvPthElm == GroupingDrvPthElm)) {
            NewFilPrm = Strip_FilPrm(FilPrm, PrmTypLst);
         }
         if (!Equal_FilPrm(DrvSpcElm->FilPrm, NewFilPrm)) {
            return FALSE;
         }
         DrvSpcElm = DrvSpcElm->Next;
      }
   }
   return (NextDrvSpc == NIL);
}

static boolean
CanCompact(tp_FilHdr FilHdr, tp_DrvSpc DrvSpc, tp_DrvSpc NextDrvSpc)
{
   tp_DrvPth DrvPth;
   tp_FilPrm FilPrm;
   tp_DrvSpc DrvSpcElm;
   boolean Can;

   FORBIDDEN(FilHdr == ERROR || DrvSpc == ERROR);

   if (FilHdr_FilTyp(FilHdr) == NextDrvSpc->FilTyp) {
      return FALSE;
   }

   DrvPth = Get_DrvPth(FilHdr, NextDrvSpc->FilTyp);
   if (DrvPth == ERROR) {
      return FALSE;
   }

   FilPrm = RootFilPrm;
   for (DrvSpcElm = DrvSpc; DrvSpcElm != NIL; DrvSpcElm = DrvSpcElm->Next) {
      FilPrm = Append_FilPrm(FilPrm, DrvSpcElm->FilPrm);
   }
   FilPrm = Append_FilPrm(FilPrm, NextDrvSpc->FilPrm);
   Can = CheckCompact(DrvSpc, NextDrvSpc, DrvPth, FilPrm);

   Ret_DrvPth(DrvPth);

   return Can;
}

void Print_FilHdr(tp_FilDsc FilDsc, tp_Str Str, tp_FilHdr FilHdr)
{
   tps_Str StrBuf;
   tp_FilHdr TmpFilHdr;
   tp_DrvSpc DrvSpc, LastDrvSpc, RestDrvSpc, DrvSpcElm;
   tp_FilPrm FilPrm;

   FORBIDDEN(((FilDsc == NIL) == (Str == NIL)) || FilHdr == ERROR);

   if (IsStr(FilHdr)) {
      {
         if (FilDsc != NIL) {
            Write(FilDsc, "=");
            Print_Unlex(FilDsc, FilHdr_Ident(FilHdr));
         } else {
            (void) strcpy(Str, "=");
            Unlex(Tail(Str), FilHdr_Ident(FilHdr));
         }
      }
      return;
   }

   TmpFilHdr = FilHdr_SrcFilHdr(Copy_FilHdr(FilHdr));
   {
      if (FilDsc != NIL) {
         FilHdr_HostFN(StrBuf, TmpFilHdr, TRUE);
         Write(FilDsc, StrBuf);
      } else {
         FilHdr_HostFN(Str, TmpFilHdr, TRUE);
      }
   }

   DrvSpc = NIL;
   RestDrvSpc = FilHdr_DrvSpc(FilHdr);

   while (RestDrvSpc != NIL) {

      DrvSpc = RestDrvSpc;
      RestDrvSpc = RestDrvSpc->Next;
      DrvSpc->Next = NIL;

      if (DrvSpc->Key == NIL) {
         while (RestDrvSpc != NIL
                && RestDrvSpc->Key == NIL
                && CanCompact(TmpFilHdr, DrvSpc, RestDrvSpc)) {
            ShiftLeft_DrvSpc(DrvSpc, &RestDrvSpc);
         }
      }

      FilPrm = RootFilPrm;
      for (DrvSpcElm = DrvSpc; DrvSpcElm != NIL;
           DrvSpcElm = DrvSpcElm->Next) {
         FilPrm = Append_FilPrm(FilPrm, DrvSpcElm->FilPrm);
      }
      Print_FilPrm(FilDsc, Tail(Str), FilPrm);

      LastDrvSpc = Last_DrvSpc(DrvSpc);
      Ret_FilHdr(TmpFilHdr);
      TmpFilHdr = Copy_FilHdr(LastDrvSpc->FilHdr);
      if (RestDrvSpc == NIL || !IsVTgt_FKind(RestDrvSpc->FKind)) {
         Print_DrvSpc(FilDsc, Tail(Str), LastDrvSpc);
      }
      Ret_DrvSpc(DrvSpc);
      DrvSpc = NIL;
   }

   if (IsGeneric(FilHdr) || IsPipe(FilHdr)) {
      {
         if (FilDsc != NIL) {
            Write(FilDsc, " :");
            Print_Unlex(FilDsc, FilTyp_FTName(FatherFilTyp));
         } else {
            (void) strcpy(Str, " :");
            Unlex(Tail(Str), FilTyp_FTName(FatherFilTyp));
         }
      }
   }

   Ret_FilHdr(TmpFilHdr);
}

void SPrint_FilHdr(tp_Str OdinExpr, tp_FilHdr FilHdr)
{
   Print_FilHdr((tp_FilDsc) NIL, OdinExpr, FilHdr);
}

void VerboseSPrint_FilHdr(tp_Str OdinExpr, tp_FilHdr FilHdr)
{
   tp_FilHdr SrcFilHdr;
   tp_DrvSpc HeadDrvSpc, DrvSpc;

   FORBIDDEN(OdinExpr == ERROR || FilHdr == ERROR);

   SrcFilHdr = FilHdr_SrcFilHdr(Copy_FilHdr(FilHdr));
   FilHdr_HostFN(OdinExpr, SrcFilHdr, TRUE);
   Ret_FilHdr(SrcFilHdr);
   HeadDrvSpc = FilHdr_DrvSpc(FilHdr);
   for (DrvSpc = HeadDrvSpc; DrvSpc != NIL; DrvSpc = DrvSpc->Next) {
      Print_FilPrm((tp_FilDsc) NIL, Tail(OdinExpr), DrvSpc->FilPrm);
      Print_DrvSpc((tp_FilDsc) NIL, Tail(OdinExpr), DrvSpc);
   }
   Ret_DrvSpc(HeadDrvSpc);
}
