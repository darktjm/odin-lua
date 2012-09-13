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

int num_ParseErrors = 0;

int num_NodS = 0;

static tp_Nod FreeNod = NIL;

typedef struct _tps_Nod {
   tp_NodTyp NodTyp;
   tp_Sym Sym;
   tp_Nod Brother;
   tp_Nod Son;
#ifdef NODATT
   tp_Nod Att;
#endif
} tps_Nod;

tp_Nod New_Nod(void)
{
   tp_Nod Nod;

   {
      if (FreeNod == NIL) {
         Nod = (tp_Nod) malloc(sizeof(tps_Nod));
         num_NodS += 1;
      } else {
         Nod = FreeNod;
         FreeNod = FreeNod->Brother;
      }
   }
   Nod->NodTyp = NIL;
   Nod->Son = NIL;
   Nod->Brother = NIL;
   Nod->Sym = NIL;
   return Nod;
}

void Ret_Nod(tp_Nod Nod)
{
   if (Nod == NIL) {
      return;
   }
   Ret_Nod(Nod->Son);
   Ret_Nod(Nod->Brother);
   Nod->Brother = FreeNod;
   FreeNod = Nod;
}

tp_NodTyp Nod_NodTyp(tp_Nod Nod)
{
   if (Nod == ERROR)
      return ERROR;
   return Nod->NodTyp;
}

void Set_Nod_NodTyp(tp_Nod Nod, tp_NodTyp NodTyp)
{
   Nod->NodTyp = NodTyp;
}

tp_Nod Nod_FirstSon(tp_Nod Nod)
{
   if (Nod == ERROR)
      return ERROR;
   return Nod->Son;
}

void Set_Nod_FirstSon(tp_Nod Nod, tp_Nod FirstSon)
{
   Nod->Son = FirstSon;
}

tp_Nod Nod_Brother(tp_Nod Nod)
{
   if (Nod == ERROR)
      return ERROR;
   return Nod->Brother;
}

void Set_Nod_Brother(tp_Nod Nod, tp_Nod Brother)
{
   Nod->Brother = Brother;
}

int Nod_NumSons(tp_Nod Nod)
{
   tp_Nod SonNod;
   int NumSons;

   FORBIDDEN(Nod == ERROR);
   NumSons = 0;
   for (SonNod = Nod->Son; SonNod != NIL; SonNod = SonNod->Brother) {
      NumSons += 1;
   }
   return NumSons;
}

tp_Nod Nod_Son(int I, tp_Nod Nod)
{
   tp_Nod SonNod;
   int i;

   if (Nod == ERROR)
      return ERROR;
   FORBIDDEN(Nod == ERROR);
   FORBIDDEN(I <= 0);
   SonNod = Nod->Son;
   for (i = 1; i < I; i++) {
      if (SonNod == NIL) {
         return NIL;
      }
      SonNod = SonNod->Brother;
   }
   return SonNod;
}

tp_Sym Nod_Sym(tp_Nod Nod)
{
   if (Nod == ERROR)
      return ERROR;
   return Nod->Sym;
}

void Set_Nod_Sym(tp_Nod Nod, tp_Sym Sym)
{
   Nod->Sym = Sym;
}

#ifdef NODATT

tp_Nod Get_NodAtt(tp_Nod Nod)
{
   FORBIDDEN(Nod == ERROR);
   return (Nod->Att);
}

tp_Nod Set_NodAtt(tp_Nod Nod, tp_Nod Att)
{
   FORBIDDEN(Nod == ERROR);
   Nod->Att = Att;
}

void Set_All_NodAtts(tp_Nod Nod, tp_Nod Att)
{
   FORBIDDEN(Nod == ERROR);
   Nod->Att = Att;
   if (Nod->Son != NIL)
      Set_All_NodAtts(Nod->Son, Att);
   if (Nod->Brother != NIL)
      Set_All_NodAtts(Nod->Brother, Att);
}

#endif

typedef char *tp_Value;
typedef struct _tps_StackElm *tp_StackElm;
typedef struct _tps_StackElm {
   tp_Value Value;
   int Count;
   tp_StackElm Next;
} tps_StackElm;

int num_StackElmS = 0;
static tp_StackElm FreeStackElm = NIL;

static tp_StackElm New_StackElm(void)
{
   tp_StackElm StackElm;

   if (FreeStackElm == NIL) {
      num_StackElmS += 1;
      return (tp_StackElm) malloc(sizeof(tps_StackElm));
   }
   StackElm = FreeStackElm;
   FreeStackElm = FreeStackElm->Next;
   return StackElm;
}

static tp_StackElm SymStack = NIL;

static boolean Empty_SymStack(void)
{
   return (SymStack == NIL);
}

void Push_SymStack(tp_Sym Sym)
{
   tp_StackElm StackElm;

   FORBIDDEN(Sym == ERROR);
   StackElm = New_StackElm();
   StackElm->Value = (tp_Value) Sym;
   StackElm->Next = SymStack;
   SymStack = StackElm;
}

static tp_Sym TopOf_SymStack(void)
{
   FORBIDDEN(SymStack == NIL);
   return (tp_Sym) SymStack->Value;
}

static void Pop_SymStack(void)
{
   tp_StackElm OldFreeStackElm;

   FORBIDDEN(SymStack == NIL);
   OldFreeStackElm = FreeStackElm;
   FreeStackElm = SymStack;
   SymStack = SymStack->Next;
   FreeStackElm->Next = OldFreeStackElm;
}

static tp_StackElm NodStack = NIL;

static boolean Empty_NodStack(void)
{
   return (NodStack == NIL);
}

static void Push_NodStack(tp_Nod Nod)
{
   tp_StackElm StackElm;

   FORBIDDEN(Nod == ERROR);
   StackElm = New_StackElm();
   StackElm->Value = (tp_Value) Nod;
   StackElm->Next = NodStack;
   NodStack = StackElm;
}

static tp_Nod TopOf_NodStack(void)
{
   FORBIDDEN(NodStack == NIL);
   return (tp_Nod) NodStack->Value;
}

static void Pop_NodStack(void)
{
   tp_StackElm OldFreeStackElm;

   FORBIDDEN(NodStack == NIL);
   OldFreeStackElm = FreeStackElm;
   FreeStackElm = NodStack;
   NodStack = NodStack->Next;
   FreeStackElm->Next = OldFreeStackElm;
}

static tp_StackElm SonStack = NIL;

static boolean Empty_SonStack(void)
{
   return (SonStack == NIL);
}

static void Push_SonStack(int NumSons)
{
   tp_StackElm StackElm;

   FORBIDDEN(NumSons < 0);
   StackElm = New_StackElm();
   StackElm->Count = NumSons;
   StackElm->Next = SonStack;
   SonStack = StackElm;
}

static int TopOf_SonStack(void)
{
   FORBIDDEN(SonStack == NIL);
   return SonStack->Count;
}

static void Pop_SonStack(void)
{
   tp_StackElm OldFreeStackElm;

   FORBIDDEN(SonStack == NIL);
   OldFreeStackElm = FreeStackElm;
   FreeStackElm = SonStack;
   SonStack = SonStack->Next;
   FreeStackElm->Next = OldFreeStackElm;
}

void Init_ConstructTree(void)
{
   while (!Empty_SymStack()) {
      Pop_SymStack();
   }
   while (!Empty_NodStack()) {
      Ret_Nod(TopOf_NodStack());
      Pop_NodStack();
   }
   while (!Empty_SonStack()) {
      Pop_SonStack();
   }
}

static void MakeLeaf(tp_NodTyp NodTyp, tp_Sym Sym)
{
   tp_Nod Nod;

   if (NodTyp == 0) {
      return;
   }
   Nod = New_Nod();
   Nod->NodTyp = NodTyp;
   Nod->Sym = Sym;
   Nod->Son = NIL;
   Nod->Brother = NIL;
   Push_NodStack(Nod);
   Push_SonStack(1);
}

static void MakeEmptyNod(tp_NodTyp NodTyp)
{
   tp_Nod Nod;

   if (NodTyp == 0)
      return;
   Nod = New_Nod();
   Nod->NodTyp = NodTyp;
   Nod->Sym = NIL;
   Nod->Son = NIL;
   Nod->Brother = NIL;
   Push_NodStack(Nod);
   Push_SonStack(1);
}

static void MakeNod(int Typ)
{
   tp_NodTyp NodTyp;
   tp_Nod Nod, NewNod, Brother;
   int NumSons, i;

   NodTyp = Typ;
   NumSons = TopOf_SonStack();
   if (Typ < 0) {
      if (NumSons == 1) {
         return;
      }
      NodTyp = -Typ;
   }
   Pop_SonStack();
   if (NumSons == 0) {
      MakeEmptyNod(NodTyp);
      return;
   }
   Brother = NIL;
   for (i = 0; i < NumSons; i++) {
      NewNod = TopOf_NodStack();
      Pop_NodStack();
      NewNod->Brother = Brother;
      Brother = NewNod;
   }
   Nod = New_Nod();
   Nod->NodTyp = NodTyp;
   Nod->Sym = NIL;
   Nod->Son = Brother;
   Nod->Brother = NIL;
   Push_NodStack(Nod);
   Push_SonStack(1);
}

static void CollectSons(int Number)
{
   int NumSons, i;

   FORBIDDEN(Number < 0);
   NumSons = 0;
   for (i = 0; i < Number; i++) {
      NumSons = NumSons + TopOf_SonStack();
      Pop_SonStack();
   }
   Push_SonStack(NumSons);
}

tp_Nod End_ConstructTree(void)
{
   tp_Nod Root;

   if (num_ParseErrors > 0 || Empty_NodStack()) {
      while (!Empty_NodStack())
         Pop_NodStack();
      while (!Empty_SonStack())
         Pop_SonStack();
      return ERROR;
   }
   Root = TopOf_NodStack();
   Pop_NodStack();
   Pop_SonStack();
   return Root;
}

void Action(int Typ, int NumSons)
{
   tp_Sym Sym;

   if (num_ParseErrors > 0)
      return;
   {
      if (NumSons < 0) {
         {
            if (Typ < 0) {
               CollectSons(-NumSons);
               MakeNod(-Typ);
               EndLex();
            } else {
               Sym = TopOf_SymStack();
               Pop_SymStack();
               MakeLeaf(Typ, Sym);
            }
         }
      } else if (NumSons == 0) {
         {
            if (Typ < 0) {
               CollectSons(NumSons);
               MakeNod(-Typ);
               EndLex();
            } else if (Typ == 0) {
               CollectSons(NumSons);
            } else {
               MakeEmptyNod(Typ);
            }
         }
      } else {
         if (NumSons > 1)
            CollectSons(NumSons);
         if (Typ != 0)
            MakeNod(Typ);
      }
   }
}
