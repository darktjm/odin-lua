/* dg-analyze.c */
extern void Set_FTClasses(GMC_P1(void));
extern boolean Is_Output(GMC_P1(tp_FilTyp) GMC_PN(tp_Tool));
extern void Make_DrvEdgs(GMC_P1(void));
extern void Make_PrmTypLsts(GMC_P1(void));
/* dg-build.c */
extern void Init_InpSpcs(GMC_P1(void));
extern void Build_DerivationGraph(GMC_P1(tp_FileName) GMC_PN(tp_Package));
extern void Write_ENV(GMC_P1(void));
extern void Write_DerivationGraph(GMC_P1(void));
/* dg-edg-c */
extern void Clear_EdgFlags(GMC_P1(void));
extern tp_FilTyp EqvEdg_FilTyp(GMC_P1(tp_EqvEdg));
extern tp_EqvEdg EqvEdg_Next(GMC_P1(tp_EqvEdg));
extern tp_FilTyp EqvEdg_FrmFilTyp(GMC_P1(tp_EqvEdg));
extern tp_EqvEdg EqvEdg_FrmNext(GMC_P1(tp_EqvEdg));
extern tp_FilTyp CastEdg_FilTyp(GMC_P1(tp_CastEdg));
extern tp_CastEdg CastEdg_Next(GMC_P1(tp_CastEdg));
extern tp_FilTyp DrvEdg_FrmFilTyp(GMC_P1(tp_DrvEdg));
extern tp_DrvEdg DrvEdg_FrmNext(GMC_P1(tp_DrvEdg));
extern tp_DrvEdg DrvEdg_Next(GMC_P1(tp_DrvEdg));
extern tp_FilTyp InpEdg_FilTyp(GMC_P1(tp_InpEdg));
extern tp_InpEdg InpEdg_Next(GMC_P1(tp_InpEdg));
extern void Add_InpEdg(GMC_P1(tp_InpSpc) GMC_PN(tp_InpKind) GMC_PN(boolean)
                       GMC_PN(tp_Tool));
extern void Add_HomInpEdg(GMC_P1(tp_InpSpc) GMC_PN(tp_Tool));
extern boolean HasInput(GMC_P1(tp_Tool));
extern void Set_SystemTool_InpKinds(GMC_P1(tp_Tool) GMC_PN(tp_InpKind));
extern void Set_SystemTool_InpKind(GMC_P1(tp_Tool) GMC_PN(int)
                                   GMC_PN(tp_InpKind));
extern tp_FilTyp MemEdg_FilTyp(GMC_P1(tp_MemEdg));
extern void Add_MemEdg(GMC_P1(tp_FilTyp) GMC_PN(tp_FilTyp));
extern void Add_EqvEdg(GMC_P1(tp_FilTyp) GMC_PN(tp_FilTyp));
extern void Add_CastEdg(GMC_P1(tp_FilTyp) GMC_PN(tp_FilTyp));
extern void Add_DrvEdg(GMC_P1(tp_FilTyp) GMC_PN(tp_FilTyp));
extern tp_PrmTypLst DrvEdg_PrmTypLst(GMC_P1(tp_DrvEdg));
extern void Print_DrvEdg(GMC_P1(tp_FilDsc) GMC_PN(tp_DrvEdg));
extern void Print_MemEdgs(GMC_P1(tp_FilDsc) GMC_PN(tp_MemEdg));
extern void Write_Edgs(GMC_P1(FILE *) GMC_PN(FILE *));
/* dg-envvar.c */
extern void Init_EnvVars(GMC_P1(void));
extern tp_EnvVar Lookup_EnvVar(GMC_P1(tp_Str));
extern tp_Desc EnvVar_Desc(GMC_P1(tp_EnvVar));
extern void Set_EnvVar_Desc(GMC_P1(tp_EnvVar) GMC_PN(tp_Desc)
                            GMC_PN(boolean));
extern void Set_EnvVar_Default(GMC_P1(tp_EnvVar) GMC_PN(tp_Str)
                               GMC_PN(boolean));
extern tp_EnvVarLst EnvVarLst_Next(GMC_P1(tp_EnvVarLst));
extern tp_EnvVarLst Make_EnvVarLst(GMC_P1(tp_EnvVar));
extern tp_EnvVarLst Union_EnvVarLst(GMC_P1(tp_EnvVarLst)
                                    GMC_PN(tp_EnvVarLst));
extern void Print_EnvVarLst(GMC_P1(tp_FilDsc) GMC_PN(tp_EnvVarLst));
extern void Write_EnvVars(GMC_P1(FILE *) GMC_PN(FILE *));
extern tp_InpSpc New_InpSpc(GMC_P1(void));
extern void Set_InpSpc_FilTyp(GMC_P1(tp_InpSpc) GMC_PN(tp_FilTyp));
extern tp_PrmTyp InpSpc_PrmTyp(GMC_P1(tp_InpSpc));
extern void Set_InpSpc_PrmTyp(GMC_P1(tp_InpSpc) GMC_PN(tp_PrmTyp));
extern tp_InpSpc InpSpc_Next(GMC_P1(tp_InpSpc));
extern void Write_InpSpcs(GMC_P1(FILE *) GMC_PN(FILE *));
/* dg-filtyp.c */
extern void Init_FilTyps(GMC_P1(void));
extern void Set_Tool(GMC_P1(tp_FilTyp) GMC_PN(tp_Tool));
extern tp_Desc FilTyp_Desc(GMC_P1(tp_FilTyp));
extern void Set_FilTyp_Desc(GMC_P1(tp_FilTyp) GMC_PN(tp_Desc)
                            GMC_PN(boolean));
extern int FilTyp_Index(GMC_P1(tp_FilTyp));
extern boolean Has_SubTypes(GMC_P1(tp_FilTyp));
extern tp_FilTyp Create_StructFilTyp(GMC_P1(tp_Package) GMC_PN(tp_FTName));
extern tp_FilTyp Create_OutputFilTyp(GMC_P1(tp_Package) GMC_PN(tp_Package)
                                     GMC_PN(tp_FTName));
extern tp_FilTyp Lookup_SecOrdFilTyp(GMC_P1(tp_FTName) GMC_PN(tp_FilTyp));
extern tp_FilTyp Lookup_FilTyp(GMC_P1(tp_FTName));
extern tp_SrcTyp Lookup_SrcTyp(GMC_P1(tp_Pattern) GMC_PN(boolean));
extern void Set_SrcTyp_FilTyp(GMC_P1(tp_SrcTyp) GMC_PN(tp_FilTyp));
extern tp_FilTyp SrcTyp_FilTyp(GMC_P1(tp_SrcTyp));
extern tp_FilTyp Key_FilTyp(GMC_P1(tp_Key));
extern void Clear_FilTypFlags(GMC_P1(void));
extern void Print_Info(GMC_P1(tp_FilDsc));
extern void Print_FilTyp(GMC_P1(tp_FilDsc) GMC_PN(tp_FilTyp));
extern void Write_FilTyps(GMC_P1(FILE *) GMC_PN(FILE *));
/* dg-main.c */
extern int main(GMC_P1(int) GMC_PN(char **));
/* dg-prmtyp.c */
extern void Init_PrmTyps(GMC_P1(void));
extern tp_PTName PrmTyp_PTName(GMC_P1(tp_PrmTyp));
extern tp_Desc PrmTyp_Desc(GMC_P1(tp_PrmTyp));
extern void Set_PrmTyp_Desc(GMC_P1(tp_PrmTyp) GMC_PN(tp_Desc)
                            GMC_PN(boolean));
extern tp_FilTyp PrmTyp_FilTyp(GMC_P1(tp_PrmTyp));
extern void Set_PrmTyp_FilTyp(GMC_P1(tp_PrmTyp) GMC_PN(tp_FilTyp));
extern tp_PrmTyp Lookup_PrmTyp(GMC_P1(tp_PTName));
extern void Print_PrmTyp(GMC_P1(tp_FilDsc) GMC_PN(tp_PrmTyp));
extern tp_PrmTypLst PrmTypLst_Next(GMC_P1(tp_PrmTypLst));
extern tp_PrmTypLst Make_PrmTypLst(GMC_P1(tp_PrmTyp));
extern tp_PrmTypLst Union_PrmTypLst(GMC_P1(tp_PrmTypLst)
                                    GMC_PN(tp_PrmTypLst));
extern void Print_PrmTypLst(GMC_P1(tp_FilDsc) GMC_PN(tp_PrmTypLst));
extern void Write_PrmTyps(GMC_P1(FILE *) GMC_PN(FILE *));
/* dg-tool.c */
extern tp_Tool New_Tool(GMC_P1(void));
extern void Init_Tools(GMC_P1(void));
extern tp_EnvVarLst Tool_EnvVarLst(GMC_P1(tp_Tool));
extern void Set_Tool_EnvVarLst(GMC_P1(tp_Tool) GMC_PN(tp_EnvVarLst));
extern void Clear_ToolFlags(GMC_P1(void));
extern tp_Str Tool_Name(GMC_P1(tp_Tool));
extern boolean IsDummy_Tool(GMC_P1(tp_Tool));
extern void Write_Tools(GMC_P1(FILE *) GMC_PN(FILE *));
/* dg-valid.c */
extern void Validate_DerivationGraph(GMC_P1(void));
/* dg-yylex.c */
extern void Init_Parse(GMC_P1(void));
extern void ParseError(GMC_P1(tp_Str));
extern void Init_Lex(GMC_P1(void));
extern void EndLex(GMC_P1(void));
extern int YY_Lex(GMC_P1(void));
extern void YY_Unparse(GMC_P1(tp_Str) GMC_PN(tp_Nod));
/* drvgrf.yacc.c */
extern tp_Nod YY_Parse(GMC_P1(void));
extern void yyerror(GMC_P1(char *));
extern int yylex(GMC_P1(void));
extern int yyparse(GMC_P1(void));
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
/* if-lvl.c */
extern boolean IsSubType(GMC_P1(tp_FilTyp) GMC_PN(tp_FilTyp));
extern void Do_Search(GMC_P1(tp_DrvPth *) GMC_PN(boolean *)
                      GMC_PN(tp_FKind) GMC_PN(tp_FilTyp)
                      GMC_PN(tp_FilTyp));
/* if.err.c */
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
/* if-file.c */
extern void Set_ModeMask(GMC_P1(tp_FileName));
/* extern void Get_FileInfo(GMC_P1(tp_SKind *) GMC_PN(int *) GMC_PN(tp_FileName)); */
extern void MakePlnFile(GMC_P1(boolean *) GMC_PN(tp_FileName));
extern void MakeDirFile(GMC_P1(boolean *) GMC_PN(tp_FileName));
extern void GetWorkingDir(GMC_P1(boolean *) GMC_PN(tp_FileName));
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
/* if-io.c */
extern void Init_IO(GMC_P1(void));
extern boolean GetIsTTY(GMC_P1(void));
extern tp_FilDsc FileName_CFilDsc(GMC_P1(tp_FileName));
extern tp_FilDsc FileName_WFilDsc(GMC_P1(tp_FileName) GMC_PN(boolean));
extern tp_FilDsc FileName_AFilDsc(GMC_P1(tp_FileName) GMC_PN(boolean));
extern tp_FilDsc FileName_RFilDsc(GMC_P1(tp_FileName) GMC_PN(boolean));
extern tp_FilDsc FileName_RWFilDsc(GMC_P1(tp_FileName) GMC_PN(boolean));
extern void Flush(GMC_P1(tp_FilDsc));
extern void Rewind(GMC_P1(tp_FilDsc));
extern void Unwind(GMC_P1(tp_FilDsc));
extern void Close(GMC_P1(tp_FilDsc));
extern boolean EndOfFile(GMC_P1(tp_FilDsc));
extern void Write(GMC_P1(tp_FilDsc) GMC_PN(tp_Str));
extern void Writech(GMC_P1(tp_FilDsc) GMC_PN(int));
extern void WriteInt(GMC_P1(tp_FilDsc) GMC_PN(int));
extern void Writeln(GMC_P1(tp_FilDsc) GMC_PN(tp_Str));
extern void WriteLine(GMC_P1(tp_FilDsc) GMC_PN(tp_Str));
extern int Readch(GMC_P1(tp_FilDsc));
extern tp_Str Readln(GMC_P1(tp_Str) GMC_PN(tp_FilDsc));
extern tp_Str ReadLine(GMC_P1(tp_Str) GMC_PN(tp_FilDsc));
extern int Equal(GMC_P1(tp_FilDsc) GMC_PN(tp_FilDsc));
extern void FileCopy(GMC_P1(tp_FilDsc) GMC_PN(tp_FilDsc));
/* if-nod.c */
extern tp_Nod New_Nod(GMC_P1(void));
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
/* if-symbol.c */
extern tp_Str GetEnv(GMC_P1(tp_Str));
extern tp_Str Malloc_Str(GMC_P1(tp_Str));
extern boolean Is_EmptyStr(GMC_P1(tp_Str));
extern int Str_PosInt(GMC_P1(tp_Str));
extern tp_Str Tail(GMC_P1(tp_Str));
extern void StrShift(GMC_P1(tp_Str) GMC_PN(int));
extern tp_Sym Str_Sym(GMC_P1(tp_Str));
extern tp_Str Sym_Str(GMC_P1(tp_Sym));
extern int Sym_Att(GMC_P1(tp_Sym));
extern void Set_Sym_Att(GMC_P1(tp_Sym) GMC_PN(int));
extern void Write_Syms(GMC_P1(tp_FilDsc));
