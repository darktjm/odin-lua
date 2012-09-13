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
#include "inc/OC_NodTyp_.h"

extern tp_Str PrevParseStr;
extern tp_Str ParseStr;
extern tp_FileName ParseFN;
extern int *ParseLNPtr;

tp_Nod OC_Parser(tp_Str Str, tp_FileName FileName, int *LNPtr)
{
   if (Str == ERROR)
      return ERROR;
   PrevParseStr = Str;
   ParseStr = Str;
   ParseFN = FileName;
   ParseLNPtr = LNPtr;
   return OC_Parse();
}

int OC_Lex(void)
{
   IsCmdLex = TRUE;
   return Lex();
}

void OC_Unparse(tp_Str Str, tp_Nod Nod)
{
   tp_Nod Son;
   tp_Str SubStr;

   switch (Nod_NodTyp(Nod)) {
   case DRVFIL:{
         Son = Nod_Son(1, Nod);
         OC_Unparse(Str, Son);
         SubStr = Tail(Str);
         for (Son = Nod_Brother(Son); Son != NIL; Son = Nod_Brother(Son)) {
            if (Nod_NodTyp(Nod_Son(1, Son)) == HELP) {
               return;
            }
            if (Nod_NodTyp(Son) != ELMOPR && Nod_NodTyp(Son) != DIROPR) {
               (void) strcat(SubStr, " ");
            }
            SubStr = Tail(SubStr);
            OC_Unparse(SubStr, Son);
         }
         break;
      }
   case WORD:{
         Unlex(Str, Sym_Str(Nod_Sym(Nod)));
         break;
      }
   case HOSTWD:{
         (void) strcpy(Str, "!");
         Unlex(Tail(Str), Sym_Str(Nod_Sym(Nod)));
         break;
      }
   case EMPFIL:{
         (void) strcpy(Str, "()");
         break;
      }
   case ARTFIL:{
         (void) strcpy(Str, "/");
         break;
      }
   case OBJTID:{
         (void) strcpy(Str, "\\0");
         Unlex(Tail(Str), Sym_Str(Nod_Sym(Nod)));
         break;
      }
   case ABSFIL:{
         (void) strcpy(Str, "/");
         OC_Unparse(Tail(Str), Nod_Son(1, Nod));
         break;
      }
   case STRING:{
         (void) strcpy(Str, "=");
         OC_Unparse(Tail(Str), Nod_Son(1, Nod));
         break;
      }
   case PRMOPR:{
         (void) strcpy(Str, "+");
         OC_Unparse(Tail(Str), Nod_Son(1, Nod));
         if (Nod_NumSons(Nod) > 1) {
            (void) strcpy(Tail(Str), "=");
            OC_Unparse(Tail(Str), Nod_Son(2, Nod));
         }
         break;
      }
   case APLOPR:{
         (void) strcpy(Str, "+(");
         OC_Unparse(Tail(Str), Nod_Son(1, Nod));
         (void) strcat(Str, ")");
         break;
      }
   case PRMVLS:{
         OC_Unparse(Str, Nod_Son(1, Nod));
         for (Son = Nod_Son(2, Nod); Son != NIL; Son = Nod_Brother(Son)) {
            (void) strcpy(Tail(Str), " ");
            OC_Unparse(Tail(Str), Son);
         }
         break;
      }
   case DRVOPR:{
         (void) strcpy(Str, ":");
         OC_Unparse(Tail(Str), Nod_Son(1, Nod));
         break;
      }
   case HODOPR:{
         (void) strcpy(Str, ":");
         OC_Unparse(Tail(Str), Nod_Son(1, Nod));
         (void) strcpy(Tail(Str), "=:");
         OC_Unparse(Tail(Str), Nod_Son(2, Nod));
         break;
      }
   case ELMOPR:{
         (void) strcpy(Str, "/");
         OC_Unparse(Tail(Str), Nod_Son(1, Nod));
         break;
      }
   case SEGOPR:{
         (void) strcpy(Str, "%");
         OC_Unparse(Tail(Str), Nod_Son(1, Nod));
         break;
      }
   case DIROPR:{
         (void) strcpy(Str, "/");
         break;
      }
   case OPRTNS:{
         (void) strcpy(Str, "(");
         OC_Unparse(Tail(Str), Nod_Son(1, Nod));
         for (Son = Nod_Son(2, Nod); Son != NIL; Son = Nod_Brother(Son)) {
            (void) strcpy(Tail(Str), " ");
            OC_Unparse(Tail(Str), Son);
         }
         (void) strcat(Str, ")");
         break;
      }
   case PVLFIL:{
         (void) strcpy(Str, "(");
         OC_Unparse(Tail(Str), Nod_Son(1, Nod));
         (void) strcat(Str, ")");
         break;
      }
   default:{
         FATALERROR("Unexpected NodTyp");
      }
   }
}
