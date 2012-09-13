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
#include "inc/InpKind_.h"
#include "inc/LogLevel_.h"
#include "inc/ModKind_.h"
#include "inc/Status_.h"
#include "inc/Str.h"
#include "inc/TClass_.h"

static boolean
IsSignificant(tp_FilHdr FilHdr, tp_ModKind ModKind, tp_InpKind InpKind)
{
   return (NeedsData(FilHdr, InpKind)
           && (ModKind == MODKIND_Input || NeedsElmData(FilHdr, InpKind)
               || (NeedsElmNameData(FilHdr, InpKind)
                   && ModKind == MODKIND_ElmName)));
}

static void
Broadcast_ElmMod(tp_FilHdr FilHdr,
                 tp_FilHdr ListFilHdr,
                 tp_ModKind ModKind, tp_Status Status)
{
   tp_ModKind ElmModKind;

   if (IsSource(ListFilHdr)) {
      if (IsSource(FilHdr)) {
         FORBIDDEN(!IsSymLink(ListFilHdr));
         if (Status == STAT_Unknown
             && FilHdr_Status(ListFilHdr) > STAT_Unknown) {
            Set_Status(ListFilHdr, STAT_Unknown);
            Do_Log("Clearing status of", ListFilHdr, LOGLEVEL_Status);
            Broadcast_Mod(ListFilHdr, MODKIND_Input, Status);
         }
         return;
      }

      if (Status == STAT_Pending) {
         if (!FilHdr_TgtValPndFlag(ListFilHdr)) {
            Set_TgtValPndFlag(ListFilHdr, TRUE);
            Do_Log("Pending TgtVal status of", ListFilHdr,
                   LOGLEVEL_Status);
            Broadcast_Mod(ListFilHdr, MODKIND_Input, Status);
         }
         return;
      }
      if (Status == STAT_TgtValError && FilHdr_TgtValPndFlag(ListFilHdr)) {
         return;
      }
      if (FilHdr_TgtValStatus(ListFilHdr) > Status) {
         Set_TgtValStatus(ListFilHdr, Status);
         Do_Log("Modifying TgtVal status of", ListFilHdr, LOGLEVEL_Status);
         Broadcast_Mod(ListFilHdr, MODKIND_Input, Status);
         return;
      }
      if (ModKind == 0) {
         Broadcast_Mod(ListFilHdr, MODKIND_Input, Status);
      }
      return;
   }

   ElmModKind = ModKind;
   if (ModKind == MODKIND_Input) {
      ElmModKind = (IsRef(FilHdr) ? MODKIND_ElmName : MODKIND_Elm);
   }
   switch (ElmModKind) {
   case MODKIND_ElmName:{
         if (Status == STAT_Pending) {
            if (!FilHdr_ElmNamePndFlag(ListFilHdr)) {
               Set_ElmNamePndFlag(ListFilHdr, TRUE);
               Do_Log("Pending element-name status of", ListFilHdr,
                      LOGLEVEL_Status);
               Broadcast_Mod(ListFilHdr, ElmModKind, Status);
            }
            return;
         }
         if (Status == STAT_TgtValError
             && FilHdr_ElmNamePndFlag(ListFilHdr)) {
            return;
         }
         if (FilHdr_ElmNameStatus(ListFilHdr) > Status) {
            Set_ElmNameStatus(ListFilHdr, Status);
            Do_Log("Modifying element-name status of", ListFilHdr,
                   LOGLEVEL_Status);
            Broadcast_Mod(ListFilHdr, ElmModKind, Status);
         }
         break;
      }
   case MODKIND_Elm:{
         if (Status == STAT_Pending) {
            if (!FilHdr_ElmPndFlag(ListFilHdr)) {
               Set_ElmPndFlag(ListFilHdr, TRUE);
               Do_Log("Pending element status of", ListFilHdr,
                      LOGLEVEL_Status);
               Broadcast_Mod(ListFilHdr, ElmModKind, Status);
            }
            return;
         }
         if (Status == STAT_TgtValError && FilHdr_ElmPndFlag(ListFilHdr)) {
            return;
         }
         if (FilHdr_ElmStatus(ListFilHdr) > Status) {
            Set_ElmStatus(ListFilHdr, Status);
            Do_Log("Modifying element status of", ListFilHdr,
                   LOGLEVEL_Status);
            Broadcast_Mod(ListFilHdr, ElmModKind, Status);
         }
         break;
      }
   default:{
         FATALERROR("unexpected ElmModKind");
      }
   }
}

void Broadcast(tp_FilHdr FilHdr, tp_Status Status)
{
   tp_FilHdr TgtValFilHdr;

   TgtValFilHdr = FilHdr_TgtValFilHdr(Copy_FilHdr(FilHdr));
   if (TgtValFilHdr == NIL) {
      Broadcast_Mod(FilHdr, MODKIND_Input, Status);
      return;
   }
   Broadcast_ElmMod(TgtValFilHdr, FilHdr, 0, Status);
   Ret_FilHdr(TgtValFilHdr);
}

void Broadcast_Mod(tp_FilHdr FilHdr, tp_ModKind ModKind, tp_Status Status)
{
   tp_FilHdr OutFilHdr, ListFilHdr;
   tp_LocInp LocInpLink, FirstInpLink;
   tp_LocElm LocElmLink, FirstElmLink;
   tp_FilInp FilInpLink;
   tp_FilElm FilElmLink;
   tp_TClass TClass;

   FirstInpLink = NIL;
   LocInpLink = FilHdr_InpLink(FilHdr);
   while (LocInpLink != FirstInpLink) {
      if (FirstInpLink == NIL)
         FirstInpLink = LocInpLink;
      FilInpLink = LocInp_FilInp(LocInpLink);
      if (IsSignificant(FilHdr, ModKind, FilInp_InpKind(FilInpLink))) {
         OutFilHdr = LocHdr_FilHdr(FilInp_OutLocHdr(FilInpLink));
         switch (Status) {
         case STAT_Unknown:{
               if (FilHdr_Status(OutFilHdr) > Status) {
                  Clr_Status(OutFilHdr);
                  Broadcast_Mod(OutFilHdr, MODKIND_Input, Status);
               }
               break;
            }
         case STAT_Pending:{
               if (!FilHdr_PndFlag(OutFilHdr)) {
                  Set_PndFlag(OutFilHdr, TRUE);
                  Do_Log("Setting pnd flag for", OutFilHdr,
                         LOGLEVEL_Status);
                  Broadcast_Mod(OutFilHdr, MODKIND_Input, Status);
               }
               break;
            }
         case STAT_TgtValError:{
               if (FilHdr_Status(OutFilHdr) > Status &&
                   !FilHdr_PndFlag(OutFilHdr)) {
                  TClass = Tool_TClass(FilHdr_Tool(OutFilHdr));
                  ; {
                     if (TClass == TC_Warning || TClass == TC_Error) {
                        ExecInternal(OutFilHdr, STAT_OK, 1);
                     } else {
                        Set_Status(OutFilHdr, Status);
                        Do_Log("Setting TgtVal-error status for",
                               OutFilHdr, LOGLEVEL_Status);
                        Broadcast_Mod(OutFilHdr, MODKIND_Input, Status);
                     }
                  }
               }
               if (FilHdr_DepStatus(OutFilHdr) > Status) {
                  Set_DepStatus(OutFilHdr, Status);
               }
               break;
            }
         default:{
               FATALERROR("Unexpected Status");
            }
         }
         Ret_FilHdr(OutFilHdr);
      }
      LocInpLink = FilInp_Link(FilInpLink);
      Ret_FilInp(FilInpLink);
   }

   FirstElmLink = NIL;
   LocElmLink = FilHdr_ElmLink(FilHdr);
   while (LocElmLink != FirstElmLink) {
      if (FirstElmLink == NIL)
         FirstElmLink = LocElmLink;
      FilElmLink = LocElm_FilElm(LocElmLink);
      ListFilHdr = FilElm_ListFilHdr(FilElmLink);
      Broadcast_ElmMod(FilHdr, ListFilHdr, ModKind, Status);
      Ret_FilHdr(ListFilHdr);
      LocElmLink = FilElm_Link(FilElmLink);
      Ret_FilElm(FilElmLink);
   }
}
