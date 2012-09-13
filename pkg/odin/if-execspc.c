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
#include "inc/FKind_.h"
#include "inc/Inputs.h"
#include "inc/Outputs.h"
#include "inc/ExecSpc.h"

tp_Tool FilHdr_Tool(tp_FilHdr FilHdr)
{
   tp_Tool Tool;
   tp_FilHdr BaseFilHdr;

   switch (FilHdr_FKind(FilHdr)) {
   case FK_SrcReg:
   case FK_SrcDir:
   case FK_SymLinkReg:
   case FK_SymLinkDir:
   case FK_BoundSrc:
   case FK_BoundSymLink:{
         Tool = SourceTool;
         break;
      }
   case FK_User:{
         Tool = FilTyp_Tool(FilHdr_FilTyp(FilHdr));
         break;
      }
   case FK_Str:{
         Tool = StrTool;
         break;
      }
   case FK_DrvDirElm:
   case FK_ActTgt:
   case FK_ActCmdTgt:
   case FK_VirTgt:
   case FK_VirCmdTgt:{
         Tool = DrvDirElmTool;
         break;
      }
   case FK_VirDirElm:{
         Tool = VirDirElmTool;
         break;
      }
   case FK_PntrHo:{
         Tool = PntrHoTool;
         break;
      }
   case FK_InpPntr:{
         Tool = InternalPntrTool;
         break;
      }
   case FK_PntrElm:{
         Tool = PntrElmTool;
         break;
      }
   case FK_ActTgtText:
   case FK_VirTgtText:
   case FK_ActTgtExText:
   case FK_VirTgtExText:{
         Tool = TextDefTool;
         break;
      }
   case FK_Instance:{
         BaseFilHdr = FilHdr_Father(Copy_FilHdr(FilHdr));
         Tool = FilHdr_Tool(BaseFilHdr);
         Ret_FilHdr(BaseFilHdr);
         break;
      }
   default:{
         FATALERROR("Bad FKind");
      }
   }
   return Tool;
}

void FilHdr_ExecSpc(tp_ExecSpc ExecSpc, tp_FilHdr FilHdr)
{
   tp_FilHdr InpFilHdr, BaseFilHdr;
   tp_FilPrm FilPrm;
   tp_FilInp FilInp;
   int IArg;
   boolean IsDerefInput;

   ExecSpc->FilHdr = Copy_FilHdr(FilHdr);
   ExecSpc->FilTyp = FilHdr_FilTyp(FilHdr);

   switch (FilHdr_FKind(FilHdr)) {
   case FK_User:{
         ExecSpc->NumInps = NumInputs(FilHdr_FilTyp(FilHdr));
         break;
      }
   case FK_Instance:{
         BaseFilHdr = FilHdr_Father(Copy_FilHdr(FilHdr));
         ExecSpc->NumInps = NumInputs(FilHdr_FilTyp(BaseFilHdr));
         Ret_FilHdr(BaseFilHdr);
         break;
      }
   case FK_DrvDirElm:
   case FK_VirDirElm:
   case FK_PntrHo:
   case FK_PntrElm:
   case FK_ActTgtText:
   case FK_VirTgtText:
   case FK_ActTgtExText:
   case FK_VirTgtExText:
   case FK_VirTgt:
   case FK_VirCmdTgt:
   case FK_ActTgt:
   case FK_ActCmdTgt:{
         ExecSpc->NumInps = 1;
         break;
      }
   case FK_Str:
   case FK_InpPntr:{
         ExecSpc->NumInps = 0;
         break;
      }
   default:{
         FATALERROR("Bad FKind");
      }
   }

   IsDerefInput = IsDerefInput_Tool(FilHdr_Tool(FilHdr));
   for (IArg = 0; IArg < ExecSpc->NumInps; IArg += 1) {
      ExecSpc->InpFilHdrs[IArg] = Copy_FilHdr(NilStrFilHdr);
   }
   for (FilInp = LocInp_FilInp(FilHdr_LocInp(FilHdr));
        FilInp != NIL; FilInp = FilInp_NextFilInp(FilInp)) {
      IArg = FilInp_IArg(FilInp);
      if (IArg >= 0) {
         InpFilHdr = FilInp_FilHdr(FilInp);
         if (IsDerefInput) {
            Deref_Pntrs(&InpFilHdr, &FilPrm, InpFilHdr, FALSE);
         }
         FORBIDDEN(InpFilHdr == ERROR);
         Ret_FilHdr(ExecSpc->InpFilHdrs[IArg]);
         ExecSpc->InpFilHdrs[IArg] = InpFilHdr;
      }
   }

   Get_OutFilHdrs(ExecSpc->OutFilHdrs, &ExecSpc->NumOuts, FilHdr);
   ExecSpc->Job = NIL;
}

void
Get_OutFilHdrs(tp_OutFilHdrs OutFilHdrs, int *NumOutsPtr, tp_FilHdr FilHdr)
{
   tps_OutTyps OutTyps;
   int i;
   tp_FilHdr TmpFilHdr;

   if (!IsStruct(FilHdr)) {
      *NumOutsPtr = 1;
      OutFilHdrs[0] = Copy_FilHdr(FilHdr);
      return;
   }
   GetOutTyps(FilHdr_FilTyp(FilHdr), OutTyps, NumOutsPtr);
   for (i = 0; i < *NumOutsPtr; i++) {
      TmpFilHdr = Copy_FilHdr(FilHdr);
      TmpFilHdr = Do_Deriv(TmpFilHdr, RootFilPrm, RootFilPrm, OutTyps[i]);
      FORBIDDEN(TmpFilHdr == ERROR);
      OutFilHdrs[i] = TmpFilHdr;
   }
}

void Ret_ExecSpc(tp_ExecSpc ExecSpc)
{
   int i;

   Ret_FilHdr(ExecSpc->FilHdr);
   for (i = 0; i < ExecSpc->NumInps; i++)
      Ret_FilHdr(ExecSpc->InpFilHdrs[i]);
   for (i = 0; i < ExecSpc->NumOuts; i++)
      Ret_FilHdr(ExecSpc->OutFilHdrs[i]);
}
