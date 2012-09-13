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

#include "inc/GMC.h"

#ifndef HAVE_PUTENV
int putenv(char *str)
{
   extern char *strdup();
   char *strbuf, *equal;
   int status;

   status = 1;
   strbuf = strdup(str);
   if (strbuf == NULL) {
      return status;
   }
   equal = index(strbuf, '=');
   if (equal != NULL) {
      *equal = '\0';
      status = setenv(strbuf, equal + 1, 1);
   }
   (void) free(strbuf);
   return status;
}
#endif

#ifdef NO_STRCASECMP
#include <ctype.h>
#define ToLower(ch) (isupper(ch)?tolower(ch):(ch))
int strcasecmp(char *str1, char *str2)
{
   while (ToLower(*str1) == ToLower(*str2)) {
      if (*str1 == 0)
         return 0;
      str1 += 1;
      str2 += 1;
   }
   if (*str1 < *str2)
      return -1;
   return 1;
}
#endif
