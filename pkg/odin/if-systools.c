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
#include "inc/FileName.h"
#include "inc/FKind_.h"
#include "inc/Flag_.h"
#include "inc/LogLevel_.h"
#include "inc/NodTyp_.h"
#include "inc/Status_.h"
#include "inc/Str.h"

void WriteCat(tp_FilDsc FilDsc, tp_FilHdr FilHdr)
{
   tp_FilElm FilElm;
   tp_FilDsc InFD;
   tps_FileName FileName;
   tp_FilHdr ElmFilHdr;

   if (FilHdr_Flag(FilHdr, FLAG_Union)) {
      FilHdr_Error("Cycle detected at: %s\n", FilHdr);
      return;
   }
   if (IsViewSpec(FilHdr)) {
      FilHdr_Error("Illegal view specification argument: %s\n", FilHdr);
      return;
   }
   Set_Flag(FilHdr, FLAG_Union);
   {
      if (IsStr(FilHdr)) {
         FilHdr_DataFileName(FileName, FilHdr);
         Writeln(FilDsc, FileName);
      } else if (!IsRef(FilHdr)) {
         FilHdr_DataFileName(FileName, FilHdr);
         InFD = FileName_RFilDsc(FileName, FALSE);
         if (InFD != NIL) {
            FileCopy(FilDsc, InFD);
            Close(InFD);
         }
      } else {
         for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
              FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
            ElmFilHdr = FilElm_FilHdr(FilElm);
            FORBIDDEN(ElmFilHdr == ERROR);
            WriteCat(FilDsc, ElmFilHdr);
            Ret_FilHdr(ElmFilHdr);
         }
      }
   }
   Clr_Flag(FilHdr, FLAG_Union);
}

void WriteFlat(tp_FilDsc FilDsc, tp_FilHdr FilHdr)
{
   tp_FilElm FilElm;
   tps_FileName FileName;
   tp_FilHdr ElmFilHdr;

   if (FilHdr_Flag(FilHdr, FLAG_Union)) {
      FilHdr_Error("Cycle detected at: %s\n", FilHdr);
      return;
   }
   Set_Flag(FilHdr, FLAG_Union);
   {
      if (!IsRef(FilHdr)) {
         FilHdr_DataFileName(FileName, FilHdr);
         Writeln(FilDsc, FileName);
      } else {
         for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
              FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
            ElmFilHdr = FilElm_FilHdr(FilElm);
            FORBIDDEN(ElmFilHdr == ERROR);
            WriteFlat(FilDsc, ElmFilHdr);
            Ret_FilHdr(ElmFilHdr);
         }
      }
   }
   Clr_Flag(FilHdr, FLAG_Union);
}

void WriteNames(tp_FilDsc FilDsc, tp_FilHdr FilHdr, tp_FilPrm FilPrm)
{
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr;

   if (FilHdr_Flag(FilHdr, FLAG_Union)) {
      FilHdr_Error("Cycle detected at: %s\n", FilHdr);
      return;
   }
   Set_Flag(FilHdr, FLAG_Union);
   {
      if (!IsRef(FilHdr)) {
         WritePrmOdinExpr(FilDsc, FilHdr, FilPrm);
      } else {
         for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
              FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
            ElmFilHdr = FilElm_FilHdr(FilElm);
            WriteNames(FilDsc, ElmFilHdr,
                       Append_FilPrm(FilElm_FilPrm(FilElm), FilPrm));
            Ret_FilHdr(ElmFilHdr);
         }
      }
   }
   Clr_Flag(FilHdr, FLAG_Union);
}

void WriteLabels(tp_FilDsc FilDsc, tp_FilHdr FilHdr)
{
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr;
   tps_Str StrBuf;

   if (FilHdr_Flag(FilHdr, FLAG_Union)) {
      FilHdr_Error("Cycle detected at: %s\n", FilHdr);
      return;
   }
   Set_Flag(FilHdr, FLAG_Union);
   {
      if (!IsRef(FilHdr)) {
         Writeln(FilDsc, FilHdr_Label(StrBuf, FilHdr, FALSE));
      } else {
         for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
              FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
            ElmFilHdr = FilElm_FilHdr(FilElm);
            FORBIDDEN(ElmFilHdr == ERROR);
            WriteLabels(FilDsc, ElmFilHdr);
            Ret_FilHdr(ElmFilHdr);
         }
      }
   }
   Clr_Flag(FilHdr, FLAG_Union);
}

static void
Get_Union(tp_LocElm * FirstLEPtr,
          tp_LocElm * LastLEPtr,
          tp_FilHdr FilHdr, tp_FilPrm FilPrm, tp_FilHdr ListFilHdr)
{
   tp_FilHdr ElmFilHdr;
   tp_LocElm LocElm;
   tp_FilElm FilElm;

   if (FilHdr_Flag(FilHdr, FLAG_Union)) {
      return;
   }
   Set_Flag(FilHdr, FLAG_Union);

   if (!IsRef(FilHdr)) {
      LocElm = Make_LocElm(FilHdr, FilPrm, ListFilHdr);
      Chain_LocElms(FirstLEPtr, LastLEPtr, LocElm);
      return;
   }

   for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      Get_Union(FirstLEPtr, LastLEPtr, ElmFilHdr,
                Append_FilPrm(FilElm_FilPrm(FilElm), FilPrm), ListFilHdr);
      Ret_FilHdr(ElmFilHdr);
   }
}

tp_LocElm Make_UnionLocElm(tp_FilHdr FilHdr, tp_FilHdr ListFilHdr)
{
   tp_LocElm FirstLE, LastLE;

   FirstLE = NIL;
   LastLE = NIL;
   Get_Union(&FirstLE, &LastLE, FilHdr, FilHdr_FilPrm(ListFilHdr),
             ListFilHdr);
   Clr_UnionFlags(FilHdr);
   return FirstLE;
}

void Clr_UnionFlags(tp_FilHdr FilHdr)
{
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr;

   if (!FilHdr_Flag(FilHdr, FLAG_Union)) {
      return;
   }
   Clr_Flag(FilHdr, FLAG_Union);
   for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      Clr_UnionFlags(ElmFilHdr);
      Ret_FilHdr(ElmFilHdr);
   }
}

static boolean TagStrCmp(tp_Str Str, tp_Str Tag)
{
   if (strcmp(Tag, "") == 0) {
      while (*Str == ' ' || *Str == '\t') {
         Str += 1;
      }
   }
   return strcmp(Str, Tag);
}

static void
Get_DefInfo(tp_Str * NamePtr,
            boolean * CmdFlagPtr,
            boolean * ExecFlagPtr,
            tp_Str * TagStrPtr, tp_Nod * TgtNodPtr, tp_Nod Nod)
{
   tp_Nod ValNod;

   *NamePtr = Sym_Str(Nod_Sym(Nod_Son(1, Nod)));
   *TagStrPtr = NIL;
   *TgtNodPtr = NIL;
   ValNod = Nod_Son(2, Nod);
   *CmdFlagPtr = (Nod_NodTyp(ValNod) == EXEDEF);
   if (*CmdFlagPtr) {
      ValNod = Nod_Brother(ValNod);
   }
   *ExecFlagPtr = (Nod_NodTyp(ValNod) == ETXDEF);
   if (*ExecFlagPtr || Nod_NodTyp(ValNod) == TEXDEF) {
      *TagStrPtr = Sym_Str(Nod_Sym(Nod_Son(1, ValNod)));
      if (*TagStrPtr == NIL) {
         *TagStrPtr = "";
      }
      return;
   }
   *TgtNodPtr = ValNod;
}

void
Exec_List(tp_FilHdr ListFilHdr,
          tp_FilHdr FilHdr, tp_FilPrm FilPrm, boolean IsOdinfile)
{
   tps_FileName FileName;
   tp_LocElm FirstLE, LastLE;
   tp_FilDsc InFD;
   tp_Str Str, Name, TagStr;
   tps_Str StrBuf;
   tp_Nod Nod, DS_Nod, TgtNod;
   tp_NodTyp NodTyp;
   int LineNum;
   tp_LocElm NewLocElm;
   tp_PrmFHdr PrmFHdr;
   tp_FilHdr ElmFilHdr;
   tp_FilPrm ElmFilPrm;
   boolean End, CmdFlag, ExecFlag;

   FirstLE = NIL;
   LastLE = NIL;

   FilHdr_DataFileName(FileName, FilHdr);
   if (IsDirectory_FileName(FileName)) {
      InFD = OpenDir(FileName);
      for (ReadDir(StrBuf, &End, InFD); !End; ReadDir(StrBuf, &End, InFD)) {
         ElmFilHdr = Do_Key(Copy_FilHdr(FilHdr), StrBuf);
         NewLocElm = Make_LocElm(ElmFilHdr, RootFilPrm, ListFilHdr);
         Ret_FilHdr(ElmFilHdr);
         Chain_LocElms(&FirstLE, &LastLE, NewLocElm);
      }
      CloseDir(InFD);
      Set_LocElm(ListFilHdr, FirstLE);
      return;
   }

   InFD = FileName_RFilDsc(FileName, FALSE);
   if (InFD == ERROR) {
      Set_LocElm(ListFilHdr, (tp_LocElm) NIL);
      return;
   }

   Push_ContextFilHdr(Copy_FilHdr(FilHdr));
   LineNum = 0;
   for (Str = ReadLine(StrBuf, InFD);
        Str != ERROR; Str = ReadLine(StrBuf, InFD)) {
      Nod = YY_Parser(Str, FileName, &LineNum);
      DS_Nod = Nod;
      NodTyp = Nod_NodTyp(DS_Nod);
      if (IsOdinfile && (NodTyp == FILDEF || NodTyp == SEGDEF)) {
         Get_DefInfo(&Name, &CmdFlag, &ExecFlag, &TagStr, &TgtNod, Nod);
         if (TagStr != NIL) {
            if (TagStr[0] == '\n') {
               TagStr = &TagStr[1];
            }
            for (Str = Readln(StrBuf, InFD), LineNum += 1;
                 Str != ERROR && TagStrCmp(Str, TagStr) != 0;
                 Str = Readln(StrBuf, InFD), LineNum += 1) {
            }
            if (Str == ERROR && strlen(TagStr) != 0) {
               SystemError
                   ("Terminator \"%s\" not found for target \"%s\".\n",
                    TagStr, Name);
            }
         }
         DS_Nod = ERROR;
      }
      if (IsOdinfile && Nod_NodTyp(DS_Nod) == NSTDEF) {
         DS_Nod = Nod_Son(1, DS_Nod);
      }
      if (Nod_NodTyp(DS_Nod) == DRVFLS) {
         DS_Nod = Nod_Son(1, DS_Nod);
      }
      while (DS_Nod != NIL) {
         PrmFHdr = Nod_PrmFHdr(DS_Nod);
         Use_PrmFHdr(&ElmFilHdr, &ElmFilPrm, PrmFHdr);
         {
            if (ElmFilHdr == ERROR) {
               SystemError("in odin expression at:\n");
               FileError("\n");
            } else {
               NewLocElm = Make_LocElm(ElmFilHdr,
                                       Append_FilPrm(ElmFilPrm, FilPrm),
                                       ListFilHdr);
               Ret_FilHdr(ElmFilHdr);
               Chain_LocElms(&FirstLE, &LastLE, NewLocElm);
            }
         }
         DS_Nod = Nod_Brother(DS_Nod);
      }
      Ret_Nod(Nod);
   }
   if (!EndOfFile(InFD)) {
      FileError("Unexpected EOF\n");
   }

   Pop_ContextFilHdr();
   Close(InFD);
   Set_LocElm(ListFilHdr, FirstLE);
}

void Exec_TargetsPtr(tp_FilHdr FilHdr, tp_FilHdr InFilHdr)
{
   tp_LocElm LocElm;
   tp_FilHdr ElmFilHdr;

   {
      if (IsDir(InFilHdr)) {
         ElmFilHdr = Do_Key(Copy_FilHdr(InFilHdr), "Odinfile");
         ElmFilHdr = Do_Deriv(ElmFilHdr, RootFilPrm, RootFilPrm,
                              TargetsPtrFilTyp);
      } else {
         ElmFilHdr = Deref(Copy_FilHdr(InFilHdr));
         if (FilHdr_Status(ElmFilHdr) != STAT_NoFile) {
            ElmFilHdr = Do_Deriv(ElmFilHdr, RootFilPrm, RootFilPrm,
                                 TargetsFilTyp);
         }
      }
   }
   LocElm = ERROR;
   if (ElmFilHdr != ERROR) {
      LocElm = Make_LocElm(ElmFilHdr, RootFilPrm, FilHdr);
      Ret_FilHdr(ElmFilHdr);
   }
   Set_LocElm(FilHdr, LocElm);
}

void Exec_Targets(tp_FilDsc OutFD, tp_FileName FileName)
{
   tp_FilDsc InFD;
   tp_Str Str, Name, TagStr;
   tps_Str StrBuf;
   tps_Str DataFileName;
   tp_Nod Nod, TgtNod;
   tp_NodTyp NodTyp;
   int LineNum;
   tp_PrmFHdr PrmFHdr;
   tp_FilHdr FilHdr;
   tp_FilPrm FilPrm;
   boolean CmdFlag, ExecFlag;

   InFD = FileName_RFilDsc(FileName, FALSE);
   if (InFD == ERROR) {
      return;
   }

   LineNum = 0;
   for (Str = ReadLine(StrBuf, InFD);
        Str != ERROR; Str = ReadLine(StrBuf, InFD)) {
      Nod = YY_Parser(Str, FileName, &LineNum);
      NodTyp = Nod_NodTyp(Nod);
      switch (NodTyp) {
      case FILDEF:
      case SEGDEF:{
            WriteLine(OutFD, Str);
            Get_DefInfo(&Name, &CmdFlag, &ExecFlag, &TagStr, &TgtNod, Nod);
            if (TagStr != NIL) {
               if (TagStr[0] == '\n') {
                  TagStr = &TagStr[1];
               }
               for (Str = Readln(StrBuf, InFD), LineNum += 1;
                    Str != ERROR && TagStrCmp(Str, TagStr) != 0;
                    Str = Readln(StrBuf, InFD), LineNum += 1) {
                  Writeln(OutFD, Str);
               }
               if (Str != ERROR) {
                  Writeln(OutFD, Str);
               }
            }
            break;
         }
      case NSTDEF:{
            PrmFHdr = Nod_PrmFHdr(Nod_Son(1, Nod));
            if (PrmFHdr == ERROR) {
               SystemError("in odin expression at:\n");
               FileError("\n");
            }
            Use_PrmFHdr(&FilHdr, &FilPrm, PrmFHdr);
            Deref_Pntrs(&FilHdr, &FilPrm, FilHdr, TRUE);
            if (FilHdr != ERROR) {
               FilHdr_DataFileName(DataFileName, FilHdr);
               Exec_Targets(OutFD, DataFileName);
               Ret_FilHdr(FilHdr);
            }
            break;
         }
      case DRVFLS:{
            {
               if (Nod_NumSons(Nod) > 0) {
                  FileError("Must be a target.\n");
               } else {
                  WriteLine(OutFD, Str);
               }
            }
            break;
         }
      case NIL:{
            break;
         }
      default:{
            FileError("Must be a target.\n");
         }
      }
      Ret_Nod(Nod);
   }
   if (!EndOfFile(InFD)) {
      FileError("Unexpected EOF\n");
   }
   Close(InFD);
}

void WriteSrcNames(tp_FilDsc OutFD, tp_FileName FileName, boolean OpFlag)
{
   tp_FilDsc InFD;
   tp_Str Str, TailStr;
   tps_Str StrBuf;
   tp_Nod Nod, DS_Nod, Son;
   int LineNum;
   boolean FoundOp;

   InFD = FileName_RFilDsc(FileName, FALSE);
   if (InFD == ERROR) {
      return;
   }

   LineNum = 0;
   for (Str = ReadLine(StrBuf, InFD);
        Str != ERROR; Str = ReadLine(StrBuf, InFD)) {
      Nod = YY_Parser(Str, FileName, &LineNum);
      if (Nod != ERROR) {
         for (DS_Nod =
              ((Nod_NodTyp(Nod) == DRVFLS) ? Nod_Son(1, Nod) : Nod);
              DS_Nod != NIL; DS_Nod = Nod_Brother(DS_Nod)) {
            {
               if (DS_Nod == ERROR) {
                  SystemError("in odin expression at:\n");
                  FileError("\n");
               } else {
                  (void) strcpy(Str, "");
                  FoundOp = FALSE;
                  Son = Nod_Son(1, DS_Nod);
                  if (Nod_NodTyp(Son) == SEGOPR && !OpFlag) {
                     (void) strcpy(Str, ".");
                     FoundOp = TRUE;
                  }
                  while (Son != NIL) {
                     TailStr = Tail(Str);
                     switch (Nod_NodTyp(Son)) {
                     case PRMOPR:
                     case APLOPR:
                     case DRVOPR:
                     case HODOPR:
                     case SEGOPR:{
                           FoundOp = TRUE;
                        }
                     }
                     if (FoundOp == OpFlag) {
                        YY_Unparse(TailStr, Son);
                     }
                     Son = Nod_Brother(Son);
                  }
                  WriteLine(OutFD, Str);
               }
            }
         }
         Ret_Nod(Nod);
      }
   }
   if (!EndOfFile(InFD)) {
      FileError("Unexpected EOF\n");
   }
   Close(InFD);
}

void Validate_ViewSpec(tp_FilHdr FilHdr)
{
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr;
   boolean StrFlag;

   StrFlag = TRUE;
   for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      StrFlag = IsStr(ElmFilHdr);
      Ret_FilHdr(ElmFilHdr);
   }
   if (!StrFlag) {
      SystemError
          ("Last element of view-specification must be a string.\n");
   }
}

tp_FilElm FilElm_NextStrFilElm(tp_FilElm FilElm)
{
   tp_FilElm StrFilElm;
   tp_FilHdr StrFilHdr;

   StrFilElm = FilElm;
   StrFilHdr = FilElm_FilHdr(StrFilElm);
   while (!IsStr(StrFilHdr)) {
      Ret_FilHdr(StrFilHdr);
      FilElm = FilElm_NextFilElm(FilElm);
      StrFilHdr = FilElm_FilHdr(FilElm);
   }
   Ret_FilHdr(StrFilHdr);
   return FilElm;
}

void
Exec_CmptView(boolean * ErrPtr, tp_FilHdr OutFilHdr, tp_FilHdr InpFilHdr)
{
   tp_LocElm FirstLE, LastLE;
   tp_FilElm FilElm;
   tp_LocElm NewLocElm;
   tp_FilHdr ElmFilHdr;
   tp_Str Str;

   *ErrPtr = FALSE;
   FirstLE = NIL;
   LastLE = NIL;
   for (FilElm = LocElm_FilElm(FilHdr_LocElm(InpFilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      {
         if (FilHdr_ElmStatus(ElmFilHdr) == STAT_NoFile) {
            if (FilHdr_TgtValStatus(ElmFilHdr) != STAT_OK) {
               *ErrPtr = TRUE;
            }
         } else {
            {
               if (IsStr(ElmFilHdr)) {
                  Str = FilHdr_Ident(ElmFilHdr);
                  if (strcmp(Str, "") != NIL) {
                     SystemError("<%s> not found in view-path.\n", Str);
                  }
               } else {
                  NewLocElm = Make_LocElm(ElmFilHdr, FilElm_FilPrm(FilElm),
                                          OutFilHdr);
                  Chain_LocElms(&FirstLE, &LastLE, NewLocElm);
                  FilElm = FilElm_NextStrFilElm(FilElm);
               }
            }
         }
      }
      Ret_FilHdr(ElmFilHdr);
   }
   Set_LocElm(OutFilHdr, FirstLE);
}

void Install_ActTgt(tp_FilHdr ActTgtFilHdr)
{
   tp_FilHdr FilHdr;
   tps_Str StrBuf;
   tp_Str Key;

   if (FilHdr_ActTgtInstalled(ActTgtFilHdr)) {
      return;
   }
   Set_ActTgtInstalled(ActTgtFilHdr, TRUE);
   Key = FilHdr_Key(StrBuf, ActTgtFilHdr);
   FORBIDDEN(Key == NIL);
   FilHdr = Do_Key(FilHdr_SrcFilHdr(Copy_FilHdr(ActTgtFilHdr)), Key);
   switch (FilHdr_FKind(FilHdr)) {
   case FK_SrcReg:{
         Set_FKind(FilHdr, FK_BoundSrc);
         break;
      }
   case FK_SymLinkReg:{
         Set_FKind(FilHdr, FK_BoundSymLink);
         break;
      }
   default:{
         FORBIDDEN(!IsSource(FilHdr));
      }
   }
   Set_TgtValLocElm(FilHdr, Make_CopyLocElm(ActTgtFilHdr, FilHdr, FilHdr));
   Push_ToBroadcast(Copy_FilHdr(FilHdr));
   Do_Log("Installing target", ActTgtFilHdr, LOGLEVEL_Target);
   Do_Log("   for", FilHdr, LOGLEVEL_Target);
   Ret_FilHdr(FilHdr);
}

void Uninstall_ActTgt(tp_FilHdr ActTgtFilHdr)
{
   tp_FilHdr FilHdr;
   tps_Str StrBuf;
   tp_Str Key;

   if (!FilHdr_ActTgtInstalled(ActTgtFilHdr)) {
      return;
   }
   Set_ActTgtInstalled(ActTgtFilHdr, FALSE);
   Key = FilHdr_Key(StrBuf, ActTgtFilHdr);
   FORBIDDEN(Key == NIL);
   FilHdr = Do_Key(FilHdr_SrcFilHdr(Copy_FilHdr(ActTgtFilHdr)), Key);
   switch (FilHdr_FKind(FilHdr)) {
   case FK_BoundSrc:{
         Set_FKind(FilHdr, FK_SrcReg);
         break;
      }
   case FK_BoundSymLink:{
         Set_FKind(FilHdr, FK_SymLinkReg);
         break;
      }
   default:{
         FORBIDDEN(!IsSource(FilHdr));
      }
   }
   Set_DfltTgtValLocElm(FilHdr);
   Push_ToBroadcast(Copy_FilHdr(FilHdr));
   Do_Log("Uninstalling odin target", ActTgtFilHdr, LOGLEVEL_Target);
   Do_Log("   for", FilHdr, LOGLEVEL_Target);
   Ret_FilHdr(FilHdr);
}

void
WriteTextDef(tp_FilHdr FilHdr,
             tp_FilDsc OutFD,
             tp_FileName OutFileName,
             tp_FilDsc InFD, tp_FileName InFileName)
{
   boolean CmdFlag, ExecFlag, Found, NeedLastEOL, NeedEOL;
   tp_Key Key;
   tps_Str StrBuf, KeyBuf;
   tp_Str Str, Name, TagStr;
   tp_Nod Nod, TgtNod;
   int LineNum;

   LineNum = 0;
   Key = FilHdr_Key(KeyBuf, FilHdr);
   FORBIDDEN(Key == NIL);
   for (Str = ReadLine(StrBuf, InFD);
        Str != ERROR; Str = ReadLine(StrBuf, InFD)) {
      Nod = YY_Parser(Str, InFileName, &LineNum);
      if (Nod == ERROR) {
         return;
      }
      switch (Nod_NodTyp(Nod)) {
      case FILDEF:
      case SEGDEF:{
            Get_DefInfo(&Name, &CmdFlag, &ExecFlag, &TagStr, &TgtNod, Nod);
            Found = (strcmp(Name, Key) == 0 && ((Nod_NodTyp(Nod) == SEGDEF)
                                                ==
                                                IsVTgtText_FKind
                                                (FilHdr_FKind(FilHdr))));
            Ret_Nod(Nod);
            if (TagStr != NIL) {
               NeedLastEOL = TRUE;
               if (TagStr[0] == '\n') {
                  NeedLastEOL = FALSE;
                  TagStr = &TagStr[1];
               }
               NeedEOL = FALSE;
               for (Str = Readln(StrBuf, InFD), LineNum += 1;
                    Str != ERROR && TagStrCmp(Str, TagStr) != 0;
                    Str = Readln(StrBuf, InFD), LineNum += 1) {
                  if (Found) {
                     if (NeedEOL)
                        Writeln(OutFD, "");
                     Write(OutFD, Str);
                     NeedEOL = TRUE;
                  }
               }
               if (Found) {
                  if (NeedLastEOL)
                     Writeln(OutFD, "");
                  if (ExecFlag)
                     MakeExecutable(OutFileName);
                  if (Str == ERROR && strlen(TagStr) != 0) {
                     SystemError
                         ("Terminator \"%s\" not found for target \"%s\".\n",
                          TagStr, Name);
                  }
                  return;
               }
            }
            Found = FALSE;
            break;
         }
      case DRVFLS:{
            if (Nod_NumSons(Nod) > 0) {
               FileError("Must be a target.\n");
               Ret_Nod(Nod);
               return;
            }
            Ret_Nod(Nod);
            break;
         }
      default:{
            Ret_Nod(Nod);
            FileError("Must be a target.\n");
            return;
         }
      }
   }
   SystemError("Text target for %s not found.\n", Key);
}

tp_LocElm
Make_TargetsLocElm(tp_FilHdr FilHdr,
                   tp_FilDsc InFD,
                   tp_FileName InFileName,
                   tp_Date DepModDate, boolean VirFlag)
{
   tp_Nod Nod, TgtNod;
   tps_Str StrBuf;
   tp_Str Str, Name, TagStr;
   int LineNum;
   boolean CmdFlag, ExecFlag;
   tp_PrmFHdr PrmFHdr;
   tp_FilPrm FilPrm;
   tp_FilHdr ValFilHdr, TgtFilHdr;
   tp_LocElm LocElm, FirstLE, LastLE;
   tp_FKind FKind;

   LineNum = 0;
   FirstLE = NIL;
   LastLE = NIL;
   for (Str = ReadLine(StrBuf, InFD);
        Str != ERROR; Str = ReadLine(StrBuf, InFD)) {
      Nod = YY_Parser(Str, InFileName, &LineNum);
      if (Nod == ERROR) {
         return FirstLE;
      }
      switch (Nod_NodTyp(Nod)) {
      case FILDEF:
      case SEGDEF:{
            Get_DefInfo(&Name, &CmdFlag, &ExecFlag, &TagStr, &TgtNod, Nod);
            if ((Nod_NodTyp(Nod) == SEGDEF) == VirFlag) {
               FKind = (VirFlag ? (CmdFlag ? FK_VirCmdTgt : FK_VirTgt)
                        : (CmdFlag ? FK_ActCmdTgt : FK_ActTgt));
               TgtFilHdr = Get_KeyDrv(Copy_FilHdr(FilHdr), FKind, Name);
               Set_FKind(TgtFilHdr, FKind);
               LocElm = NIL;
               {
                  if (TgtNod != NIL) {
                     PrmFHdr = Nod_PrmFHdr(TgtNod);
                     Use_PrmFHdr(&ValFilHdr, &FilPrm, PrmFHdr);
                  } else {
                     FKind =
                         (VirFlag
                          ? (ExecFlag ? FK_VirTgtExText : FK_VirTgtText)
                          : (ExecFlag ? FK_ActTgtExText : FK_ActTgtText));
                     ValFilHdr =
                         Get_KeyDrv(Deref
                                    (FilHdr_Father(Copy_FilHdr(FilHdr))),
                                    FKind, Name);
                     FilPrm = RootFilPrm;
                  }
               }
               if (ValFilHdr != ERROR) {
                  Set_LocElm(TgtFilHdr,
                             Make_LocElm(ValFilHdr, FilPrm, TgtFilHdr));
                  Ret_FilHdr(ValFilHdr);
                  Update_RefFile(TgtFilHdr, STAT_OK, DepModDate);
                  LocElm = Make_LocElm(TgtFilHdr, RootFilPrm, FilHdr);
                  Chain_LocElms(&FirstLE, &LastLE, LocElm);
               }
               Ret_FilHdr(TgtFilHdr);
            }
            if (TagStr != NIL) {
               if (TagStr[0] == '\n') {
                  TagStr = &TagStr[1];
               }
               for (Str = Readln(StrBuf, InFD), LineNum += 1;
                    Str != ERROR && TagStrCmp(Str, TagStr) != 0;
                    Str = Readln(StrBuf, InFD), LineNum += 1) {
               }
               if (Str == ERROR && strlen(TagStr) != 0) {
                  SystemError
                      ("Terminator \"%s\" not found for target \"%s\".\n",
                       TagStr, Name);
               }
            }
            break;
         }
      case DRVFLS:{
            if (Nod_NumSons(Nod) > 0) {
               FileError("Must be a target.\n");
               Ret_Nod(Nod);
               return FirstLE;
            }
            break;
         }
      default:{
            FileError("Must be a target.\n");
            Ret_Nod(Nod);
            return FirstLE;
         }
      }
      Ret_Nod(Nod);
   }
   return FirstLE;
}

void Exec_VirDir(tp_FilHdr FilHdr, tp_FilHdr InpFilHdr)
{
   tps_FileName FileName, ToFileName, ElmFileName;
   tps_Str KeyBuf;
   tp_Str Key;
   tp_FilHdr ElmFilHdr;
   tp_FilElm FilElm;
   boolean Abort;
   size_t sz;

   FilHdr_DataFileName(FileName, FilHdr);
   if (!Data_Exists(FilHdr)) {
      MakeDirFile(&Abort, FileName);
      FORBIDDEN(Abort);
      Set_Size(FilHdr, 1);
   }
   ClearDir(FileName);
   for (FilElm = LocElm_FilElm(FilHdr_LocElm(InpFilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      Key = FilHdr_Key(KeyBuf, ElmFilHdr);
      ElmFilHdr = Deref(ElmFilHdr);
      sz = snprintf(ToFileName, MAX_FileName, "%s/%s", FileName, Key);
      if (sz >= MAX_FileName) {
         (void) fprintf(stderr,
                        "File name too long (MAX_FileName=%d): %s/%s\n",
                        MAX_FileName, FileName, Key);
         exit(1);
      }
      FilHdr_DataFileName(ElmFileName, ElmFilHdr);
      SymLink(&Abort, ToFileName, ElmFileName);
      if (Abort) {
         SystemError("Multiple entries exist with name: %s\n", Key);
      }
      Ret_FilHdr(ElmFilHdr);
   }
}

void
FilPVal_LocElm(tp_LocElm * FirstLEPtr,
               tp_LocElm * LastLEPtr, tp_FilPVal FilPVal, tp_FilHdr FilHdr)
{
   tp_LocPVal ValLocPVal;
   tp_LocElm LocElm;
   tp_FilHdr ElmFilHdr;

   if (IsRootFilPVal(FilPVal)) {
      return;
   }

   FilPVal_LocElm(FirstLEPtr, LastLEPtr, FilPVal_Father(FilPVal), FilHdr);
   ValLocPVal = FilPVal_ValLocPVal(FilPVal);
   if (ValLocPVal != NIL) {
      FilPVal_LocElm(FirstLEPtr, LastLEPtr, LocPVal_FilPVal(ValLocPVal),
                     FilHdr);
      return;
   }
   ElmFilHdr = LocHdr_FilHdr(FilPVal_LocHdr(FilPVal));
   LocElm = Make_LocElm(ElmFilHdr, RootFilPrm, FilHdr);
   Chain_LocElms(FirstLEPtr, LastLEPtr, LocElm);
   Ret_FilHdr(ElmFilHdr);
}

tp_LocElm Make_PntrHoLocElm(tp_FilHdr InFilHdr, tp_FilHdr FilHdr)
{
   tp_LocElm LocElm;
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr, NewElmFilHdr;

   FORBIDDEN(!IsPntr(InFilHdr));

   FilElm = LocElm_FilElm(FilHdr_LocElm(InFilHdr));
   FORBIDDEN(FilElm_NextFilElm(FilElm) != NIL);
   ElmFilHdr = FilElm_FilHdr(FilElm);
   FORBIDDEN(ElmFilHdr == ERROR);
   NewElmFilHdr = Do_Deriv
       (Copy_FilHdr(ElmFilHdr), RootFilPrm,
        Append_FilPrm(FilElm_FilPrm(FilElm), FilHdr_FilPrm(FilHdr)),
        FilHdr_FilTyp(FilHdr));
   {
      if (NewElmFilHdr == ERROR) {
         FilHdr_Error(" from: %s\n", ElmFilHdr);
         LocElm = NIL;
      } else {
         LocElm = Make_LocElm(NewElmFilHdr, RootFilPrm, FilHdr);
         Ret_FilHdr(NewElmFilHdr);
      }
   }
   Ret_FilHdr(ElmFilHdr);
   return LocElm;
}

static tp_FilTyp FilHdr_ArgFilTyp(tp_FilHdr FilHdr)
{
   return FilTyp_ArgFilTyp(FilHdr_FilTyp(FilHdr));
}

static void
Get_Recurse(tp_LocElm * FirstLEPtr,
            tp_LocElm * LastLEPtr,
            tp_FilHdr FilHdr,
            tp_FilPrm FilPrm, tp_FilTyp FilTyp, tp_FilHdr ListFilHdr)
{
   tp_FilHdr ElmFilHdr;
   tp_DrvPth DrvPth;
   tp_LocElm LocElm;
   tp_FilElm FilElm;

   if (IsViewSpec(FilHdr)) {
      FilHdr_Error("Illegal view specification argument: %s\n", FilHdr);
      return;
   }

   if (FilHdr_Flag(FilHdr, FLAG_Union)) {
      return;
   }
   Set_Flag(FilHdr, FLAG_Union);

   if (!IsRef(FilHdr)) {
      ElmFilHdr = Copy_FilHdr(FilHdr);
      if (FilHdr_FilTyp(ElmFilHdr) != FilTyp) {
         DrvPth = Get_DrvPth(ElmFilHdr, FilTyp);
         if (DrvPth != ERROR) {
            ElmFilHdr = Do_DrvPth(ElmFilHdr, RootFilPrm, FilPrm, DrvPth);
            Ret_DrvPth(DrvPth);
            ElmFilHdr = Do_Deriv(ElmFilHdr, RootFilPrm, FilPrm,
                                 FilHdr_FilTyp(ListFilHdr));
         }
      }
      LocElm = Make_LocElm(ElmFilHdr, FilPrm, ListFilHdr);
      Chain_LocElms(FirstLEPtr, LastLEPtr, LocElm);
      Ret_FilHdr(ElmFilHdr);
      return;
   }

   for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      Get_Recurse(FirstLEPtr, LastLEPtr, ElmFilHdr,
                  Append_FilPrm(FilElm_FilPrm(FilElm), FilPrm),
                  FilTyp, ListFilHdr);
      Ret_FilHdr(ElmFilHdr);
   }
}

tp_LocElm Make_RecurseLocElm(tp_FilHdr FilHdr, tp_FilHdr ListFilHdr)
{
   tp_FilTyp FilTyp;
   tp_FilPrm FilPrm;
   tp_LocElm FirstLE, LastLE;

   FilTyp = FilHdr_ArgFilTyp(ListFilHdr);
   FORBIDDEN(FilTyp == ERROR);
   FORBIDDEN(!IsList_FilTyp(FilTyp));

   FilPrm = FilHdr_FilPrm(ListFilHdr);
   FirstLE = NIL;
   LastLE = NIL;
   Get_Recurse(&FirstLE, &LastLE, FilHdr, FilPrm, FilTyp, ListFilHdr);
   Clr_UnionFlags(FilHdr);
   return FirstLE;
}

tp_LocElm Make_DerefPrmValLocElm(tp_FilHdr InFilHdr, tp_FilHdr FilHdr)
{
   tp_FilTyp FilTyp;
   tp_FilHdr ElmFilHdr;
   tp_LocElm LocElm;

   FilTyp = FilHdr_ArgFilTyp(FilHdr);
   FORBIDDEN(FilTyp == ERROR);

   ElmFilHdr = Do_Deriv(Deref(Copy_FilHdr(InFilHdr)), RootFilPrm,
                        FilPrm_DerefPrmVal(FilHdr_FilPrm(FilHdr)), FilTyp);
   if (ElmFilHdr == ERROR) {
      return ERROR;
   }
   LocElm = Make_LocElm(ElmFilHdr, RootFilPrm, FilHdr);
   Ret_FilHdr(ElmFilHdr);
   return LocElm;
}

static void
Get_ExDel(tp_LocElm * FirstLEPtr,
          tp_LocElm * LastLEPtr,
          tp_FilHdr FilHdr,
          tp_FilPrm FilPrm,
          tp_FilTyp FilTyp, tp_FilHdr ListFilHdr, boolean ExFlag)
{
   tp_LocElm LocElm;
   tp_FilElm FilElm;
   tp_FilHdr ElmFilHdr;
   tp_FilPrm ElmFilPrm;

   if (FilHdr_Flag(FilHdr, FLAG_Union)) {
      return;
   }
   Set_Flag(FilHdr, FLAG_Union);

   if (!IsRef(FilHdr)) {
      if (ExFlag == IsSubType(FilHdr_FilTyp(FilHdr), FilTyp)) {
         LocElm = Make_LocElm(FilHdr, FilPrm, ListFilHdr);
         Chain_LocElms(FirstLEPtr, LastLEPtr, LocElm);
      }
      return;
   }

   for (FilElm = LocElm_FilElm(FilHdr_LocElm(FilHdr));
        FilElm != NIL; FilElm = FilElm_NextFilElm(FilElm)) {
      ElmFilHdr = FilElm_FilHdr(FilElm);
      ElmFilPrm = Append_FilPrm(FilElm_FilPrm(FilElm), FilPrm);
      Get_ExDel(FirstLEPtr, LastLEPtr,
                ElmFilHdr, ElmFilPrm, FilTyp, ListFilHdr, ExFlag);
      Ret_FilHdr(ElmFilHdr);
   }
}

tp_LocElm
Make_ExDelLocElm(tp_FilHdr FilHdr, tp_FilHdr ListFilHdr, boolean ExFlag)
{
   tp_LocElm FirstLE, LastLE;
   tp_FilTyp FilTyp;

   if (!(IsList(FilHdr) || IsPntr(FilHdr))) {
      SystemError("Input to :%s must be a list.\n",
                  (ExFlag ? "extract" : "delete"));
      return ERROR;
   }
   if (IsViewSpec(FilHdr)) {
      FilHdr_Error("Illegal view specification argument: %s\n", FilHdr);
      return ERROR;
   }

   FilTyp = FilHdr_ArgFilTyp(ListFilHdr);
   FORBIDDEN(FilTyp == ERROR);
   if (!IsAtmc_FilTyp(FilTyp)) {
      SystemError("The argument %s to :%s must be an atomic type.\n",
                  FilTyp_FTName(FilTyp), (ExFlag ? "extract" : "delete"));
      return ERROR;
   }

   FirstLE = NIL;
   LastLE = NIL;
   Get_ExDel(&FirstLE, &LastLE,
             FilHdr, RootFilPrm, FilTyp, ListFilHdr, ExFlag);
   Clr_UnionFlags(FilHdr);
   return FirstLE;
}
