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
#include "inc/Sym.h"

int num_StrS = 0;

#define		MAX_HashedSyms 512
static tp_Sym HashedSyms[MAX_HashedSyms];

static int num_Syms = 0;

tp_Str GetEnv(tp_Str VarStr)
{
   tp_Str ValStr;

   ValStr = getenv(VarStr);
   if (ValStr == NULL || strlen(ValStr) == 0) {
      return NIL;
   }
   return ValStr;
}

tp_Str Malloc_Str(const char *Str)
{
   num_StrS += 1;
   return strcpy((tp_Str) malloc((unsigned) strlen(Str) + 1), Str);
}

boolean Is_EmptyStr(tp_Str Str)
{
   return Str[0] == 0;
}

int Str_PosInt(tp_Str Str)
{
   int i;

   i = atoi(Str);
   if (i == 0 && strcmp(Str, "0") != 0)
      i = -1;
   return i;
}

tp_Str Tail(tp_Str Str)
{
   if (Str == ERROR) {
      return ERROR;
   }
   return &Str[strlen(Str)];
}

void StrShift(tp_Str Str, int Count)
{
   int i, Len;
   tp_Str ToStr, FromStr;

   if (Count < 0) {
      FromStr = Str - Count;
      ToStr = Str;
      while (*FromStr != 0) {
         *ToStr = *FromStr;
         ToStr += 1;
         FromStr += 1;
      }
      *ToStr = 0;
      return;
   }
   Len = strlen(Str);
   FromStr = Str + Len;
   ToStr = FromStr + Count;
   *ToStr = 0;
   for (i = 0; i < Len; i += 1) {
      ToStr -= 1;
      FromStr -= 1;
      *ToStr = *FromStr;
   }
}

static int Str_HashInt(tp_Str Str)
{
   int Length, HashInt;

   Length = strlen(Str);
   if (Length == 0)
      return 0;
   HashInt =
       (Str[0] & 0xff) + (Str[Length / 2] & 0xff) +
       (Str[Length - 1] & 0xff)
       + (Length << 5);
   FORBIDDEN(HashInt < 0);
   return HashInt;
}

tp_Sym Str_Sym(tp_Str Str)
{
   int HashInt;
   tp_Sym Sym;

   if (Str == ERROR)
      return ERROR;
   HashInt = Str_HashInt(Str) % MAX_HashedSyms;
   for (Sym = HashedSyms[HashInt]; Sym != NIL; Sym = Sym->Next) {
      if (strcmp(Str, Sym->Str) == 0) {
         return Sym;
      }
   }
   Sym = (tp_Sym) malloc(sizeof(tps_Sym));
   num_Syms += 1;
   Sym->Str = Malloc_Str(Str);
   Sym->Att = NIL;
   Sym->Next = HashedSyms[HashInt];
   HashedSyms[HashInt] = Sym;
   return Sym;
}

tp_Str Sym_Str(tp_Sym Sym)
{
   if (Sym == ERROR)
      return ERROR;
   return Sym->Str;
}

int Sym_Att(tp_Sym Sym)
{
   FORBIDDEN(Sym == ERROR);
   return Sym->Att;
}

void Set_Sym_Att(tp_Sym Sym, int Att)
{
   FORBIDDEN(Sym == ERROR);
   Sym->Att = Att;
}

void Write_Syms(tp_FilDsc FilDsc)
{
   int i, Used = 0;
   tp_Sym Sym;

   for (i = 0; i < MAX_HashedSyms; i++) {
      Sym = HashedSyms[i];
      if (Sym != NIL) {
         Used += 1;
         WriteInt(FilDsc, i);
         while (Sym != NIL) {
            Write(FilDsc, "\t: ");
            Writeln(FilDsc, Sym->Str);
            Sym = Sym->Next;
         }
      }
   }
   if (Used > 0) {
      WriteInt(FilDsc, (int) (num_Syms / Used));
      Writeln(FilDsc, "");
   }
}
