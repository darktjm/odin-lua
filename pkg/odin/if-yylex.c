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
#include "inc/NodTyp_.h"
#include "inc/Str.h"

extern tp_Str PrevParseStr;
extern tp_Str ParseStr;
extern tp_FileName ParseFN;
extern int *ParseLNPtr;

tp_Nod YY_Parser(const char *Str, tp_FileName FileName, int *LNPtr)
{
   tps_Str StrBuf;

   if (Str == ERROR)
      return ERROR;
   (void) strcpy(StrBuf, Str);
   PrevParseStr = StrBuf;
   ParseStr = StrBuf;
   ParseFN = FileName;
   ParseLNPtr = LNPtr;
   return YY_Parse();
}

int YY_Lex(void)
{
   IsCmdLex = FALSE;
   return Lex();
}

void YY_Unparse(tp_Str Str, tp_Nod Nod)
{
   tp_Nod Son;
   tp_Str SubStr;
   boolean First;

   switch (Nod_NodTyp(Nod)) {
   case DRVFIL:{
         Son = Nod_Son(1, Nod);
         YY_Unparse(Str, Son);
         SubStr = Tail(Str);
         for (Son = Nod_Brother(Son); Son != NIL; Son = Nod_Brother(Son)) {
            if (Nod_NodTyp(Son) != ELMOPR && Nod_NodTyp(Son) != DIROPR) {
               (void) strcat(SubStr, " ");
            }
            SubStr = Tail(SubStr);
            YY_Unparse(SubStr, Son);
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
         (void) strcpy(Str, "\0");
         Unlex(Tail(Str), Sym_Str(Nod_Sym(Nod)));
         break;
      }
   case ABSFIL:{
         (void) strcpy(Str, "/");
         YY_Unparse(Tail(Str), Nod_Son(1, Nod));
         break;
      }
   case STRING:{
         (void) strcpy(Str, "=");
         YY_Unparse(Tail(Str), Nod_Son(1, Nod));
         break;
      }
   case PRMOPR:{
         (void) strcpy(Str, "+");
         YY_Unparse(Tail(Str), Nod_Son(1, Nod));
         if (Nod_NumSons(Nod) > 1) {
            (void) strcpy(Tail(Str), "=");
            YY_Unparse(Tail(Str), Nod_Son(2, Nod));
         }
         break;
      }
   case APLOPR:{
         (void) strcpy(Str, "+(");
         YY_Unparse(Tail(Str), Nod_Son(1, Nod));
         (void) strcat(Str, ")");
         break;
      }
   case PRMVLS:{
         (void) strcpy(Str, "");
         First = TRUE;
         for (Son = Nod_Son(1, Nod); Son != NIL; Son = Nod_Brother(Son)) {
            if (!First)
               (void) strcpy(Tail(Str), " ");
            if (Nod_NodTyp(Son) == DRVFIL)
               (void) strcat(Str, "(");
            YY_Unparse(Tail(Str), Son);
            if (Nod_NodTyp(Son) == DRVFIL)
               (void) strcat(Str, ")");
            First = FALSE;
         }
         break;
      }
   case DRVOPR:{
         (void) strcpy(Str, ":");
         YY_Unparse(Tail(Str), Nod_Son(1, Nod));
         break;
      }
   case HODOPR:{
         (void) strcpy(Str, ":");
         YY_Unparse(Tail(Str), Nod_Son(1, Nod));
         (void) strcpy(Tail(Str), "=:");
         Son = Nod_Son(2, Nod);
         YY_Unparse(Tail(Str), Son);
         break;
      }
   case ELMOPR:{
         (void) strcpy(Str, "/");
         YY_Unparse(Tail(Str), Nod_Son(1, Nod));
         break;
      }
   case SEGOPR:{
         (void) strcpy(Str, "%");
         YY_Unparse(Tail(Str), Nod_Son(1, Nod));
         break;
      }
   case DIROPR:{
         (void) strcpy(Str, "/");
         break;
      }
   case OPRTNS:{
         (void) strcpy(Str, "(");
         YY_Unparse(Tail(Str), Nod_Son(1, Nod));
         for (Son = Nod_Son(2, Nod); Son != NIL; Son = Nod_Brother(Son)) {
            (void) strcpy(Tail(Str), " ");
            YY_Unparse(Tail(Str), Son);
         }
         (void) strcat(Str, ")");
         break;
      }
   default:{
         FATALERROR("Unexpected NodTyp");
      }
   }
}
