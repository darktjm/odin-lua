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
#include "inc/CastEdg.h"
#include "inc/DrvEdg.h"
#include "inc/EqvEdg.h"
#include "inc/MemEdg.h"
#include "inc/InpEdg.h"
#include "inc/InpKind_.h"

tp_PrmTypLst DrvEdg_PrmTypLst(tp_DrvEdg DrvEdg)
{
   FORBIDDEN(DrvEdg == ERROR);
   return DrvEdg->PrmTypLst;
}

tp_InpSpc InpEdg_InpSpc(tp_InpEdg InpEdg)
{
   FORBIDDEN(InpEdg == ERROR);
   return InpEdg->InpSpc;
}

tp_InpKind InpEdg_InpKind(tp_InpEdg InpEdg)
{
   FORBIDDEN(InpEdg == ERROR);
   return InpEdg->InpKind;
}

boolean InpEdg_IsUserArg(tp_InpEdg InpEdg)
{
   FORBIDDEN(InpEdg == ERROR);
   return InpEdg->IsUserArg;
}

tp_InpEdg InpEdg_Next(tp_InpEdg InpEdg)
{
   FORBIDDEN(InpEdg == ERROR);
   return InpEdg->Next;
}

tp_FilTyp EqvEdg_FilTyp(tp_EqvEdg EqvEdg)
{
   return EqvEdg->FilTyp;
}

tp_FilTyp MemEdg_FilTyp(tp_MemEdg MemEdg)
{
   return MemEdg->FilTyp;
}

static boolean InpKind_IsName(tp_InpKind InpKind)
{
   return (InpKind == IK_Name);
}

static boolean InpKind_IsTransName(tp_InpKind InpKind)
{
   return (InpKind == IK_TransName);
}

static boolean InpKind_IsTrans(tp_InpKind InpKind)
{
   return (InpKind == IK_Trans || InpKind == IK_AnyOK);
}

boolean InpKind_IsAnyOK(tp_InpKind InpKind)
{
   return (InpKind == IK_AnyOK);
}

boolean NeedsData(tp_FilHdr FilHdr, tp_InpKind InpKind)
{
   return !(InpKind_IsName(InpKind)
            || (InpKind_IsTransName(InpKind) && !IsRef(FilHdr)));
}

boolean NeedsElmData(tp_FilHdr FilHdr, tp_InpKind InpKind)
{
   return ((InpKind_IsTrans(InpKind) && IsList(FilHdr))
           || (InpKind != IK_Pntr && InpKind != IK_TransName
               && IsPntr(FilHdr)));
}

boolean NeedsElmNameData(tp_FilHdr FilHdr, tp_InpKind InpKind)
{
   return (InpKind_IsTransName(InpKind) && IsRef(FilHdr));
}

int NumInputs(tp_FilTyp FilTyp)
{
   int i;
   tp_InpEdg InpEdg;
   boolean IsUserArg;

   IsUserArg = TRUE;
   i = 0;
   for (InpEdg = Tool_InpEdg(FilTyp_Tool(FilTyp));
        InpEdg != NIL; InpEdg = InpEdg->Next) {
      {
         if (InpEdg->IsUserArg) {
            FORBIDDEN(!IsUserArg);
            i++;
         } else {
            IsUserArg = FALSE;
         }
      }
   }
   return i;
}

void GetOutTyps(tp_FilTyp FilTyp, tp_OutTyps OutTyps, int *NumOutputsPtr)
{
   int i;
   tp_MemEdg MemEdg;

   if (!IsStruct_FilTyp(FilTyp)) {
      OutTyps[0] = FilTyp;
      *NumOutputsPtr = 1;
      return;
   }

   for (i = 0, MemEdg = FilTyp_MemEdg(FilTyp);
        MemEdg != NIL; i += 1, MemEdg = MemEdg->Next) {
      OutTyps[i] = MemEdg->FilTyp;
   }
   *NumOutputsPtr = i;
}

void
SetEqvEdg_Marks(tp_EqvEdg EqvEdg, boolean CastFlag, boolean PrmTypFlag)
{
   tp_EqvEdg TmpEE;

   for (TmpEE = EqvEdg; TmpEE != NIL; TmpEE = TmpEE->Next) {
      if (!PrmTypFlag)
         SetFilTyp_Mark(TmpEE->FilTyp);
      SetFilTyp_Marks(TmpEE->FilTyp, CastFlag, PrmTypFlag);
   }
}

void SetCastEdg_Marks(tp_CastEdg CastEdg, boolean PrmTypFlag)
{
   tp_CastEdg TmpCE;

   for (TmpCE = CastEdg; TmpCE != NIL; TmpCE = TmpCE->Next) {
      if (!PrmTypFlag)
         SetFilTyp_Mark(TmpCE->FilTyp);
      SetFilTyp_Marks(TmpCE->FilTyp, TRUE, PrmTypFlag);
   }
}

void SetDrvEdg_Marks(tp_DrvEdg DrvEdg, boolean PrmTypFlag)
{
   tp_DrvEdg TmpDE;

   for (TmpDE = DrvEdg; TmpDE != NIL; TmpDE = TmpDE->Next) {
      ; {
         if (PrmTypFlag) {
            SetPrmTypLst_Marks(TmpDE->PrmTypLst);
         } else {
            SetFilTyp_Mark(TmpDE->FilTyp);
         }
      }
      SetFilTyp_Marks(TmpDE->FilTyp, FALSE, PrmTypFlag);
   }
}
