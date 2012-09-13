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
#include <time.h>
#include <fcntl.h>
#include "inc/sys_param.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <signal.h>

#include "inc/GMC.h"
#include "inc/Str.h"

time_t time();
char *ctime();

#define		stdin_fd 0
#define		IPC_MT_Reply 1

boolean IsAny_ReadyServerAction = FALSE;

int *IPC_IArg1, *IPC_IArg2, *IPC_IArg3;
tp_Str IPC_SArg1, IPC_SArg2, IPC_SArg3;

boolean IsServer = FALSE;
boolean IsClient = FALSE;

boolean IPC_Do_Return = FALSE;
static int IPC_Nesting = 0;

void (*IPC_Action) (int *, char *);

static int ListenFD = -1;
static int ServerFD = -1;
static int ServerPId = 0;

boolean IsServerPId(int PId)
{
   return (ServerPId > 0 && ServerPId == PId);
}

void IPC_Init(void)
{
#ifdef HAVE_SIGACTION
   struct sigaction actbuf;
#endif
   int SocketFD, ServerPId, fd, i, Port, status, count;
   socklen_t len;
   struct sockaddr_in InSockAddr;
   struct sockaddr_un UnSockAddr;
   struct sockaddr *SockAddrPtr;
   struct hostent *HostEnt;
   tps_Str SocketFileName;
   boolean Abort;
   tp_FilDsc FilDsc;
   char LocalHostName[MAXHOSTNAMELEN], ServerHostName[MAXHOSTNAMELEN];
   time_t t;

#ifndef HAVE_SIGACTION
   (void) signal(SIGPIPE, SIG_IGN);
#else
   actbuf.sa_handler = SIG_IGN;
   (void) sigemptyset(&actbuf.sa_mask);
   actbuf.sa_flags = 0;
   status = sigaction(SIGPIPE, &actbuf, (struct sigaction *) NULL);
   FORBIDDEN(status != 0);
#endif

   SocketFD = -1;
   {
      if (LocalIPCFlag) {
         SockAddrPtr = (struct sockaddr *) &UnSockAddr;
         SocketFD = socket(AF_UNIX, SOCK_STREAM, 0);
         FORBIDDEN(SocketFD < 0);
         UnSockAddr.sun_family = AF_UNIX;
         Get_SocketFileName(UnSockAddr.sun_path);
         status = connect(SocketFD, SockAddrPtr, sizeof(UnSockAddr));
         {
            if (status == 0) {
               ServerFD = SocketFD;
               SocketFD = -1;
            } else {
               if (GetEnv("ODIN_SERVER_SHUTDOWN") != NIL) {
                  SystemError("No odin server to shut down.\n");
                  exit(1);
               }
               status = bind(SocketFD, SockAddrPtr, sizeof(UnSockAddr));
               if (status != 0) {
                  if (GetEnv("ODIN_SERVER_SHUTDOWN") == NIL) {
                     SystemError("Cannot initiate Odin server.\n");
                     SystemError("Reset cache with the -r option.\n");
                  }
                  exit(1);
               }
            }
         }
      } else {
         SockAddrPtr = (struct sockaddr *) &InSockAddr;
         for (i = 0; i < sizeof(InSockAddr); i += 1) {
            ((char *) &InSockAddr)[i] = '\0';
         }
         InSockAddr.sin_family = AF_INET;
#ifdef INADDR_ANY
         InSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
         InSockAddr.sin_port = 0;
#endif
         Get_SocketFileName(SocketFileName);
         FilDsc = FileName_CFilDsc(SocketFileName);
         if (FilDsc != ERROR) {
            if (GetEnv("ODIN_SERVER_SHUTDOWN") != NIL) {
               SystemError("No odin server to shut down.\n");
               exit(1);
            }
            SocketFD = socket(AF_INET, SOCK_STREAM, 0);
            FORBIDDEN(SocketFD < 0);
            status = bind(SocketFD, SockAddrPtr, sizeof(InSockAddr));
            FORBIDDEN(status != 0);
            len = sizeof(InSockAddr);
            status = getsockname(SocketFD, SockAddrPtr, &len);
            FORBIDDEN(status != 0);
            status = gethostname(LocalHostName, MAXHOSTNAMELEN);
            FORBIDDEN(status != 0);
            HostEnt = gethostbyname(LocalHostName);
            if (HostEnt == NIL) {
               SystemError("Remote IPC not available on this host.\n");
               SystemError
                   ("Set $ODIN_LOCALIPC to 1 and use the -R option.\n");
               exit(1);
            }
            status = fprintf((FILE *) FilDsc, "%s %d\n",
                             LocalHostName, ntohs(InSockAddr.sin_port));
            Close(FilDsc);
         }
      }
   }

   if (SocketFD >= 0) {
      status = listen(SocketFD, 7);     /* because 7 is a lucky number */
      FORBIDDEN(status != 0);
      ListenFD = SocketFD;
      LocalClient = New_Client(0);
      CurrentClient = LocalClient;
      if (GetEnv("ODIN_SERVER") == NIL) {
         IsServer = TRUE;
         IsClient = TRUE;
         return;
      }
      ServerPId = fork();
      if (ServerPId < 0) {
         SysCallError(StdErrFD, "fork");
         SystemError("Could not fork Odin server.\n");
         exit(1);
      }
      if (ServerPId == 0) {
         IsServer = TRUE;
         ChangeDir(&Abort, OdinDirName);
         if (Abort) {
            SystemError("Cannot access Odin cache directory: %s.\n");
            exit(1);
         }
         (void) close(0);
         fd = open("LOG", O_WRONLY | O_CREAT | O_APPEND, 0666);
         if (fd < 0) {
            SysCallError(StdOutFD, "open");
            SystemError("Cannot open Odin LOG file.\n");
            exit(1);
         }
         status = dup2(fd, 1);
         FORBIDDEN(status != 1);
         fd = open("ERR", O_WRONLY | O_CREAT | O_APPEND, 0666);
         if (fd < 0) {
            SysCallError(StdOutFD, "open");
            SystemError("Cannot open Odin ERR file.\n");
            exit(1);
         }
         status = dup2(fd, 2);
         FORBIDDEN(status != 2);
         Set_IPC_Err(TRUE);
         Lose_ControlTTY();
         (void) time(&t);
         (void) printf("Odin server started on %s", ctime(&t));
         return;
      }
      (void) close(SocketFD);
   }

   IsClient = TRUE;
   if (ServerFD < 0) {
      {
         if (LocalIPCFlag) {
            SocketFD = socket(AF_UNIX, SOCK_STREAM, 0);
            FORBIDDEN(SocketFD < 0);
            status = connect(SocketFD, SockAddrPtr, sizeof(UnSockAddr));
         } else {
            FilDsc = FileName_RFilDsc(SocketFileName, FALSE);
            if (FilDsc == ERROR) {
               SystemError("Cache at %s is read-only.\n", OdinDirName);
               exit(1);
            }
            count =
                fscanf((FILE *) FilDsc, "%s %d\n", ServerHostName, &Port);
            if (count != 2 || Port <= 0) {
               if (GetEnv("ODIN_SERVER_SHUTDOWN") == NIL) {
                  SystemError("Cannot find Odin server address.\n");
                  SystemError("Reset cache with the -r option.\n");
               }
               exit(1);
            }
            Close(FilDsc);
            HostEnt = gethostbyname(ServerHostName);
            for (i = 0; i < HostEnt->h_length; i += 1) {
               ((char *) &InSockAddr.sin_addr)[i] = HostEnt->h_addr[i];
            }
            InSockAddr.sin_port = htons(Port);

            SocketFD = socket(AF_INET, SOCK_STREAM, 0);
            FORBIDDEN(SocketFD < 0);
            status = connect(SocketFD, SockAddrPtr, sizeof(InSockAddr));
         }
      }
      if (status != 0) {
         if (GetEnv("ODIN_SERVER_SHUTDOWN") == NIL) {
            SystemError("Cannot connect to Odin server.\n");
            SystemError("Reset cache with the -r option.\n");
         }
         exit(1);
      }
      ServerFD = SocketFD;
   }
   if (GetEnv("ODIN_SERVER_SHUTDOWN") != NIL) {
      SystemError("Shutting down Odin server.");
      ShutDown();
      IPC_Get_Commands(&Abort, (tp_Str) NIL);
      exit(0);
   }
}

int IPC_Read(int fd, char *buf, int len)
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

static void
IPC_Read_Line(boolean * EndFlagPtr,
              tp_Str * StrPtr,
              tps_Str StrBuf, int *OffsetPtr, int *LengthPtr)
{
   int count, i, j;

   FORBIDDEN(*OffsetPtr != 0);
   *EndFlagPtr = FALSE;
   *StrPtr = NIL;
   count = read(stdin_fd, &StrBuf[*LengthPtr], MAX_Str - (*LengthPtr));
   FORBIDDEN(count < 0);
   if (count <= 0) {
      *EndFlagPtr = TRUE;
      return;
   }
   i = *LengthPtr;
   *LengthPtr += count;
   while (i < *LengthPtr) {
      if (StrBuf[i] == '\r') {
         if ((i + 1) < *LengthPtr && StrBuf[i + 1] == '\n') {
            for (j = i; j < *LengthPtr; j += 1) {
               StrBuf[j - 1] = StrBuf[j];
            }
            *LengthPtr -= 1;
         }
      }
      if (StrBuf[i] == '\n') {
         if (i == 0 || StrBuf[i - 1] != '\\') {
            StrBuf[i] = 0;
            *OffsetPtr = i + 1;
            *StrPtr = &StrBuf[0];
            return;
         }
         for (j = i; j < *LengthPtr; j += 1) {
            StrBuf[j - 1] = StrBuf[j];
         }
         *LengthPtr -= 1;
      }
      i += 1;
   }
   if (i == MAX_Str) {
      StrBuf[MAX_Str - 1] = 0;
      SystemError("\nInput line too long: <%s>.\n", StrBuf);
      Exit(1);
   }
   return;
}

static tp_Str IPC_Find_Line(tps_Str StrBuf, int *OffsetPtr, int *LengthPtr)
{
   int i, j;
   tp_Str Str;

   FORBIDDEN(*OffsetPtr > *LengthPtr);
   if (*OffsetPtr == *LengthPtr) {
      *OffsetPtr = 0;
      *LengthPtr = 0;
      return NIL;
   }
   for (i = *OffsetPtr; i < *LengthPtr; i++) {
      if (StrBuf[i] == '\n') {
         if (i == *OffsetPtr || StrBuf[i - 1] != '\\') {
            StrBuf[i] = 0;
            Str = &StrBuf[*OffsetPtr];
            *OffsetPtr = i + 1;
            return Str;
         }
         for (j = i; j < *LengthPtr; j += 1) {
            StrBuf[j - 1] = StrBuf[j];
         }
         *LengthPtr -= 1;
      }
   }
   for (i = *OffsetPtr, j = 0; i < *LengthPtr; i++, j++) {
      StrBuf[j] = StrBuf[i];
   }
   *OffsetPtr = 0;
   *LengthPtr = j;
   return NIL;
}

static tp_Str EditLine(tp_Str StrBuf, tp_Str Prompt)
{
   tp_Str Str;
   int Length;

   Str = readline(Prompt);
   if (Str == NIL) {
      return Str;
   }
   (void) strcpy(StrBuf, Str);
   (void) free(Str);
   Length = strlen(StrBuf);
   while (Length > 0 && StrBuf[Length - 1] == '\\') {
      StrBuf[Length - 1] = '\n';
      Str = readline("");
      if (Str == NIL) {
         return Str;
      }
      (void) strcat(StrBuf, Str);
      (void) free(Str);
      Length = strlen(StrBuf);
   }
   add_history(StrBuf);
   return StrBuf;
}

static void IPC_Get_Msg(boolean * AbortPtr, int FD)
{
   int cc;
   int MsgType;

   cc = IPC_Read(FD, (char *) &MsgType, sizeof(MsgType));
   if (cc <= 0) {
      FORBIDDEN(cc < 0);
      *AbortPtr = TRUE;
      return;
   }
   IPC_Do_Msg(AbortPtr, MsgType);
}

void IPC_Get_Commands(boolean * AbortPtr, char *Prompt)
{
   boolean WasPrompted, Abort, Done, EndFlag;
   fd_set _readfds, *readfds = &_readfds;
   int nfds;
   tp_Client Client, OldCurrentClient;
   tp_Host Host;
   tps_Str StrBuf;
   int Offset = 0, Length = 0, status;
   tp_Str Str = NIL;
   struct sockaddr Addr;
   socklen_t AddrLen;
   int FD;

   *AbortPtr = FALSE;
   FORBIDDEN(Prompt != NIL && IPC_Nesting > 0);
   FORBIDDEN(IPC_Nesting > 1);
   IPC_Nesting += 1;

   if (IsServer) {
      FORBIDDEN(!Is_LocalClient(CurrentClient));
      FORBIDDEN(IPC_Do_Return);
      ServerAction();
      if (IPC_Do_Return) {
         FORBIDDEN(Prompt != NIL);
         IPC_Nesting -= 1;
         return;
      }
   }

   WasPrompted = FALSE;

   while (TRUE) {
      FD_ZERO(readfds);

      if (IsClient) {
         if (IsTTY && Prompt != NIL && !WasPrompted && !Is_ServerAction()) {
            Flush(StdOutFD);
            {
               if (History && !IsServer) {
                  Str = EditLine(StrBuf, Prompt);
                  if (Str == NIL) {
                     IPC_Nesting -= 1;
                     return;
                  }
               } else {
                  status = write(1, Prompt, strlen(Prompt));
                  if (status == -1) {
                     SysCallError(StdOutFD, "write(IPC_Get_Commands)");
                  }
               }
            }
            WasPrompted = TRUE;
         }
         if (Prompt != NIL && Str == NIL) {
            FD_SET(stdin_fd, readfds);
         }
         if (ServerFD > 0) {
            FD_SET(ServerFD, readfds);
         }
         for (Host = FirstHost; Host != NIL; Host = Host_Next(Host)) {
            if (Host_FD(Host) > 0) {
               FD_SET(Host_FD(Host), readfds);
            }
         }
      }

      if (IsServer) {
         FD_SET(ListenFD, readfds);
         FOREACH_CLIENT(Client) {
            if (!Is_LocalClient(Client)) {
               FD_SET(Client_FD(Client), readfds);
            }
         }
      }

      Unblock_Signals();
      nfds = Await_Event(readfds, (Str != NIL || IsAny_ReadyServerAction));
      Block_Signals();
      if (nfds < 0) {
         if (!Signalled && !SigChild)
            SysCallError(StdOutFD, "select");
         FD_ZERO(readfds);
      }
      if (Signalled) {
         if (!IsTTY) {
            Exit(1);
         }
         if (!Handled) {
            Handled = TRUE;
            InterruptAction();
            Offset = 0;
            Length = 0;
            Str = NIL;
         }
         if (Prompt != NIL) {
            Handled = FALSE;
            Signalled = FALSE;
         }
      }

      if (SigChild) {
         SigChild = FALSE;
         ChildAction(AbortPtr, &Done);
         if (Done) {
            IPC_Nesting -= 1;
            return;
         }
      }

      if (IsClient) {
         if (Str == NIL && FD_ISSET(stdin_fd, readfds)) {
            IPC_Read_Line(&EndFlag, &Str, StrBuf, &Offset, &Length);
            if (EndFlag) {
               IPC_Nesting -= 1;
               return;
            }
         }
         if (Str != NIL && !Is_ServerAction()) {
            IPC_Action(AbortPtr, Str);
            if (*AbortPtr && !IsTTY) {
               IPC_Nesting -= 1;
               return;
            }
            WasPrompted = FALSE;
            Str = IPC_Find_Line(StrBuf, &Offset, &Length);
         }

         for (Host = FirstHost; Host != NIL; Host = Host_Next(Host)) {
            if (Host_FD(Host) > 0 && FD_ISSET(Host_FD(Host), readfds)) {
               RBS_Get_Msg(Host);
            }
         }

         if (ServerFD > 0) {
            FORBIDDEN(IsServer);
            if (FD_ISSET(ServerFD, readfds)) {
               IPC_Get_Msg(&Abort, ServerFD);
               if (Abort) {
                  IPC_Do_Abort();
               }
            }
         }
      }

      if (IsServer) {
         if (FD_ISSET(ListenFD, readfds)) {
            AddrLen = sizeof(Addr);
            FD = accept(ListenFD, &Addr, &AddrLen);
            {
               if (FD >= 0) {
                  Client = New_Client(FD);
                  Activate_Client(Client);
               } else {
                  SysCallError(StdOutFD, "accept");
               }
            }
         }

         FOREACH_CLIENT(Client) {
            if (!Is_LocalClient(Client)
                && FD_ISSET(Client_FD(Client), readfds)) {
               OldCurrentClient = CurrentClient;
               CurrentClient = Client;
               IPC_Get_Msg(&Abort, Client_FD(Client));
               if (Abort) {
                  Ret_Client(Client);
               }
               CurrentClient = OldCurrentClient;
            }
         }
         Purge_Clients();

         if (IsAny_ReadyServerAction) {
            IsAny_ReadyServerAction = FALSE;
            FOREACH_CLIENT(Client) {
               OldCurrentClient = CurrentClient;
               CurrentClient = Client;
               ServerAction();
               CurrentClient = OldCurrentClient;
            }
         }

         if (!IsAny_ServerAction()) {
            CleanUp();
         }
      }

      if (IPC_Do_Return) {
         FORBIDDEN(Prompt != NIL);
         IPC_Nesting -= 1;
         return;
      }
   }
}

void IPC_Write_Int(boolean * AbortPtr, int Int)
{
   int fd, cc;

   fd = ServerFD;

   if (IsServer) {
      if (!Is_ActiveClient(CurrentClient)) {
         *AbortPtr = TRUE;
         return;
      }
      FORBIDDEN(Is_LocalClient(CurrentClient));
      fd = Client_FD(CurrentClient);
   }
   cc = write(fd, (char *) &Int, sizeof(Int));
   *AbortPtr = (cc != sizeof(Int));
}

void IPC_Read_Int(boolean * AbortPtr, int *IntPtr)
{
   int fd, cc;

   fd = ServerFD;

   if (IsServer) {
      if (!Is_ActiveClient(CurrentClient)) {
         *AbortPtr = TRUE;
         return;
      }
      FORBIDDEN(Is_LocalClient(CurrentClient));
      fd = Client_FD(CurrentClient);
   }

   cc = IPC_Read(fd, (char *) IntPtr, sizeof(*IntPtr));
   *AbortPtr = (cc != sizeof(*IntPtr));
}

void IPC_Write_Str(boolean * AbortPtr, const char *Str)
{
   int fd, cc, len;

   fd = ServerFD;

   if (IsServer) {
      if (!Is_ActiveClient(CurrentClient)) {
         *AbortPtr = TRUE;
         return;
      }
      FORBIDDEN(Is_LocalClient(CurrentClient));
      fd = Client_FD(CurrentClient);
   }

   len = strlen(Str);
   IPC_Write_Int(AbortPtr, len);
   if (*AbortPtr)
      return;
   if (len > 0) {
      cc = write(fd, Str, len);
      *AbortPtr = (cc != len);
   }
}

void IPC_Read_Str(boolean * AbortPtr, char *Str)
{
   int fd, cc, len;

   fd = ServerFD;

   if (IsServer) {
      if (!Is_ActiveClient(CurrentClient)) {
         *AbortPtr = TRUE;
         return;
      }
      FORBIDDEN(Is_LocalClient(CurrentClient));
      fd = Client_FD(CurrentClient);
   }

   IPC_Read_Int(AbortPtr, &len);
   if (*AbortPtr)
      return;
   if (len > 0) {
      cc = IPC_Read(fd, Str, len);
      *AbortPtr = (cc != len);
   }
   Str[len] = 0;
}

void IPC_Do_Abort(void)
{
   if (!IsServer) {
      DeadServerExit();
   }

   if (Is_ActiveClient(CurrentClient)) {
      Ret_Client(CurrentClient);
   }
}

void IPC_Close(tp_ClientID ClientID)
{
   int status;

   status = close(ClientID);
   if (status == EOF)
      SysCallError(StdOutFD, "close(IPC_Close)");
}

void IPC_Finish(void)
{
   tps_Str SocketFileName;
   int status;

   if (!IsServer) {
      FORBIDDEN(ServerFD == 0);
      status = close(ServerFD);
      if (status == EOF)
         SysCallError(StdOutFD, "close(IPC_Finish)");
   };

   if (IsServer) {
      status = close(ListenFD);
      if (status == -1)
         SysCallError(StdOutFD, "close(IPC_Finish)");
      Get_SocketFileName(SocketFileName);
      status = unlink(SocketFileName);
      if (status == -1)
         SysCallError(StdOutFD, "unlink(IPC_Finish)");
   };
}
