/* if-bcast.c */
extern void Broadcast(GMC_P1(tp_FilHdr) GMC_PN(tp_Status));
extern void Broadcast_Mod(GMC_P1(tp_FilHdr) GMC_PN(tp_ModKind)
                          GMC_PN(tp_Status));
/* if-build.c */
extern tp_Build JobID_Build(GMC_P1(tp_JobID));
extern void Extend_Builds(GMC_P1(int));
extern void Set_BuildHosts(GMC_P1(boolean *) GMC_PN(tp_Str));
extern void Write_BuildHosts(GMC_P1(tp_FilDsc));
extern void Local_Add_BuildArg(GMC_P1(tp_FileName));
extern void Local_Do_Build(GMC_P1(tp_JobID) GMC_PN(tp_FileName)
                           GMC_PN(tp_FileName));
extern void Local_Abort_Build(GMC_P1(tp_JobID));
extern void SystemExecCmdWait(GMC_P1(boolean *)
                              GMC_PN(const char *)GMC_PN(boolean));
extern void ChildAction(GMC_P1(boolean *) GMC_PN(boolean *));
extern void Cancel_Builds(GMC_P1(tp_Host));
extern void Build_Done(GMC_P1(tp_Build) GMC_PN(int));
extern void Local_Do_MakeReadOnly(GMC_P1(tp_FileName));
/* if-candrv.c */
extern tp_DrvPth Get_DrvPth(GMC_P1(tp_FilHdr) GMC_PN(tp_FilTyp));
extern tp_PrmTypLst DrvPth_PrmTypLst(GMC_P1(tp_DrvPth));
extern tp_DrvPth Find_GroupingDrvPthElm(GMC_P1(tp_DrvPth));
extern tp_FilHdr Do_DrvPth(GMC_P1(tp_FilHdr) GMC_PN(tp_FilPrm)
                           GMC_PN(tp_FilPrm) GMC_PN(tp_DrvPth));
extern tp_FilHdr Do_Deriv(GMC_P1(tp_FilHdr) GMC_PN(tp_FilPrm)
                          GMC_PN(tp_FilPrm) GMC_PN(tp_FilTyp));
extern tp_FilHdr Do_Key(GMC_P1(tp_FilHdr) GMC_PN(tp_Key));
extern tp_FilHdr Do_Keys(GMC_P1(tp_FilHdr) GMC_PN(tp_Key));
extern tp_FilHdr Str_FilHdr(GMC_P1(tp_Str) GMC_PN(tp_PrmTyp));
extern tp_FilHdr Do_VTgt(GMC_P1(tp_FilHdr) GMC_PN(tp_Key));
extern void WriteDrvHelp(GMC_P1(tp_FilDsc) GMC_PN(tp_FilHdr));
extern void WritePrmHelp(GMC_P1(tp_FilDsc) GMC_PN(tp_FilHdr)
                         GMC_PN(tp_FilPrm));
extern void WriteNameDesc(GMC_P1(tp_FilDsc) GMC_PN(tp_Str)
                          GMC_PN(tp_Desc));
extern tp_FilHdr Get_BaseVTgtFilHdr(GMC_P1(tp_FilHdr));
extern tp_PrmFHdr Nod_PrmFHdr(GMC_P1(tp_Nod));
extern tp_LocElm Make_ApplyLocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr)
                                  GMC_PN(tp_FileName));
extern tp_LocElm Make_MapLocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr));
extern tp_LocElm Make_RecurseLocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr));
extern void Local_Get_OdinFile(GMC_P1(tp_Str) GMC_PN(boolean));
extern void End_Get_OdinFile(GMC_P1(void));
/* if-client.c */
extern void Ret_ToDo(GMC_P1(void));
extern tp_Client New_Client(GMC_P1(tp_ClientID));
extern void Activate_Client(GMC_P1(tp_Client));
extern void Ret_Client(GMC_P1(tp_Client));
extern void Purge_Clients(GMC_P1(void));
extern boolean Is_ActiveClient(GMC_P1(tp_Client));
extern int Client_FD(GMC_P1(tp_Client));
extern boolean Client_Interrupted(GMC_P1(tp_Client));
extern void Set_Client_Interrupted(GMC_P1(tp_Client) GMC_PN(boolean));
extern boolean Client_KeepGoing(GMC_P1(tp_Client));
extern int Client_ErrLevel(GMC_P1(tp_Client));
extern int Client_WarnLevel(GMC_P1(tp_Client));
extern tp_LogLevel Client_LogLevel(GMC_P1(tp_Client));
extern tp_FilHdr Client_FilHdr(GMC_P1(tp_Client));
extern void Set_Client_FilHdr(GMC_P1(tp_Client) GMC_PN(tp_FilHdr)
                              GMC_PN(boolean));
extern boolean Client_NeedsData(GMC_P1(tp_Client));
extern void Push_AllReqs(GMC_P1(boolean *));
extern tp_FHLst Client_ToDo(GMC_P1(tp_Client));
extern void Push_ToDo(GMC_P1(tp_FilHdr));
extern void Push_Pending(GMC_P1(tp_FilHdr) GMC_PN(tp_InpKind));
extern void Push_ToBroadcast(GMC_P1(tp_FilHdr));
extern void Do_ToBroadcast(GMC_P1(void));
extern tp_Job Client_Job(GMC_P1(tp_Client));
extern tp_Client Client_Next(GMC_P1(tp_Client));
extern boolean Is_LocalClient(GMC_P1(tp_Client));
extern boolean Is_ServerAction(GMC_P1(void));
extern tp_Job New_Job(GMC_P1(void));
extern tp_Job Get_Job(GMC_P1(tp_JobID));
extern void Ret_Job(GMC_P1(tp_Job));
extern tp_FilHdr Job_FilHdr(GMC_P1(tp_Job));
extern tp_Job Add_Job(GMC_P1(tp_FilHdr));
extern void Del_Job(GMC_P1(tp_Job));
extern void Clr_Status(GMC_P1(tp_FilHdr));
extern boolean Is_TgtValErrStatus(GMC_P1(tp_FilHdr));
extern tp_FilHdr FilHdr_TgtValFilHdr(GMC_P1(tp_FilHdr));
extern void ServerAction(GMC_P1(void));
extern void Local_Do_Interrupt(GMC_P1(boolean));
extern boolean IsAny_ServerAction(GMC_P1(void));
extern tp_FilHdr Top_CWDFilHdr(GMC_P1(void));
extern tp_FilHdr Top_ContextFilHdr(GMC_P1(void));
extern void Push_ContextFilHdr(GMC_P1(tp_FilHdr));
extern void Pop_ContextFilHdr(GMC_P1(void));
extern void Local_Set_CWD(GMC_P1(tp_FileName));
extern void Local_Push_Context(GMC_P1(tp_FileName) GMC_PN(tp_FileName));
extern void Local_Pop_Context(GMC_P1(tp_FileName));
extern void Local_Set_KeepGoing(GMC_P1(boolean));
extern void Local_Set_ErrLevel(GMC_P1(int));
extern void Local_Set_WarnLevel(GMC_P1(int));
extern void Local_Set_LogLevel(GMC_P1(tp_LogLevel));
extern void Local_Set_HelpLevel(GMC_P1(int));
extern void Local_Set_MaxJobs(GMC_P1(int));
extern void Local_Get_UseCount(GMC_P1(int *));
/* if-cmd.c */
extern void CommandInterpreter(GMC_P1(boolean *) GMC_PN(tp_Nod)
                               GMC_PN(boolean));
extern boolean IsIncremental_MsgLevel(GMC_P1(int));
extern void UtilityHelp(GMC_P1(void));
extern void UtilityDefaultHelp(GMC_P1(void));
extern void Print_Banner(GMC_P1(void));
/* if-depend.c */
extern void WriteReport(GMC_P1(tp_FilDsc) GMC_PN(tp_FilHdr)
                        GMC_PN(tp_Status));
extern void GetDepend(GMC_P1(tp_LocElm *) GMC_PN(tp_LocElm *)
                      GMC_PN(tp_FilHdr) GMC_PN(tp_FilHdr));
extern void Local_Get_DPath(GMC_P1(tp_Str));
/* if-dir.c */
extern tp_FilDsc OpenDir(GMC_P1(tp_FileName));
extern void CloseDir(GMC_P1(tp_FilDsc));
extern void ReadDir(GMC_P1(tp_FileName) GMC_PN(boolean *)
                    GMC_PN(tp_FilDsc));
extern void ClearDir(GMC_P1(tp_FileName));
/* if-drvgrf.c */
extern tp_FilTyp IFilTyp_FilTyp(GMC_P1(int));
extern tp_PrmTyp IPrmTyp_PrmTyp(GMC_P1(int));
extern tp_PrmTyp I_PrmTyp(GMC_P1(int));
extern void Read_DrvGrf(GMC_P1(void));
extern void Local_Get_Banner(GMC_P1(tp_Str));
/* if-drvpth.c */
extern void AppendDrvPth(GMC_P1(tp_DrvPth *) GMC_PN(tp_DrvPth));
extern tp_DrvPth FilTyp_Cast_DrvPth(GMC_P1(tp_FilTyp));
extern tp_DrvPth FilTyp_Eqv_DrvPth(GMC_P1(tp_FilTyp));
extern tp_DrvPth FilTyp_Drv_DrvPth(GMC_P1(tp_FilTyp) GMC_PN(tp_DrvEdg));
extern void Ret_DrvPth(GMC_P1(tp_DrvPth));
extern tp_DPType DrvPth_DPType(GMC_P1(tp_DrvPth));
extern tp_FKind DrvPth_FKind(GMC_P1(tp_DrvPth));
extern tp_FilTyp DrvPth_FilTyp(GMC_P1(tp_DrvPth));
extern tp_DrvEdg DrvPth_DrvEdg(GMC_P1(tp_DrvPth));
extern tp_DrvPth DrvPth_Next(GMC_P1(tp_DrvPth));
/* if-drvspc.c */
extern void Print_FilHdr(GMC_P1(tp_FilDsc) GMC_PN(tp_Str)
                         GMC_PN(tp_FilHdr));
extern void SPrint_FilHdr(GMC_P1(tp_Str) GMC_PN(tp_FilHdr));
extern void VerboseSPrint_FilHdr(GMC_P1(tp_Str) GMC_PN(tp_FilHdr));
/* if-edg.c */
extern tp_PrmTypLst DrvEdg_PrmTypLst(GMC_P1(tp_DrvEdg));
extern tp_InpSpc InpEdg_InpSpc(GMC_P1(tp_InpEdg));
extern tp_InpKind InpEdg_InpKind(GMC_P1(tp_InpEdg));
extern boolean InpEdg_IsUserArg(GMC_P1(tp_InpEdg));
extern tp_InpEdg InpEdg_Next(GMC_P1(tp_InpEdg));
extern tp_FilTyp EqvEdg_FilTyp(GMC_P1(tp_EqvEdg));
extern tp_FilTyp MemEdg_FilTyp(GMC_P1(tp_MemEdg));
extern boolean InpKind_IsAnyOK(GMC_P1(tp_InpKind));
extern boolean NeedsData(GMC_P1(tp_FilHdr) GMC_PN(tp_InpKind));
extern boolean NeedsElmData(GMC_P1(tp_FilHdr) GMC_PN(tp_InpKind));
extern boolean NeedsElmNameData(GMC_P1(tp_FilHdr) GMC_PN(tp_InpKind));
extern int NumInputs(GMC_P1(tp_FilTyp));
extern void GetOutTyps(GMC_P1(tp_FilTyp) GMC_PN(tp_OutTyps) GMC_PN(int *));
extern void SetEqvEdg_Marks(GMC_P1(tp_EqvEdg) GMC_PN(boolean)
                            GMC_PN(boolean));
extern void SetCastEdg_Marks(GMC_P1(tp_CastEdg) GMC_PN(boolean));
extern void SetDrvEdg_Marks(GMC_P1(tp_DrvEdg) GMC_PN(boolean));
/* if-env.c */
extern void Get_SocketFileName(GMC_P1(tp_FileName));
extern void Get_DGFileName(GMC_P1(tp_FileName));
extern void Get_PkgDirName(GMC_P1(tp_FileName) GMC_PN(tp_Package));
extern void Get_InfoFileName(GMC_P1(tp_FileName));
extern void Get_DebugFileName(GMC_P1(tp_FileName));
extern void Get_WorkFileName(GMC_P1(tp_FileName) GMC_PN(tp_Job)
                             GMC_PN(tp_FilHdr));
extern void JobID_LogFileName(GMC_P1(tp_FileName) GMC_PN(int));
extern void Local_ShutDown(GMC_P1(void));
extern void Init_Env(GMC_P1(void));
extern void Write_ENV2(GMC_P1(void));
extern void Read_ENV2(GMC_P1(void));
extern boolean IsDef_EnvVar(GMC_P1(tp_Str));
extern void Init_CWD(GMC_P1(void));
extern void DeadServerExit(GMC_P1(void));
extern void Exit(GMC_P1(int));
/* if-err.c */
extern void Init_Err(GMC_P1(void));
extern void Set_IPC_Err(GMC_P1(boolean));
extern void Set_ErrFile(GMC_P1(tp_FileName) GMC_PN(boolean)
                        GMC_PN(tp_FilDsc));
extern void Save_ErrFile(GMC_P1(tp_FileName *) GMC_PN(boolean *)
                         GMC_PN(tp_FilDsc *));
extern boolean IsErr(GMC_P1(void));
extern void Reset_Err(GMC_P1(void));
extern void Increment_Errors(GMC_P1(void));
extern int Num_Errors(GMC_P1(void));
extern void SysCallError(GMC_P1(tp_FilDsc) GMC_PN(char *));
extern void FatalError(GMC_P1(char *)GMC_PN(char *)GMC_PN(int));
extern void SystemError(GMC_P1(char *)GMC_PN(...));
extern void Local_ErrMessage(GMC_P1(tp_Str));
extern void fatal_err(GMC_P1(char *));
/* if-exec.c */
extern void Exec(GMC_P1(tp_FilHdr));
extern void Local_Job_Done(GMC_P1(tp_JobID) GMC_PN(boolean));
/* if-execint.c */
extern void ExecInternal(GMC_P1(tp_FilHdr) GMC_PN(tp_Status)
                         GMC_PN(tp_Date));
/* if-execspc.c */
extern tp_Tool FilHdr_Tool(GMC_P1(tp_FilHdr));
extern void FilHdr_ExecSpc(GMC_P1(tp_ExecSpc) GMC_PN(tp_FilHdr));
extern void Get_OutFilHdrs(GMC_P1(tp_OutFilHdrs)
                           GMC_PN(int *)GMC_PN(tp_FilHdr));
extern void Ret_ExecSpc(GMC_P1(tp_ExecSpc));
/* if-fhacc.c */
extern boolean IsSource_FKind(GMC_P1(tp_FKind));
extern boolean IsSource(GMC_P1(tp_FilHdr));
extern boolean IsSymLink(GMC_P1(tp_FilHdr));
extern boolean IsDir(GMC_P1(tp_FilHdr));
extern boolean IsStr(GMC_P1(tp_FilHdr));
extern boolean IsBound(GMC_P1(tp_FilHdr));
extern boolean IsATgt(GMC_P1(tp_FilHdr));
extern boolean IsVTgt(GMC_P1(tp_FilHdr));
extern boolean IsVTgtText(GMC_P1(tp_FilHdr));
extern boolean IsDfltTgtVal(GMC_P1(tp_FilHdr));
extern boolean IsPntr(GMC_P1(tp_FilHdr));
extern boolean IsGeneric(GMC_P1(tp_FilHdr));
extern boolean IsPipe(GMC_P1(tp_FilHdr));
extern boolean IsInstance(GMC_P1(tp_FilHdr));
extern boolean IsAtmc(GMC_P1(tp_FilHdr));
extern boolean IsList(GMC_P1(tp_FilHdr));
extern boolean IsViewSpec(GMC_P1(tp_FilHdr));
extern boolean IsStruct(GMC_P1(tp_FilHdr));
extern boolean IsStructMem(GMC_P1(tp_FilHdr));
extern boolean IsVoid(GMC_P1(tp_FilHdr));
extern boolean IsTargetsPtr(GMC_P1(tp_FilHdr));
extern boolean IsTargets(GMC_P1(tp_FilHdr));
extern boolean IsDrvDir(GMC_P1(tp_FilHdr));
extern boolean IsDrvDirElm(GMC_P1(tp_FilHdr));
extern boolean IsKeyList(GMC_P1(tp_FilHdr));
extern boolean IsKeyListElm(GMC_P1(tp_FilHdr));
extern boolean IsVirDir(GMC_P1(tp_FilHdr));
extern boolean IsCopy(GMC_P1(tp_FilHdr));
extern boolean IsAutoExec(GMC_P1(tp_FilHdr));
extern boolean HasKey_FKind(GMC_P1(tp_FKind));
extern boolean IsRef(GMC_P1(tp_FilHdr));
extern tp_LocHdr FilHdr_LocHdr(GMC_P1(tp_FilHdr));
extern tp_LocHdr FilHdr_AliasLocHdr(GMC_P1(tp_FilHdr));
extern void Set_AliasLocHdr(GMC_P1(tp_FilHdr) GMC_PN(tp_LocHdr));
extern tp_FilHdr FilHdr_AliasFilHdr(GMC_P1(tp_FilHdr));
extern tp_FKind FilHdr_FKind(GMC_P1(tp_FilHdr));
extern void Set_FKind(GMC_P1(tp_FilHdr) GMC_PN(tp_FKind));
extern tp_FilTyp FilHdr_FilTyp(GMC_P1(tp_FilHdr));
extern tp_FilPrm FilHdr_FilPrm(GMC_P1(tp_FilHdr));
extern tp_Ident FilHdr_Ident(GMC_P1(tp_FilHdr));
extern void Update_SrcFilHdr(GMC_P1(tp_FilHdr) GMC_PN(boolean));
extern void FilHdr_Error(GMC_P1(tp_Str) GMC_PN(tp_FilHdr));
extern boolean IsAllDone(GMC_P1(tp_FilHdr) GMC_PN(tp_InpKind));
extern boolean IsAllUpToDate(GMC_P1(tp_FilHdr) GMC_PN(tp_InpKind));
extern boolean IsSrcUpToDate(GMC_P1(tp_FilHdr));
extern boolean IsUpToDate(GMC_P1(tp_FilHdr));
extern boolean IsElmNameUpToDate(GMC_P1(tp_FilHdr));
extern boolean IsElmUpToDate(GMC_P1(tp_FilHdr));
extern boolean IsTgtValUpToDate(GMC_P1(tp_FilHdr));
extern tp_FilHdr FilHdr_Father(GMC_P1(tp_FilHdr));
extern tp_FilHdr FilHdr_SrcFilHdr(GMC_P1(tp_FilHdr));
extern tp_FilHdr FilHdr_DirFilHdr(GMC_P1(tp_FilHdr));
extern tp_Str FilHdr_Key(GMC_P1(tp_Str) GMC_PN(tp_FilHdr));
extern tp_Label FilHdr_Label(GMC_P1(tp_Str) GMC_PN(tp_FilHdr)
                             GMC_PN(boolean));
extern tp_FilHdr FilHdr_ElmFilHdr(GMC_P1(tp_FilHdr));
/* if-fhnam.c */
extern void FilHdr_DataFileName(GMC_P1(tp_FileName) GMC_PN(tp_FilHdr));
extern void FilHdr_ErrorFileName(GMC_P1(tp_FileName) GMC_PN(tp_FilHdr));
extern void FilHdr_WarningFileName(GMC_P1(tp_FileName) GMC_PN(tp_FilHdr));
extern void Local_Do_Alias(GMC_P1(tp_FileName) GMC_PN(boolean));
extern void Local_Get_Alias(GMC_P1(tp_FileName) GMC_PN(tp_FileName));
extern void FilHdr_HostFN(GMC_P1(tp_FileName) GMC_PN(tp_FilHdr)
                          GMC_PN(boolean));
extern tp_FilHdr HostFN_FilHdr(GMC_P1(tp_FileName));
extern tp_FilHdr CacheFileName_FilHdr(GMC_P1(tp_FileName));
extern tp_FilHdr DataFileName_FilHdr(GMC_P1(tp_FileName));
/* if-fhnew.c */
extern void Make_RootHdrInf(GMC_P1(tp_HdrInf) GMC_PN(tp_LocHdr));
extern tp_FilHdr Insert_FilHdr(GMC_P1(tp_FilHdr) GMC_PN(tp_FKind)
                               GMC_PN(tp_FilTyp) GMC_PN(tp_FilPrm)
                               GMC_PN(tp_Ident));
extern tp_FilHdr Extend_FilHdr(GMC_P1(tp_FilHdr) GMC_PN(tp_FKind)
                               GMC_PN(tp_FilTyp) GMC_PN(tp_FilPrm)
                               GMC_PN(tp_Str));
extern tp_FilHdr Get_Drv(GMC_P1(tp_FilHdr) GMC_PN(tp_FKind)
                         GMC_PN(tp_FilTyp) GMC_PN(tp_FilPrm)
                         GMC_PN(tp_Ident));
extern tp_FilHdr Get_KeyDrv(GMC_P1(tp_FilHdr) GMC_PN(tp_FKind)
                            GMC_PN(tp_Key));
/* if-fhsrc.c */
extern void Deref_Pntrs(GMC_P1(tp_FilHdr *) GMC_PN(tp_FilPrm *)
                        GMC_PN(tp_FilHdr) GMC_PN(boolean));
extern tp_FilHdr Deref(GMC_P1(tp_FilHdr));
extern tp_FilHdr Deref_SymLink(GMC_P1(tp_FilHdr));
extern void Local_Test(GMC_P1(tp_FileName));
extern void Local_Test_All(GMC_P1(void));
extern tp_FilHdr Get_Copy_DestFilHdr(GMC_P1(tp_FilHdr));
extern tp_LocElm Make_CopyLocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr)
                                 GMC_PN(tp_FilHdr));
extern void Exec_CopyCmd(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr)
                         GMC_PN(tp_FilHdr));
/* if-fhstat.c */
extern boolean Is_PRB_Status(GMC_P1(tp_Status));
extern void Clr_ErrStatus(GMC_P1(tp_FilHdr));
extern void Add_ErrStatus(GMC_P1(tp_FilHdr) GMC_PN(tp_Status));
extern boolean FilHdr_HasErrStatus(GMC_P1(tp_FilHdr) GMC_PN(tp_Status));
extern tp_Status FilHdr_MinErrStatus(GMC_P1(tp_FilHdr));
extern void Add_StatusFile(GMC_P1(tp_FilHdr) GMC_PN(tp_Status)
                           GMC_PN(tp_FileName));
extern void Set_DepStatus(GMC_P1(tp_FilHdr) GMC_PN(tp_Status));
extern tp_Status FilHdr_DepStatus(GMC_P1(tp_FilHdr));
extern void Set_DepModDate(GMC_P1(tp_FilHdr) GMC_PN(tp_Date));
extern tp_Date FilHdr_DepModDate(GMC_P1(tp_FilHdr));
extern void Set_Status(GMC_P1(tp_FilHdr) GMC_PN(tp_Status));
extern tp_Status FilHdr_Status(GMC_P1(tp_FilHdr));
extern void Set_ElmNameStatus(GMC_P1(tp_FilHdr) GMC_PN(tp_Status));
extern tp_Status FilHdr_ElmNameStatus(GMC_P1(tp_FilHdr));
extern void Set_ElmStatus(GMC_P1(tp_FilHdr) GMC_PN(tp_Status));
extern tp_Status FilHdr_ElmStatus(GMC_P1(tp_FilHdr));
extern void Set_TgtValStatus(GMC_P1(tp_FilHdr) GMC_PN(tp_Status));
extern tp_Status FilHdr_TgtValStatus(GMC_P1(tp_FilHdr));
extern tp_Status FilHdr_TgtValMinStatus(GMC_P1(tp_FilHdr));
extern tp_Status FilHdr_MinStatus(GMC_P1(tp_FilHdr) GMC_PN(tp_InpKind));
extern void Set_ModDate(GMC_P1(tp_FilHdr));
extern tp_Date FilHdr_ModDate(GMC_P1(tp_FilHdr));
extern void Set_ConfirmDate(GMC_P1(tp_FilHdr) GMC_PN(tp_Date));
extern void Clr_ConfirmDate(GMC_P1(tp_FilHdr));
extern tp_Date FilHdr_ConfirmDate(GMC_P1(tp_FilHdr));
extern void Set_ElmNameConfirmDate(GMC_P1(tp_FilHdr));
extern void Set_ElmConfirmDate(GMC_P1(tp_FilHdr));
extern void Set_ElmModDate(GMC_P1(tp_FilHdr) GMC_PN(tp_Date));
extern tp_Date FilHdr_ElmModDate(GMC_P1(tp_FilHdr));
extern void Set_ElmNameModDate(GMC_P1(tp_FilHdr) GMC_PN(tp_Date));
extern tp_Date FilHdr_ElmNameModDate(GMC_P1(tp_FilHdr));
extern void Set_Flag(GMC_P1(tp_FilHdr) GMC_PN(tp_Flag));
extern void Clr_Flag(GMC_P1(tp_FilHdr) GMC_PN(tp_Flag));
extern boolean FilHdr_Flag(GMC_P1(tp_FilHdr) GMC_PN(tp_Flag));
extern void Set_AnyOKDepth(GMC_P1(tp_FilHdr) GMC_PN(int));
extern int FilHdr_AnyOKDepth(GMC_P1(tp_FilHdr));
extern void Set_ElmDepth(GMC_P1(tp_FilHdr) GMC_PN(int));
extern int FilHdr_ElmDepth(GMC_P1(tp_FilHdr));
extern void Set_ElmTag(GMC_P1(tp_FilHdr) GMC_PN(int));
extern int FilHdr_ElmTag(GMC_P1(tp_FilHdr));
extern void Set_SCC(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr));
extern tp_FilHdr FilHdr_SCC(GMC_P1(tp_FilHdr));
extern void Set_ListPndFlag(GMC_P1(tp_FilHdr) GMC_PN(boolean));
extern void Set_PndFlag(GMC_P1(tp_FilHdr) GMC_PN(boolean));
extern boolean FilHdr_PndFlag(GMC_P1(tp_FilHdr));
extern void Set_ElmNamePndFlag(GMC_P1(tp_FilHdr) GMC_PN(boolean));
extern boolean FilHdr_ElmNamePndFlag(GMC_P1(tp_FilHdr));
extern void Set_ElmPndFlag(GMC_P1(tp_FilHdr) GMC_PN(boolean));
extern boolean FilHdr_ElmPndFlag(GMC_P1(tp_FilHdr));
extern void Set_TgtValPndFlag(GMC_P1(tp_FilHdr) GMC_PN(boolean));
extern boolean FilHdr_TgtValPndFlag(GMC_P1(tp_FilHdr));
extern tp_LocInp FilHdr_LocInp(GMC_P1(tp_FilHdr));
extern void Set_LocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_LocElm));
extern tp_LocElm FilHdr_LocElm(GMC_P1(tp_FilHdr));
extern void Set_OldLocElm(GMC_P1(tp_FilHdr));
extern tp_LocElm FilHdr_OldLocElm(GMC_P1(tp_FilHdr));
extern void Set_TgtValLocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_LocElm));
extern void Set_DfltTgtValLocElm(GMC_P1(tp_FilHdr));
extern tp_LocElm FilHdr_TgtValLocElm(GMC_P1(tp_FilHdr));
extern boolean FilHdr_ActTgtInstalled(GMC_P1(tp_FilHdr));
extern void Set_ActTgtInstalled(GMC_P1(tp_FilHdr) GMC_PN(boolean));
extern void Set_InpLink(GMC_P1(tp_FilHdr) GMC_PN(tp_LocInp));
extern tp_LocInp FilHdr_InpLink(GMC_P1(tp_FilHdr));
extern void Set_ElmLink(GMC_P1(tp_FilHdr) GMC_PN(tp_LocElm));
extern tp_LocElm FilHdr_ElmLink(GMC_P1(tp_FilHdr));
extern int FilHdr_Size(GMC_P1(tp_FilHdr));
extern void Set_Size(GMC_P1(tp_FilHdr) GMC_PN(int));
extern boolean Data_Exists(GMC_P1(tp_FilHdr));
extern void Local_Get_CurSize(GMC_P1(int *));
extern void Set_OrigLocHdr(GMC_P1(tp_FilHdr) GMC_PN(tp_LocHdr));
extern tp_LocHdr FilHdr_OrigLocHdr(GMC_P1(tp_FilHdr));
extern void Set_OrigModDate(GMC_P1(tp_FilHdr) GMC_PN(tp_Date));
extern tp_Date FilHdr_OrigModDate(GMC_P1(tp_FilHdr));
/* if-file.c */
extern void Set_ModeMask(GMC_P1(tp_FileName));
extern void Get_FileInfo(GMC_P1(tp_SKind *)
                         GMC_PN(int *)GMC_PN(tp_FileName));
extern void MakePlnFile(GMC_P1(boolean *) GMC_PN(tp_FileName));
extern void MakeDirFile(GMC_P1(boolean *) GMC_PN(tp_FileName));
extern void GetWorkingDir(GMC_P1(boolean *) GMC_PN(tp_Str));
extern void ChangeDir(GMC_P1(boolean *) GMC_PN(tp_FileName));
extern boolean IsExecutable(GMC_P1(tp_FileName));
extern void MakeExecutable(GMC_P1(tp_FileName));
extern void MakeReadOnly(GMC_P1(boolean *) GMC_PN(tp_FileName));
extern void SymLink(GMC_P1(boolean *) GMC_PN(tp_FileName)
                    GMC_PN(tp_FileName));
extern void FileName_SymLinkFileName(GMC_P1(tp_FileName)
                                     GMC_PN(tp_FileName));
extern boolean IsDirectory_FileName(GMC_P1(tp_FileName));
extern boolean Exists(GMC_P1(tp_FileName));
extern boolean Empty(GMC_P1(tp_FileName));
extern void FileSize(GMC_P1(boolean *) GMC_PN(int *)GMC_PN(tp_FileName));
extern void Remove(GMC_P1(tp_FileName));
extern void RemoveDir(GMC_P1(tp_FileName));
extern void Rename(GMC_P1(boolean *) GMC_PN(tp_FileName)
                   GMC_PN(tp_FileName));
/* if-filelm.c */
extern void Init_FilElms(GMC_P1(void));
extern void Ret_FilElm(GMC_P1(tp_FilElm));
extern void Free_FilElms(GMC_P1(void));
extern tp_FilElm LocElm_FilElm(GMC_P1(tp_LocElm));
extern void WriteFilElms(GMC_P1(void));
extern void DeAlloc_ElmInf(GMC_P1(tp_LocElm));
extern boolean FilElms_InUse(GMC_P1(void));
extern tp_LocHdr FilElm_LocHdr(GMC_P1(tp_FilElm));
extern tp_FilHdr FilElm_FilHdr(GMC_P1(tp_FilElm));
extern tp_LocHdr FilElm_ListLocHdr(GMC_P1(tp_FilElm));
extern tp_FilHdr FilElm_ListFilHdr(GMC_P1(tp_FilElm));
extern tp_FilPrm FilElm_FilPrm(GMC_P1(tp_FilElm));
extern tp_LocElm FilElm_Next(GMC_P1(tp_FilElm));
extern tp_FilElm FilElm_NextFilElm(GMC_P1(tp_FilElm));
extern tp_LocElm FilElm_Link(GMC_P1(tp_FilElm));
extern tp_LocElm Make_LocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_FilPrm)
                             GMC_PN(tp_FilHdr));
extern void Chain_LocElms(GMC_P1(tp_LocElm *) GMC_PN(tp_LocElm *)
                          GMC_PN(tp_LocElm));
extern boolean IsEquiv_LocElms(GMC_P1(tp_LocElm) GMC_PN(tp_LocElm));
/* if-filhdr.c */
extern void Init_FilHdrs(GMC_P1(void));
extern void Init_FilHdrTree(GMC_P1(void));
extern tp_LocHdr Alloc_HdrInf(GMC_P1(void));
extern tp_FilHdr Copy_FilHdr(GMC_P1(tp_FilHdr));
extern void Ret_FilHdr(GMC_P1(tp_FilHdr));
extern void Free_FilHdrs(GMC_P1(void));
extern tp_FilHdr New_FilHdr(GMC_P1(void));
extern tp_FilHdr LocHdr_FilHdr(GMC_P1(tp_LocHdr));
extern void Init_HdrInf(GMC_P1(tp_HdrInf));
extern void SetModified(GMC_P1(tp_FilHdr));
extern void WriteFilHdrs(GMC_P1(void));
extern boolean FilHdrs_InUse(GMC_P1(void));
extern void CleanUp(GMC_P1(void));
/* if-filinp.c */
extern void Init_FilInps(GMC_P1(void));
extern void Ret_FilInp(GMC_P1(tp_FilInp));
extern void Free_FilInps(GMC_P1(void));
extern tp_FilInp LocInp_FilInp(GMC_P1(tp_LocInp));
extern void WriteFilInps(GMC_P1(void));
extern boolean FilInps_InUse(GMC_P1(void));
extern tp_FilHdr FilInp_FilHdr(GMC_P1(tp_FilInp));
extern tp_LocHdr FilInp_OutLocHdr(GMC_P1(tp_FilInp));
extern int FilInp_IArg(GMC_P1(tp_FilInp));
extern tp_InpKind FilInp_InpKind(GMC_P1(tp_FilInp));
extern tp_FilInp FilInp_NextFilInp(GMC_P1(tp_FilInp));
extern tp_LocInp FilInp_Link(GMC_P1(tp_FilInp));
extern tp_LocInp Make_LocInp(GMC_P1(tp_FilHdr) GMC_PN(int)
                             GMC_PN(tp_InpKind) GMC_PN(tp_FilHdr));
extern void Chain_LocInps(GMC_P1(tp_LocInp *) GMC_PN(tp_LocInp *)
                          GMC_PN(tp_LocInp));
extern tp_LocInp Get_LocInp(GMC_P1(tp_FilHdr));
/* if-filprm.c */
extern void Init_FilPrm(GMC_P1(void));
extern void Add_RootLocPVal(GMC_P1(tp_PrmTyp) GMC_PN(tp_LocPVal));
extern boolean Equal_FilPrm(GMC_P1(tp_FilPrm) GMC_PN(tp_FilPrm));
extern tp_FilPrm Append_PrmInf(GMC_P1(tp_FilPrm) GMC_PN(tp_PrmTyp)
                               GMC_PN(tp_LocHdr) GMC_PN(tp_LocPVal));
extern tp_FilPrm Append_FilPrm(GMC_P1(tp_FilPrm) GMC_PN(tp_FilPrm));
extern tp_LocPrm FilPrm_LocPrm(GMC_P1(tp_FilPrm));
extern tp_FilPrm LocPrm_FilPrm(GMC_P1(tp_LocPrm));
extern tp_FilPrm Strip_FilPrm(GMC_P1(tp_FilPrm) GMC_PN(tp_PrmTypLst));
extern tp_FilPrm StripExcept_FilPrm(GMC_P1(tp_FilPrm) GMC_PN(tp_PrmTyp));
extern tp_FilPVal FilPrm_FilPVal(GMC_P1(tp_FilPrm));
extern tp_FilPVal Get_FilPVal(GMC_P1(tp_FilPrm) GMC_PN(tp_PrmTyp));
extern tp_FilPrm FilPrm_DerefPrmVal(GMC_P1(tp_FilPrm));
extern void Chain_FilPrm_DerefPrmVal(GMC_P1(tp_LocInp *)
                                     GMC_PN(tp_LocInp *) GMC_PN(tp_FilPrm)
                                     GMC_PN(tp_FilHdr));
extern tp_FilHdr Get_FPVFilHdr(GMC_P1(tp_PrmTyp) GMC_PN(tp_FilPrm));
extern void Print_FilPrm(GMC_P1(tp_FilDsc) GMC_PN(tp_Str)
                         GMC_PN(tp_FilPrm));
extern void SetPrmTypLst_Marks(GMC_P1(tp_PrmTypLst));
/* if-filpval.c */
extern tp_FilPVal New_FilPVal(GMC_P1(void));
extern boolean IsRootFilPVal(GMC_P1(tp_FilPVal));
extern tp_FilPVal Add_PValInf(GMC_P1(tp_FilPVal) GMC_PN(tp_LocHdr)
                              GMC_PN(tp_LocPVal));
extern tp_FilPVal Append_PValInf(GMC_P1(tp_FilPVal) GMC_PN(tp_LocHdr)
                                 GMC_PN(tp_LocPVal));
extern tp_FilPVal Append_FilPVal(GMC_P1(tp_FilPVal) GMC_PN(tp_FilPVal));
extern tp_LocPVal FilPVal_LocPVal(GMC_P1(tp_FilPVal));
extern tp_FilPVal LocPVal_FilPVal(GMC_P1(tp_LocPVal));
extern void Print_FilPVal(GMC_P1(tp_FilDsc) GMC_PN(tp_Str)
                          GMC_PN(tp_PrmTyp) GMC_PN(tp_FilPVal));
extern tp_LocHdr FilPVal_LocHdr(GMC_P1(tp_FilPVal));
extern tp_LocPVal FilPVal_ValLocPVal(GMC_P1(tp_FilPVal));
extern void Set_FilPVal_DataLocHdr(GMC_P1(tp_FilPVal) GMC_PN(tp_LocHdr));
extern tp_LocHdr FilPVal_DataLocHdr(GMC_P1(tp_FilPVal));
extern tp_FilPVal FilPVal_Father(GMC_P1(tp_FilPVal));
extern tp_FilPVal FilPVal_DerefPrmVal(GMC_P1(tp_FilPVal)
                                      GMC_PN(tp_PrmTyp));
extern void Chain_FilPVal_DerefPrmVal(GMC_P1(tp_LocInp *)
                                      GMC_PN(tp_LocInp *)
                                      GMC_PN(tp_FilPVal)
                                      GMC_PN(tp_FilHdr));
/* if-filtyp.c */
extern tp_TClass Tool_TClass(GMC_P1(tp_Tool));
extern tp_InpEdg Tool_InpEdg(GMC_P1(tp_Tool));
extern tp_Package Tool_Package(GMC_P1(tp_Tool));
extern boolean IsDerefInput_Tool(GMC_P1(tp_Tool));
extern boolean IsReport_Tool(GMC_P1(tp_Tool));
extern boolean IsDerefPrmVal_Tool(GMC_P1(tp_Tool));
extern tp_Status Get_ToolStatus(GMC_P1(tp_Tool) GMC_PN(tp_Status));
extern tp_FilTyp Key_FilTyp(GMC_P1(tp_Key));
extern void Key_InstanceLabel(GMC_P1(tp_Str) GMC_PN(tp_Key));
extern tp_FilTyp FTName_FilTyp(GMC_P1(tp_FTName));
extern void Build_Label(GMC_P1(tp_Str) GMC_PN(tp_Ident) GMC_PN(tp_FilTyp)
                        GMC_PN(tp_LocHdr) GMC_PN(boolean));
extern tp_LocHdr CacheFileName_LocHdr(GMC_P1(tp_FileName));
extern void SetFilHdr_DrvMarks(GMC_P1(tp_FilHdr));
extern void SetFilHdr_Marks(GMC_P1(tp_FilHdr) GMC_PN(boolean));
extern void SetFilTyp_Marks(GMC_P1(tp_FilTyp) GMC_PN(boolean)
                            GMC_PN(boolean));
extern void SetFilTyp_Mark(GMC_P1(tp_FilTyp));
extern void WriteSrcFilTyps(GMC_P1(tp_FilDsc) GMC_PN(boolean));
extern void Clr_FilTypMarks(GMC_P1(void));
extern void WriteMarkedFilTyps(GMC_P1(tp_FilDsc));
extern tp_FilTyp Nod_FilTyp(GMC_P1(tp_Nod));
/* if-ft.c */
extern boolean IsPntr_FKind(GMC_P1(tp_FKind));
extern boolean CanPntrHo_FKind(GMC_P1(tp_FKind));
extern boolean IsATgt_FKind(GMC_P1(tp_FKind));
extern boolean IsVTgt_FKind(GMC_P1(tp_FKind));
extern boolean IsATgtText_FKind(GMC_P1(tp_FKind));
extern boolean IsVTgtText_FKind(GMC_P1(tp_FKind));
extern boolean IsExternal_Tool(GMC_P1(tp_Tool));
extern tp_FTName FilTyp_ShortFTName(GMC_P1(tp_FilTyp));
extern tp_FTName FilTyp_FTName(GMC_P1(tp_FilTyp));
extern tp_MemEdg FilTyp_MemEdg(GMC_P1(tp_FilTyp));
extern tp_CastEdg FilTyp_CastEdg(GMC_P1(tp_FilTyp));
extern tp_PrmTypLst FilTyp_MapPrmTypLst(GMC_P1(tp_FilTyp));
extern tp_FilTyp FilTyp_ArgFilTyp(GMC_P1(tp_FilTyp));
extern tp_Tool FilTyp_Tool(GMC_P1(tp_FilTyp));
extern boolean IsCopy_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsGrouping_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsGroupingInput_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsSecOrd_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsExec_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsVoid_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsAtmc_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsPntr_FilTyp(GMC_P1(tp_FilTyp));
extern int IsList_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsDrvDir_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsStruct_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsStructMem_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsGeneric_FilTyp(GMC_P1(tp_FilTyp));
extern boolean IsPipe_FilTyp(GMC_P1(tp_FilTyp));
/* if-get.c */
extern void GetAllReqs(GMC_P1(tp_FilHdr) GMC_PN(tp_InpKind));
/* if-help.c */
extern void Do_Help(GMC_P1(boolean *) GMC_PN(boolean *) GMC_PN(boolean *)
                    GMC_PN(tp_Nod));
extern void Local_Next_OdinFile(GMC_P1(tp_Str) GMC_PN(int));
/* if-hook.c */
extern void NestedHooks(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr)
                        GMC_PN(tp_FilDsc) GMC_PN(tp_FilDsc)
                        GMC_PN(tp_FilPrm));
extern void ExpandHooks(GMC_P1(tp_FilDsc) GMC_PN(tp_FilDsc)
                        GMC_PN(tp_FilHdr));
/* if-info.c */
extern void Hash_Item(GMC_P1(tp_Item) GMC_PN(tp_Loc));
extern void UnHash_Item(GMC_P1(tp_Item));
extern tp_Item Lookup_Item(GMC_P1(tp_Loc));
extern void Append_DataNum(GMC_P1(tp_Str) GMC_PN(int));
extern tp_Loc Alloc(GMC_P1(int));
extern tp_LocStr WriteStr(GMC_P1(tp_Str));
extern tp_Str ReadStr(GMC_P1(tp_LocStr));
extern void WritePrmInf(GMC_P1(tp_PrmInf) GMC_PN(tp_LocPrm));
extern void ReadPrmInf(GMC_P1(tp_PrmInf) GMC_PN(tp_LocPrm));
extern void WritePValInf(GMC_P1(tp_PValInf) GMC_PN(tp_LocPVal));
extern void ReadPValInf(GMC_P1(tp_PValInf) GMC_PN(tp_LocPVal));
extern void WriteHdrInf(GMC_P1(tp_HdrInf) GMC_PN(tp_LocHdr));
extern void ReadHdrInf(GMC_P1(tp_HdrInf) GMC_PN(tp_LocHdr));
extern void WriteInpInf(GMC_P1(tp_InpInf) GMC_PN(tp_LocInp));
extern void ReadInpInf(GMC_P1(tp_InpInf) GMC_PN(tp_LocInp));
extern void WriteElmInf(GMC_P1(tp_ElmInf) GMC_PN(tp_LocElm));
extern void ReadElmInf(GMC_P1(tp_ElmInf) GMC_PN(tp_LocElm));
extern void Init_Info(GMC_P1(boolean *));
extern void Close_Info(GMC_P1(void));
extern void Update_Info(GMC_P1(void));
/* if-io.c */
extern void Init_IO(GMC_P1(void));
extern boolean GetIsTTY(GMC_P1(void));
extern tp_FilDsc FileName_CFilDsc(GMC_P1(tp_FileName));
extern tp_FilDsc FileName_WFilDsc(GMC_P1(tp_FileName) GMC_PN(boolean));
extern tp_FilDsc FileName_WBFilDsc(GMC_P1(tp_FileName) GMC_PN(boolean));
extern tp_FilDsc FileName_AFilDsc(GMC_P1(tp_FileName) GMC_PN(boolean));
extern tp_FilDsc FileName_RFilDsc(GMC_P1(tp_FileName) GMC_PN(boolean));
extern tp_FilDsc FileName_RWFilDsc(GMC_P1(tp_FileName) GMC_PN(boolean));
extern tp_FilDsc FileName_RWBFilDsc(GMC_P1(tp_FileName) GMC_PN(boolean));
extern void Flush(GMC_P1(tp_FilDsc));
extern void Rewind(GMC_P1(tp_FilDsc));
extern void Unwind(GMC_P1(tp_FilDsc));
extern void Close(GMC_P1(tp_FilDsc));
extern boolean EndOfFile(GMC_P1(tp_FilDsc));
extern void Write(GMC_P1(tp_FilDsc) GMC_PN(tp_Str));
extern void Writech(GMC_P1(tp_FilDsc) GMC_PN(char));
extern void WriteInt(GMC_P1(tp_FilDsc) GMC_PN(int));
extern void Writeln(GMC_P1(tp_FilDsc) GMC_PN(const char *));
extern void WriteLine(GMC_P1(tp_FilDsc) GMC_PN(tp_Str));
extern int Readch(GMC_P1(tp_FilDsc));
extern tp_Str Readln(GMC_P1(tp_Str) GMC_PN(tp_FilDsc));
extern tp_Str ReadLine(GMC_P1(tp_Str) GMC_PN(tp_FilDsc));
extern int Equal(GMC_P1(tp_FilDsc) GMC_PN(tp_FilDsc));
extern void FileCopy(GMC_P1(tp_FilDsc) GMC_PN(tp_FilDsc));
/* if-ipc.c */
extern boolean IsServerPId(GMC_P1(int));
extern void IPC_Init(GMC_P1(void));
extern int IPC_Read(GMC_P1(int) GMC_PN(char *)GMC_PN(int));
extern void IPC_Get_Commands(GMC_P1(boolean *) GMC_PN(char *));
extern void IPC_Write_Int(GMC_P1(boolean *) GMC_PN(int));
extern void IPC_Read_Int(GMC_P1(boolean *) GMC_PN(int *));
extern void IPC_Write_Str(GMC_P1(boolean *) GMC_PN(const char *));
extern void IPC_Read_Str(GMC_P1(boolean *) GMC_PN(char *));
extern void IPC_Do_Abort(GMC_P1(void));
extern void IPC_Close(GMC_P1(tp_ClientID));
extern void IPC_Finish(GMC_P1(void));
/* if-lex.c */
extern boolean IsWordChr(GMC_P1(char));
extern void FileError(GMC_P1(tp_Str));
extern void ParseError(GMC_P1(tp_Str));
extern void Init_Lex(GMC_P1(void));
extern void EndLex(GMC_P1(void));
extern int Lex(GMC_P1(void));
extern void Unlex(GMC_P1(tp_Str) GMC_PN(tp_Str));
extern void Print_Unlex(GMC_P1(tp_FilDsc) GMC_PN(tp_Str));
/* if-lvl.c */
extern boolean IsSubType(GMC_P1(tp_FilTyp) GMC_PN(tp_FilTyp));
extern void Do_Search(GMC_P1(tp_DrvPth *) GMC_PN(boolean *)
                      GMC_PN(tp_FKind) GMC_PN(tp_FilTyp)
                      GMC_PN(tp_FilTyp));
/* if-main.c */
extern void InterruptAction(GMC_P1(void));
extern void TopLevelCI(GMC_P1(boolean *) GMC_PN(tp_Str));
extern void Get_Commands(GMC_P1(boolean *));
extern int main(GMC_P1(int) GMC_PN(char **));
/* if-nod.c */
extern void Ret_Nod(GMC_P1(tp_Nod));
extern tp_NodTyp Nod_NodTyp(GMC_P1(tp_Nod));
extern void Set_Nod_NodTyp(GMC_P1(tp_Nod) GMC_PN(tp_NodTyp));
extern tp_Nod Nod_FirstSon(GMC_P1(tp_Nod));
extern void Set_Nod_FirstSon(GMC_P1(tp_Nod) GMC_PN(tp_Nod));
extern tp_Nod Nod_Brother(GMC_P1(tp_Nod));
extern void Set_Nod_Brother(GMC_P1(tp_Nod) GMC_PN(tp_Nod));
extern int Nod_NumSons(GMC_P1(tp_Nod));
extern tp_Nod Nod_Son(GMC_P1(int) GMC_PN(tp_Nod));
extern tp_Sym Nod_Sym(GMC_P1(tp_Nod));
extern void Set_Nod_Sym(GMC_P1(tp_Nod) GMC_PN(tp_Sym));
extern void Push_SymStack(GMC_P1(tp_Sym));
extern void Init_ConstructTree(GMC_P1(void));
extern tp_Nod End_ConstructTree(GMC_P1(void));
extern void Action(GMC_P1(int) GMC_PN(int));
/* if-oclex.c */
extern tp_Nod OC_Parser(GMC_P1(tp_Str) GMC_PN(tp_FileName) GMC_PN(int *));
extern int OC_Lex(GMC_P1(void));
extern void OC_Unparse(GMC_P1(tp_Str) GMC_PN(tp_Nod));
/* if-pfilhdr.c */
extern tp_PrmFHdr New_PrmFHdr(GMC_P1(tp_FilHdr) GMC_PN(tp_FilPrm));
extern void Use_PrmFHdr(GMC_P1(tp_FilHdr *) GMC_PN(tp_FilPrm *)
                        GMC_PN(tp_PrmFHdr));
extern boolean PrmFHdrs_InUse(GMC_P1(void));
/* if-prmtyp.c */
extern tp_PTName PrmTyp_PTName(GMC_P1(tp_PrmTyp));
extern tp_FilTyp PrmTyp_FilTyp(GMC_P1(tp_PrmTyp));
extern boolean IsFirst_PrmTyp(GMC_P1(tp_PrmTyp));
extern int PrmTyp_I(GMC_P1(tp_PrmTyp));
extern void SetPrmTyp_Mark(GMC_P1(tp_PrmTyp));
extern tp_FilHdr PrmTyp_StrDirFilHdr(GMC_P1(tp_PrmTyp));
extern tp_FilPVal PrmTyp_RootFilPVal(GMC_P1(tp_PrmTyp));
extern void SetPrmTyp_RootLocPVal(GMC_P1(tp_PrmTyp) GMC_PN(tp_LocPVal));
extern void SetFilHdr_PrmTypMarks(GMC_P1(tp_FilHdr));
extern void Clr_PrmTypMarks(GMC_P1(void));
extern void WriteMarkedPrmTyps(GMC_P1(tp_FilDsc));
extern tp_PrmTyp Nod_PrmTyp(GMC_P1(tp_Nod));
/* if-rbs.c */
extern tp_Str Host_HostName(GMC_P1(tp_Host));
extern int Host_FD(GMC_P1(tp_Host));
extern tp_Host Host_Next(GMC_P1(tp_Host));
extern tp_Host Lookup_Host(GMC_P1(tp_Str));
extern tp_Host PId_Host(GMC_P1(int));
extern void RBS_Done(GMC_P1(tp_Host));
extern void RBS_Get_Msg(GMC_P1(tp_Host));
extern void RBS_Do_Build(GMC_P1(tp_Host) GMC_PN(int) GMC_PN(tp_FileName)
                         GMC_PN(tp_FileName) GMC_PN(char **));
extern void RBS_Abort_Build(GMC_P1(tp_Host) GMC_PN(int));
extern void RBS_VarDef(GMC_P1(tp_Str));
/* if-symbol.c */
extern tp_Str GetEnv(GMC_P1(tp_Str));
extern tp_Str Malloc_Str(GMC_P1(const char *));
extern boolean Is_EmptyStr(GMC_P1(tp_Str));
extern int Str_PosInt(GMC_P1(tp_Str));
extern tp_Str Tail(GMC_P1(tp_Str));
extern void StrShift(GMC_P1(tp_Str) GMC_PN(int));
extern tp_Sym Str_Sym(GMC_P1(tp_Str));
extern tp_Str Sym_Str(GMC_P1(tp_Sym));
extern int Sym_Att(GMC_P1(tp_Sym));
extern void Set_Sym_Att(GMC_P1(tp_Sym) GMC_PN(int));
extern void Write_Syms(GMC_P1(tp_FilDsc));
/* if-system.c */
extern void Init_Sigs(GMC_P1(boolean));
extern void Block_Signals(GMC_P1(void));
extern void Unblock_Signals(GMC_P1(void));
extern void Lose_ControlTTY(GMC_P1(void));
extern int
SystemExec(GMC_P1(const char *)GMC_PN(char *const *)GMC_PN(const char *));
extern int SystemExecCmd(GMC_P1(const char *)GMC_PN(boolean));
extern void SystemWait(GMC_P1(int *)GMC_PN(boolean *));
extern void SystemInterrupt(GMC_P1(int));
extern tp_Str GetHome(GMC_P1(tp_Str));
extern int Await_Event(GMC_P1(fd_set *) GMC_PN(boolean));
/* if-systools.c */
extern void WriteCat(GMC_P1(tp_FilDsc) GMC_PN(tp_FilHdr));
extern void WriteFlat(GMC_P1(tp_FilDsc) GMC_PN(tp_FilHdr));
extern void WriteNames(GMC_P1(tp_FilDsc) GMC_PN(tp_FilHdr)
                       GMC_PN(tp_FilPrm));
extern void WriteLabels(GMC_P1(tp_FilDsc) GMC_PN(tp_FilHdr));
extern tp_LocElm Make_UnionLocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr));
extern void Clr_UnionFlags(GMC_P1(tp_FilHdr));
extern void Exec_List(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr) GMC_PN(tp_FilPrm)
                      GMC_PN(boolean));
extern void Exec_TargetsPtr(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr));
extern void Exec_Targets(GMC_P1(tp_FilDsc) GMC_PN(tp_FileName));
extern void WriteSrcNames(GMC_P1(tp_FilDsc) GMC_PN(tp_FileName)
                          GMC_PN(boolean));
extern void Validate_ViewSpec(GMC_P1(tp_FilHdr));
extern tp_FilElm FilElm_NextStrFilElm(GMC_P1(tp_FilElm));
extern void Exec_CmptView(GMC_P1(boolean *) GMC_PN(tp_FilHdr)
                          GMC_PN(tp_FilHdr));
extern void Install_ActTgt(GMC_P1(tp_FilHdr));
extern void Uninstall_ActTgt(GMC_P1(tp_FilHdr));
extern void WriteTextDef(GMC_P1(tp_FilHdr) GMC_PN(tp_FilDsc)
                         GMC_PN(tp_FileName) GMC_PN(tp_FilDsc)
                         GMC_PN(tp_FileName));
extern tp_LocElm Make_TargetsLocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_FilDsc)
                                    GMC_PN(tp_FileName) GMC_PN(tp_Date)
                                    GMC_PN(boolean));
extern void Exec_VirDir(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr));
extern void FilPVal_LocElm(GMC_P1(tp_LocElm *) GMC_PN(tp_LocElm *)
                           GMC_PN(tp_FilPVal) GMC_PN(tp_FilHdr));
extern tp_LocElm Make_PntrHoLocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr));
extern tp_LocElm Make_DerefPrmValLocElm(GMC_P1(tp_FilHdr)
                                        GMC_PN(tp_FilHdr));
extern tp_LocElm Make_RecurseLocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr));
extern tp_LocElm Make_ExDelLocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_FilHdr)
                                  GMC_PN(boolean));
/* if-update.c */
extern void Do_Update(GMC_P1(tp_FilHdr) GMC_PN(tp_OutFilHdrs) GMC_PN(int)
                      GMC_PN(tp_Job) GMC_PN(tp_Status) GMC_PN(tp_Date)
                      GMC_PN(boolean));
extern void Validate_IsPntr(GMC_P1(tp_FilHdr));
extern void Update_RefFile(GMC_P1(tp_FilHdr) GMC_PN(tp_Status)
                           GMC_PN(tp_Date));
extern void Set_DrvDirConfirm(GMC_P1(tp_FilHdr) GMC_PN(tp_Status));
extern void Set_ListStatus(GMC_P1(tp_FilHdr) GMC_PN(tp_Status));
/* if-util.c */
extern void Local_Redo(GMC_P1(tp_Str));
extern void Local_OdinExpr_ID(GMC_P1(int *)GMC_PN(tp_Str));
extern void Local_ID_OdinExpr(GMC_P1(tp_Str) GMC_PN(int));
extern void Local_ID_LongOdinExpr(GMC_P1(tp_Str) GMC_PN(int));
extern void Do_Log(GMC_P1(tp_Str) GMC_PN(tp_FilHdr) GMC_PN(tp_LogLevel));
extern tp_FilHdr OdinExpr_FilHdr(GMC_P1(const char *));
extern void WritePrmOdinExpr(GMC_P1(tp_FilDsc) GMC_PN(tp_FilHdr)
                             GMC_PN(tp_FilPrm));
extern void Local_Set_Debug(GMC_P1(tp_Str));
extern void Local_Get_Status(GMC_P1(tp_Status *) GMC_PN(tp_Status *)
                             GMC_PN(int));
extern void Local_Get_Elements(GMC_P1(int));
extern void Local_Get_ElementsOf(GMC_P1(int));
extern void Local_Get_Inputs(GMC_P1(int));
extern void Local_Get_Outputs(GMC_P1(int));
extern void Debug_Alloc_ElmInf(GMC_P1(tp_LocElm) GMC_PN(tp_LocElm));
extern void Debug_Ret_ElmInf(GMC_P1(tp_LocElm));
extern void Validate_LocElm(GMC_P1(tp_FilHdr) GMC_PN(tp_LocElm));
extern void Print_OdinExpr(GMC_P1(tp_LocHdr) GMC_PN(tp_FilPrm));
extern void printte(GMC_P1(tp_LocHdr));
/* if-var.c */
extern void Init_Vars(GMC_P1(void));
extern void Local_LogMessage(GMC_P1(char *));
extern void Local_FileErrMessage(GMC_P1(tp_FileName));
extern void ShowVars(GMC_P1(void));
extern void HelpVar(GMC_P1(tp_Nod));
extern void ShowVar(GMC_P1(tp_Nod));
extern void SetVar(GMC_P1(boolean *) GMC_PN(tp_Str) GMC_PN(tp_Str));
extern void Set_HostVar(GMC_P1(boolean *) GMC_PN(tp_Str) GMC_PN(tp_Str));
/* if-yylex.c */
extern tp_Nod YY_Parser(GMC_P1(const char *)GMC_PN(tp_FileName)
                        GMC_PN(int *));
extern int YY_Lex(GMC_P1(void));
extern void YY_Unparse(GMC_P1(tp_Str) GMC_PN(tp_Nod));
/* client.yacc.c */
extern tp_Nod OC_Parse(GMC_P1(void));
extern void ocerror(GMC_P1(char *));
extern int oclex(GMC_P1(void));
extern int ocparse(GMC_P1(void));
/* fsys.yacc.c */
extern tp_Nod YY_Parse(GMC_P1(void));
extern void yyerror(GMC_P1(char *));
extern int yylex(GMC_P1(void));
extern int yyparse(GMC_P1(void));
/* stub.in.c */
extern void IPC_Do_Msg(GMC_P1(boolean *) GMC_PN(int));
extern void LocalEnd_Get_OdinFile(GMC_P1(tp_FileName) GMC_PN(tp_Status)
                                  GMC_PN(boolean));
/* stub.out.c */
extern void Add_BuildArg(GMC_P1(tp_FileName));
extern void Do_Build(GMC_P1(tp_JobID) GMC_PN(tp_FileName)
                     GMC_PN(tp_FileName));
extern void Abort_Build(GMC_P1(tp_JobID));
extern void Do_MakeReadOnly(GMC_P1(tp_FileName));
extern void ErrMessage(GMC_P1(char *));
extern void LogMessage(GMC_P1(char *));
extern void FileErrMessage(GMC_P1(tp_FileName));
extern void Next_OdinFile(GMC_P1(tp_Str) GMC_PN(int));
extern void Get_UseCount(GMC_P1(int *));
extern void Get_CurSize(GMC_P1(int *));
extern void ShutDown(GMC_P1(void));
extern void Get_Banner(GMC_P1(tp_Str));
extern void Do_Interrupt(GMC_P1(boolean));
extern void Do_Alias(GMC_P1(tp_FileName) GMC_PN(boolean));
extern void Get_Alias(GMC_P1(tp_FileName) GMC_PN(tp_FileName));
extern void Job_Done(GMC_P1(tp_JobID) GMC_PN(boolean));
extern void Test(GMC_P1(tp_FileName));
extern void Test_All(GMC_P1(void));
extern void Get_OdinFile(GMC_P1(tp_FileName) GMC_PN(tp_Status *)
                         GMC_PN(boolean *) GMC_PN(tp_Str) GMC_PN(boolean));
extern void Set_CWD(GMC_P1(tp_FileName));
extern void Push_Context(GMC_P1(tp_FileName) GMC_PN(tp_FileName));
extern void Pop_Context(GMC_P1(tp_FileName));
extern void Set_KeepGoing(GMC_P1(int));
extern void Set_ErrLevel(GMC_P1(int));
extern void Set_WarnLevel(GMC_P1(int));
extern void Set_LogLevel(GMC_P1(tp_LogLevel));
extern void Set_HelpLevel(GMC_P1(int));
extern void Set_Debug(GMC_P1(tp_Str));
extern void Set_MaxJobs(GMC_P1(int));
extern void Redo(GMC_P1(tp_Str));
extern void OdinExpr_ID(GMC_P1(int *)GMC_PN(tp_Str));
extern void ID_OdinExpr(GMC_P1(tp_Str) GMC_PN(int));
extern void ID_LongOdinExpr(GMC_P1(tp_Str) GMC_PN(int));
extern void Get_Status(GMC_P1(tp_Status *) GMC_PN(tp_Status *)
                       GMC_PN(int));
extern void Get_Elements(GMC_P1(int));
extern void Get_ElementsOf(GMC_P1(int));
extern void Get_Inputs(GMC_P1(int));
extern void Get_Outputs(GMC_P1(int));
extern void Get_DPath(GMC_P1(tp_Str));
/* editline/complete.c */
extern char *rl_complete(GMC_P1(char *)GMC_PN(int *));
extern int rl_list_possib(GMC_P1(char *)GMC_PN(char ***));
/* editline/editline.c */
extern void rl_reset_terminal(GMC_P1(char *));
extern void rl_initialize(GMC_P1(void));
extern char *readline(GMC_P1(const char *));
extern void add_history(GMC_P1(char *));
/* editline/sysunix.c */
extern void rl_ttyset(GMC_P1(int));
extern void rl_add_slash(GMC_P1(char *)GMC_PN(char *));
