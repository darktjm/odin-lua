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

#include <stdio.h>

#include "inc/GMC.h"
#include "inc/Str.h"
#include "inc/AttTyp_.h"
#include "inc/TokTyp_.h"

int NextChar;

boolean LastEOL;
int LineNum;
int TokNum;

int GetChar(void)
{
   int Ch;

   if (LastEOL) {
      LineNum += 1;
      TokNum = 0;
      LastEOL = FALSE;
   }
   Ch = getchar();
   if (Ch == '\n')
      LastEOL = TRUE;
   return Ch;
}

void Init_Lex(void)
{
   tp_Sym Sym;
   tp_Att Attribute;

   LineNum = 0;
   TokNum = 0;
   LastEOL = TRUE;
   NextChar = GetChar();

   Sym = Str_Sym("EOF");
   Attribute.Int = TOK_EOF_;
   Set_Att(ATP_Keyword, Sym, Attribute);
   Sym = Str_Sym("ERR");
   Attribute.Int = TOK_ERR_;
   Set_Att(ATP_Keyword, Sym, Attribute);
   Sym = Str_Sym("INCLUDE");
   Attribute.Int = TOK_INCLUDE;
   Set_Att(ATP_Keyword, Sym, Attribute);
   Sym = Str_Sym("SCANNER");
   Attribute.Int = TOK_SCANNER;
   Set_Att(ATP_Keyword, Sym, Attribute);
   Sym = Str_Sym("NODES");
   Attribute.Int = TOK_NODES;
   Set_Att(ATP_Keyword, Sym, Attribute);
   Sym = Str_Sym("RULES");
   Attribute.Int = TOK_RULES;
   Set_Att(ATP_Keyword, Sym, Attribute);
}

void EndLex(void)
{
   SystemError("Unexpected call to EndLex");
}

int YY_Lex(void)
{
   int Chr;
   tps_Str Str;
   int iStr;
   tp_Sym Sym;
   int SymTok;

   TokNum += 1;
   while (TRUE) {
      Chr = NextChar;
      if (Chr != EOF)
         NextChar = GetChar();
      switch (Chr) {
      case EOF:{
            return TOK_EOF;
            break;
         }
      case ' ':
      case '\t':
      case '\n':{
            break;
         }
      case '#':{
            while (NextChar != '\n' && NextChar != EOF)
               NextChar = GetChar();
            if (NextChar == EOF)
               ParseError("Unterminated comment");
            break;
         }
      case '=':{
            if (NextChar == '>') {
               NextChar = GetChar();
               return TOK_DoubleArrow;
            }
            return TOK_Equals;
            break;
         }
      case '-':{
            if (NextChar != '>') {
               return TOK_ERR;
            }
            NextChar = GetChar();
            return TOK_SingleArrow;
            break;
         }
      case '+':{
            return TOK_Plus;
            break;
         }
      case '*':{
            return TOK_Star;
            break;
         }
      case '/':{
            if (NextChar != '/') {
               return TOK_ERR;
            }
            NextChar = GetChar();
            return TOK_DoubleSlash;
            break;
         }
      case ';':{
            return TOK_SemiColon;
            break;
         }
      case '?':{
            return TOK_Question;
            break;
         }
      case '(':{
            return TOK_LeftParen;
            break;
         }
      case ')':{
            return TOK_RightParen;
            break;
         }
      case '\\':{
            {
               if (NextChar == '\n') {
                  NextChar = GetChar();
               } else {
                  ParseError("\\ should be followed by <CR>");
               }
            }
            break;
         }
      case '\'':{
            iStr = 0;
            while (NextChar != '\'' && NextChar != EOF) {
               if (NextChar == '\\') {
                  NextChar = GetChar();
                  if (NextChar == EOF) {
                     ParseError("Unterminated string");
                     return TOK_ERR;
                  }
               }
               Str[iStr] = NextChar;
               iStr += 1;
               NextChar = GetChar();
            }
            if (NextChar == EOF) {
               ParseError("Unterminated string");
               return TOK_ERR;
            }
            NextChar = GetChar();
            Str[iStr] = 0;
            Sym = Str_Sym(Str);
            Push_SymStack(Sym);
            return TOK_AString;
            break;
         }
      case '"':{
            iStr = 0;
            while (NextChar != '"' && NextChar != EOF) {
               if (NextChar == '\\') {
                  NextChar = GetChar();
                  if (NextChar == EOF) {
                     ParseError("Unterminated string");
                     return TOK_ERR;
                  }
               }
               Str[iStr] = NextChar;
               iStr += 1;
               NextChar = GetChar();
            }
            if (NextChar == EOF) {
               ParseError("Unterminated string");
               return TOK_ERR;
            }
            NextChar = GetChar();
            Str[iStr] = 0;
            Sym = Str_Sym(Str);
            Push_SymStack(Sym);
            return TOK_QString;
            break;
         }
      default:{
            if (IsNameChr(Chr)) {
               iStr = 0;
               Str[iStr] = Chr;
               iStr += 1;
               while (NextChar != EOF && IsNameChr(NextChar)) {
                  Str[iStr] = NextChar;
                  iStr += 1;
                  NextChar = GetChar();
               }
               Str[iStr] = 0;
               Sym = Str_Sym(Str);
               SymTok = Get_Att(ATP_Keyword, Sym).Int;
               if (SymTok != 0) {
                  return SymTok;
               }
               Push_SymStack(Sym);
               return TOK_Name;
            }
            ParseError("Unexpected character");
         }
      }
   }
}

boolean IsNameChr(char Chr)
{
   switch (Chr) {
   case 'a':
   case 'b':
   case 'c':
   case 'd':
   case 'e':
   case 'f':
   case 'g':
   case 'h':
   case 'i':
   case 'j':
   case 'k':
   case 'l':
   case 'm':
   case 'n':
   case 'o':
   case 'p':
   case 'q':
   case 'r':
   case 's':
   case 't':
   case 'u':
   case 'v':
   case 'w':
   case 'x':
   case 'y':
   case 'z':
   case 'A':
   case 'B':
   case 'C':
   case 'D':
   case 'E':
   case 'F':
   case 'G':
   case 'H':
   case 'I':
   case 'J':
   case 'K':
   case 'L':
   case 'M':
   case 'N':
   case 'O':
   case 'P':
   case 'Q':
   case 'R':
   case 'S':
   case 'T':
   case 'U':
   case 'V':
   case 'W':
   case 'X':
   case 'Y':
   case 'Z':
   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
   case '-':
   case '_':
   case '.':
   case '/':
   case ',':{
         return TRUE;
         break;
      }
   default:{
         return FALSE;
      }
   }
/* NOTREACHED */
}

void ParseError(tp_Str Str)
{
   SystemError("%s at line %d, token %d.\n", Str, LineNum, TokNum);
}
