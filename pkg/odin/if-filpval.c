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
#include "inc/InpKind_.h"
#include "inc/FilPVal.h"
#include "inc/FileName.h"
#include "inc/FKind_.h"
#include "inc/Flag_.h"

int num_FilPValS = 0;

static void WriteFilPVal(tp_FilPVal FilPVal)
{
   WritePValInf(&FilPVal->PValInf, FilPVal->LocPVal);
}

static tp_LocPVal Alloc_PValInf(void)
{
   return (tp_LocPVal) Alloc(sizeof(tps_PValInf));
}

tp_FilPVal New_FilPVal(void)
{
   tp_FilPVal FilPVal;

   FilPVal = (tp_FilPVal) malloc(sizeof(tps_FilPVal));
   num_FilPValS += 1;
   FilPVal->LocPVal = NIL;
   FilPVal->NextHash = NIL;

   FilPVal->PValInf.Father = NIL;
   FilPVal->PValInf.Brother = NIL;
   FilPVal->PValInf.Son = NIL;

   FilPVal->PValInf.LocHdr = NIL;
   FilPVal->PValInf.ValLocPVal = NIL;
   FilPVal->PValInf.DataLocHdr = NIL;

   FilPVal->Father = NIL;
   FilPVal->Brother = NIL;
   FilPVal->Son = NIL;
   return FilPVal;
}

boolean IsRootFilPVal(tp_FilPVal FilPVal)
{
   return (FilPVal->Father == NIL);
}

static tp_FilPVal
Read_PValLayer(tp_LocPVal LocPVal, tp_FilPVal FatherFilPVal)
{
   tps_PValInf _PValInf;
   tp_PValInf PValInf = &_PValInf;
   tp_FilPVal FilPVal;

   if (LocPVal == NIL) {
      return NIL;
   }
   FORBIDDEN(FatherFilPVal->Son != NIL);
   ReadPValInf(PValInf, LocPVal);
   FilPVal = New_FilPVal();
   FilPVal->PValInf = *PValInf;
   FilPVal->Father = FatherFilPVal;
   FilPVal->Brother =
       Read_PValLayer(FilPVal->PValInf.Brother, FatherFilPVal);
   Hash_Item((tp_Item) FilPVal, (tp_Loc) LocPVal);
   return FilPVal;
}

tp_FilPVal
Add_PValInf(tp_FilPVal FilPVal, tp_LocHdr LocHdr, tp_LocPVal LocPVal)
{
   tp_FilPVal TmpFPV;

   FORBIDDEN(FilPVal == ERROR || (LocHdr == ERROR && LocPVal == ERROR));

   if (FilPVal->Son == NIL && FilPVal->PValInf.Son != NIL) {
      FilPVal->Son = Read_PValLayer(FilPVal->PValInf.Son, FilPVal);
   }

   for (TmpFPV = FilPVal->Son; TmpFPV != NIL; TmpFPV = TmpFPV->Brother) {
      if (TmpFPV->PValInf.LocHdr == LocHdr
          && TmpFPV->PValInf.ValLocPVal == LocPVal) {
         return TmpFPV;
      }
   }
   TmpFPV = New_FilPVal();
   TmpFPV->PValInf.LocHdr = LocHdr;
   TmpFPV->PValInf.ValLocPVal = LocPVal;
   TmpFPV->Father = FilPVal;
   TmpFPV->Brother = FilPVal->Son;
   FilPVal->Son = TmpFPV;
   return TmpFPV;
}

tp_FilPVal
Append_PValInf(tp_FilPVal FilPVal, tp_LocHdr LocHdr, tp_LocPVal ValLocPVal)
{
   tp_FilPVal TmpFPV;

   FORBIDDEN(FilPVal == ERROR || (LocHdr == ERROR && ValLocPVal == ERROR));

   if (ValLocPVal != NIL && IsRootFilPVal(LocPVal_FilPVal(ValLocPVal))) {
      return FilPVal;
   }
   for (TmpFPV = FilPVal; !IsRootFilPVal(TmpFPV); TmpFPV = TmpFPV->Father) {
      if (TmpFPV->PValInf.LocHdr == LocHdr
          && TmpFPV->PValInf.ValLocPVal == ValLocPVal) {
         return FilPVal;
      }
   }
   return Add_PValInf(FilPVal, LocHdr, ValLocPVal);
}

tp_FilPVal Append_FilPVal(tp_FilPVal FilPVal1, tp_FilPVal FilPVal2)
{
   if (FilPVal1 == ERROR || FilPVal2 == ERROR)
      return ERROR;
   if (IsRootFilPVal(FilPVal1)) {
      return FilPVal2;
   }
   if (IsRootFilPVal(FilPVal2)) {
      return FilPVal1;
   }
   return Append_PValInf(Append_FilPVal(FilPVal1, FilPVal2->Father),
                         FilPVal2->PValInf.LocHdr,
                         FilPVal2->PValInf.ValLocPVal);
}

tp_LocPVal FilPVal_LocPVal(tp_FilPVal FilPVal)
{
   if (FilPVal == ERROR) {
      return ERROR;
   }
   if (FilPVal->LocPVal == NIL) {
      Hash_Item((tp_Item) FilPVal, (tp_Loc) Alloc_PValInf());
      if (FilPVal->Father != NIL) {
         FilPVal->PValInf.Father = FilPVal_LocPVal(FilPVal->Father);
         FilPVal->PValInf.Brother = FilPVal->Father->PValInf.Son;
         FilPVal->Father->PValInf.Son = FilPVal->LocPVal;
         WriteFilPVal(FilPVal->Father);
      }
      WriteFilPVal(FilPVal);
   }
   return FilPVal->LocPVal;
}

static tp_FilPVal Lookup_FilPVal(tp_LocPVal LocPVal)
{
   return (tp_FilPVal) Lookup_Item(LocPVal);
}

tp_FilPVal LocPVal_FilPVal(tp_LocPVal LocPVal)
{
   tp_FilPVal FilPVal, FatherFilPVal;
   tps_PValInf _PValInf;
   tp_PValInf PValInf = &_PValInf;

   if (LocPVal == ERROR)
      return ERROR;

   FilPVal = Lookup_FilPVal(LocPVal);
   if (FilPVal != ERROR) {
      return FilPVal;
   }

   ReadPValInf(PValInf, LocPVal);
   if (PValInf->Father == NIL) {
      FORBIDDEN(PValInf->LocHdr != NIL);
      FilPVal = New_FilPVal();
      FilPVal->PValInf = *PValInf;
      Hash_Item((tp_Item) FilPVal, (tp_Loc) LocPVal);
      FORBIDDEN(!IsRootFilPVal(FilPVal));
      return FilPVal;
   }
   FatherFilPVal = LocPVal_FilPVal(PValInf->Father);
   FORBIDDEN(FatherFilPVal->Son != NIL
             || FatherFilPVal->PValInf.Son == NIL);
   FatherFilPVal->Son =
       Read_PValLayer(FatherFilPVal->PValInf.Son, FatherFilPVal);
   FilPVal = Lookup_FilPVal(LocPVal);
   FORBIDDEN(FilPVal == ERROR);
   return FilPVal;
}

static void
Print_FilPValLocHdr(tp_FilDsc FilDsc, tp_Str Str, tp_LocHdr LocHdr)
{
   tp_FilHdr FilHdr;

   FilHdr = LocHdr_FilHdr(LocHdr);
   {
      if (IsStr(FilHdr)) {
         {
            if (FilDsc != NIL) {
               Print_Unlex(FilDsc, FilHdr_Ident(FilHdr));
            } else {
               Unlex(Str, FilHdr_Ident(FilHdr));
            }
         }
      } else {
         {
            if (FilDsc != NIL) {
               Write(FilDsc, "(");
               Print_FilHdr(FilDsc, Str, FilHdr);
               Write(FilDsc, ")");
            } else {
               (void) sprintf(Str, "(\\0%d)", (int) LocHdr);
            }
         }
      }
   }
   Ret_FilHdr(FilHdr);
}

static void
Print_ValFilPVal(tp_FilDsc FilDsc, tp_Str Str, tp_FilPVal FilPVal)
{
   FORBIDDEN(IsRootFilPVal(FilPVal));

   if (!IsRootFilPVal(FilPVal->Father)) {
      Print_ValFilPVal(FilDsc, Str, FilPVal->Father);
      {
         if (FilDsc != NIL) {
            Write(FilDsc, " ");
         } else {
            (void) strcat(Str, " ");
         }
      }
   }
   Print_FilPValLocHdr(FilDsc, Tail(Str), FilPVal->PValInf.LocHdr);
}

static void
Print_FilPVal1(tp_FilDsc FilDsc,
               tp_Str Str, tp_PrmTyp PrmTyp, tp_FilPVal FilPVal)
{
   tp_Str TailStr;
   tp_FilHdr FilHdr;

   FORBIDDEN(FilPVal == ERROR);

   if (IsRootFilPVal(FilPVal)) {
      if (FilDsc == NIL) {
         (void) strcpy(Str, "");
      }
      return;
   }

   Print_FilPVal1(FilDsc, Str, PrmTyp, FilPVal->Father);

   TailStr = Tail(Str);
   {
      if (FilDsc != NIL) {
         Write(FilDsc, " +");
      } else {
         (void) strcat(TailStr, " +");
      }
   }

   if (PrmTyp == ApplyPrmTyp) {
      FORBIDDEN(FilPVal->PValInf.LocHdr == NIL);
      FilHdr = LocHdr_FilHdr(FilPVal->PValInf.LocHdr);
      {
         if (FilDsc != NIL) {
            Write(FilDsc, "(");
         } else {
            (void) strcat(TailStr, "(");
         }
      }
      TailStr = Tail(Str);
      Print_FilHdr(FilDsc, TailStr, FilHdr);
      {
         if (FilDsc != NIL) {
            Write(FilDsc, ")");
         } else {
            (void) strcat(TailStr, ")");
         }
      }
      Ret_FilHdr(FilHdr);
      return;
   }

   {
      if (FilDsc != NIL) {
         Print_Unlex(FilDsc, PrmTyp_PTName(PrmTyp));
         Write(FilDsc, "=");
      } else {
         Unlex(Tail(TailStr), PrmTyp_PTName(PrmTyp));
         (void) strcat(TailStr, "=");
      }
   }

   if (FilPVal->PValInf.LocHdr != NIL) {
      Print_FilPValLocHdr(FilDsc, Tail(TailStr), FilPVal->PValInf.LocHdr);
      return;
   }

   Print_ValFilPVal(FilDsc, Tail(TailStr),
                    LocPVal_FilPVal(FilPVal->PValInf.ValLocPVal));
}

void
Print_FilPVal(tp_FilDsc FilDsc,
              tp_Str Str, tp_PrmTyp PrmTyp, tp_FilPVal FilPVal)
{
   FORBIDDEN((FilDsc == NIL) == (Str == NIL));
   FORBIDDEN(PrmTyp == ERROR || FilPVal == ERROR);

   if (IsRootFilPVal(FilPVal)) {
      {
         if (FilDsc != NIL) {
            Write(FilDsc, " +");
            Print_Unlex(FilDsc, PrmTyp_PTName(PrmTyp));
         } else {
            (void) strcpy(Str, " +");
            Unlex(Tail(Str), PrmTyp_PTName(PrmTyp));
         }
      }
      return;
   }

   Print_FilPVal1(FilDsc, Str, PrmTyp, FilPVal);
}

tp_LocHdr FilPVal_LocHdr(tp_FilPVal FilPVal)
{
   if (FilPVal == ERROR) {
      return ERROR;
   }
   return FilPVal->PValInf.LocHdr;
}

tp_LocPVal FilPVal_ValLocPVal(tp_FilPVal FilPVal)
{
   if (FilPVal == ERROR) {
      return ERROR;
   }
   return FilPVal->PValInf.ValLocPVal;
}

void Set_FilPVal_DataLocHdr(tp_FilPVal FilPVal, tp_LocHdr LocHdr)
{
   FORBIDDEN(FilPVal == ERROR);
   if (FilPVal->PValInf.DataLocHdr != LocHdr) {
      FilPVal->PValInf.DataLocHdr = LocHdr;
      WriteFilPVal(FilPVal);
   }
}

tp_LocHdr FilPVal_DataLocHdr(tp_FilPVal FilPVal)
{
   if (FilPVal == ERROR) {
      return ERROR;
   }
   return FilPVal->PValInf.DataLocHdr;
}

tp_FilPVal FilPVal_Father(tp_FilPVal FilPVal)
{
   if (FilPVal == ERROR) {
      return ERROR;
   }
   return FilPVal->Father;
}

static tp_FilPVal
Add_FilHdr_DerefPrmVal(tp_FilPVal FilPVal, tp_FilHdr FilHdr)
{
   tp_FilPVal NewFilPVal;
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr;

   FORBIDDEN(FilPVal == ERROR || FilHdr == ERROR);

   if (!IsRef(FilHdr) || FilHdr_Flag(FilHdr, FLAG_Union)) {
      return Add_PValInf(FilPVal, FilHdr_LocHdr(FilHdr), NIL);
   }

   NewFilPVal = FilPVal;
   FORBIDDEN(!IsElmNameUpToDate(FilHdr));
   Set_Flag(FilHdr, FLAG_Union);
   for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      FORBIDDEN(ElmFilHdr == ERROR);
      NewFilPVal = Add_FilHdr_DerefPrmVal(NewFilPVal, ElmFilHdr);
      Ret_FilHdr(ElmFilHdr);
   }
   Clr_Flag(FilHdr, FLAG_Union);
   return NewFilPVal;
}

static tp_FilPVal ValFilPVal_DerefPrmVal(tp_FilPVal FilPVal)
{
   tp_FilHdr FilHdr;
   tp_FilPVal ValFilPVal;

   FORBIDDEN(FilPVal == ERROR);

   if (IsRootFilPVal(FilPVal)) {
      return FilPVal;
   }

   ValFilPVal = ValFilPVal_DerefPrmVal(FilPVal->Father);
   FilHdr = LocHdr_FilHdr(FilPVal->PValInf.LocHdr);
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(FilPVal->PValInf.ValLocPVal != NIL);
   ValFilPVal = Add_FilHdr_DerefPrmVal(ValFilPVal, FilHdr);
   Ret_FilHdr(FilHdr);
   return ValFilPVal;
}

tp_FilPVal FilPVal_DerefPrmVal(tp_FilPVal FilPVal, tp_PrmTyp PrmTyp)
{
   tp_FilHdr FilHdr;
   tp_FilPVal ValFilPVal, NewFilPVal;

   if (FilPVal == ERROR) {
      return ERROR;
   }

   if (IsRootFilPVal(FilPVal)) {
      return FilPVal;
   }

   NewFilPVal = FilPVal_DerefPrmVal(FilPVal->Father, PrmTyp);
   FilHdr = LocHdr_FilHdr(FilPVal->PValInf.LocHdr);
   if (FilHdr != NIL && !IsRef(FilHdr)) {
      Ret_FilHdr(FilHdr);
      FORBIDDEN(FilPVal->PValInf.ValLocPVal != NIL);
      return Append_PValInf(NewFilPVal, FilPVal->PValInf.LocHdr, NIL);
   }
   {
      if (FilHdr != NIL) {
         ValFilPVal = Add_FilHdr_DerefPrmVal
             (PrmTyp_RootFilPVal(PrmTyp), FilHdr);
         Ret_FilHdr(FilHdr);
      } else {
         ValFilPVal = ValFilPVal_DerefPrmVal
             (LocPVal_FilPVal(FilPVal->PValInf.ValLocPVal));
      }
   }
   if (IsRootFilPVal(ValFilPVal)) {
      return NewFilPVal;
   }
   if (IsRootFilPVal(ValFilPVal->Father)) {
      return Append_PValInf(NewFilPVal, ValFilPVal->PValInf.LocHdr,
                            ValFilPVal->PValInf.ValLocPVal);
   }
   return Append_PValInf(NewFilPVal, NIL, FilPVal_LocPVal(ValFilPVal));
}

void
Chain_FilPVal_DerefPrmVal(tp_LocInp * FirstLIPtr,
                          tp_LocInp * LastLIPtr,
                          tp_FilPVal FilPVal, tp_FilHdr OutFilHdr)
{
   tp_FilPVal TmpFPV;
   tp_FilHdr FilHdr;
   tp_LocInp LocInp;

   for (TmpFPV = FilPVal; !IsRootFilPVal(TmpFPV); TmpFPV = TmpFPV->Father) {
      {
         if (TmpFPV->PValInf.LocHdr != NIL) {
            FilHdr = LocHdr_FilHdr(TmpFPV->PValInf.LocHdr);
            if (IsRef(FilHdr)) {
               LocInp = Make_LocInp(FilHdr, -1, IK_TransName, OutFilHdr);
               Chain_LocInps(FirstLIPtr, LastLIPtr, LocInp);
            }
            Ret_FilHdr(FilHdr);
         } else {
            Chain_FilPVal_DerefPrmVal
                (FirstLIPtr, LastLIPtr,
                 LocPVal_FilPVal(TmpFPV->PValInf.ValLocPVal), OutFilHdr);
         }
      }
   }
}
