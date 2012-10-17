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
#include <glib.h>

tp_Str Author = "odin-build-users@lists.sourceforge.net";

boolean IsTTY;

void InterruptAction(void)
{
   Do_Interrupt(TRUE);
}

void TopLevelCI(boolean * AbortPtr, tp_Str Str)
{
   tp_Nod Root;

   Root = OC_Parser(Str, (tp_FileName) NIL, (int *) NIL);
   if (Root == ERROR) {
      *AbortPtr = TRUE;
      return;
   }
   if (VerifyLevel >= 2 && IsTTY) {
      Test_All();
   }
   CommandInterpreter(AbortPtr, Root, IsTTY);
   Ret_Nod(Root);
}

void Get_Commands(boolean * AbortPtr)
{
   static boolean In_Get_Commands = FALSE;

   if (In_Get_Commands) {
      SystemError("Already reading commands.\n");
      *AbortPtr = TRUE;
      return;
   }
   In_Get_Commands = TRUE;
   if (IsTTY)
      Print_Banner();
   IPC_Get_Commands(AbortPtr, (IsServer ? "=> " : "-> "));
   if (IsTTY)
      Writeln(StdOutFD, "");
   In_Get_Commands = FALSE;
}

int main(int argc, char **argv)
{
   boolean Abort, NewFlag;
   int i;

   g_set_application_name("Odin");
   g_set_prgname(g_path_get_basename(argv[0]));
   Init_IO();
   Init_Err();
   Init_Env();
   Init_Sigs(FALSE);
   IPC_Init();
   IPC_Action = TopLevelCI;

   if (IsServer) {
      Init_Sigs(TRUE);
      Read_DrvGrf();
      Init_Info(&NewFlag);
      Init_FilHdrs();
      Init_FilInps();
      Init_FilElms();
      Init_FilPrm();
      Init_FilHdrTree();
      Activate_Client(LocalClient);
      if (NewFlag) {
         Write_ENV2();
      }
   }

   if (IsClient) {
      IsTTY = GetIsTTY();
      Init_Vars();
      Init_CWD();
   }

   Read_ENV2();

   if (IsClient) {
      if (VerifyLevel >= 1)
         Test_All();

      if (argc <= 1) {
         Get_Commands(&Abort);
         Exit((Abort ? 1 : 0));
      }

      for (i = 1; i < argc; i += 1) {
         ; {
            if (strlen(argv[i]) == 0) {
               Get_Commands(&Abort);
            } else {
               TopLevelCI(&Abort, argv[i]);
            }
         }
         if (Abort) {
            Exit(1);
         }
      }
      Exit(0);
   }

   IPC_Get_Commands(&Abort, (tp_Str) NIL);
   Exit((Abort ? 1 : 0));
   return 0;                    /*to make lint happy */
}
