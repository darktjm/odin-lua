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
#include "inc/Str.h"
#include "inc/TokTyp_.h"

boolean IsCmdLex;

tp_FileName ParseFN;
int *ParseLNPtr;
tp_Str PrevParseStr;
tp_Str ParseStr;

void FileError(tp_Str Str)
{
   if (ParseFN == NIL) {
      FORBIDDEN(ParseLNPtr != NIL);
      SystemError(Str);
      return;
   }
   if (ParseLNPtr == NIL) {
      SystemError("\"%s\": %s", ParseFN, Str);
      return;
   }
   SystemError("\"%s\", line %d: %s", ParseFN, *ParseLNPtr, Str);
}

void ParseError(tp_Str Str)
{
   FileError(Str);
   SystemError(" at <%s>.\n", PrevParseStr);
}

void Init_Lex(void)
{
   if (ParseLNPtr != NIL)
      *ParseLNPtr = *ParseLNPtr + 1;
}

void EndLex(void)
{
   SystemError("Unexpected call to EndLex.\n");
}

static void ExpandVar(boolean * AbortPtr, tp_Str * RestStrPtr, tp_Str Str)
{
   tp_Str TmpStr, VarStr, ValStr;
   tps_Str VarStrBuf;

   FORBIDDEN(*Str != '$');
   *AbortPtr = FALSE;
   TmpStr = Str;
   VarStr = VarStrBuf;
   for (TmpStr = Str + 1; IsWordChr(*TmpStr); TmpStr += 1) {
      *VarStr = *TmpStr;
      VarStr += 1;
   }
   *VarStr = 0;
   if (!(IsCmdLex || IsDef_EnvVar(VarStrBuf))) {
      SystemError("Environment variable must be declared in a package.\n");
      *AbortPtr = TRUE;
      return;
   }
   ValStr = GetEnv(VarStrBuf);
   if (ValStr == NIL)
      ValStr = "";
   StrShift(Str, strlen(ValStr) - (TmpStr - Str));
   (void) strncpy(Str, ValStr, strlen(ValStr));
   *RestStrPtr = &Str[strlen(ValStr)];
}

static void ExpandHome(boolean * AbortPtr, tp_Str Str)
{
   tp_Str TmpStr, HomeStr, ValStr;
   tps_Str HomeStrBuf;

   FORBIDDEN(*Str != '~');
   *AbortPtr = FALSE;
   if (!IsCmdLex) {
      SystemError("Cannot use ~ in Odinfile filenames.\n");
      *AbortPtr = TRUE;
      return;
   }
   TmpStr = Str;
   HomeStr = HomeStrBuf;
   for (TmpStr = Str + 1; IsWordChr(*TmpStr); TmpStr += 1) {
      *HomeStr = *TmpStr;
      HomeStr += 1;
   }
   *HomeStr = 0;
   ValStr = GetHome(HomeStrBuf);
   if (ValStr == NIL) {
      SystemError("Home directory ~%s not found.\n", HomeStrBuf);
      *AbortPtr = TRUE;
      return;
   }
   StrShift(Str, strlen(ValStr) - (TmpStr - Str));
   (void) strncpy(Str, ValStr, strlen(ValStr));
}

int Lex(void)
{
   tps_Str Str;
   tp_Str RestStr;
   int iStr;
   tp_Sym Sym;
   boolean Abort;

   PrevParseStr = ParseStr;
   while (TRUE) {
      switch (*ParseStr) {
      case 0:{
            return EOFTOK;
            break;
         }
      case ' ':
      case '\t':{
            ParseStr += 1;
            break;
         }
      case '\n':{
            ParseStr += 1;
            if (ParseLNPtr != NIL)
               *ParseLNPtr = *ParseLNPtr + 1;
            break;
         }
      case '#':{
            while (*ParseStr != 0 && *ParseStr != '\n')
               ParseStr += 1;
            break;
         }
      case '(':{
            ParseStr += 1;
            return LPAREN;
            break;
         }
      case ')':{
            ParseStr += 1;
            return RPAREN;
            break;
         }
      case ':':{
            ParseStr += 1;
            return COLON;
            break;
         }
      case '=':{
            ParseStr += 1;
            return EQUALS;
            break;
         }
      case '/':{
            ParseStr += 1;
            return SLASH;
            break;
         }
      case '+':{
            ParseStr += 1;
            return PLUS;
            break;
         }
      case '<':{
            ParseStr += 1;
            return LANGLE;
            break;
         }
      case '>':{
            ParseStr += 1;
            return RANGLE;
            break;
         }
      case ';':{
            ParseStr += 1;
            return SCOLON;
            break;
         }
      case '?':{
            ParseStr += 1;
            return QUESMK;
            break;
         }
      case '%':{
            ParseStr += 1;
            return PERCNT;
            break;
         }
      case '!':{
            ParseStr += 1;
            while (*ParseStr == ' ' || *ParseStr == '\t'
                   || *ParseStr == '\n') {
               if (*ParseStr == '\n' && ParseLNPtr != NIL) {
                  *ParseLNPtr = *ParseLNPtr + 1;
               }
               ParseStr += 1;
            }
            if (*ParseStr == ':' || *ParseStr == '=' || *ParseStr == '<'
                || *ParseStr == ';' || *ParseStr == 0) {
               return EXCLPT;
            }
            if (*ParseStr == '\\') {
               ParseStr += 1;
            }
            iStr = 0;
            while (*ParseStr != 0) {
               if (*ParseStr == '\n' && ParseLNPtr != NIL) {
                  *ParseLNPtr = *ParseLNPtr + 1;
               }
               Str[iStr] = *ParseStr;
               iStr += 1;
               ParseStr += 1;
            }
            Str[iStr] = 0;
            Sym = Str_Sym(Str);
            Push_SymStack(Sym);
            return HOSTWD;
            break;
         }
      case '$':{
            ExpandVar(&Abort, &RestStr, ParseStr);
            if (Abort) {
               ParseStr += 1;
               return ERRTOK;
            }
            break;
         }
      case '~':{
            ExpandHome(&Abort, ParseStr);
            if (Abort) {
               ParseStr += 1;
               return ERRTOK;
            }
            break;
         }
      default:{
            iStr = 0;
            if (ParseStr[0] == '\\' && ParseStr[1] == '0') {
               ParseStr += 2;
               if (!isdigit(*ParseStr)) {
                  ParseError("Object-ID expected following \\0");
                  return ERRTOK;
               }
               while (isdigit(*ParseStr)) {
                  Str[iStr] = *ParseStr;
                  iStr += 1;
                  ParseStr += 1;
               }
               Str[iStr] = 0;
               Sym = Str_Sym(Str);
               Push_SymStack(Sym);
               return OBJTID;
            }
            while (TRUE) {
               {
                  if (IsWordChr(*ParseStr)) {
                     Str[iStr] = *ParseStr;
                     iStr += 1;
                     ParseStr += 1;
                  } else if (*ParseStr == '\\') {
                     ParseStr += 1;
                     if (*ParseStr == 0) {
                        ParseError("backslash followed by EOF");
                        return ERRTOK;
                     }
                     if (*ParseStr == '\n' && ParseLNPtr != NIL) {
                        *ParseLNPtr = *ParseLNPtr + 1;
                     }
                     Str[iStr] = *ParseStr;
                     iStr += 1;
                     ParseStr += 1;
                  } else if (*ParseStr == '\'') {
                     ParseStr += 1;
                     while (*ParseStr != '\'') {
                        if (*ParseStr == '\\') {
                           ParseStr += 1;
                        }
                        if (*ParseStr == 0) {
                           ParseError("Unterminated string");
                           return ERRTOK;
                        }
                        if (*ParseStr == '\n' && ParseLNPtr != NIL) {
                           *ParseLNPtr = *ParseLNPtr + 1;
                        }
                        Str[iStr] = *ParseStr;
                        iStr += 1;
                        ParseStr += 1;
                     }
                     ParseStr += 1;
                     if (*ParseStr == '$') {
                        ExpandVar(&Abort, &RestStr, ParseStr);
                        if (Abort) {
                           ParseStr += 1;
                           return ERRTOK;
                        }
                        while (ParseStr != RestStr) {
                           Str[iStr] = *ParseStr;
                           iStr += 1;
                           ParseStr += 1;
                        }
                     }
                  } else if (*ParseStr == '$') {
                     ExpandVar(&Abort, &RestStr, ParseStr);
                     if (Abort) {
                        ParseStr += 1;
                        return ERRTOK;
                     }
                  } else {
                     Str[iStr] = 0;
                     Sym = Str_Sym(Str);
                     Push_SymStack(Sym);
                     return WORDTK;
                  }
               }
            }
         }
      }
   }
 /*NOTREACHED*/}

boolean IsWordChr(char Chr)
{
   switch (Chr) {
   case '\0':
   case ' ':
   case '\t':
   case '\n':
   case '#':
   case ';':
   case '?':
   case '!':
   case '%':
   case '<':
   case '>':
   case '(':
   case ')':
   case ':':
   case '=':
   case '/':
   case '+':
   case '\\':
   case '\'':
   case '$':{
         return FALSE;
         break;
      }
   default:{
         return TRUE;
      }
   }
/* NOTREACHED */
}

void Unlex(tp_Str OutStr, tp_Str InStr)
{
   if (*InStr == 0) {
      (void) strcpy(OutStr, "''");
      return;
   }
   if (*InStr == '~' || !IsWordChr(*InStr))
      *OutStr++ = '\\';
   *OutStr++ = *InStr++;
   while (*InStr != 0) {
      if (!IsWordChr(*InStr))
         *OutStr++ = '\\';
      *OutStr++ = *InStr++;
   }
   *OutStr = 0;
}

void Print_Unlex(tp_FilDsc FilDsc, tp_Str InStr)
{
   if (*InStr == 0) {
      Write(FilDsc, "''");
      return;
   }
   if (*InStr == '~' || !IsWordChr(*InStr))
      Writech(FilDsc, '\\');
   Writech(FilDsc, *InStr++);
   while (*InStr != 0) {
      if (!IsWordChr(*InStr))
         Writech(FilDsc, '\\');
      Writech(FilDsc, *InStr++);
   }
}
