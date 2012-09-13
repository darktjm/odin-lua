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
#include "inc/AttTyp_.h"
#include "inc/MarkTyp_.h"
#include "inc/NodTyp_.h"

void Write_Node_Grammar(tp_FilDsc FilDsc, tp_Nod Rules_Nod)
{
   tp_Nod Rule_Nod, Var_Nod, Prod_Nod, Node_Nod;
   tp_Sym Node_Sym, Var_Sym;
   tp_Att Attribute;

   Reset_Att(ATP_VarRule);
   FOREACH_SON(Rule_Nod, Rules_Nod) {
      Attribute.Nod = Rule_Nod;
      Set_Att(ATP_VarRule, Nod_Sym(Nod_FirstSon(Rule_Nod)), Attribute);
   }

   Reset_Att(ATP_Marked);
   Reset_Att(ATP_VarTree);
   Reset_Att(ATP_NodeTree);
   (void) Create_Node_Grammar(Rules_Nod);

   Reset_Att(ATP_NodePrinted);
   FOREACH_SON(Rule_Nod, Rules_Nod) {
      Var_Nod = Nod_FirstSon(Rule_Nod);
      Var_Sym = Nod_Sym(Var_Nod);
      if (Get_Att(ATP_Marked, Var_Sym).Int == MT_Recursive) {
         Write(FilDsc, Sym_Str(Var_Sym));
         Write(FilDsc, " -> ");
         Write_Node(FilDsc, Get_Att(ATP_VarTree, Var_Sym).Nod);
         Write(FilDsc, " ;\n");
      }
      FOREACH_SON(Prod_Nod, Nod_Brother(Var_Nod)) {
         Node_Nod = Nod_Son(2, Prod_Nod);
         if (Node_Nod != NIL) {
            if (Nod_NodTyp(Node_Nod) != KSTRNG) {
               /* should do something more here */
               Node_Nod = Nod_FirstSon(Node_Nod);
            }
            Node_Sym = Nod_Sym(Node_Nod);
            if (!Get_Att(ATP_NodePrinted, Node_Sym).Int) {
               Attribute.Int = TRUE;
               Set_Att(ATP_NodePrinted, Node_Sym, Attribute);
               Write(FilDsc, "<");
               Write(FilDsc, Sym_Str(Node_Sym));
               Write(FilDsc, "> -> ");
               Write_Node(FilDsc, Get_Att(ATP_NodeTree, Node_Sym).Nod);
               Write(FilDsc, " ;\n");
            }
         }
      }
   }
}

tp_Nod Create_Node_Grammar(tp_Nod Nod)
{
   tp_Nod Node_Nod, Rule_Nod, Var_Nod, Son_Nod, Prod_Nod, TreeSpec_Nod;
   tp_Nod Node1_Nod, Node2_Nod;
   tp_Sym Var_Sym, Node_Sym, Sym;
   int Mark;
   tp_NodTyp NodTyp;
   tp_Att Attribute;

   Node_Nod = NIL;
   switch (Nod_NodTyp(Nod)) {
   case RULES:{
         FOREACH_SON(Rule_Nod, Nod) (void) Create_Node_Grammar(Rule_Nod);
         break;
      }
   case RULE:{
         Var_Nod = Nod_FirstSon(Nod);
         Var_Sym = Nod_Sym(Var_Nod);
         if (Get_Att(ATP_Marked, Var_Sym).Int != MT_Null) {
            FORBIDDEN(Get_Att(ATP_Marked, Var_Sym).Int == MT_Active);
            return NIL;
         }
         Attribute.Int = MT_Active;
         Set_Att(ATP_Marked, Var_Sym, Attribute);
         Attribute.Nod = Create_Node_Grammar(Nod_Brother(Var_Nod));
         Set_Att(ATP_VarTree, Var_Sym, Attribute);
         if (Get_Att(ATP_Marked, Var_Sym).Int != MT_Recursive) {
            Attribute.Int = MT_Complete;
            Set_Att(ATP_Marked, Var_Sym, Attribute);
         }
         break;
      }
   case ALTLST:{
         FOREACH_SON(Son_Nod, Nod) {
            Node_Nod = Union_Alternative(Node_Nod,
                                         Create_Node_Grammar(Son_Nod));
         }
         break;
      }
   case ALTRNT:{
         Prod_Nod = Nod_FirstSon(Nod);
         Node_Nod = Create_Node_Grammar(Prod_Nod);
         TreeSpec_Nod = Nod_Brother(Prod_Nod);
         if (TreeSpec_Nod == NIL) {
            return Node_Nod;
         }
         if (Nod_NodTyp(TreeSpec_Nod) != KSTRNG) {
            /* should do something more here */
            TreeSpec_Nod = Nod_FirstSon(TreeSpec_Nod);
         }
         Node_Sym = Nod_Sym(TreeSpec_Nod);
         Attribute.Nod =
             Union_Alternative
             (Get_Att(ATP_NodeTree, Node_Sym).Nod, Node_Nod);
         Set_Att(ATP_NodeTree, Node_Sym, Attribute);
         return TreeSpec_Nod;
         break;
      }
   case SEQ:{
         FOREACH_SON(Son_Nod, Nod) {
            Node_Nod = Append_Seq(Node_Nod, Create_Node_Grammar(Son_Nod));
         }
         break;
      }
   case LIST:{
         Node1_Nod = Create_Node_Grammar(Nod_Son(1, Nod));
         Node2_Nod = Create_Node_Grammar(Nod_Son(2, Nod));
         if (Node1_Nod == NIL) {
            return Create_StarNod(Node2_Nod);
         }
         if (Node2_Nod == NIL) {
            return Create_PlusNod(Node1_Nod);
         }
         Node_Nod = Make_Nod(LIST, NConc(Node1_Nod, Node2_Nod));
         break;
      }
   case PLUS:{
         Node_Nod = Create_PlusNod(Create_Node_Grammar(Nod_FirstSon(Nod)));
         break;
      }
   case STAR:{
         Node_Nod = Create_StarNod(Create_Node_Grammar(Nod_FirstSon(Nod)));
         break;
      }
   case QUESTION:{
         Node_Nod = Create_Node_Grammar(Nod_FirstSon(Nod));
         NodTyp = Nod_NodTyp(Node_Nod);
         if (NodTyp == QUESTION || NodTyp == STAR) {
            return Node_Nod;
         }
         if (NodTyp == PLUS) {
            Set_Nod_NodTyp(Node_Nod, STAR);
            return Node_Nod;
         }
         Node_Nod = Make_Nod(QUESTION, Node_Nod);
         break;
      }
   case OPTNAL:{
         break;
      }
   case OUTNOD:{
         break;
      }
   case NAME:{
         Sym = Nod_Sym(Nod);
         Mark = Get_Att(ATP_Marked, Sym).Int;
         if (Mark == MT_Null) {
            (void) Create_Node_Grammar(Get_Att(ATP_VarRule, Sym).Nod);
            Mark = Get_Att(ATP_Marked, Sym).Int;
            FORBIDDEN(Mark == MT_Null);
         }
         if (Mark != MT_Complete) {
            if (Mark != MT_Recursive) {
               Attribute.Int = MT_Recursive;
               Set_Att(ATP_Marked, Sym, Attribute);
            }
            return Make_SymNod(Nod_NodTyp(Nod), Nod_Sym(Nod));
         }
         Node_Nod = Copy_Nod(Get_Att(ATP_VarTree, Sym).Nod);
         break;
      }
   case DSTRNG:{
         break;
      }
   case KSTRNG:{
         Node_Nod = Make_SymNod(Nod_NodTyp(Nod), Nod_Sym(Nod));
         /* really should look up node name */
         break;
      }
   default:{
         FATALERROR("unknown NodTyp");
      }
   }
   return Node_Nod;
}

void Write_Node(tp_FilDsc FilDsc, tp_Nod Nod)
{
   tp_Nod Sub_Nod;

   switch (Nod_NodTyp(Nod)) {
   case NIL:{
         Write(FilDsc, "( )");
         break;
      }
   case ALTLST:{
         Write(FilDsc, "( ");
         Sub_Nod = Nod_FirstSon(Nod);
         Write_Node(FilDsc, Sub_Nod);
         FOREACH_BROTHER(Sub_Nod, Sub_Nod) {
            Write(FilDsc, " | ");
            Write_Node(FilDsc, Sub_Nod);
         }
         Write(FilDsc, " )");
         break;
      }
   case SEQ:{
         Write(FilDsc, "( ");
         Sub_Nod = Nod_FirstSon(Nod);
         Write_Node(FilDsc, Sub_Nod);
         FOREACH_BROTHER(Sub_Nod, Sub_Nod) {
            Write(FilDsc, " ");
            Write_Node(FilDsc, Sub_Nod);
         }
         Write(FilDsc, " )");
         break;
      }
   case LIST:{
         Write(FilDsc, "( ");
         Write_Node(FilDsc, Nod_Son(1, Nod));
         Write(FilDsc, " // ");
         Write_Node(FilDsc, Nod_Son(2, Nod));
         Write(FilDsc, " )");
         break;
      }
   case PLUS:{
         Write_Node(FilDsc, Nod_Son(1, Nod));
         Write(FilDsc, "+");
         break;
      }
   case STAR:{
         Write_Node(FilDsc, Nod_Son(1, Nod));
         Write(FilDsc, "*");
         break;
      }
   case QUESTION:{
         Write_Node(FilDsc, Nod_Son(1, Nod));
         Write(FilDsc, "?");
         break;
      }
   case NAME:{
         Write(FilDsc, Sym_Str(Nod_Sym(Nod)));
         break;
      }
   case KSTRNG:{
         Write(FilDsc, "<");
         Write(FilDsc, Sym_Str(Nod_Sym(Nod)));
         Write(FilDsc, ">");
         break;
      }
   default:{
         FATALERROR("unexpected node type.");
      }
   }
}

tp_Nod Append_Seq(Seq1, Seq2)
tp_Nod Seq1, Seq2;
{
   if (Seq1 == NIL) {
      return Seq2;
   }
   if (Seq2 == NIL) {
      return Seq1;
   }
   if (Nod_NodTyp(Seq1) == SEQ && Nod_NodTyp(Seq2) == SEQ) {
      Set_Nod_FirstSon(Seq1,
                       NConc(Nod_FirstSon(Seq1), Nod_FirstSon(Seq2)));
      return Seq1;
   }
   if (Nod_NodTyp(Seq1) == SEQ) {
      Set_Nod_FirstSon(Seq1, NConc(Nod_FirstSon(Seq1), Seq2));
      return Seq1;
   }
   if (Nod_NodTyp(Seq2) == SEQ) {
      Set_Nod_FirstSon(Seq2, NConc(Seq1, Nod_FirstSon(Seq2)));
      return Seq2;
   }
   return Make_Nod(SEQ, NConc(Seq1, Seq2));
}

tp_Nod Union_Alternative(Alt1, Alt2)
tp_Nod Alt1, Alt2;
{
   if (Alt1 == NIL) {
      return Alt2;
   }
   if (Alt2 == NIL) {
      return Alt1;
   }
   if (Nod_NodTyp(Alt1) == ALTLST && Nod_NodTyp(Alt2) == ALTLST) {
      Set_Nod_FirstSon(Alt1,
                       NUnion(Nod_FirstSon(Alt1), Nod_FirstSon(Alt2)));
      return Alt1;
   }
   if (Nod_NodTyp(Alt1) == ALTLST) {
      Set_Nod_FirstSon(Alt1, NUnion(Nod_FirstSon(Alt1), Alt2));
      return Alt1;
   }
   if (Nod_NodTyp(Alt2) == ALTLST) {
      Set_Nod_FirstSon(Alt2, NUnion(Alt1, Nod_FirstSon(Alt2)));
      return Alt2;
   }
   return Make_Nod(ALTLST, NUnion(Alt1, Alt2));
}

tp_Nod NUnion(Nod1, Nod2)
tp_Nod Nod1, Nod2;
{
   tp_Nod Nod, NextNod;

   if (Nod1 == NIL) {
      return Nod2;
   }
   /* this should remove an element if it is "subsumed" by another one */
   return NConc(Nod1, Nod2);
}

tp_Nod NConc(Nod1, Nod2)
tp_Nod Nod1, Nod2;
{
   tp_Nod Nod;

   if (Nod1 == NIL) {
      return Nod2;
   }
   if (Nod2 == NIL) {
      return Nod1;
   }
   Nod = Nod1;
   while (Nod_Brother(Nod) != NIL)
      Nod = Nod_Brother(Nod);
   Set_Nod_Brother(Nod, Nod2);
   return Nod1;
}

tp_Nod Create_StarNod(tp_Nod ArgNod)
{
   tp_NodTyp NodTyp;

   NodTyp = Nod_NodTyp(ArgNod);
   if (NodTyp == STAR) {
      return ArgNod;
   }
   if (NodTyp == QUESTION || NodTyp == PLUS) {
      Set_Nod_NodTyp(ArgNod, STAR);
      return ArgNod;
   }
   return Make_Nod(STAR, ArgNod);
}

tp_Nod Create_PlusNod(tp_Nod ArgNod)
{
   tp_NodTyp NodTyp;

   NodTyp = Nod_NodTyp(ArgNod);
   if (NodTyp == PLUS || NodTyp == STAR) {
      return ArgNod;
   }
   if (NodTyp == QUESTION) {
      Set_Nod_NodTyp(ArgNod, STAR);
      return ArgNod;
   }
   return Make_Nod(PLUS, ArgNod);
}

tp_Nod Copy_Nod(tp_Nod OldNod)
{
   tp_Nod Nod;

   if (OldNod == NIL) {
      return NIL;
   }
   Nod = New_Nod();
   Set_Nod_NodTyp(Nod, Nod_NodTyp(OldNod));
   Set_Nod_FirstSon(Nod, Copy_Nod(Nod_FirstSon(OldNod)));
   Set_Nod_Brother(Nod, Copy_Nod(Nod_Brother(OldNod)));
   Set_Nod_Sym(Nod, Nod_Sym(OldNod));
   return Nod;
}

tp_Nod Make_Nod(tp_NodTyp NodTyp, tp_Nod FirstSon)
{
   tp_Nod Nod;

   Nod = New_Nod();
   Set_Nod_NodTyp(Nod, NodTyp);
   Set_Nod_FirstSon(Nod, FirstSon);
   return Nod;
}

tp_Nod Make_SymNod(tp_NodTyp NodTyp, tp_Sym Sym)
{
   tp_Nod Nod;

   Nod = New_Nod();
   Set_Nod_NodTyp(Nod, NodTyp);
   Set_Nod_Sym(Nod, Sym);
   return Nod;
}
