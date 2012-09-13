/*	Copyright (c) 1991 Geoffrey M. Clemm	*/
/*	geoff@boulder.colorado.edu		*/

#include <stdio.h>
#include <string.h>

#define	FALSE 0
#define	TRUE 1

int main(int argc, char **argv)
{
   char StrBuf[500], FileName[500], NewFileName[500];
   char *WDir, *Str, *EndStr;
   int i;

   if (argc < 2) {
      fprintf(stderr, "Working directory required\n");
      return;
   }

   WDir = argv[1];

   if (argc > 2) {
      InitDefine(argv[2]);
   }

   if (argc > 3) {
      if (argc > 4)
         fprintf(stderr, "Only four arguments expected\n");
      InitIgnore(argv[3]);
   }

   Str = gets(StrBuf);
   while (Str != 0) {
      if (Str[0] == '#') {
         i = 1;
         while (Str[i] == ' ')
            i += 1;
         {
            if (strncmp(&Str[i], "include", 7) == 0) {
               (void) strcpy(FileName, "");
               i += 7;
               while (Str[i] != '<' && Str[i] != '"' && Str[i] != 0)
                  i += 1;
               {
                  if (Str[i] == '<') {
                     i += 1;
                     if (Str[i] != '/') {
                        (void) strcpy(FileName, "/usr/include/");
                     }
                     EndStr = strrchr(Str, '>');
                     {
                        if (EndStr != 0) {
                           EndStr[0] = 0;
                           (void) strcat(FileName, &Str[i]);
                           ExpandFileName(NewFileName, FileName);
                           if (!IsIgnore(NewFileName))
                              puts(NewFileName);
                        } else {
                           fprintf(stderr, "Bad include : \"%s\"\n", Str);
                        }
                     }
                  } else if (Str[i] == '"') {
                     i += 1;
                     if (Str[i] != '/') {
                        (void) strcpy(FileName, WDir);
                        (void) strcat(FileName, "/");
                     }
                     EndStr = strrchr(Str, '"');
                     {
                        if (EndStr != 0) {
                           EndStr[0] = 0;
                           (void) strcat(FileName, &Str[i]);
                           ExpandFileName(NewFileName, FileName);
                           if (!IsIgnore(NewFileName))
                              puts(NewFileName);
                        } else {
                           fprintf(stderr, "Bad include : \"%s\"\n", Str);
                        }
                     }
                  } else {
                     fprintf(stderr, "Bad include : \"%s\"\n", Str);
                  }
               }
            } else if (strncmp(&Str[i], "ifndef", 6) == 0) {
               i += 6;
               while (Str[i] == ' ')
                  i += 1;
               if (IsDefine(&Str[i]))
                  SkipToEndIf();
            } else {
            }
         }
      }
      Str = gets(StrBuf);
   }
   exit(0);
};

int num_IgnoreS = 0;
char IgnoreS[10][200];

int InitIgnore(char *FileName)
{
   FILE *stream;
   char *Line, LineBuf[200];

   stream = fopen(FileName, "r");
   if (stream == NULL) {
      return;
   }
   Line = fgets(LineBuf, sizeof(LineBuf), stream);
   while (Line != NULL) {
      Line[strlen(Line) - 1] = 0;
      (void) strcpy(IgnoreS[num_IgnoreS], Line);
      num_IgnoreS += 1;
      Line = fgets(LineBuf, sizeof(LineBuf), stream);
   }
   (void) fclose(stream);
};

int IsIgnore(char *FileName)
{
   int i;

   for (i = 0; i < num_IgnoreS; i++) {
      if (strncmp(FileName, IgnoreS[i], strlen(IgnoreS[i])) == 0) {
         return TRUE;
      }
   }
   return FALSE;
};

int ExpandFileName(char *NewStr, char *OldStr)
{
   int iNew, iOld, iSeg;
   char SegStr[200];

   iNew = 0;
   iOld = 0;
   while (OldStr[iOld] != 0) {
      {
         if (OldStr[iOld] == '/') {
            NewStr[iNew] = '/';
            iOld += 1;
            iNew += 1;
         } else {
            iSeg = 0;
            while (OldStr[iOld] != '/' && OldStr[iOld] != 0) {
               SegStr[iSeg] = OldStr[iOld];
               iSeg += 1;
               iOld += 1;
            }
            SegStr[iSeg] = 0;
            {
               if (strcmp(SegStr, "..") == 0) {
                  if (NewStr[iNew - 1] != '/' || iNew <= 1) {
                     fprintf(stderr, "Bad filename : \"%s\"\n", OldStr);
                  }
                  iNew -= 2;
                  while (NewStr[iNew] != '/')
                     iNew -= 1;
               } else if (strcmp(SegStr, ".") == 0) {
                  iNew -= 1;
                  if (NewStr[iNew] != '/') {
                     fprintf(stderr, "Bad filename : \"%s\"\n", OldStr);
                  }
               } else {
                  (void) strcpy(&NewStr[iNew], SegStr);
                  iNew += strlen(SegStr);
               }
            }
         }
      }
   }
   NewStr[iNew] = 0;
};

int num_DefineS = 0;
char DefineS[20][200];

int InitDefine(char *FileName)
{
   FILE *stream;
   char *Line, LineBuf[200];

   stream = fopen(FileName, "r");
   if (stream == NULL) {
      return;
   }
   Line = fgets(LineBuf, sizeof(LineBuf), stream);
   while (Line != NULL) {
      Line[strlen(Line) - 1] = 0;
      (void) strcpy(DefineS[num_DefineS], Line);
      num_DefineS += 1;
      Line = fgets(LineBuf, sizeof(LineBuf), stream);
   }
   (void) fclose(stream);
};

int IsDefine(char *Macro)
{
   int i;

   for (i = 0; i < num_DefineS; i++) {
      if (strncmp(Macro, DefineS[i], strlen(DefineS[i])) == 0) {
         return TRUE;
      }
   }
   return FALSE;
};

int SkipToEndIf(void)
{
   char StrBuf[500];
   char *Str;
   int i;

   Str = gets(StrBuf);
   while (Str != 0) {
      if (Str[0] == '#') {
         i = 1;
         while (Str[i] == ' ')
            i += 1;
         if (strncmp(&Str[i], "endif", 5) == 0) {
            return;
         }
         if (strncmp(&Str[i], "if", 2) == 0) {
            SkipToEndIf();
         }
      }
      Str = gets(StrBuf);
   }
};
