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
#include "inc/FilTyp.h"
#include "inc/FKind_.h"
#include "inc/FTClass_.h"
#include "inc/SrcTyp.h"
#include "inc/TClass_.h"
#include "inc/Tool.h"

boolean IsPntr_FKind(tp_FKind FKind)
{
   FORBIDDEN(FKind == ERROR);
   return (FKind == FK_PntrHo || FKind == FK_InpPntr
           || FKind == FK_VirDirElm || FKind == FK_PntrElm
           || IsVTgt_FKind(FKind) || IsATgt_FKind(FKind));
}

boolean CanPntrHo_FKind(tp_FKind FKind)
{
   FORBIDDEN(FKind == ERROR);
   return IsPntr_FKind(FKind);
}

boolean IsATgt_FKind(tp_FKind FKind)
{
   FORBIDDEN(FKind == ERROR);
   return (FKind == FK_ActTgt || FKind == FK_ActCmdTgt);
}

boolean IsVTgt_FKind(tp_FKind FKind)
{
   FORBIDDEN(FKind == ERROR);
   return (FKind == FK_VirTgt || FKind == FK_VirCmdTgt);
}

boolean IsATgtText_FKind(tp_FKind FKind)
{
   FORBIDDEN(FKind == ERROR);
   return (FKind == FK_ActTgtText || FKind == FK_ActTgtExText);
}

boolean IsVTgtText_FKind(tp_FKind FKind)
{
   FORBIDDEN(FKind == ERROR);
   return (FKind == FK_VirTgtText || FKind == FK_VirTgtExText);
}

boolean IsExternal_Tool(tp_Tool Tool)
{
   FORBIDDEN(Tool == ERROR);
   return (Tool->TClass == TC_External);
}

tp_MemEdg FilTyp_MemEdg(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == NIL);
   return FilTyp->MemEdg;
}

tp_CastEdg FilTyp_CastEdg(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return FilTyp->CastEdg;
}

tp_PrmTypLst FilTyp_MapPrmTypLst(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return FilTyp->MapPrmTypLst;
}

tp_FilTyp FilTyp_ArgFilTyp(tp_FilTyp FilTyp)
{
   if (FilTyp == ERROR)
      return ERROR;
   return FilTyp->ArgFilTyp;
}

tp_Tool FilTyp_Tool(tp_FilTyp FilTyp)
{
   if (FilTyp == ERROR)
      return ERROR;
   return FilTyp->Tool;
}

tp_FTName FilTyp_ShortFTName(tp_FilTyp FilTyp)
{
   if (FilTyp == ERROR)
      return ERROR;
   if (IsSecOrd_FilTyp(FilTyp)) {
      return FilTyp_ShortFTName(FilTyp_ArgFilTyp(FilTyp));
   }
   if (IsStruct_FilTyp(FilTyp)) {
      return FilTyp_ShortFTName(MemEdg_FilTyp(FilTyp->MemEdg));
   }
   if (FilTyp->Tool == NIL) {
      return FilTyp_FTName(FilTyp);
   }
   return FilTyp_FTName(EqvEdg_FilTyp(FilTyp->EqvEdg));
}

tp_FTName FilTyp_FTName(tp_FilTyp FilTyp)
{
   if (FilTyp == ERROR)
      return ERROR;
   return FilTyp->FTName;
}

boolean IsCopy_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return FilTyp->IsCopy;
}

boolean IsGrouping_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return FilTyp->IsGrouping;
}

boolean IsGroupingInput_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return FilTyp->IsGroupingInput;
}

boolean IsSecOrd_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return (FilTyp->ArgFilTyp != NIL);
}

boolean IsRecurse_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return (IsSecOrd_FilTyp(FilTyp)
           && (strcmp(FilTyp->FTName, "recurse") == 0));
}

boolean IsExec_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return (FilTyp->FTClass == FTC_Exec);
}

boolean IsVoid_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return (FilTyp->FTClass == FTC_Void);
}

boolean IsAtmc_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return (FilTyp->FTClass == FTC_Atmc || FilTyp->FTClass == FTC_DrvDir
           || FilTyp->FTClass == FTC_Generic || FilTyp->FTClass == FTC_Pipe
           || FilTyp->FTClass == FTC_Exec || FilTyp->FTClass == FTC_Void);
}

boolean IsPntr_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return (FilTyp->FTClass == FTC_Pntr);
}

boolean IsList_FilTyp(tp_FilTyp FilTyp)
{
   tp_FTClass FTClass;

   FORBIDDEN(FilTyp == ERROR);
   FTClass = FilTyp->FTClass;
   return (FTClass == FTC_List);
}

boolean IsDrvDir_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return (FilTyp->FTClass == FTC_DrvDir);
}

boolean IsStruct_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return (FilTyp->FTClass == FTC_Struct);
}

boolean IsStructMem_FilTyp(tp_FilTyp FilTyp)
{
   FORBIDDEN(FilTyp == ERROR);
   return (FilTyp->Tool != NIL && FilTyp->Tool->TClass == TC_StructMem);
}

boolean IsGeneric_FilTyp(tp_FilTyp FilTyp)
{
   return FilTyp->FTClass == FTC_Generic;
}

boolean IsPipe_FilTyp(tp_FilTyp FilTyp)
{
   return FilTyp->FTClass == FTC_Pipe;
}
