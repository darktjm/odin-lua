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
#include <fcntl.h>
#include "inc/sys_param.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>

#include "inc/FileName.h"
#include "inc/Build.h"
#include "inc/Str.h"

extern char **environ;

tp_Str Author = "odin-build-users@lists.sourceforge.net";

int SocketFD;

tp_Build FirstBuild = NIL;

#define		MAX_BUILDARGV 100
int Num_BuildArgV = 0;
char *BuildArgV[MAX_BUILDARGV];

void Local_Add_BuildArg(tp_FileName Arg)
{
   FORBIDDEN((Num_BuildArgV + 2) > MAX_BUILDARGV);
   if (BuildArgV[Num_BuildArgV] != NIL)
      free(BuildArgV[Num_BuildArgV]);
   BuildArgV[Num_BuildArgV] = Malloc_Str(Arg);
   Num_BuildArgV += 1;
}

static tp_Build BuildID_Build(tp_BuildID BuildID)
{
   tp_Build Build;

   for (Build = FirstBuild; Build != NIL; Build = Build->Next) {
      if (BuildID == Build->BuildID) {
         return Build;
      }
   }
   return NIL;
}

tp_Build JobID_Build(tp_JobID JobID)
{
   tp_Build Build;

   for (Build = FirstBuild; Build != NIL; Build = Build->Next) {
      if (JobID == Build->JobID) {
         return Build;
      }
   }
   return NIL;
}

static tp_Build New_Build(void)
{
   tp_Build Build;

   Build = JobID_Build(0);
   if (Build == NIL) {
      Build = (tp_Build) malloc(sizeof(tps_Build));
      Build->JobID = 0;
      Build->BuildID = 0;
      Build->Next = FirstBuild;
      FirstBuild = Build;
   }
   return Build;
}

static int RBS_Read(int fd, char *buf, int len)
{
   int n, i;

   for (i = 0; i < len; i += n) {
      n = read(fd, &buf[i], len - i);
      if (n <= 0) {
         return i;
      }
   }
   return i;
}

static void RBS_Write_Int(int Int)
{
   int cc;

   cc = write(SocketFD, (char *) &Int, sizeof(Int));
   if (cc != sizeof(Int))
      Exit(1);
}

static void RBS_Read_Int(int *IntPtr)
{
   int cc;

   cc = RBS_Read(SocketFD, (char *) IntPtr, sizeof(*IntPtr));
   if (cc != sizeof(*IntPtr))
      Exit(1);
}

static void RBS_Write_Str(const char *Str)
{
   int cc, len;

   len = strlen(Str);
   RBS_Write_Int(len);
   if (len > 0) {
      cc = write(SocketFD, Str, len);
      if (cc != len)
         Exit(1);
   };
}

static void RBS_Read_Str(char *Str)
{
   int cc, len;

   RBS_Read_Int(&len);
   if (len > 0) {
      cc = RBS_Read(SocketFD, Str, len);
      if (cc != len)
         Exit(1);
   };
   Str[len] = 0;
}

static void RBS_Read_VarDef(void)
{
   tps_Str StrBuf;
   int status;

   RBS_Read_Str(StrBuf);
   status = putenv(Malloc_Str(StrBuf));
   FORBIDDEN(status != 0);
}

static void Set_ODINRBSHOST(void)
{
   int status;
   char LocalHostName[MAXHOSTNAMELEN];
   tps_Str EnvStr;

   status = gethostname(LocalHostName, MAXHOSTNAMELEN);
   FORBIDDEN(status != 0);
   (void) sprintf(EnvStr, "ODINRBSHOST=%s: ", LocalHostName);
   (void) putenv(strdup(EnvStr));
}

int main(int argc, char **argv)
{
#ifndef BSD_SIGNALS
   struct sigaction actbuf;
#endif
   tp_Str HostName, PortStr;
   struct sockaddr_in InSockAddr;
   struct sockaddr *SockAddrPtr = (struct sockaddr *) &InSockAddr;
   struct hostent *HostEnt;
   int i, status, MsgType, BuildID, JobID;
   boolean Abort;
   fd_set _readfds, *readfds = &_readfds;
   int fd, nfds;
   tps_FileName JobDirName, LogFileName, Arg;
   tp_Build Build;

   FORBIDDEN(argc != 3);
   Init_IO();
   Init_Err();
   Init_Sigs(FALSE);

   Unblock_Signals();
   HostName = argv[1];
   PortStr = argv[2];
#ifdef BSD_SIGNALS
   (void) signal(SIGPIPE, SIG_IGN);
#else
   actbuf.sa_handler = SIG_IGN;
   status = sigemptyset(&actbuf.sa_mask);
   FORBIDDEN(status != 0);
   actbuf.sa_flags = 0;
   status = sigaction(SIGPIPE, &actbuf, (struct sigaction *) NULL);
   FORBIDDEN(status != 0);
#endif
   SocketFD = socket(AF_INET, SOCK_STREAM, 0);
   FORBIDDEN(SocketFD < 0);
   for (i = 0; i < sizeof(InSockAddr); i += 1) {
      ((char *) &InSockAddr)[i] = '\0';
   }
   InSockAddr.sin_family = AF_INET;
   HostEnt = gethostbyname(HostName);
   for (i = 0; i < HostEnt->h_length; i += 1) {
      ((char *) &InSockAddr.sin_addr)[i] = HostEnt->h_addr[i];
   }
   InSockAddr.sin_port = atoi(PortStr);

   Lose_ControlTTY();
   fd = open("/dev/null", O_RDONLY);
   status = dup2(fd, 0);
   FORBIDDEN(status != 0);

   Set_ODINRBSHOST();

   status = connect(SocketFD, SockAddrPtr, sizeof(InSockAddr));
   FORBIDDEN(status != 0);
   while (TRUE) {
      FD_SET(SocketFD, readfds);
      nfds = Await_Event(readfds, FALSE);
      if (nfds < 0)
         FD_ZERO(readfds);
      if (SigChild) {
         SigChild = FALSE;
         for (SystemWait(&BuildID, &Abort);
              BuildID != 0; SystemWait(&BuildID, &Abort)) {
            Build = BuildID_Build(BuildID);
            FORBIDDEN(Build == NIL);
            RBS_Write_Int(Build->JobID);
            RBS_Write_Int(Abort);
            Build->BuildID = 0;
            Build->JobID = 0;
         }
      }
      if (FD_ISSET(SocketFD, readfds)) {
         RBS_Read_Int(&MsgType);
         switch (MsgType) {
         case 1:{
               RBS_Read_Int(&JobID);
               RBS_Read_Str(JobDirName);
               RBS_Read_Str(LogFileName);
               break;
            }
         case 2:{
               RBS_Read_Str(Arg);
               Local_Add_BuildArg(Arg);
               break;
            }
         case 3:{
               if (BuildArgV[Num_BuildArgV] != NIL) {
                  free(BuildArgV[Num_BuildArgV]);
               }
               BuildArgV[Num_BuildArgV] = 0;
               Num_BuildArgV = 0;
               Build = New_Build();
               Build->JobID = JobID;
               ChangeDir(&Abort, JobDirName);
               if (Abort) {
                  SystemError("Cannot find %s on remote machine.\n",
                              JobDirName);
                  SystemError("Trying agin.\n");
                  sleep(5);
                  ChangeDir(&Abort, JobDirName);
                  if (Abort) {
                     SystemError("Giving up.\n");
                     Exit(1);
                  }
               }
               ClearDir(JobDirName);
               Build->BuildID =
                   SystemExec(BuildArgV[0], BuildArgV, LogFileName);
	       if(Build->BuildID <= 0) {
		  RBS_Write_Int(JobID);
		  RBS_Write_Int(1);
		  Build->BuildID = Build->JobID = 0;
	       }
               break;
            }
         case 4:{
               RBS_Read_Int(&JobID);
               Build = JobID_Build(JobID);
               if (Build != NIL) {
                  SystemInterrupt(Build->BuildID);
               }
               break;
            }
         case 5:{
               RBS_Read_VarDef();
               break;
            }
         default:{
               FATALERROR("unexpected message type.\n");
            }
         }
      }
   }
}

void Exit(int Status)
{
   tp_Build Build;

   for (Build = FirstBuild; Build != NIL; Build = Build->Next) {
      if (Build->BuildID != 0) {
         SystemInterrupt(Build->BuildID);
      }
   }
   exit(Status);
}
