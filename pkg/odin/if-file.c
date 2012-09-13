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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

#include "inc/GMC.h"
#include "inc/Str.h"
#include "inc/FileName.h"
#include "inc/SKind_.h"

static mode_t ModeMask = 0777;

void Set_ModeMask(tp_FileName FileName)
{
   int status;
   struct stat buf;

   FORBIDDEN(FileName == ERROR);
   status = stat(FileName, &buf);
   FORBIDDEN(status != 0);
   ModeMask = buf.st_mode;
   (void) umask(ModeMask ^ 0777);
}

void
Get_FileInfo(tp_SKind * SKindPtr, int *SysModTimePtr, tp_FileName FileName)
{
   struct stat buf;
   int status;

   FORBIDDEN(FileName == ERROR);

   status = lstat(FileName, &buf);
   if (status != 0) {
      *SKindPtr = SK_NoFile;
      *SysModTimePtr = 0;
      return;
   }
   *SysModTimePtr = buf.st_mtime;
   {
      if ((buf.st_mode & S_IFLNK) == S_IFLNK) {
         *SKindPtr = SK_SymLink;
         status = stat(FileName, &buf); /*give automounter a kick */
      } else if ((buf.st_mode & S_IFDIR) == S_IFDIR) {
         *SKindPtr = SK_Dir;
      } else if ((buf.st_mode & S_IEXEC) == S_IEXEC) {
         *SKindPtr = SK_Exec;
      } else if ((buf.st_mode & S_IFREG) == S_IFREG) {
         *SKindPtr = SK_Reg;
      } else {
         *SKindPtr = SK_Special;
      }
   }
}

void MakePlnFile(boolean * AbortPtr, tp_FileName FileName)
{
   int fd, status;

   *AbortPtr = TRUE;
   fd = open(FileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
   if (fd < 0) {
      return;
   }
   status = fchmod(fd, 0666 & ModeMask);
   if (status != 0) {
      (void) close(fd);
      return;
   }
   status = close(fd);
   if (status != 0) {
      (void) close(fd);
      return;
   }
   *AbortPtr = FALSE;
}

void MakeDirFile(boolean * AbortPtr, tp_FileName FileName)
{
   struct stat buf;
   int status;
   tps_FileName DirName;
   int i;

   FORBIDDEN(FileName == ERROR);

   *AbortPtr = FALSE;
   status = stat(FileName, &buf);
   if (status == 0) {
      FORBIDDEN((buf.st_mode & S_IFDIR) != S_IFDIR);
      return;
   }
   status = mkdir(FileName, (0777 & ModeMask));
   if (status != 0) {
      i = strlen(FileName) - 2;
      while (i > 0 && FileName[i] != '/')
         i -= 1;
      if (i < 1) {
         SystemError("\"%s\": cannot create.\n", FileName);
         *AbortPtr = TRUE;
         return;
      }
      (void) strcpy(DirName, FileName);
      DirName[i] = 0;
      MakeDirFile(AbortPtr, DirName);
      if (*AbortPtr) {
         return;
      }
      status = mkdir(FileName, (0777 & ModeMask));
      if (status != 0) {
         SystemError("\"%s\": cannot create.\n", FileName);
         *AbortPtr = TRUE;
         return;
      }
   }
}

char *getcwd();

void GetWorkingDir(boolean * AbortPtr, tp_Str DirName)
{
   char *WDstat;

   WDstat = getcwd(DirName, MAX_Str);
   *AbortPtr = (WDstat == 0);
}

#ifndef HAVE_GETCWD
char *getcwd(char *buf, int size)
{
   char *result, *getwd();

   result = getwd(buf);
   FORBIDDEN(result != 0 && strlen(result) >= size);
   return result;
}
#endif

void ChangeDir(boolean * AbortPtr, tp_FileName DirName)
{
   int status;

   status = chdir(DirName);
   *AbortPtr = (status != 0);
}

boolean IsExecutable(tp_FileName FileName)
{
   int status;
   struct stat buf;

   status = stat(FileName, &buf);
   return ((status == 0) && ((buf.st_mode & S_IEXEC) == S_IEXEC));
}

void MakeExecutable(tp_FileName FileName)
{
   int status;
   struct stat buf;
   int mode;

   FORBIDDEN(FileName == ERROR);
   status = stat(FileName, &buf);
   if (status == 0) {
      if ((buf.st_mode & S_IEXEC) != S_IEXEC) {
         mode = ((buf.st_mode | 0111) & ModeMask);
         status = chmod(FileName, mode);
      }
   }
   if (status != 0) {
      SystemError("\"%s\": cannot make executable.\n", FileName);
   }
}

void MakeReadOnly(boolean * AbortPtr, tp_FileName FileName)
{
   int status;
   struct stat buf;
   mode_t NewMode;

   FORBIDDEN(FileName == ERROR);
   status = stat(FileName, &buf);
   if (status != 0) {
      *AbortPtr = TRUE;
      return;
   }
   NewMode = ((buf.st_mode | 0444) & 0555 & ModeMask);
   if (NewMode == buf.st_mode) {
      *AbortPtr = FALSE;
      return;
   }
   status = chmod(FileName, NewMode);
   *AbortPtr = (status != 0);
}

void
SymLink(boolean * AbortPtr,
        tp_FileName ToFileName, tp_FileName FromFileName)
{
   int status;

   FORBIDDEN(ToFileName == ERROR || FromFileName == ERROR);
   *AbortPtr = FALSE;
   status = symlink(FromFileName, ToFileName);
   if (status != 0) {
      SysCallError(StdOutFD, "symlink");
      SystemError("\"%s\": Cannot make symbolic link to %s.\n", ToFileName,
                  FromFileName);
      *AbortPtr = TRUE;
   }
}

void
FileName_SymLinkFileName(tp_FileName SymLinkFileName, tp_FileName FileName)
{
   int cc;
   tps_Str buf;
   size_t sz;

   cc = readlink(FileName, buf, MAX_Str - 1);
   if (cc < 0) {
      perror("readlink");
      exit(1);
   }
   FORBIDDEN(cc == 0);
   buf[cc] = '\0';
   sz = snprintf(SymLinkFileName, MAX_FileName, "%s", buf);
   if (sz >= MAX_FileName) {
      (void) fprintf(stderr, "File name too long (MAX_FileName=%d): %s\n",
                     MAX_FileName, buf);
      exit(1);
   }
}

boolean IsDirectory_FileName(tp_FileName FileName)
{
   int status;
   struct stat buf;

   status = stat(FileName, &buf);
   return ((status == 0) && ((buf.st_mode & S_IFDIR) == S_IFDIR));
}

boolean Exists(tp_FileName FileName)
{
   int status;
   struct stat buf;

   FORBIDDEN(FileName == ERROR);
   status = stat(FileName, &buf);
   return (status == 0);
}

boolean Empty(tp_FileName FileName)
{
   int status;
   struct stat buf;

   FORBIDDEN(FileName == ERROR);
   status = stat(FileName, &buf);
   FORBIDDEN(status != 0);
   return (buf.st_size == 0);
}

void FileSize(boolean * AbortPtr, int *SizePtr, tp_FileName FileName)
{
   int status;
   struct stat buf;

   FORBIDDEN(FileName == ERROR);
   *AbortPtr = FALSE;
   *SizePtr = 0;
   status = stat(FileName, &buf);
   if (status != 0) {
      *AbortPtr = TRUE;
      return;
   }
   *SizePtr = buf.st_size;
}

void Remove(tp_FileName FileName)
{
   int status;

   FORBIDDEN(FileName == ERROR);
   status = unlink(FileName);
   if (status != 0)
      SystemError("\"%s\": rm failed.\n", FileName);
}

void RemoveDir(tp_FileName DirName)
{
   int status;
   tps_Str NFS_Hack;

   FORBIDDEN(DirName == ERROR);
   status = rmdir(DirName);
   if (status != 0) {
      (void) sprintf(NFS_Hack, "rm -f %s/.nfs*", DirName);
      status = system(NFS_Hack);
      status = rmdir(DirName);
      if (status != 0) {
         SystemError("\"%s\": rmdir failed.\n", DirName);
      }
   }
}

void
Rename(boolean * AbortPtr,
       tp_FileName OldFileName, tp_FileName NewFileName)
{
   int status;

   FORBIDDEN(OldFileName == ERROR || NewFileName == ERROR);
   status = rename(OldFileName, NewFileName);
   if (status != 0) {
      SystemError("\"%s\": bad status from rename to %s.\n", OldFileName,
                  NewFileName);
      if (Exists(OldFileName) || !Exists(NewFileName)) {
         *AbortPtr = TRUE;
         return;
      }
      SystemError("  (but it apparently worked).\n");
   }
   *AbortPtr = FALSE;
}
