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
#include "inc/Client.h"
#include "inc/FKind_.h"
#include "inc/PrmTyp.h"
#include "inc/Str.h"

extern int num_PrmTypS;
extern tp_PrmTyp PrmTypS;

tp_PTName PrmTyp_PTName(tp_PrmTyp PrmTyp)
{
   if (PrmTyp == ERROR)
      return ERROR;
   return PrmTyp->PTName;
}

static tp_PrmTyp PTName_PrmTyp(tp_PTName PTName)
{
   int i;
   tp_PrmTyp PrmTyp;

   if (PTName == ERROR) {
      return ERROR;
   }
   for (i = 0; i < num_PrmTypS; i++) {
      PrmTyp = &PrmTypS[i];
      if (strcmp(PTName, PrmTyp->PTName) == 0) {
         return PrmTyp;
      }
   }
   return ERROR;
}

tp_FilTyp PrmTyp_FilTyp(tp_PrmTyp PrmTyp)
{
   if (PrmTyp == ERROR)
      return ERROR;
   return PrmTyp->FilTyp;
}

boolean IsFirst_PrmTyp(tp_PrmTyp PrmTyp)
{
   FORBIDDEN(PrmTyp == ERROR);
   return (PrmTyp->FilTyp == FirstFilTyp);
}

int PrmTyp_I(tp_PrmTyp PrmTyp)
{
   FORBIDDEN(PrmTyp == ERROR);
   return PrmTyp->IPrmTyp;
}

void SetPrmTyp_Mark(tp_PrmTyp PrmTyp)
{
   PrmTyp->Mark = TRUE;
}

tp_FilHdr PrmTyp_StrDirFilHdr(tp_PrmTyp PrmTyp)
{
   tp_FilHdr FilHdr;

   if (PrmTyp == ERROR) {
      return ERROR;
   }
   if (PrmTyp->StrDirLocHdr == NIL) {
      FilHdr = Extend_FilHdr(Copy_FilHdr(StrDirFilHdr), FK_Str,
                             ObjectFilTyp, RootFilPrm,
                             PrmTyp_PTName(PrmTyp));
      PrmTyp->StrDirLocHdr = FilHdr_LocHdr(FilHdr);
      return FilHdr;
   }
   return LocHdr_FilHdr(PrmTyp->StrDirLocHdr);
}

tp_FilPVal PrmTyp_RootFilPVal(tp_PrmTyp PrmTyp)
{
   tp_FilPVal FilPVal;
   tp_LocPVal LocPVal;

   if (PrmTyp->RootLocPVal == NIL) {
      FilPVal = New_FilPVal();
      LocPVal = FilPVal_LocPVal(FilPVal);
      Add_RootLocPVal(PrmTyp, LocPVal);
      PrmTyp->RootLocPVal = LocPVal;
   }
   return LocPVal_FilPVal(PrmTyp->RootLocPVal);
}

void SetPrmTyp_RootLocPVal(tp_PrmTyp PrmTyp, tp_LocPVal LocPVal)
{
   FORBIDDEN(PrmTyp->RootLocPVal != NIL || LocPVal == NIL);
   PrmTyp->RootLocPVal = LocPVal;
}

void SetFilHdr_PrmTypMarks(tp_FilHdr FilHdr)
{
   SetFilHdr_Marks(FilHdr, TRUE);
}

void Clr_PrmTypMarks(void)
{
   int i;

   for (i = 0; i < num_PrmTypS; i++) {
      PrmTypS[i].Mark = FALSE;
   }
   Clr_FilTypMarks();
}

void WriteMarkedPrmTyps(tp_FilDsc FilDsc)
{
   int i;
   tp_PrmTyp PrmTyp;
   boolean Found;

   Found = FALSE;
   for (i = 0; i < num_PrmTypS; i++) {
      PrmTyp = &PrmTypS[i];
      if (PrmTyp->Mark && CurrentClient->HelpLevel >= PrmTyp->HelpLevel) {
         WriteNameDesc(FilDsc, PrmTyp->PTName, PrmTyp->Desc);
         Found = TRUE;
      }
   }
   if (!Found) {
      Writeln(FilDsc, "(none)");
   }
}

tp_PrmTyp Nod_PrmTyp(tp_Nod DrvTyp_Nod)
{
   tp_Str Str;
   tp_PrmTyp PrmTyp;

   Str = Sym_Str(Nod_Sym(DrvTyp_Nod));
   PrmTyp = PTName_PrmTyp(Str);
   if (PrmTyp == ERROR)
      SystemError("No parameter type, <%s>.\n", Str);
   return PrmTyp;
}
