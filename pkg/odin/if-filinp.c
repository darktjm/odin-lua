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
#include "inc/FilInp.h"
#include "inc/FKind_.h"
#include "inc/InpSpc.h"
#include "inc/InpInf.h"
#include "inc/InpKind_.h"
#include "inc/Inputs.h"
#include "inc/ISKind_.h"
#include "inc/NodTyp_.h"
#include "inc/Str.h"

int num_FilInpS = 0;

tp_FilInp ModFilInp = NIL;

static tps_FilInp _UsedFilInp;
static tp_FilInp UsedFilInp = &_UsedFilInp;
static tps_FilInp _FreeFilInp;
static tp_FilInp FreeFilInp = &_FreeFilInp;

static tp_FilHdr Do_InpSpc(tp_FilHdr, tp_FilPrm, tp_InpSpc, tp_Tool);

void Init_FilInps(void)
{
   UsedFilInp->PrevFree = UsedFilInp;
   UsedFilInp->NextFree = UsedFilInp;

   FreeFilInp->PrevFree = FreeFilInp;
   FreeFilInp->NextFree = FreeFilInp;
}

static void Transfer_FilInp(tp_FilInp FilInp, tp_FilInp FilInpLst)
{
   FilInp->PrevFree->NextFree = FilInp->NextFree;
   FilInp->NextFree->PrevFree = FilInp->PrevFree;
   FilInp->PrevFree = FilInpLst->PrevFree;
   FilInp->NextFree = FilInpLst;
   FilInp->PrevFree->NextFree = FilInp;
   FilInp->NextFree->PrevFree = FilInp;
}

static tp_FilInp Copy_FilInp(tp_FilInp FilInp)
{
   if (FilInp == ERROR)
      return ERROR;
   if (FilInp->Cnt == 0) {
      Transfer_FilInp(FilInp, UsedFilInp);
   }
   FilInp->Cnt += 1;
   return FilInp;
}

void Ret_FilInp(tp_FilInp FilInp)
{
   if (FilInp == ERROR)
      return;
   FilInp->Cnt -= 1;
   FORBIDDEN(FilInp->Cnt < 0);
}

void Free_FilInps(void)
{
   tp_FilInp FilInp, NextFilInp;

   NextFilInp = UsedFilInp->NextFree;
   while (NextFilInp != UsedFilInp) {
      FilInp = NextFilInp;
      NextFilInp = NextFilInp->NextFree;
      if (FilInp->Cnt == 0) {
         Transfer_FilInp(FilInp, FreeFilInp);
      }
   }
}

static void UnHash_FilInp(tp_FilInp FilInp)
{
   UnHash_Item((tp_Item) FilInp);
}

static tp_FilInp New_FilInp(tp_LocInp LocInp)
{
   tp_FilInp FilInp;
   tp_InpInf InpInf;

   FilInp = FreeFilInp->NextFree;
   {
      if (FilInp == FreeFilInp) {
         FilInp = (tp_FilInp) malloc(sizeof(tps_FilInp));
         num_FilInpS += 1;
         InpInf = &(FilInp->InpInf);
         InpInf->IArg = -1;
         InpInf->LocHdr = NIL;
         InpInf->BackLink = NIL;
         InpInf->Link = NIL;
         InpInf->InpKind = NIL;
         InpInf->OutLocHdr = NIL;
         InpInf->Next = NIL;

         FilInp->LocInp = NIL;
         FilInp->Cnt = 0;
         FilInp->Modified = FALSE;
         FilInp->PrevFree = FreeFilInp->PrevFree;
         FilInp->NextFree = FreeFilInp;
         FilInp->PrevFree->NextFree = FilInp;
         FilInp->NextFree->PrevFree = FilInp;
      } else if (FilInp->LocInp != NIL) {
         FORBIDDEN(FilInp->Cnt != 0);
         if (FilInp->Modified)
            WriteFilInps();
         FORBIDDEN(FilInp->Modified);
         UnHash_FilInp(FilInp);
      }
   }
   Hash_Item((tp_Item) FilInp, (tp_Loc) LocInp);
   return Copy_FilInp(FilInp);
}

static tp_FilInp Lookup_FilInp(tp_LocInp LocInp)
{
   return Copy_FilInp((tp_FilInp) Lookup_Item(LocInp));
}

tp_FilInp LocInp_FilInp(tp_LocInp LocInp)
{
   tp_FilInp FilInp;
   tp_InpInf InpInf;

   if (LocInp == ERROR) {
      return ERROR;
   }

   FilInp = Lookup_FilInp(LocInp);
   if (FilInp != ERROR) {
      return FilInp;
   }

   FilInp = New_FilInp(LocInp);
   InpInf = &(FilInp->InpInf);
   ReadInpInf(InpInf, LocInp);
   return FilInp;
}

static void SetFilInpModified(tp_FilInp FilInp)
{
   if (FilInp->Modified)
      return;
   FilInp->Modified = TRUE;
   FilInp->NextMod = ModFilInp;
   ModFilInp = FilInp;
}

static void WriteFilInp(tp_FilInp FilInp)
{
   WriteInpInf(&(FilInp->InpInf), FilInp->LocInp);
}

void WriteFilInps(void)
{
   while (ModFilInp != NIL) {
      FORBIDDEN(!ModFilInp->Modified);
      ModFilInp->Modified = FALSE;
      WriteFilInp(ModFilInp);
      ModFilInp = ModFilInp->NextMod;
   }
}

static tp_LocInp Alloc_InpInf(void)
{
   return (tp_LocInp) Alloc(sizeof(tps_InpInf));
}

boolean FilInps_InUse(void)
{
   tp_FilInp FilInp;

   Free_FilInps();
   FilInp = UsedFilInp->NextFree;
   while (FilInp != UsedFilInp) {
      Write(StdOutFD, "LocInp=");
      WriteInt(StdOutFD, (int) FilInp->LocInp);
      Write(StdOutFD, ", Cnt=");
      WriteInt(StdOutFD, FilInp->Cnt);
      Writeln(StdOutFD, "");
      FilInp = FilInp->NextFree;
   }
   return (UsedFilInp->NextFree != UsedFilInp);
}

tp_FilHdr FilInp_FilHdr(tp_FilInp FilInp)
{
   tp_FilHdr FilHdr;

   if (FilInp == ERROR) {
      return ERROR;
   }
   FilHdr = LocHdr_FilHdr(FilInp->InpInf.LocHdr);
   return FilHdr;
}

tp_LocHdr FilInp_OutLocHdr(tp_FilInp FilInp)
{
   return FilInp->InpInf.OutLocHdr;
}

int FilInp_IArg(tp_FilInp FilInp)
{
   return FilInp->InpInf.IArg;
}

tp_InpKind FilInp_InpKind(tp_FilInp FilInp)
{
   return FilInp->InpInf.InpKind;
}

tp_FilInp FilInp_NextFilInp(tp_FilInp FilInp)
{
   tp_LocInp LocInp;

   FORBIDDEN(FilInp == ERROR);
   LocInp = FilInp->InpInf.Next;
   Ret_FilInp(FilInp);
   return LocInp_FilInp(LocInp);
}

tp_LocInp FilInp_Link(tp_FilInp FilInp)
{
   return FilInp->InpInf.Link;
}

static void Link_LocInp(tp_LocInp LocInp, tp_FilHdr FilHdr)
{
   tp_FilInp FilInp, RiteFilInp, LeftFilInp;
   tp_InpInf InpInf;
   tp_LocInp RiteLocInp, LeftLocInp;

   RiteLocInp = FilHdr_InpLink(FilHdr);
   {
      if (RiteLocInp == NIL) {
         Set_InpLink(FilHdr, LocInp);
         LeftLocInp = LocInp;
         RiteLocInp = LocInp;
      } else {
         RiteFilInp = LocInp_FilInp(RiteLocInp);
         FORBIDDEN(RiteFilInp->InpInf.LocHdr != FilHdr_LocHdr(FilHdr));
         LeftLocInp = RiteFilInp->InpInf.BackLink;
         RiteFilInp->InpInf.BackLink = LocInp;
         SetFilInpModified(RiteFilInp);
         Ret_FilInp(RiteFilInp);

         LeftFilInp = LocInp_FilInp(LeftLocInp);
         LeftFilInp->InpInf.Link = LocInp;
         SetFilInpModified(LeftFilInp);
         Ret_FilInp(LeftFilInp);
      }
   }

   FilInp = LocInp_FilInp(LocInp);
   InpInf = &(FilInp->InpInf);
   FORBIDDEN(InpInf->LocHdr != ERROR);
   FORBIDDEN(InpInf->BackLink != NIL || InpInf->Link != NIL);
   InpInf->LocHdr = FilHdr_LocHdr(FilHdr);
   InpInf->BackLink = LeftLocInp;
   InpInf->Link = RiteLocInp;
   SetFilInpModified(FilInp);
   Ret_FilInp(FilInp);
}

tp_LocInp
Make_LocInp(tp_FilHdr FilHdr,
            int IArg, tp_InpKind InpKind, tp_FilHdr OutFilHdr)
{
   tp_FilInp FilInp;
   tp_InpInf InpInf;
   tp_LocInp LocInp;

   FORBIDDEN(FilHdr == ERROR || InpKind == ERROR || OutFilHdr == ERROR);

   LocInp = Alloc_InpInf();
   FilInp = New_FilInp(LocInp);
   InpInf = &(FilInp->InpInf);
   InpInf->IArg = IArg;
   InpInf->LocHdr = ERROR;
   InpInf->BackLink = NIL;
   InpInf->Link = NIL;
   InpInf->InpKind = InpKind;
   if (InpKind == IK_Trans && !IsRef(FilHdr)) {
      InpInf->InpKind = IK_Simple;
   }
   if (InpKind == IK_TransName && !IsRef(FilHdr)) {
      InpInf->InpKind = IK_Name;
   }
   InpInf->OutLocHdr = FilHdr_LocHdr(OutFilHdr);
   InpInf->Next = NIL;
   SetFilInpModified(FilInp);
   Ret_FilInp(FilInp);

   Link_LocInp(LocInp, FilHdr);

   return LocInp;
}

void
Chain_LocInps(tp_LocInp * FirstLocInpPtr,
              tp_LocInp * LastLocInpPtr, tp_LocInp LocInp)
{
   tp_FilInp PrvFilInp;

   FORBIDDEN(LocInp == NIL);
   if (*FirstLocInpPtr == NIL) {
      FORBIDDEN(*LastLocInpPtr != NIL);
      *FirstLocInpPtr = LocInp;
      *LastLocInpPtr = LocInp;
      return;
   }
   PrvFilInp = LocInp_FilInp(*LastLocInpPtr);
   PrvFilInp->InpInf.Next = LocInp;
   SetFilInpModified(PrvFilInp);
   Ret_FilInp(PrvFilInp);
   *LastLocInpPtr = LocInp;
}

static tp_Str Expand_InpSpcStr(tp_InpSpc InpSpc)
{
   tp_Str Str;

   if (InpSpc->Str == NIL) {
      return NIL;
   }
   Str = InpSpc->Str;
   if (InpSpc->IsEnvVar) {
      Str = GetEnv(InpSpc->Str);
      if (Str == NIL) {
         Str = "";
      }
   }
   return Str;
}

static void
Get_InpSpc_PrmVals(tp_LocHdr * LocHdrPtr,
                   tp_LocPVal * LocPValPtr,
                   tp_InpSpc InpSpc,
                   tp_Str Str,
                   tp_FilHdr BaseFilHdr, tp_FilPrm InhFilPrm, tp_Tool Tool)
{
   tp_FilHdr PrmValFilHdr;
   tp_InpSpc ValInpSpc;
   tp_FilPVal FilPVal;
   tp_LocHdr LocHdr;
   tp_LocPVal LocPVal;

   *LocHdrPtr = NIL;
   *LocPValPtr = NIL;

   if (Str == NIL && InpSpc->InpSpc == NIL) {
      *LocPValPtr = FilPVal_LocPVal(PrmTyp_RootFilPVal(InpSpc->PrmTyp));
      return;
   }

   if (Str != NIL) {
      PrmValFilHdr = Str_FilHdr(Sym_Str(Str_Sym(Str)), InpSpc->PrmTyp);
      *LocHdrPtr = FilHdr_LocHdr(PrmValFilHdr);
      Ret_FilHdr(PrmValFilHdr);
      return;
   }

   if (InpSpc->InpSpc->ISKind == ISK_PrmVal) {
      FilPVal = PrmTyp_RootFilPVal(InpSpc->PrmTyp);
      for (ValInpSpc = InpSpc->InpSpc;
           ValInpSpc != NIL; ValInpSpc = ValInpSpc->Next) {
         Get_InpSpc_PrmVals(&LocHdr, &LocPVal,
                            ValInpSpc, Expand_InpSpcStr(ValInpSpc),
                            BaseFilHdr, InhFilPrm, Tool);
         FilPVal = Add_PValInf(FilPVal, LocHdr, LocPVal);
      }
      *LocPValPtr = FilPVal_LocPVal(FilPVal);
      return;
   }

   PrmValFilHdr = Do_InpSpc(Copy_FilHdr(BaseFilHdr),
                            InhFilPrm, InpSpc->InpSpc, Tool);
   *LocHdrPtr = FilHdr_LocHdr(PrmValFilHdr);
   Ret_FilHdr(PrmValFilHdr);
}

static tp_FilHdr
Do_InpSpc(tp_FilHdr BaseFilHdr,
          tp_FilPrm InhFilPrm, tp_InpSpc InpSpc, tp_Tool Tool)
{
   tp_InpSpc OpInpSpc;
   tp_FilHdr FilHdr;
   tp_FilPrm PrecFilPrm;
   tp_FilHdr ElmFilHdr;
   tp_LocHdr LocHdr;
   tp_LocPVal LocPVal;
   tp_Str Str;
   tps_Str PkgDirName;

   PrecFilPrm = RootFilPrm;
   Str = Expand_InpSpcStr(InpSpc);
   OpInpSpc = InpSpc->Next;
   switch (InpSpc->ISKind) {
   case ISK_EmptyFile:{
         FilHdr = Copy_FilHdr(EmptyFilHdr);
         break;
      }
   case ISK_Str:{
         FilHdr = Str_FilHdr(Str, NullPrmTyp);
         break;
      }
   case ISK_Key:{
         Get_PkgDirName(PkgDirName, Tool_Package(Tool));
         Push_ContextFilHdr(HostFN_FilHdr(PkgDirName));
         FilHdr = HostFN_FilHdr(Str);
         Pop_ContextFilHdr();
         break;
      }
   case ISK_Drv:{
         FilHdr = Do_Deriv(Copy_FilHdr(BaseFilHdr),
                           InhFilPrm, PrecFilPrm, InpSpc->FilTyp);
         break;
      }
   case ISK_Prm:{
         FilHdr = Get_FPVFilHdr(InpSpc->PrmTyp, InhFilPrm);
         if (FilHdr == NilStrFilHdr) {
            Ret_FilHdr(BaseFilHdr);
            return FilHdr;
         }
         break;
      }
   case ISK_Sel:{
         FilHdr = Copy_FilHdr(RootFilHdr);
         if (Str != NIL)
            FilHdr = Do_Keys(FilHdr, Str);
         break;
      }
   case ISK_VTgt:{
         Get_PkgDirName(PkgDirName, Tool_Package(Tool));
         FilHdr = Get_BaseVTgtFilHdr(HostFN_FilHdr(PkgDirName));
         OpInpSpc = InpSpc;
         break;
      }
   default:{
         FATALERROR("bad ISKind.\n");
      }
   }

   while (OpInpSpc != NIL) {
      Str = Expand_InpSpcStr(OpInpSpc);
      switch (OpInpSpc->ISKind) {
      case ISK_Drv:{
            FilHdr =
                Do_Deriv(FilHdr, InhFilPrm, PrecFilPrm, OpInpSpc->FilTyp);
            PrecFilPrm = RootFilPrm;
            break;
         }
      case ISK_Prm:{
            Get_InpSpc_PrmVals(&LocHdr, &LocPVal,
                               OpInpSpc, Str, BaseFilHdr, InhFilPrm, Tool);
            PrecFilPrm = Append_PrmInf(PrecFilPrm, OpInpSpc->PrmTyp,
                                       LocHdr, LocPVal);
            break;
         }
      case ISK_Sel:{
            FilHdr = Do_Key(FilHdr, "");
            if (Str != NIL) {
               FilHdr = Do_Keys(FilHdr, Str);
            }
            PrecFilPrm = RootFilPrm;
            break;
         }
      case ISK_VTgt:{
            FilHdr = Do_VTgt(FilHdr, Str);
            PrecFilPrm = RootFilPrm;
            break;
         }
      default:{
            FATALERROR("bad ISKind.\n");
         }
      }
      OpInpSpc = OpInpSpc->Next;
   }

   if (PrecFilPrm != RootFilPrm) {
      ElmFilHdr = Copy_FilHdr(FilHdr);
      FilHdr =
          Get_Drv(FilHdr, FK_InpPntr, ObjectFilTyp, RootFilPrm, DfltIdent);
      Set_LocElm(FilHdr, Make_LocElm(ElmFilHdr, PrecFilPrm, FilHdr));
      Ret_FilHdr(ElmFilHdr);
   }

   Ret_FilHdr(BaseFilHdr);
   return FilHdr;
}

tp_LocInp Get_LocInp(tp_FilHdr FilHdr)
{
   tp_LocInp FirstLI, LastLI;
   tp_FilHdr ToolFilHdr, BaseFilHdr, InpFilHdr;
   tp_Tool Tool;
   tp_FilPrm FilPrm;
   tp_InpEdg InpEdg;
   tp_InpKind InpKind;
   int IArg, InpNum;
   tp_InpSpc InpSpc;
   tp_LocInp LocInp;

   FirstLI = NIL;
   LastLI = NIL;
   ToolFilHdr = Copy_FilHdr(FilHdr);
   if (IsInstance(ToolFilHdr)) {
      ToolFilHdr = FilHdr_Father(ToolFilHdr);
   }
   BaseFilHdr = FilHdr_Father(Copy_FilHdr(ToolFilHdr));

   switch (FilHdr_FKind(ToolFilHdr)) {
   case FK_SrcReg:
   case FK_SrcDir:
   case FK_SymLinkReg:
   case FK_SymLinkDir:
   case FK_BoundSrc:
   case FK_BoundSymLink:
   case FK_Str:{
         break;
      }
   case FK_User:{
         Tool = FilHdr_Tool(ToolFilHdr);
         FilPrm = FilHdr_FilPrm(ToolFilHdr);
         for (InpEdg = Tool_InpEdg(Tool), IArg = 0;
              InpEdg != NIL; InpEdg = InpEdg_Next(InpEdg), IArg += 1) {
            InpSpc = InpEdg_InpSpc(InpEdg);
            if (InpSpc->FilTyp != NoInputFilTyp) {
               InpFilHdr = Do_InpSpc(Copy_FilHdr(BaseFilHdr),
                                     FilPrm, InpSpc, Tool);
               if (InpFilHdr != NilStrFilHdr) {
                  InpNum = (InpEdg_IsUserArg(InpEdg) ? IArg : -1);
                  InpKind = InpEdg_InpKind(InpEdg);
                  LocInp = Make_LocInp(InpFilHdr, InpNum, InpKind, FilHdr);
                  Chain_LocInps(&FirstLI, &LastLI, LocInp);
               }
               Ret_FilHdr(InpFilHdr);
            }
         }
         if (IsDerefPrmVal_Tool(Tool)) {
            Chain_FilPrm_DerefPrmVal(&FirstLI, &LastLI, FilPrm, FilHdr);
         }
         break;
      }
   case FK_DrvDirElm:
   case FK_VirDirElm:
   case FK_ActTgtText:
   case FK_VirTgtText:
   case FK_ActTgtExText:
   case FK_VirTgtExText:
   case FK_VirTgt:
   case FK_VirCmdTgt:
   case FK_ActTgt:
   case FK_ActCmdTgt:{
         FirstLI = Make_LocInp(BaseFilHdr, 0, IK_Simple, FilHdr);
         break;
      }
   case FK_PntrHo:{
         FirstLI = Make_LocInp(BaseFilHdr, 0, IK_Pntr, FilHdr);
         break;
      }
   case FK_PntrElm:{
         FirstLI = Make_LocInp(BaseFilHdr, 0, IK_TransName, FilHdr);
         break;
      }
   case FK_InpPntr:{
         break;
      }
   default:{
         FATALERROR("Bad FKind");
      }
   }
   Ret_FilHdr(BaseFilHdr);
   Ret_FilHdr(ToolFilHdr);
   return FirstLI;
}
