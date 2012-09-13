/*
Copyright (C) 1991 Geoffrey M. Clemm

This file is part of the Odin system.

The Odin system is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 1,
or (at your option) any later version (see the file COPYING).

The Odin system is distributed WITHOUT ANY WARRANTY, without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

geoff@boulder.colorado.edu
*/

#include "inc/GMC.h"
#include "inc/Str.h"
#include "inc/TokTyp_.h"

int LineNum;

tps_Str ParseBuf;

tp_Str OrigParseStr;
tp_Str PrevParseStr;
tp_Str ParseStr;

int Init_Lex(void)
{
   tp_Sym Sym;

   LineNum = 0;
   ParseStr = "";

   Sym = Str_Sym("server");
   Set_Sym_Att(Sym, TOK_server);
   Sym = Str_Sym("client");
   Set_Sym_Att(Sym, TOK_client);
   Sym = Str_Sym("request");
   Set_Sym_Att(Sym, TOK_request);
   Sym = Str_Sym("split-request");
   Set_Sym_Att(Sym, TOK_splitrequest);
   Sym = Str_Sym("notice");
   Set_Sym_Att(Sym, TOK_notice);
   Sym = Str_Sym("in");
   Set_Sym_Att(Sym, TOK_in);
   Sym = Str_Sym("out");
   Set_Sym_Att(Sym, TOK_out);
   Sym = Str_Sym("inout");
   Set_Sym_Att(Sym, TOK_inout);
   Sym = Str_Sym("str");
   Set_Sym_Att(Sym, TOK_str);
   Sym = Str_Sym("int");
   Set_Sym_Att(Sym, TOK_int);
   return 0;
}

/*private*/ boolean
 YY_IsWordChr(char Chr);

int YY_Lex(void)
{
   char Chr;
   tps_Str Str;
   int iStr;
   int Token;

   if (ParseStr == ERROR) {
      return TOK_EOF;
   }
   PrevParseStr = ParseStr;
   while (TRUE) {
      Chr = *ParseStr;
      while (Chr == 0) {
         LineNum += 1;
         ParseStr = Readln(ParseBuf, StdInFD);
         if (ParseStr == ERROR) {
            return TOK_EOF;
         }
         Chr = *ParseStr;
         OrigParseStr = ParseStr;
         PrevParseStr = ParseStr;
      }
      ParseStr += 1;
      switch (Chr) {
      case ' ':
      case '\t':
      case ';':{
            break;
         }
      case '/':{
            if (*ParseStr != '*') {
               ParseError("'*' expected following '/'");
               return TOK_ERR;
            }
            ParseStr += 1;
            iStr = 0;
            while (*ParseStr != '*') {
               Str[iStr] = *ParseStr;
               iStr += 1;
               ParseStr += 1;
            }
            Str[iStr] = 0;
            ParseStr += 1;
            if (*ParseStr != '/') {
               return TOK_ERR;
            }
            ParseStr += 1;
            Token = (int) Sym_Att(Str_Sym(Str));
            if (Token == 0) {
               return TOK_ERR;
            }
            return Token;
            break;
         }
      case '*':{
            return TOK_star;
            break;
         }
      case ',':{
            return TOK_comma;
            break;
         }
      case '(':{
            return TOK_lparen;
            break;
         }
      case ')':{
            return TOK_rparen;
            break;
         }
      case '{':{
            return TOK_lbrkt;
            break;
         }
      case '}':{
            return TOK_rbrkt;
            break;
         }
      default:{
            iStr = 0;
            Str[iStr] = Chr;
            iStr += 1;
            while (YY_IsWordChr(*ParseStr)) {
               Str[iStr] = *ParseStr;
               iStr += 1;
               ParseStr += 1;
            }
            Str[iStr] = 0;
            Push_SymStack(Str_Sym(Str));
            return TOK_ident;
         }
      }
   }
}

/*private*/ boolean
YY_IsWordChr(char Chr)
{
   switch (Chr) {
   case '\0':
   case ' ':
   case '\t':
   case '{':
   case '}':
   case '(':
   case ')':
   case '*':
   case ',':
   case ';':
   case '/':{
         return FALSE;
         break;
      }
   default:{
         return TRUE;
      }
   }
/* NOTREACHED */
}

void EndLex(void)
{
   SystemError("Unexpected call to EndLex");
}

void ParseError(tp_Str Str)
{
   SystemError("%s at <%s> at line %d.\n", Str, PrevParseStr, LineNum);
}
