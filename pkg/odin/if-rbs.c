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

#include "inc/System.hh"
#include "inc/sys_param.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>

#include "inc/GMC.h"
#include "inc/Host.h"
#include "inc/LogLevel_.h"
#include "inc/Str.h"

tp_Host FirstHost = NIL;

static int RBS_SocketFD = -1;
static int RBS_Port = -1;

static void Init_RBS(void)
{
   struct sockaddr_in InSockAddr;
   struct sockaddr *SockAddrPtr = (struct sockaddr *) &InSockAddr;
   int i, status;
   socklen_t len;

   FORBIDDEN(RBS_SocketFD >= 0);
   RBS_SocketFD = socket(AF_INET, SOCK_STREAM, 0);
   FORBIDDEN(RBS_SocketFD < 0);
   for (i = 0; i < sizeof(InSockAddr); i += 1) {
      ((char *) &InSockAddr)[i] = '\0';
   }
   InSockAddr.sin_family = AF_INET;
   status = bind(RBS_SocketFD, SockAddrPtr, sizeof(InSockAddr));
   FORBIDDEN(status != 0);
   len = sizeof(InSockAddr);
   status = getsockname(RBS_SocketFD, SockAddrPtr, &len);
   FORBIDDEN(status != 0);
   RBS_Port = InSockAddr.sin_port;
   FORBIDDEN(RBS_Port <= 0);
   status = listen(RBS_SocketFD, 7);    /* because 7 is a lucky number */
   FORBIDDEN(status != 0);
}

tp_Str Host_HostName(tp_Host Host)
{
   return Host->HostName;
}

int Host_FD(tp_Host Host)
{
   return Host->FD;
}

tp_Host Host_Next(tp_Host Host)
{
   return Host->Next;
}

tp_Host Lookup_Host(tp_Str HostName)
{
   tp_Host Host;

   if (strcmp(HostName, "LOCAL") == 0) {
      return NIL;
   }
   for (Host = FirstHost; Host != NIL; Host = Host->Next) {
      if (HostName == Host->HostName) {
         return Host;
      }
   }
   Host = (tp_Host) malloc(sizeof(tps_Host));
   Host->HostName = HostName;
   Host->FD = -1;
   Host->Next = FirstHost;
   FirstHost = Host;
   return Host;
}

tp_Host PId_Host(int PId)
{
   tp_Host Host;

   for (Host = FirstHost; Host != NIL; Host = Host->Next) {
      if (Host->RBS_Id == PId) {
         return Host;
      }
   }
   return NIL;
}

static void RBS_Close(tp_Host Host)
{
   if (Host->FD < 0) {
      return;
   }
   Cancel_Builds(Host);
   (void) close(Host->FD);
   Host->FD = -1;
}

void RBS_Done(tp_Host Host)
{
   if (Host->RBS_Id < 0) {
      return;
   }
   RBS_Close(Host);
   Host->RBS_Id = -1;
   SystemError("Remote build server on %s died.  Will try to restart.\n",
               Host->HostName);
}

static void RBS_Write_Int(boolean * AbortPtr, tp_Host Host, int Int)
{
   int cc;

   if (Host->FD < 0) {
      *AbortPtr = TRUE;
      return;
   }
   cc = write(Host->FD, (char *) &Int, sizeof(Int));
   *AbortPtr = (cc != sizeof(Int));
   if (*AbortPtr) {
      RBS_Close(Host);
   }
}

static void RBS_Read_Int(boolean * AbortPtr, tp_Host Host, int *IntPtr)
{
   int cc;

   if (Host->FD < 0) {
      *AbortPtr = TRUE;
      return;
   }
   cc = IPC_Read(Host->FD, (char *) IntPtr, sizeof(*IntPtr));
   *AbortPtr = (cc != sizeof(*IntPtr));
   if (*AbortPtr) {
      RBS_Close(Host);
   }
}

static void
RBS_Write_Str(boolean * AbortPtr, tp_Host Host, const char *Str)
{
   int cc, len;

   len = strlen(Str);
   RBS_Write_Int(AbortPtr, Host, len);
   if (*AbortPtr) {
      return;
   }
   if (len > 0) {
      cc = write(Host->FD, Str, len);
      *AbortPtr = (cc != len);
      if (*AbortPtr) {
         RBS_Close(Host);
      }
   }
}

static void RBS_Read_Str(boolean * AbortPtr, tp_Host Host, char *Str)
{
   int cc, len;

   RBS_Read_Int(AbortPtr, Host, &len);
   if (*AbortPtr) {
      return;
   }
   if (len > 0) {
      cc = IPC_Read(Host->FD, Str, len);
      *AbortPtr = (cc != len);
      if (*AbortPtr) {
         RBS_Close(Host);
      }
   }
   Str[len] = 0;
}

void RBS_Get_Msg(tp_Host Host)
{
   boolean RBS_Abort, Abort;
   int JobID;

   RBS_Read_Int(&RBS_Abort, Host, &JobID);
   if (RBS_Abort)
      return;
   RBS_Read_Int(&RBS_Abort, Host, &Abort);
   if (RBS_Abort)
      return;
   Build_Done(JobID_Build(JobID), Abort);
}

static void
RBS_Write_VarDef(boolean * RBS_AbortPtr, tp_Host Host, tp_Str VarDef)
{
   RBS_Write_Int(RBS_AbortPtr, Host, (int) 5);
   if (*RBS_AbortPtr)
      return;
   RBS_Write_Str(RBS_AbortPtr, Host, VarDef);
}

static void Init_RBS_Env(tp_Host Host)
{
   extern char **environ;
   char **env;
   boolean Abort;

   for (env = environ; *env != 0; env += 1) {
      RBS_Write_VarDef(&Abort, Host, *env);
      if (Abort) {
         return;
      }
   }
}

void
RBS_Do_Build(tp_Host Host,
             int JobID,
             tp_FileName JobDirName, tp_FileName LogFileName, char **ArgV)
{
   int status, i;
   socklen_t AddrLen;
   char *RBS_ArgV[6];
   tps_Str RBS_CmdPath, PortStr;
   char LocalHostName[MAXHOSTNAMELEN];
   struct sockaddr Addr;
   boolean RBS_Abort;

   if (RBS_Port < 0)
      Init_RBS();
   if (Host->FD < 0) {
      status = gethostname(LocalHostName, MAXHOSTNAMELEN);
      FORBIDDEN(status != 0);
      (void) sprintf(RBS_CmdPath, "%s/PKGS/%s", OdinDirName, RBS_Cmd);
      (void) sprintf(PortStr, "%d", RBS_Port);
      RBS_ArgV[0] = RBS_Cmd;
      RBS_ArgV[1] = Host->HostName;
      RBS_ArgV[2] = OdinDirName;
      RBS_ArgV[3] = LocalHostName;
      RBS_ArgV[4] = PortStr;
      RBS_ArgV[5] = 0;
      Host->RBS_Id = SystemExec(RBS_CmdPath, RBS_ArgV, (tp_Str) NIL);
      if (Host->RBS_Id <= 0) {
         SystemError("Could not start remote build server: %s.\n",
                     RBS_CmdPath);
         Exit(1);
      }
      AddrLen = sizeof(Addr);
      Unblock_Signals();
      while (Host->FD < 0 && !Signalled) {
         sleep(1);
         Host->FD = accept(RBS_SocketFD, &Addr, &AddrLen);
      }
      Block_Signals();
      if (Host->FD < 0) {
         SystemError("Remote build server not responding: %s.\n",
                     RBS_CmdPath);
         Exit(1);
      }
      Init_RBS_Env(Host);
   }
   RBS_Write_Int(&RBS_Abort, Host, (int) 1);
   if (RBS_Abort)
      return;
   RBS_Write_Int(&RBS_Abort, Host, JobID);
   if (RBS_Abort)
      return;
   RBS_Write_Str(&RBS_Abort, Host, JobDirName);
   if (RBS_Abort)
      return;
   RBS_Write_Str(&RBS_Abort, Host, LogFileName);
   if (RBS_Abort)
      return;
   for (i = 0; ArgV[i] != NIL; i += 1) {
      RBS_Write_Int(&RBS_Abort, Host, (int) 2);
      if (RBS_Abort)
         return;
      RBS_Write_Str(&RBS_Abort, Host, ArgV[i]);
      if (RBS_Abort)
         return;
   };
   RBS_Write_Int(&RBS_Abort, Host, (int) 3);
   if (RBS_Abort)
      return;
}

void RBS_Abort_Build(tp_Host Host, int JobID)
{
   boolean RBS_Abort;

   RBS_Write_Int(&RBS_Abort, Host, (int) 4);
   if (RBS_Abort)
      return;
   RBS_Write_Int(&RBS_Abort, Host, JobID);
}

void RBS_VarDef(tp_Str VarDef)
{
   tp_Host Host;
   boolean RBS_Abort;

   for (Host = FirstHost; Host != NIL; Host = Host->Next) {
      if (Host->FD > 0) {
         RBS_Write_VarDef(&RBS_Abort, Host, VarDef);
      }
   }
}
