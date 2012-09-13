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
#include "inc/Str.h"

static tp_Str Get_DrvOprName(tp_Nod Root)
{
   tp_Nod Brother;

   for (Brother = Root; Brother != NIL; Brother = Nod_Brother(Brother)) {
      if (Nod_NodTyp(Brother) == DRVOPR
          && Nod_NodTyp(Nod_Son(1, Brother)) == WORD) {
         return Sym_Str(Nod_Sym(Nod_Son(1, Brother)));
      }
   }
   return (tp_Str) NIL;
}

void
Do_Help(boolean * AbortPtr,
        boolean * IsHelpPtr, boolean * IsHandledPtr, tp_Nod Root)
{
   tp_Str DrvOprName;
   tps_Str StrBuf;
   tp_Nod Son;

   *AbortPtr = FALSE;
   *IsHelpPtr = FALSE;
   *IsHandledPtr = FALSE;
   if (Root == NIL) {
      return;
   }
   switch (Nod_NodTyp(Root)) {
   case HELP:{
         *IsHelpPtr = TRUE;
         break;
      }
   case PFHELP:{
         *IsHelpPtr = TRUE;
         *IsHandledPtr = TRUE;
         TopLevelCI(AbortPtr, "() :prefix_help>");
         break;
      }
   case SFHELP:{
         *IsHelpPtr = TRUE;
         *IsHandledPtr = TRUE;
         TopLevelCI(AbortPtr, "() :suffix_help>");
         break;
      }
   case EPHELP:{
         *IsHelpPtr = TRUE;
         *IsHandledPtr = TRUE;
         Writeln(StdOutFD, "?*? An arbitrary string of characters.");
         Writeln(StdOutFD,
                 "?*? ( an initial special character must be escaped with a \\ )");
         break;
      }
   case STRING:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr) {
            if (!*IsHandledPtr) {
               *IsHandledPtr = TRUE;
               Write(StdOutFD, "?*? An arbitrary string of characters");
               Writeln(StdOutFD, " (special characters must be escaped).");
            }
         }
         break;
      }
   case WORD:
   case HOSTWD:
   case NULLCD:
   case DIROPR:
   case EMPFIL:
   case ARTFIL:
   case OBJTID:{
         break;
      }
   case CMANDS:{
         for (Son = Nod_FirstSon(Root); Son != NIL; Son = Nod_Brother(Son)) {
            Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Son);
            if (*IsHelpPtr) {
               return;
            }
         }
         break;
      }
   case DRVFIL:{
         Son = Nod_Son(1, Root);
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Son);
         if (*IsHelpPtr) {
            if (!*IsHandledPtr && Nod_NumSons(Root) > 1) {
               *IsHandledPtr = TRUE;
               Writeln(StdOutFD, "*?* An Odin expression.");
            }
            return;
         }
         for (Son = Nod_Brother(Son); Son != NIL; Son = Nod_Brother(Son)) {
            Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Son);
            if (*IsHelpPtr) {
               if (!*IsHandledPtr) {
                  *IsHandledPtr = TRUE;
                  OC_Unparse(StrBuf, Root);
                  {
                     if (Nod_NodTyp(Son) == DRVOPR) {
                        (void) strcat(StrBuf, " :drv_help>");
                        TopLevelCI(AbortPtr, StrBuf);
                     } else {
                        DrvOprName = Get_DrvOprName(Nod_Brother(Son));
                        if (DrvOprName != NIL) {
                           (void) strcat(StrBuf, " +prm_help=");
                           Unlex(Tail(StrBuf), DrvOprName);
                        }
                        (void) strcat(StrBuf, ":prm_help>");
                        TopLevelCI(AbortPtr, StrBuf);
                     }
                  }
                  return;
               }
               return;
            }
         }
         break;
      }
   case COPYTR:
   case COPYTL:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr) {
            if (!*IsHandledPtr) {
               *IsHandledPtr = TRUE;
               Writeln(StdOutFD, "*?* An Odin expression.");
            }
            return;
         }
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(2, Root));
         if (*IsHelpPtr && !*IsHandledPtr) {
            *IsHandledPtr = TRUE;
            Writeln(StdOutFD,
                    "*?* An Odin expression or can be left blank.");
         }
         break;
      }
   case DISPLY:
   case EDIT:
   case EXECUT:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr && !*IsHandledPtr) {
            *IsHandledPtr = TRUE;
            Writeln(StdOutFD, "*?* An Odin expression.");
            return;
         }
         if (Nod_NumSons(Root) > 1) {
            Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(2, Root));
         }
         break;
      }
   case UTILTY:{
         if (Nod_NumSons(Root) == 1) {
            Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
            if (*IsHelpPtr && !*IsHandledPtr) {
               *IsHandledPtr = TRUE;
               UtilityDefaultHelp();
            }
            return;
         }
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr) {
            if (!*IsHandledPtr) {
               *IsHandledPtr = TRUE;
               Writeln(StdOutFD, "*?* An Odin expression.");
            }
            return;
         }
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(2, Root));
         if (*IsHelpPtr && !*IsHandledPtr) {
            *IsHandledPtr = TRUE;
            UtilityHelp();
         }
         break;
      }
   case VARVAL:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr && !*IsHandledPtr) {
            *IsHandledPtr = TRUE;
            ShowVars();
         }
         break;
      }
   case VARSET:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr) {
            if (!*IsHandledPtr) {
               *IsHandledPtr = TRUE;
               ShowVars();
            }
            return;
         }
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(2, Root));
         if (*IsHelpPtr && !*IsHandledPtr) {
            *IsHandledPtr = TRUE;
            HelpVar(Root);
         }
         break;
      }
   case SEGOPR:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr && !*IsHandledPtr) {
            *IsHandledPtr = TRUE;
            Writeln(StdOutFD, "*?* A virtual name.");
         }
         break;
      }
   case ABSFIL:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr && !*IsHandledPtr) {
            *IsHandledPtr = TRUE;
            Writeln(StdOutFD, "*?* A file in the root directory.");
         }
         break;
      }
   case PRMOPR:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr) {
            return;
         }
         if (Nod_NumSons(Root) > 1) {
            Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(2, Root));
            if (*IsHelpPtr && !*IsHandledPtr) {
               *IsHandledPtr = TRUE;
               Writeln(StdOutFD, "*?* A sequence of parameter values.");
               return;
            }
         }
         break;
      }
   case APLOPR:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr && !*IsHandledPtr) {
            *IsHandledPtr = TRUE;
            Writeln(StdOutFD, "*?* An Odin expression.");
            return;
         }
         break;
      }
   case PRMVLS:{
         for (Son = Nod_Son(1, Root); Son != NIL; Son = Nod_Brother(Son)) {
            Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Son);
            if (*IsHelpPtr && !*IsHandledPtr) {
               *IsHandledPtr = TRUE;
               Writeln(StdOutFD,
                       "*?* A word or an Odin expression in parentheses.");
               return;
            }
         }
         break;
      }
   case DRVOPR:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         break;
      }
   case HODOPR:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr) {
            if (!*IsHandledPtr) {
               *IsHandledPtr = TRUE;
               Write(StdOutFD, "*?* \"lookup\", \"map\", \"recurse\", ");
               Writeln(StdOutFD, "\"extract\", or \"delete\".");
            }
            return;
         }
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(2, Root));
         if (*IsHelpPtr && !*IsHandledPtr) {
            *IsHandledPtr = TRUE;
            Writeln(StdOutFD, "*?* An Odin derivation type.");
         }
         break;
      }
   case ELMOPR:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr && !*IsHandledPtr) {
            *IsHandledPtr = TRUE;
            Writeln(StdOutFD, "*?* An element of the directory.");
         }
         break;
      }
   case OPRTNS:{
         for (Son = Nod_Son(1, Root); Son != NIL; Son = Nod_Brother(Son)) {
            Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Son);
            if (*IsHelpPtr && !*IsHandledPtr) {
               *IsHandledPtr = TRUE;
               Writeln(StdOutFD, "*?* An Odin operation.");
               return;
            }
         }
         break;
      }
   case PVLFIL:{
         Do_Help(AbortPtr, IsHelpPtr, IsHandledPtr, Nod_Son(1, Root));
         if (*IsHelpPtr && !*IsHandledPtr) {
            *IsHandledPtr = TRUE;
            Writeln(StdOutFD, "*?* An Odin expression.");
         }
         break;
      }
   default:{
         FATALERROR("Unexpected NodTyp");
      }
   }
}

void Local_Next_OdinFile(tp_Str OdinExpr, int ID)
{
   WriteInt(StdOutFD, ID);
   Write(StdOutFD, "\t- ");
   WriteLine(StdOutFD, OdinExpr);
}
