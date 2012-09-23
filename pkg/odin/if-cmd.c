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
#include "inc/LogLevel_.h"
#include "inc/OC_NodTyp_.h"
#include "inc/Status_.h"
#include "inc/Str.h"
#include "lualib.h"

static int CommandNesting = 0;
static int MaxCommandNesting = 100;

static void
Execute(boolean * AbortPtr,
        tp_FileName FileName, tp_Str OdinExpr, boolean Interactive)
{
   tp_FilDsc InFD;
   tp_Nod Nod;
   tps_Str StrBuf, DirName;
   tp_Str Str;
   int LineNum;
   boolean Abort;

   if (IsExecutable(FileName)) {
      SystemExecCmdWait(AbortPtr, FileName, Interactive);
      if (*AbortPtr) {
         SystemError("\"%s\": exec failed with non-zero status.\n",
                     OdinExpr);
      }
      return;
   }

   InFD = FileName_RFilDsc(FileName, FALSE);
   if (InFD == ERROR) {
      SystemError("\"%s\": cannot read.\n", FileName);
      *AbortPtr = TRUE;
      return;
   }
   LineNum = 0;

   Push_Context(DirName, FileName);

   ChangeDir(AbortPtr, DirName);
   if (*AbortPtr) {
      SystemError("\"%s\": cd failed.\n", DirName);
      goto done;
   }

   Unblock_Signals();
   Str = ReadLine(StrBuf, InFD);
   Block_Signals();
   if (Str == NIL) {
      goto done;
   }

   while (Str != NIL && !Signalled) {
      if (Str[0] != '@' && LogLevel >= LOGLEVEL_OdinCommand) {
         Write(StdOutFD, "<> ");
         WriteLine(StdOutFD, Str);
      }
      if (Str[0] == '@')
	 Str = Str + 1;
      Nod = OC_Parser(Str, FileName, &LineNum);
      if (Nod == ERROR) {
         *AbortPtr = TRUE;
         goto done;
      }
      CommandInterpreter(AbortPtr, Nod, FALSE);
      Ret_Nod(Nod);
      if (*AbortPtr) {
         goto done;
      }
      Unblock_Signals();
      Str = ReadLine(StrBuf, InFD);
      Block_Signals();
   }

 done:;
   if (Signalled) {
      *AbortPtr = TRUE;
   }
   Pop_Context(DirName);

   ChangeDir(&Abort, DirName);
   if (Abort) {
      SystemError("\"%s\": cd failed.\n", DirName);
      *AbortPtr = TRUE;
   }
   Close(InFD);
}

static boolean IsStatus_MsgLevel(int Level)
{
   return (Level > 0);
}

boolean IsIncremental_MsgLevel(int Level)
{
   return (Level == 2 || Level == 4);
}

static boolean IsSummary_MsgLevel(int Level)
{
   return (Level > 2);
}

static void
Report_Status(tp_Str OdinExpr, tp_Status Status, tp_Status ElmStatus)
{
   tp_Str Message;

   if (!((Status <= STAT_Warning && IsStatus_MsgLevel(WarnLevel))
         || (Status <= STAT_TgtValError && IsStatus_MsgLevel(ErrLevel)))) {
      return;
   }

   switch ((Status < ElmStatus) ? Status : ElmStatus) {
   case STAT_Unknown:{
         Message = "--- <%s> is not up-to-date ---\n";
         break;
      }
   case STAT_Pending:{
         Message = "--- <%s> is pending input computations ---\n";
         break;
      }
   case STAT_Ready:{
         Message = "--- <%s> is ready to be computed ---\n";
         break;
      }
   case STAT_Busy:{
         Message = "--- <%s> is being computed ---\n";
         break;
      }
   case STAT_SysAbort:{
         Message = "--- System abort status set for <%s> ---\n";
         break;
      }
   case STAT_NoFile:{
         Message = ((Status == STAT_NoFile)
                    ? "--- <%s> does not exist ---\n"
                    : "--- An element of <%s> does not exist ---\n");
         break;
      }
   case STAT_ElmCircular:{
         Message = "--- An element of <%s> depends on itself ---\n";
         break;
      }
   case STAT_Circular:{
         Message = "--- <%s> depends on itself ---\n";
         break;
      }
   case STAT_Error:{
         Message = "--- Error status set for <%s> ---\n";
         break;
      }
   case STAT_TgtValError:{
         Message = "--- Target-Error status set for <%s> ---\n";
         break;
      }
   case STAT_Warning:{
         Message = "--- Warning status set for <%s> ---\n";
         break;
      }
   case STAT_OK:{
         Message = NIL;
         break;
      }
   default:{
         FATALERROR("bad status");
      }
   }
   if (Message != NIL) {
      SystemError(Message, OdinExpr);
   }
}

static void
ShowStatus(tp_Str OdinExpr, tp_Status Status, tp_Status ElmStatus)
{
   tp_Str DerivStr;
   tp_Status RepStatus;
   tps_FileName FileName;
   tp_FilDsc FilDsc;
   boolean ExecFlag;

   if (Status == STAT_Unknown) {
      return;
   }

   {
      if (Status <= STAT_Warning && IsSummary_MsgLevel(WarnLevel)) {
         DerivStr = ":warn";
      } else if (Status <= STAT_TgtValError
                 && IsSummary_MsgLevel(ErrLevel)) {
         DerivStr = ":err";
      } else {
         DerivStr = 0;
      }
   }
   if (DerivStr != 0) {
      (void) strcat(OdinExpr, DerivStr);
      Get_OdinFile(FileName, &RepStatus, &ExecFlag, OdinExpr, TRUE);
      if (RepStatus == STAT_OK) {
         FORBIDDEN(FileName[0] == 0);
         FilDsc = FileName_RFilDsc(FileName, FALSE);
         {
            if (FilDsc == ERROR) {
               SystemError("\"%s\": could not read error file.\n",
                           FileName);
            } else {
               FileCopy(StdOutFD, FilDsc);
               Close(FilDsc);
            }
         }
      }
      return;
   }

   Report_Status(OdinExpr, Status, STAT_OK);
}

static void Do_ShowStatus(boolean * AbortPtr, tp_Nod Root)
{
   tps_Str OdinExpr;
   tp_Status Status;
   tps_FileName FileName;
   boolean ExecFlag, Abort;

   OC_Unparse(OdinExpr, Root);
   Get_OdinFile(FileName, &Status, &ExecFlag, OdinExpr, FALSE);
   *AbortPtr = (Status <= STAT_TgtValError);
   if (ExecFlag && !*AbortPtr) {
      if (LogLevel >= LOGLEVEL_OdinCommand) {
         Write(StdOutFD, "<> ");
         Write(StdOutFD, OdinExpr);
         Writeln(StdOutFD, " !");
      }
      Execute(&Abort, FileName, OdinExpr, FALSE);
      if (Abort) {
         *AbortPtr = TRUE;
      }
   }
   ShowStatus(OdinExpr, Status, STAT_OK);
}

static void Display(boolean * AbortPtr, tp_Nod Root)
{
   tps_Str OdinExpr;
   tp_Status Status;
   tps_FileName FileName;
   tp_FilDsc InFD;
   boolean ExecFlag;

   OC_Unparse(OdinExpr, Nod_Son(1, Root));
   Get_OdinFile(FileName, &Status, &ExecFlag, OdinExpr, TRUE);
   *AbortPtr = (Status < STAT_TgtValError);
   if (*AbortPtr) {
      goto done;
   }
   if (strcmp(FileName, "") == 0) {
      SystemError("No file value associated with: %s\n", OdinExpr);
      *AbortPtr = TRUE;
      goto done;
   }
   if (!Exists(FileName)) {
      goto done;
   }
   if (IsDirectory_FileName(FileName)) {
      SystemError("\"%s\": cannot display a directory.\n", OdinExpr);
      goto done;
   }
   InFD = FileName_RFilDsc(FileName, FALSE);
   if (InFD == ERROR) {
      if (Exists(FileName)) {
         SystemError("\"%s\": cannot read.\n", OdinExpr);
         *AbortPtr = TRUE;
      }
      goto done;
   }
   Unblock_Signals();
   FileCopy(StdOutFD, InFD);
   Block_Signals();
   Close(InFD);
   if (Signalled) {
      *AbortPtr = TRUE;
   }
 done:;
   ShowStatus(OdinExpr, Status, STAT_OK);
}

static void Copy(boolean * AbortPtr, tp_Nod FromNod, tp_Nod ToNod)
{
   tps_Str OdinExpr;
   tps_FileName FileName;
   tp_Status Status;
   boolean ExecFlag;

   *AbortPtr = FALSE;
   OC_Unparse(OdinExpr, FromNod);
   (void) strcat(OdinExpr, "+copy_dest_desc=(");
   OC_Unparse(Tail(OdinExpr), ToNod);
   (void) strcat(OdinExpr, "):copy_cmd");
   Get_OdinFile(FileName, &Status, &ExecFlag, OdinExpr, FALSE);
   *AbortPtr = (Status < STAT_TgtValError);
   ShowStatus(OdinExpr, Status, STAT_OK);
}

static void Edit(boolean * AbortPtr, tp_Nod Root, boolean Interactive)
{
   tps_FileName FileName;
   tp_Status Status;
   tp_Str Editor;
   tps_Str CmdStr, OdinExpr;
   boolean ExecFlag;

   Editor = GetEnv("EDITOR");
   if (Editor == NIL)
      Editor = "vi";

   OC_Unparse(OdinExpr, Nod_Son(1, Root));
   Get_OdinFile(FileName, &Status, &ExecFlag, OdinExpr, TRUE);
   *AbortPtr = (Status < STAT_NoFile);
   if (*AbortPtr) {
      ShowStatus(OdinExpr, Status, STAT_OK);
      return;
   }
   if (strcmp(FileName, "") == 0) {
      SystemError("No file value associated with: %s\n", OdinExpr);
      *AbortPtr = TRUE;
      return;
   }

   (void) strcpy(CmdStr, Editor);
   (void) strcat(CmdStr, " ");
   (void) strcat(CmdStr, FileName);
   SystemExecCmdWait(AbortPtr, CmdStr, Interactive);
   Test(OdinExpr);
   Get_OdinFile(FileName, &Status, &ExecFlag, OdinExpr, TRUE);
   ShowStatus(OdinExpr, Status, STAT_OK);
}

static void
Do_Execute(boolean * AbortPtr, tp_Nod Root, boolean Interactive)
{
   tp_Nod TgtNod, CmdNod;
   tps_FileName FileName;
   tp_Status Status;
   tp_Str Cmd;
   tps_Str CmdStr, OdinExpr;
   boolean ExecFlag;

   CmdNod = Nod_Son(1, Root);
   if (CmdNod == NIL) {
      Get_Commands(AbortPtr);
      return;
   }

   TgtNod = NIL;
   if (Nod_NodTyp(CmdNod) == DRVFIL) {
      TgtNod = CmdNod;
      CmdNod = Nod_Brother(CmdNod);
      OC_Unparse(OdinExpr, TgtNod);
      Get_OdinFile(FileName, &Status, &ExecFlag, OdinExpr, TRUE);
      *AbortPtr = (Status < STAT_NoFile);
      if (*AbortPtr) {
         goto done;
      }
      if (strcmp(FileName, "") == 0) {
         SystemError("No file value associated with: %s\n", OdinExpr);
         *AbortPtr = TRUE;
         goto done;
      }
      if (CmdNod == NIL) {
         if (Status <= STAT_Error) {
            *AbortPtr = TRUE;
            goto done;
         }
         Execute(AbortPtr, FileName, OdinExpr, Interactive);
         goto done;
      }
   }

   FORBIDDEN(Nod_NodTyp(CmdNod) != HOSTWD);
   Cmd = Sym_Str(Nod_Sym(CmdNod));
   (void) strcpy(CmdStr, Cmd);
   if (TgtNod != NIL) {
      (void) strcat(CmdStr, " ");
      (void) strcat(CmdStr, FileName);
   }
   /* OK, maybe it is good to be able to do Lua directly */
   /* there isn't much efficiency benefit, but it is more portable */
   /* plus it takes care of ensuring the standard library is loaded */
   if (strncmp(CmdStr, "lua!", 4) == 0) {
      lua_State *L = Lua_Init();
      if(!L)
	 *AbortPtr = TRUE;
      else {
	 if (Lua_String(L, "os.exit = nil") || Lua_String(L, CmdStr + 4))
	    lua_error(L);
	 lua_close(L);
      }
      goto done;
   }
   SystemExecCmdWait(AbortPtr, CmdStr, Interactive);
   if (Signalled) {
      *AbortPtr = TRUE;
   }
   if (TgtNod == NIL) {
      return;
   }
   Test(OdinExpr);

 done:;
   ShowStatus(OdinExpr, Status, STAT_OK);
}

void UtilityHelp(void)
{
   Writeln(StdOutFD,
           "*?* test redo inputs outputs elements elements-of alias");
}

static void Utility(boolean * AbortPtr, tp_Nod Root)
{
   int OdinFileID;
   tp_Str Cmd;
   tps_Str OdinExpr;
   tp_Status Status, ElmStatus;

   *AbortPtr = FALSE;
   if (Nod_NumSons(Root) == 1) {
      Cmd = Sym_Str(Nod_Sym(Nod_Son(1, Root)));
      {
         if (strcmp(Cmd, "test") == 0) {
            Test_All();
         } else if (strcmp(Cmd, "quit") == 0) {
            Exit(0);
         } else {
            SystemError("Unknown default utility <%s>.\n", Cmd);
            *AbortPtr = TRUE;
         }
      }
      return;
   }
   Cmd = Sym_Str(Nod_Sym(Nod_Son(2, Root)));
   OC_Unparse(OdinExpr, Nod_Son(1, Root));
   {
      if (strlen(Cmd) == 2) {
         OdinFileID = Str_PosInt(OdinExpr);
         if (OdinFileID <= 0) {
            SystemError("<%s> seen where a number was expected.\n",
                        OdinExpr);
            *AbortPtr = TRUE;
            return;
         }
         ID_OdinExpr(OdinExpr, OdinFileID);
      } else {
         OdinExpr_ID(&OdinFileID, OdinExpr);
         if (OdinFileID == 0) {
            *AbortPtr = TRUE;
            return;
         }
      }
   }
   ; {
      if (strcmp(Cmd, "name") == 0 || strcmp(Cmd, "NA") == 0) {
         WriteInt(StdOutFD, OdinFileID);
         Write(StdOutFD, "\t- ");
         Writeln(StdOutFD, OdinExpr);
      } else if (strcmp(Cmd, "long-name") == 0 || strcmp(Cmd, "LN") == 0) {
         ID_LongOdinExpr(OdinExpr, OdinFileID);
         Writeln(StdOutFD, OdinExpr);
      } else if (strcmp(Cmd, "elements") == 0 || strcmp(Cmd, "EL") == 0) {
         Get_Elements(OdinFileID);
      } else if (strcmp(Cmd, "element-of") == 0 || strcmp(Cmd, "EO") == 0) {
         Get_ElementsOf(OdinFileID);
      } else if (strcmp(Cmd, "inputs") == 0 || strcmp(Cmd, "IN") == 0) {
         Get_Inputs(OdinFileID);
      } else if (strcmp(Cmd, "outputs") == 0 || strcmp(Cmd, "OU") == 0) {
         Get_Outputs(OdinFileID);
      } else if (strcmp(Cmd, "dpath") == 0 || strcmp(Cmd, "DP") == 0) {
         Get_DPath(OdinExpr);
      } else if (strcmp(Cmd, "redo") == 0 || strcmp(Cmd, "RE") == 0) {
         Redo(OdinExpr);
         return;
      } else if (strcmp(Cmd, "test") == 0 || strcmp(Cmd, "TE") == 0) {
         Test(OdinExpr);
      } else if (strcmp(Cmd, "alias") == 0) {
         Do_Alias(OdinExpr, TRUE);
      } else {
         SystemError("Unknown utility <%s>.\n", Cmd);
         *AbortPtr = TRUE;
         return;
      }
   }
   Get_Status(&Status, &ElmStatus, OdinFileID);
   Report_Status(OdinExpr, Status, ElmStatus);
}

void UtilityDefaultHelp(void)
{
   Writeln(StdOutFD, "*?* test\n");
}

static void Do_SetVar(boolean * AbortPtr, tp_Nod Root)
{
   tp_Str VarStr, ValStr;
   tp_Nod ValNod;
   tps_Str ValBuf;

   VarStr = Sym_Str(Nod_Sym(Nod_Son(1, Root)));
   ValNod = Nod_Son(2, Root);
   if (Nod_NodTyp(ValNod) == HOSTWD) {
      Set_HostVar(AbortPtr, VarStr, Sym_Str(Nod_Sym(ValNod)));
      return;
   }
   OC_Unparse(ValBuf, ValNod);
   ValStr = Sym_Str(Str_Sym(ValBuf));
   SetVar(AbortPtr, VarStr, ValStr);
}

void Print_Banner(void)
{
   tps_Str Banner;

   Get_Banner(Banner);
   Writeln(StdOutFD, Banner);
}

static void
Do_Command(boolean * AbortPtr, tp_Nod Root, boolean Interactive)
{
   tp_Nod Nod;

   *AbortPtr = FALSE;
   switch (Nod_NodTyp(Root)) {
   case NULLCD:{
         break;
      }
   case CMANDS:{
         for (Nod = Nod_FirstSon(Root); Nod != NIL; Nod = Nod_Brother(Nod)) {
            Do_Command(AbortPtr, Nod, Interactive);
            if (Signalled) {
               return;
            }
         }
         break;
      }
   case EXECUT:{
         Do_Execute(AbortPtr, Root, Interactive);
         break;
      }
   case DRVFIL:{
         Do_ShowStatus(AbortPtr, Root);
         break;
      }
   case DISPLY:{
         Display(AbortPtr, Root);
         break;
      }
   case COPYTR:{
         Copy(AbortPtr, Nod_Son(1, Root), Nod_Son(2, Root));
         break;
      }
   case EDIT:{
         Edit(AbortPtr, Root, Interactive);
         break;
      }
   case COPYTL:{
         Copy(AbortPtr, Nod_Son(2, Root), Nod_Son(1, Root));
         break;
      }
   case UTILTY:{
         Utility(AbortPtr, Root);
         break;
      }
   case VARVAL:{
         ShowVar(Root);
         break;
      }
   case VARSET:{
         Do_SetVar(AbortPtr, Root);
         break;
      }
   default:{
         SystemError("Illegal command type.\n");
      }
   }
}

void
CommandInterpreter(boolean * AbortPtr, tp_Nod Root, boolean Interactive)
{
   tp_Nod Nod;
   boolean IsHelp, IsHandled;

   FORBIDDEN(Root == ERROR);

   Do_Help(AbortPtr, &IsHelp, &IsHandled, Root);
   if (*AbortPtr) {
      return;
   }

   if (IsHelp) {
      if (!IsHandled) {
         Nod = OC_Parser(".:odin_help!", (tp_FileName) NIL, (int *) NIL);
         FORBIDDEN(Nod == ERROR);
         CommandInterpreter(AbortPtr, Nod, Interactive);
         Ret_Nod(Nod);
      }
      return;
   }
   Nod = Root;
   if (CommandNesting > MaxCommandNesting) {
      SystemError("Maximum CommandNesting (%d) exceeded.\n",
                  MaxCommandNesting);
      *AbortPtr = TRUE;
      return;
   }
   CommandNesting += 1;
   Do_Command(AbortPtr, Nod, Interactive);
   CommandNesting -= 1;
}
