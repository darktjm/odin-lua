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
#include "inc/FKind_.h"
#include "inc/Flag_.h"
#include "inc/SKind_.h"
#include "inc/Status_.h"
#include "inc/Str.h"

tp_Date PendingDate = 0;

boolean IsSource_FKind(tp_FKind FKind)
{
   switch (FKind) {
   case FK_SrcReg:
   case FK_SrcDir:
   case FK_SymLinkReg:
   case FK_SymLinkDir:
   case FK_BoundSrc:
   case FK_BoundSymLink:{
         return TRUE;
      }
   case FK_User:
   case FK_Instance:
   case FK_Str:
   case FK_DrvDirElm:
   case FK_VirDirElm:
   case FK_PntrHo:
   case FK_InpPntr:
   case FK_PntrElm:
   case FK_ActTgtText:
   case FK_VirTgtText:
   case FK_ActTgtExText:
   case FK_VirTgtExText:
   case FK_VirTgt:
   case FK_VirCmdTgt:
   case FK_ActTgt:
   case FK_ActCmdTgt:{
         return FALSE;
      }
   default:{
         FATALERROR("unknown FKind");
      }
   }
   /* NOTREACHED */
   return FALSE;
}

boolean IsSource(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return IsSource_FKind(FilHdr->HdrInf.FKind);
}

boolean IsSymLink(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.FKind == FK_SymLinkReg
           || FilHdr->HdrInf.FKind == FK_SymLinkDir
           || FilHdr->HdrInf.FKind == FK_BoundSymLink);
}

boolean IsDir(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.FKind == FK_SrcDir
           || FilHdr->HdrInf.FKind == FK_SymLinkDir);
}

boolean IsStr(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.FKind == FK_Str);
}

boolean IsBound(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.FKind == FK_BoundSrc
           || FilHdr->HdrInf.FKind == FK_BoundSymLink);
}

boolean IsATgt(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return IsATgt_FKind(FilHdr->HdrInf.FKind);
}

boolean IsVTgt(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return IsVTgt_FKind(FilHdr->HdrInf.FKind);
}

boolean IsVTgtText(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return IsVTgtText_FKind(FilHdr->HdrInf.FKind);
}

boolean IsDfltTgtVal(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->FilTyp == ActTargetsFilTyp);
}

boolean IsPntr(tp_FilHdr FilHdr)
{
   tp_FKind FKind;

   FORBIDDEN(FilHdr == ERROR);
   FKind = FilHdr->HdrInf.FKind;
   return (IsPntr_FKind(FKind)
           || (FKind == FK_User && IsPntr_FilTyp(FilHdr->FilTyp)));
}

boolean IsGeneric(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.FKind == FK_User
           && IsGeneric_FilTyp(FilHdr->FilTyp));
}

boolean IsPipe(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.FKind == FK_User
           && IsPipe_FilTyp(FilHdr->FilTyp));
}

boolean IsInstance(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.FKind == FK_Instance);
}

boolean IsAtmc(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   switch (FilHdr->HdrInf.FKind) {
   case FK_SrcReg:
   case FK_SrcDir:
   case FK_SymLinkReg:
   case FK_SymLinkDir:
   case FK_BoundSrc:
   case FK_BoundSymLink:
   case FK_DrvDirElm:
   case FK_Instance:
   case FK_ActTgtText:
   case FK_VirTgtText:
   case FK_ActTgtExText:
   case FK_VirTgtExText:{
         return TRUE;
      }
   case FK_User:
   case FK_Str:{
         return IsAtmc_FilTyp(FilHdr->FilTyp);
      }
   case FK_ActTgt:
   case FK_ActCmdTgt:
   case FK_VirTgt:
   case FK_VirCmdTgt:
   case FK_PntrHo:
   case FK_PntrElm:
   case FK_InpPntr:
   case FK_VirDirElm:{
         return FALSE;
      }
   default:{
         FATALERROR("Unexpected FKind");
      }
   }
    /*NOTREACHED*/ return FALSE;
}

boolean IsList(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.FKind == FK_User
           && IsList_FilTyp(FilHdr->FilTyp));
}

boolean IsViewSpec(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.FKind == FK_User
           && FilHdr->FilTyp == ViewSpecFilTyp);
}

boolean IsStruct(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.FKind != FK_User) {
      return FALSE;
   }
   return IsStruct_FilTyp(FilHdr->FilTyp);
}

boolean IsStructMem(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.FKind != FK_User) {
      return FALSE;
   }
   return IsStructMem_FilTyp(FilHdr->FilTyp);
}

boolean IsVoid(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (IsStr(FilHdr) || IsVoid_FilTyp(FilHdr->FilTyp));
}

boolean IsTargetsPtr(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.FKind != FK_User) {
      return FALSE;
   }
   return (FilHdr->FilTyp == TargetsPtrFilTyp);
}

boolean IsTargets(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.FKind != FK_User) {
      return FALSE;
   }
   return (FilHdr->FilTyp == TargetsFilTyp);
}

boolean IsDrvDir(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.FKind != FK_User) {
      return FALSE;
   }
   return IsDrvDir_FilTyp(FilHdr->FilTyp);
}

boolean IsDrvDirElm(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.FKind == FK_DrvDirElm);
}

boolean IsVirDir(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.FKind != FK_User) {
      return FALSE;
   }
   return (FilHdr->FilTyp == VirDirFilTyp);
}

boolean IsKeyList(tp_FilHdr FilHdr)
{
   tp_FilTyp FilTyp;

   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.FKind != FK_User) {
      return FALSE;
   }
   FilTyp = FilHdr->FilTyp;
   return ((IsDrvDir_FilTyp(FilTyp) && FilTyp != VirDirFilTyp)
           || FilTyp == ActTargetsFilTyp || FilTyp == VirTargetsFilTyp);
}

boolean IsKeyListElm(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (FilHdr->HdrInf.FKind == FK_DrvDirElm
           || FilHdr->HdrInf.FKind == FK_VirDirElm
           || IsATgt_FKind(FilHdr->HdrInf.FKind)
           || IsVTgt_FKind(FilHdr->HdrInf.FKind));
}

boolean IsCopy(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.FKind != FK_User) {
      return FALSE;
   }
   return IsCopy_FilTyp(FilHdr->FilTyp);
}

boolean IsAutoExec(tp_FilHdr FilHdr)
{
   tp_FilHdr TgtValFilHdr;
   boolean Flag;

   if (IsExec_FilTyp(FilHdr->FilTyp)
       || FilHdr->HdrInf.FKind == FK_ActCmdTgt
       || FilHdr->HdrInf.FKind == FK_VirCmdTgt) {
      return TRUE;
   }
   if (!IsBound(FilHdr)) {
      return FALSE;
   }
   TgtValFilHdr = FilHdr_Father(FilHdr_TgtValFilHdr(Copy_FilHdr(FilHdr)));
   FORBIDDEN(TgtValFilHdr == NIL);
   Flag = IsAutoExec(TgtValFilHdr);
   Ret_FilHdr(TgtValFilHdr);
   return Flag;
}

boolean HasKey_FKind(tp_FKind FKind)
{
   FORBIDDEN(FKind == ERROR);
   return (FKind == FK_DrvDirElm || FKind == FK_VirDirElm
           || FKind == FK_PntrElm || IsSource_FKind(FKind)
           || IsATgt_FKind(FKind) || IsVTgt_FKind(FKind)
           || IsATgtText_FKind(FKind) || IsVTgtText_FKind(FKind));
}

boolean IsRef(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return (IsList(FilHdr) || IsPntr(FilHdr));
}

tp_LocHdr FilHdr_LocHdr(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   return FilHdr->LocHdr;
}

tp_LocHdr FilHdr_AliasLocHdr(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   return FilHdr->HdrInf.AliasLocHdr;
}

void Set_AliasLocHdr(tp_FilHdr FilHdr, tp_LocHdr LocHdr)
{

   FORBIDDEN(FilHdr == ERROR);
   if (LocHdr == FilHdr->HdrInf.AliasLocHdr) {
      return;
   }
   FilHdr->HdrInf.AliasLocHdr = LocHdr;
   SetModified(FilHdr);
}

tp_FilHdr FilHdr_AliasFilHdr(tp_FilHdr FilHdr)
{
   tp_FilHdr AliasFilHdr;

   if (FilHdr->HdrInf.AliasLocHdr == NIL) {
      return FilHdr;
   }
   AliasFilHdr = LocHdr_FilHdr(FilHdr->HdrInf.AliasLocHdr);
   Ret_FilHdr(FilHdr);
   FORBIDDEN(AliasFilHdr->HdrInf.AliasLocHdr != NIL);
   return AliasFilHdr;
}

tp_FKind FilHdr_FKind(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   return FilHdr->HdrInf.FKind;
}

void Set_FKind(tp_FilHdr FilHdr, tp_FKind FKind)
{
   FORBIDDEN(FilHdr == ERROR || FKind == ERROR);
   if (FilHdr->HdrInf.FKind != FKind) {
      FilHdr->HdrInf.FKind = FKind;
      SetModified(FilHdr);
   }
}

tp_FilTyp FilHdr_FilTyp(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   return FilHdr->FilTyp;
}

tp_FilPrm FilHdr_FilPrm(tp_FilHdr FilHdr)
{
   if (FilHdr == ERROR)
      return ERROR;
   FORBIDDEN(FilHdr->FilPrm == NIL);
   return FilHdr->FilPrm;
}

tp_Ident FilHdr_Ident(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   return FilHdr->Ident;
}

void Update_SrcFilHdr(tp_FilHdr FilHdr, boolean InitOnly)
{
   tp_Date OldModDate;
   boolean Changed;
   tp_SKind SKind;
   tp_FKind FKind;
   tp_HdrInf HdrInf;
   tp_Status Status;
   tps_FileName FileName, SymLinkFileName;
   int SysModTime;
   tp_FilHdr DirFilHdr, SymLinkFH;
   tp_FilElm FilElm;
   tp_LocHdr SymLocHdr;

   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(!IsSource(FilHdr));
   HdrInf = &(FilHdr->HdrInf);

   if (FilHdr == RootFilHdr) {
      Set_Status(FilHdr, STAT_OK);
      return;
   }

   if (FilHdr_Flag(FilHdr, FLAG_SymLink)) {
      Set_Status(FilHdr, STAT_Circular);
      return;
   }
   Set_Flag(FilHdr, FLAG_SymLink);

   OldModDate = HdrInf->ModDate;
   IsAny_ReadyServerAction = TRUE;

   DirFilHdr = FilHdr_Father(Copy_FilHdr(FilHdr));
   if (!IsSrcUpToDate(DirFilHdr)) {
      Update_SrcFilHdr(DirFilHdr, InitOnly);
   }

   Changed = FALSE;
   SymLinkFH = NIL;
   {
      if (IsSymLink(DirFilHdr)) {
         SymLinkFH = Extend_FilHdr
             (Deref_SymLink(Copy_FilHdr(DirFilHdr)), FK_SrcReg,
              FilHdr->FilTyp, RootFilPrm, FilHdr->Ident);
      } else {
         FilHdr_DataFileName(FileName, FilHdr);
         Get_FileInfo(&SKind, &SysModTime, FileName);
         if (HdrInf->SysModTime != SysModTime || HdrInf->ModDate == 0) {
            Changed = TRUE;
         }
         switch (SKind) {
         case SK_NoFile:
         case SK_Reg:
         case SK_Exec:
         case SK_Special:{
               FKind = (IsBound(FilHdr) ? FK_BoundSrc : FK_SrcReg);
               Status = ((SKind == SK_NoFile) ? STAT_NoFile : STAT_OK);
               break;
            }
         case SK_Dir:{
               FKind = FK_SrcDir;
               Status = STAT_OK;
               break;
            }
         case SK_SymLink:{
               Push_ContextFilHdr(Copy_FilHdr(DirFilHdr));
               FileName_SymLinkFileName(SymLinkFileName, FileName);
               SymLinkFH = HostFN_FilHdr(SymLinkFileName);
               {
                  if (SymLinkFH == ERROR) {
                     FKind = FK_SrcReg;
                     Status = STAT_NoFile;
                  } else if (!IsSource(SymLinkFH)) {
                     SystemError("Symbolic link into cache ignored: %s\n",
                                 SymLinkFileName);
                     Ret_FilHdr(SymLinkFH);
                     SymLinkFH = ERROR;
                     FKind = FK_SrcReg;
                     Status = STAT_NoFile;
                  }
               }
               Pop_ContextFilHdr();
               break;
            }
         default:{
               FATALERROR("Unexpected SKind");
            }
         }
      }
   }

   if (SymLinkFH != NIL) {
      if (!IsSrcUpToDate(SymLinkFH)) {
         Update_SrcFilHdr(SymLinkFH, InitOnly);
      }
      SymLinkFH = Deref_SymLink(SymLinkFH);
      if (!IsSrcUpToDate(SymLinkFH)) {
         Update_SrcFilHdr(SymLinkFH, InitOnly);
      }
      SymLinkFH = Deref_SymLink(SymLinkFH);
      if (HdrInf->ModDate < SymLinkFH->HdrInf.ModDate) {
         Changed = TRUE;
      }

      FilElm = LocElm_FilElm(HdrInf->LocElm);
      SymLocHdr = FilElm_LocHdr(FilElm);
      Ret_FilElm(FilElm);
      if (SymLocHdr != SymLinkFH->LocHdr) {
         Set_LocElm(FilHdr, Make_LocElm(SymLinkFH, RootFilPrm, FilHdr));
         Changed = TRUE;
      }

      if (HdrInf->AliasLocHdr != NIL) {
         if (SymLinkFH->HdrInf.AliasLocHdr == NIL) {
            Set_AliasLocHdr(SymLinkFH, HdrInf->AliasLocHdr);
         }
         Set_AliasLocHdr(FilHdr, (tp_LocHdr) NIL);
      }

      FKind = (IsBound(FilHdr) ? FK_BoundSymLink :
               (IsDir(SymLinkFH) ? FK_SymLinkDir : FK_SymLinkReg));
      Status = SymLinkFH->HdrInf.Status;
      Ret_FilHdr(SymLinkFH);
   }

   if (HdrInf->FKind == FK_SrcDir && FKind == FK_SymLinkDir) {
      FilHdr_Error
          ("<%s> has changed from a directory to a symbolic link.\n",
           FilHdr);
      SystemError("The cache should be reset with the -r option.\n");
      FKind = FK_SrcDir;
   }

   if (Changed || HdrInf->FKind != FKind) {
      Set_ModDate(FilHdr);
      HdrInf->SysModTime = SysModTime;
      HdrInf->FKind = FKind;
      if (!IsSymLink(FilHdr)) {
         Set_LocElm(FilHdr, (tp_LocElm) NIL);
      }
   }

   {
      if (IsDir(FilHdr)) {
         Set_TgtValLocElm(FilHdr, (tp_LocElm) NIL);
      } else if (FilHdr_TgtValLocElm(FilHdr) == NIL) {
         Set_DfltTgtValLocElm(FilHdr);
      }
   }

   if (OldModDate != 0
       && (HdrInf->ModDate != OldModDate
           || (HdrInf->Status != STAT_Unknown
               && HdrInf->Status != Status))) {
      {
         if (InitOnly) {
            Push_ToBroadcast(Copy_FilHdr(FilHdr));
         } else {
            Broadcast(FilHdr, STAT_Unknown);
         }
      }
   }

   Ret_FilHdr(DirFilHdr);
   Set_Status(FilHdr, Status);
   Clr_Flag(FilHdr, FLAG_SymLink);
}

void FilHdr_Error(tp_Str Str, tp_FilHdr FilHdr)
{
   tps_Str ObjName;

   SPrint_FilHdr(ObjName, FilHdr);
   SystemError(Str, ObjName);
}

boolean IsAllDone(tp_FilHdr FilHdr, tp_InpKind InpKind)
{
   return (IsAllUpToDate(FilHdr, InpKind)
           && !Is_PRB_Status(FilHdr_MinStatus(FilHdr, InpKind)));
}

boolean IsAllUpToDate(tp_FilHdr FilHdr, tp_InpKind InpKind)
{
   if (!IsUpToDate(FilHdr)) {
      return FALSE;
   }
   if (!IsTgtValUpToDate(FilHdr)) {
      return FALSE;
   }
   if (NeedsElmData(FilHdr, InpKind)) {
      return IsElmUpToDate(FilHdr);
   }
   if (NeedsElmNameData(FilHdr, InpKind)) {
      return IsElmNameUpToDate(FilHdr);
   }
   return TRUE;
}

boolean IsSrcUpToDate(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.VerifyDate < VerifyDate) {
      return FALSE;
   }
   return (FilHdr->HdrInf.Status > STAT_Unknown);
}

boolean IsUpToDate(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (FilHdr->HdrInf.VerifyDate < VerifyDate) {
      return FALSE;
   }
   if (!IsTgtValUpToDate(FilHdr)
       || ((Is_PRB_Status(FilHdr->HdrInf.Status) || FilHdr->PndFlag)
           && FilHdr->HdrInf.VerifyDate < PendingDate)) {
      return FALSE;
   }
   return (FilHdr->HdrInf.Status > STAT_Unknown);
}

boolean IsElmNameUpToDate(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(!IsRef(FilHdr));
   if (FilHdr->HdrInf.Status <= STAT_Error) {
      return TRUE;
   }
   if (FilHdr->HdrInf.ElmNameVerifyDate < VerifyDate) {
      return FALSE;
   }
   if ((Is_PRB_Status(FilHdr->HdrInf.ElmNameStatus)
        || FilHdr->ElmNamePndFlag)
       && FilHdr->HdrInf.ElmNameVerifyDate < PendingDate) {
      return FALSE;
   }
   return (FilHdr->HdrInf.ElmNameStatus > STAT_Unknown);
}

boolean IsElmUpToDate(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   FORBIDDEN(!IsRef(FilHdr));
   if (FilHdr->HdrInf.Status <= STAT_Error) {
      return TRUE;
   }
   if (FilHdr->HdrInf.ElmVerifyDate < VerifyDate) {
      return FALSE;
   }
   if ((Is_PRB_Status(FilHdr->HdrInf.ElmStatus) || FilHdr->ElmPndFlag)
       && FilHdr->HdrInf.ElmVerifyDate < PendingDate) {
      return FALSE;
   }
   return (FilHdr->HdrInf.ElmStatus > STAT_Unknown);
}

boolean IsTgtValUpToDate(tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);
   if (!IsSource(FilHdr)) {
      return TRUE;
   }
   if (FilHdr->HdrInf.ElmVerifyDate < VerifyDate) {
      return FALSE;
   }
   if ((Is_PRB_Status(FilHdr->HdrInf.ElmStatus) || FilHdr->ElmPndFlag)
       && FilHdr->HdrInf.ElmVerifyDate < PendingDate) {
      return FALSE;
   }
   return (FilHdr->HdrInf.ElmStatus > STAT_Unknown);
}

tp_FilHdr FilHdr_Father(tp_FilHdr FilHdr)
{
   tp_LocHdr FatherLocHdr;

   FORBIDDEN(FilHdr == ERROR);
   FatherLocHdr = FilHdr->HdrInf.Father;
   Ret_FilHdr(FilHdr);
   return LocHdr_FilHdr(FatherLocHdr);
}

tp_FilHdr FilHdr_SrcFilHdr(tp_FilHdr FilHdr)
{
   tp_LocHdr SrcLocHdr;

   while (!IsSource(FilHdr)) {
      SrcLocHdr = FilHdr->HdrInf.Father;
      FORBIDDEN(SrcLocHdr == ERROR);
      Ret_FilHdr(FilHdr);
      FilHdr = LocHdr_FilHdr(SrcLocHdr);
   }
   return FilHdr;
}

tp_FilHdr FilHdr_DirFilHdr(tp_FilHdr FilHdr)
{
   tp_FilHdr DirFilHdr;

   DirFilHdr = FilHdr_SrcFilHdr(FilHdr);
   if (DirFilHdr == RootFilHdr) {
      return DirFilHdr;
   }
   return FilHdr_Father(DirFilHdr);
}

tp_Str FilHdr_Key(tp_Str StrBuf, tp_FilHdr FilHdr)
{
   FORBIDDEN(FilHdr == ERROR);

   if (!HasKey_FKind(FilHdr->HdrInf.FKind)) {
      return NIL;
   }
   return FilHdr_Label(StrBuf, FilHdr, FALSE);
}

tp_Label FilHdr_Label(tp_Str StrBuf, tp_FilHdr FilHdr, boolean UniqueFlag)
{
   FORBIDDEN(FilHdr == ERROR);

   if (HasKey_FKind(FilHdr_FKind(FilHdr)) && !UniqueFlag) {
      (void) strcpy(StrBuf, FilHdr->Ident);
      return StrBuf;
   }
   Build_Label(StrBuf, FilHdr->Ident, FilHdr->FilTyp,
               FilHdr_LocHdr(FilHdr), UniqueFlag);
   return StrBuf;
}

tp_FilHdr FilHdr_ElmFilHdr(tp_FilHdr FilHdr)
{
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr;

   FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
   FORBIDDEN(FilElm != NIL && FilElm_Next(FilElm) != NIL);
   ElmFilHdr = FilElm_FilHdr(FilElm);
   Ret_FilHdr(FilHdr);
   Ret_FilElm(FilElm);
   return ElmFilHdr;
}
