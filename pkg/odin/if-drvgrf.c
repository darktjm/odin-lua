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
#include "inc/CastEdg.h"
#include "inc/DG_Version.h"
#include "inc/DrvEdg.h"
#include "inc/EnvVar.h"
#include "inc/EnvVarLst.h"
#include "inc/EqvEdg.h"
#include "inc/FileName.h"
#include "inc/FilTyp.h"
#include "inc/InpEdg.h"
#include "inc/InpSpc.h"
#include "inc/MemEdg.h"
#include "inc/PrmTyp.h"
#include "inc/PrmTypLst.h"
#include "inc/SrcTyp.h"
#include "inc/Str.h"
#include "inc/Tool.h"
#include "inc/Version.h"

extern char *DG_Version;
extern int num_SrcTypS;
extern tps_SrcTyp _SrcTypS[];
extern int num_FilTypS;
extern tps_FilTyp _FilTypS[];
extern int num_PrmTypS;
extern tps_PrmTyp _PrmTypS[];
extern int num_PrmTypLstS;
extern tps_PrmTypLst _PrmTypLstS[];
extern int num_EnvVarS;
extern tps_EnvVar _EnvVarS[];
extern int num_EnvVarLstS;
extern tps_EnvVarLst _EnvVarLstS[];
extern int num_ToolS;
extern tps_Tool _ToolS[];
extern int num_InpEdgS;
extern tps_InpEdg _InpEdgS[];
extern int num_MemEdgS;
extern tps_MemEdg _MemEdgS[];
extern int num_EqvEdgS;
extern tps_EqvEdg _EqvEdgS[];
extern int num_CastEdgS;
extern tps_CastEdg _CastEdgS[];
extern int num_DrvEdgS;
extern tps_DrvEdg _DrvEdgS[];
extern int num_InpSpcS;
extern tps_InpSpc _InpSpcS[];

tp_SrcTyp SrcTypS = _SrcTypS;
tp_FilTyp FilTypS = _FilTypS;
tp_PrmTyp PrmTypS = _PrmTypS;
tp_EnvVar EnvVarS = _EnvVarS;
static tp_Tool ToolS = _ToolS;
static tp_InpEdg InpEdgS = _InpEdgS;
static tp_MemEdg MemEdgS = _MemEdgS;
static tp_EqvEdg EqvEdgS = _EqvEdgS;
static tp_CastEdg CastEdgS = _CastEdgS;
static tp_DrvEdg DrvEdgS = _DrvEdgS;
static tp_PrmTypLst PrmTypLstS = _PrmTypLstS;
static tp_EnvVarLst EnvVarLstS = _EnvVarLstS;
static tp_InpSpc InpSpcS = _InpSpcS;

static tp_SrcTyp I_SrcTyp(int i)
{
   if (i < 0 || i >= num_SrcTypS)
      return 0;
   return &SrcTypS[i];
}

tp_FilTyp IFilTyp_FilTyp(int i)
{
   return &FilTypS[i];
}

static tp_FilTyp I_FilTyp(int i)
{
   if (i < 0 || i >= num_FilTypS)
      return 0;
   return &FilTypS[i];
}

tp_PrmTyp IPrmTyp_PrmTyp(int i)
{
   return &PrmTypS[i];
}

tp_PrmTyp I_PrmTyp(int i)
{
   if (i < 0 || i >= num_PrmTypS)
      return 0;
   return &PrmTypS[i];
}

static tp_EnvVar I_EnvVar(int i)
{
   if (i < 0 || i >= num_EnvVarS)
      return 0;
   return &EnvVarS[i];
}

static tp_Tool I_Tool(int i)
{
   if (i == -1)
      return 0;
   return &ToolS[i];
}

static tp_InpEdg I_InpEdg(int i)
{
   if (i == -1)
      return 0;
   return &InpEdgS[i];
}

static tp_MemEdg I_MemEdg(int i)
{
   if (i == -1)
      return 0;
   return &MemEdgS[i];
}

static tp_EqvEdg I_EqvEdg(int i)
{
   if (i == -1)
      return 0;
   return &EqvEdgS[i];
}

static tp_CastEdg I_CastEdg(int i)
{
   if (i == -1)
      return 0;
   return &CastEdgS[i];
}

static tp_DrvEdg I_DrvEdg(int i)
{
   if (i == -1)
      return 0;
   return &DrvEdgS[i];
}

static tp_PrmTypLst I_PrmTypLst(int i)
{
   if (i == -1)
      return 0;
   return &PrmTypLstS[i];
}

static tp_EnvVarLst I_EnvVarLst(int i)
{
   if (i == -1)
      return 0;
   return &EnvVarLstS[i];
}

static tp_InpSpc I_InpSpc(int i)
{
   if (i == -1)
      return 0;
   return &InpSpcS[i];
}

static void Read_FilTyps(FILE * DrvGrfFILE)
{
   int i, count;
   int iFilTyp, iTool;
   int iMemEdg, iEqvEdg, iCastEdg, iDrvEdg;
   int iPrmTypLst;
   tp_SrcTyp SrcTyp;
   tp_FilTyp FilTyp;
   tps_Str Pattern, FTNameStr, DescStr;

   for (i = 0; i < num_SrcTypS; i++) {
      SrcTyp = I_SrcTyp(i);
      count = fscanf(DrvGrfFILE, "%[^\1]\1 %d %d\n",
                     Pattern, &SrcTyp->IsPrefix, &iFilTyp);
      FORBIDDEN(count != 3);
      SrcTyp->Pattern = Malloc_Str(&Pattern[1]);
      SrcTyp->FilTyp = I_FilTyp(iFilTyp);
   }

   for (i = 0; i < num_FilTypS; i++) {
      FilTyp = I_FilTyp(i);
      FilTyp->IFilTyp = i;
      count = fscanf(DrvGrfFILE,
                     "%d %[^\1]\1 %[^\1]\1 %d %d %d %d %d %d %d %d %d %d %d\n",
                     &FilTyp->FTClass, FTNameStr, DescStr,
                     &FilTyp->HelpLevel, &iFilTyp, &iTool, &iMemEdg,
                     &iEqvEdg, &iCastEdg, &iDrvEdg, &iPrmTypLst,
                     &FilTyp->IsCopy, &FilTyp->IsGrouping,
                     &FilTyp->IsGroupingInput);
      FORBIDDEN(count != 14);
      FilTyp->FTName = Malloc_Str(&FTNameStr[1]);
      FilTyp->Desc = Malloc_Str(&DescStr[1]);
      FilTyp->ArgFilTyp = I_FilTyp(iFilTyp);
      FilTyp->Tool = I_Tool(iTool);
      FilTyp->MemEdg = I_MemEdg(iMemEdg);
      FilTyp->EqvEdg = I_EqvEdg(iEqvEdg);
      FilTyp->CastEdg = I_CastEdg(iCastEdg);
      FilTyp->DrvEdg = I_DrvEdg(iDrvEdg);
      FilTyp->MapPrmTypLst = I_PrmTypLst(iPrmTypLst);
      FilTyp->Reach = FALSE;
      FilTyp->Mark = FALSE;
      FilTyp->Pos = NIL;
   }

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   GenericFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   PipeFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   ListFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   FileFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   ObjectFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   NoInputFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   ExpandHooksFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   TargetsPtrFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   TargetsFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   ActTargetsFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   VirTargetsFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   VirDirFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   CopyFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   PrmValuesFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   ViewSpecFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   FirstFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   ApplyFilTyp = I_FilTyp(iFilTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iFilTyp);
   FORBIDDEN(count != 1);
   FatherFilTyp = I_FilTyp(iFilTyp);
}

static void Read_PrmTyps(FILE * DrvGrfFILE)
{
   int i, count;
   int iPrmTyp, iFilTyp, iNext;
   tp_PrmTyp PrmTyp;
   tps_Str PTNameStr, DescStr;
   tp_PrmTypLst PrmTypLst;

   for (i = 0; i < num_PrmTypS; i++) {
      PrmTyp = I_PrmTyp(i);
      PrmTyp->IPrmTyp = i;
      count = fscanf(DrvGrfFILE, "%[^\1]\1 %[^\1]\1 %d %d\n",
                     PTNameStr, DescStr, &PrmTyp->HelpLevel, &iFilTyp);
      FORBIDDEN(count != 4);
      PrmTyp->PTName = Malloc_Str(&PTNameStr[1]);
      PrmTyp->Desc = Malloc_Str(&DescStr[1]);
      PrmTyp->FilTyp = I_FilTyp(iFilTyp);
      PrmTyp->RootLocPVal = NIL;
      PrmTyp->StrDirLocHdr = NIL;
      PrmTyp->Mark = FALSE;
   }

   count = fscanf(DrvGrfFILE, "%d\n", &iPrmTyp);
   FORBIDDEN(count != 1);
   NullPrmTyp = I_PrmTyp(iPrmTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iPrmTyp);
   FORBIDDEN(count != 1);
   HookValPrmTyp = I_PrmTyp(iPrmTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iPrmTyp);
   FORBIDDEN(count != 1);
   CopyDestPrmTyp = I_PrmTyp(iPrmTyp);

   count = fscanf(DrvGrfFILE, "%d\n", &iPrmTyp);
   FORBIDDEN(count != 1);
   ApplyPrmTyp = I_PrmTyp(iPrmTyp);

   for (i = 0; i < num_PrmTypLstS; i++) {
      PrmTypLst = I_PrmTypLst(i);
      count = fscanf(DrvGrfFILE, "%d %d\n", &iPrmTyp, &iNext);
      FORBIDDEN(count != 2);
      PrmTypLst->PrmTyp = I_PrmTyp(iPrmTyp);
      PrmTypLst->Next = I_PrmTypLst(iNext);
   }
}

static void Read_EnvVars(FILE * DrvGrfFILE)
{
   int i, count;
   int iEnvVar, iNext;
   tp_EnvVar EnvVar;
   tps_Str NameStr, DescStr, DefaultStr;
   tp_EnvVarLst EnvVarLst;

   for (i = 0; i < num_EnvVarS; i++) {
      EnvVar = I_EnvVar(i);
      count = fscanf(DrvGrfFILE, "%[^\1]\1 %[^\1]\1 %d  %[^\1]\1 %d\n",
                     NameStr, DescStr, &EnvVar->HelpLevel, DefaultStr,
                     &EnvVar->IsFile);
      FORBIDDEN(count != 5);
      EnvVar->Name = Malloc_Str(&NameStr[1]);
      EnvVar->Desc = Malloc_Str(&DescStr[1]);
      EnvVar->Default = Malloc_Str(&DefaultStr[1]);
   }

   for (i = 0; i < num_EnvVarLstS; i++) {
      EnvVarLst = I_EnvVarLst(i);
      count = fscanf(DrvGrfFILE, "%d %d\n", &iEnvVar, &iNext);
      FORBIDDEN(count != 2);
      EnvVarLst->EnvVar = I_EnvVar(iEnvVar);
      EnvVarLst->Next = I_EnvVarLst(iNext);
   }
}

static void Read_Tools(FILE * DrvGrfFILE)
{
   int i, count;
   int iTool, iInpEdg, iPrmTypLst, iEnvVarLst;
   tp_Tool Tool;
   tps_Str Package;

   for (i = 0; i < num_ToolS; i++) {
      Tool = I_Tool(i);
      count = fscanf(DrvGrfFILE, "%d %d %d %d %[^\1]\1\n",
                     &Tool->TClass, &iInpEdg, &iPrmTypLst, &iEnvVarLst,
                     Package);
      FORBIDDEN(count != 5);
      Tool->InpEdg = I_InpEdg(iInpEdg);
      Tool->PrmTypLst = I_PrmTypLst(iPrmTypLst);
      Tool->EnvVarLst = I_EnvVarLst(iEnvVarLst);
      Tool->Package = Malloc_Str(&Package[1]);
   }

   count = fscanf(DrvGrfFILE, "%d\n", &iTool);
   FORBIDDEN(count != 1);
   SourceTool = I_Tool(iTool);

   count = fscanf(DrvGrfFILE, "%d\n", &iTool);
   FORBIDDEN(count != 1);
   StrTool = I_Tool(iTool);

   count = fscanf(DrvGrfFILE, "%d\n", &iTool);
   FORBIDDEN(count != 1);
   DrvDirElmTool = I_Tool(iTool);

   count = fscanf(DrvGrfFILE, "%d\n", &iTool);
   FORBIDDEN(count != 1);
   VirDirElmTool = I_Tool(iTool);

   count = fscanf(DrvGrfFILE, "%d\n", &iTool);
   FORBIDDEN(count != 1);
   PntrHoTool = I_Tool(iTool);

   count = fscanf(DrvGrfFILE, "%d\n", &iTool);
   FORBIDDEN(count != 1);
   PntrElmTool = I_Tool(iTool);

   count = fscanf(DrvGrfFILE, "%d\n", &iTool);
   FORBIDDEN(count != 1);
   InternalPntrTool = I_Tool(iTool);

   count = fscanf(DrvGrfFILE, "%d\n", &iTool);
   FORBIDDEN(count != 1);
   TextDefTool = I_Tool(iTool);
}

static void Read_Edgs(FILE * DrvGrfFILE)
{
   int i, count;
   tp_InpEdg InpEdg;
   tp_MemEdg MemEdg;
   tp_EqvEdg EqvEdg;
   tp_CastEdg CastEdg;
   tp_DrvEdg DrvEdg;
   int iFilTyp, iInpSpc, iNext, iPrmTypLst;

   for (i = 0; i < num_InpEdgS; i++) {
      InpEdg = I_InpEdg(i);
      count = fscanf(DrvGrfFILE, "%d %d %d %d\n",
                     &iInpSpc, &InpEdg->InpKind, &InpEdg->IsUserArg,
                     &iNext);
      FORBIDDEN(count != 4);
      InpEdg->InpSpc = I_InpSpc(iInpSpc);
      InpEdg->Next = I_InpEdg(iNext);
   }

   for (i = 0; i < num_MemEdgS; i++) {
      MemEdg = I_MemEdg(i);
      count = fscanf(DrvGrfFILE, "%d %d\n", &iFilTyp, &iNext);
      FORBIDDEN(count != 2);
      MemEdg->FilTyp = I_FilTyp(iFilTyp);
      MemEdg->Next = I_MemEdg(iNext);
   }

   for (i = 0; i < num_EqvEdgS; i++) {
      EqvEdg = I_EqvEdg(i);
      count = fscanf(DrvGrfFILE, "%d %d\n", &iFilTyp, &iNext);
      FORBIDDEN(count != 2);
      EqvEdg->FilTyp = I_FilTyp(iFilTyp);
      EqvEdg->Next = I_EqvEdg(iNext);
   }

   for (i = 0; i < num_CastEdgS; i++) {
      CastEdg = I_CastEdg(i);
      count = fscanf(DrvGrfFILE, "%d %d\n", &iFilTyp, &iNext);
      FORBIDDEN(count != 2);
      CastEdg->FilTyp = I_FilTyp(iFilTyp);
      CastEdg->Next = I_CastEdg(iNext);
   }

   for (i = 0; i < num_DrvEdgS; i++) {
      DrvEdg = I_DrvEdg(i);
      count =
          fscanf(DrvGrfFILE, "%d %d %d\n", &iFilTyp, &iPrmTypLst, &iNext);
      FORBIDDEN(count != 3);
      DrvEdg->FilTyp = I_FilTyp(iFilTyp);
      DrvEdg->PrmTypLst = I_PrmTypLst(iPrmTypLst);
      DrvEdg->Next = I_DrvEdg(iNext);
   }
}

static void Read_InpSpcs(FILE * DrvGrfFILE)
{
   int count, i;
   int iFilTyp, iPrmTyp, iInpSpc, iNext;
   tp_InpSpc InpSpc;
   tps_Str Str;

   for (i = 0; i < num_InpSpcS; i++) {
      InpSpc = &InpSpcS[i];
      count = fscanf(DrvGrfFILE, "%d %d %d %[^\1]\1 %d %d %d\n",
                     &InpSpc->ISKind, &iFilTyp, &iPrmTyp, Str,
                     &InpSpc->IsEnvVar, &iInpSpc, &iNext);
      FORBIDDEN(count != 7);
      InpSpc->FilTyp = I_FilTyp(iFilTyp);
      InpSpc->PrmTyp = I_PrmTyp(iPrmTyp);
      {
         if (Str[1] == '\2') {
            InpSpc->Str = NIL;
         } else {
            InpSpc->Str = Malloc_Str(&Str[1]);
         }
      }
      InpSpc->InpSpc = I_InpSpc(iInpSpc);
      InpSpc->Next = I_InpSpc(iNext);
   }
}

#ifdef __STDC__
#define BUILD(x) x##S = (tp_##x)malloc((unsigned)(num_##x##S * sizeof(tps_##x)))
#else
#define BUILD(x) x/**/S = (tp_/**/x)malloc((unsigned)(num_/**/x/**/S * sizeof(tps_/**/x)))
#endif

void Read_DrvGrf(void)
{
   tps_Str VersionBuf, BannerBuf;
   tps_FileName DGFileName;
   tp_FilDsc DrvGrfFD;
   tp_Str Version;
   int count;

   FORBIDDEN(strcmp(DG_Version, DG_VERSION_STR) != 0);
   Get_DGFileName(DGFileName);
   DrvGrfFD = FileName_RFilDsc(DGFileName, FALSE);
   if (DrvGrfFD == ERROR) {
      return;
   }

   count = fscanf((FILE *) DrvGrfFD, "%s\n", VersionBuf);
   FORBIDDEN(count != 1);
   if (strcmp(DG_Version, VersionBuf) != 0) {
      SystemError("Error: incompatible local.dg.tab file.\n");
      SystemError("Reinstall derivation graph with -R option.\n");
      exit(1);
   }
   count = fscanf((FILE *) DrvGrfFD,
                  "%[^\1]\1 %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
                  BannerBuf,
                  &num_SrcTypS, &num_FilTypS, &num_PrmTypS,
                  &num_EnvVarS, &num_EnvVarLstS, &num_ToolS,
                  &num_InpEdgS, &num_MemEdgS, &num_EqvEdgS, &num_CastEdgS,
                  &num_DrvEdgS, &num_PrmTypLstS, &num_InpSpcS);
   FORBIDDEN(count != 14);
   if (strcmp(BannerBuf, ".") == 0) {
      (void) strcpy(BannerBuf, ".Odin Version ");
      Version = GetEnv("ODIN_VERSION_STR");
      (void) strcat(BannerBuf,
                    Version != NIL ? Version : ODIN_VERSION_STR);
      (void) strcat(BannerBuf, " (local DG: type ? for help)");
   }
   Banner = Malloc_Str(&BannerBuf[1]);
   BUILD(SrcTyp);
   BUILD(FilTyp);
   BUILD(PrmTyp);
   BUILD(EnvVar);
   BUILD(EnvVarLst);
   BUILD(Tool);
   BUILD(InpEdg);
   BUILD(MemEdg);
   BUILD(EqvEdg);
   BUILD(CastEdg);
   BUILD(DrvEdg);
   BUILD(PrmTypLst);
   BUILD(InpSpc);

   Read_FilTyps((FILE *) DrvGrfFD);
   Read_PrmTyps((FILE *) DrvGrfFD);
   Read_EnvVars((FILE *) DrvGrfFD);
   Read_Tools((FILE *) DrvGrfFD);
   Read_Edgs((FILE *) DrvGrfFD);
   Read_InpSpcs((FILE *) DrvGrfFD);

   Close(DrvGrfFD);
}

void Local_Get_Banner(tp_Str Str)
{
   tp_Str Version;

   if (strcmp(Banner, "") == 0) {
      (void) strcpy(Str, "Odin Version ");
      Version = GetEnv("ODIN_VERSION_STR");
      (void) strcat(Str, Version != NIL ? Version : ODIN_VERSION_STR);
      (void) strcat(Str, " (type ? for help)");
      return;
   }
   (void) strcpy(Str, Banner);
}
