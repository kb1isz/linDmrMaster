ProjectName            :=Master_Server
ConfigurationName      :=Release
IntermediateDirectory  :=./Release
OutDir                 := $(IntermediateDirectory)
WorkspacePath          := 
ProjectPath            := 
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=
Date                   :=
LinkerName             :=gcc
ArchiveTool            :=ar rcus
SharedObjectLinkerName :=gcc -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.o.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
CompilerName           :=gcc
C_CompilerName         :=gcc
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E 
MakeDirCommand         :=mkdir -p
CmpOptions             := -g $(Preprocessors)
LinkOptions            :=  
IncludePath            :=  "$(IncludeSwitch)." "$(IncludeSwitch)." 
RcIncludePath          :=
Libs                   :=$(LibrarySwitch)pthread $(LibrarySwitch)sqlite3 $(LibrarySwitch)m
LibPath                := "$(LibraryPathSwitch)." 


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects=$(IntermediateDirectory)/main$(ObjectSuffix) $(IntermediateDirectory)/dmr$(ObjectSuffix) $(IntermediateDirectory)/rdac$(ObjectSuffix) $(IntermediateDirectory)/smaster$(ObjectSuffix) $(IntermediateDirectory)/sqlite$(ObjectSuffix) $(IntermediateDirectory)/webserv$(ObjectSuffix) $(IntermediateDirectory)/htmlreplace$(ObjectSuffix) $(IntermediateDirectory)/htmlpost$(ObjectSuffix) $(IntermediateDirectory)/convbin$(ObjectSuffix) $(IntermediateDirectory)/BPTC1969$(ObjectSuffix) $(IntermediateDirectory)/decode34Rate$(ObjectSuffix) $(IntermediateDirectory)/hyteraDecode$(ObjectSuffix) $(IntermediateDirectory)/aprs$(ObjectSuffix)

##
## Main Build Targets 
##
all: $(OutputFile)

$(OutputFile): makeDirStep $(Objects)
	@$(MakeDirCommand) $(@D)
	$(LinkerName) $(OutputSwitch)$(OutputFile) $(Objects) $(LibPath) $(Libs) $(LinkOptions)

makeDirStep:
	@test -d ./Release || $(MakeDirCommand) ./Release

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/main$(ObjectSuffix): main.c $(IntermediateDirectory)/main$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "main.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/main$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main$(DependSuffix): main.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/main$(ObjectSuffix) -MF$(IntermediateDirectory)/main$(DependSuffix) -MM "main.c"

$(IntermediateDirectory)/main$(PreprocessSuffix): main.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main$(PreprocessSuffix) "main.c"

$(IntermediateDirectory)/dmr$(ObjectSuffix): dmr.c $(IntermediateDirectory)/dmr$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "dmr.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/dmr$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dmr$(DependSuffix): dmr.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/dmr$(ObjectSuffix) -MF$(IntermediateDirectory)/dmr$(DependSuffix) -MM "dmr.c"

$(IntermediateDirectory)/dmr$(PreprocessSuffix): dmr.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dmr$(PreprocessSuffix) "dmr.c"

$(IntermediateDirectory)/rdac$(ObjectSuffix): rdac.c $(IntermediateDirectory)/rdac$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "rdac.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/rdac$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/rdac$(DependSuffix): rdac.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/rdac$(ObjectSuffix) -MF$(IntermediateDirectory)/rdac$(DependSuffix) -MM "rdac.c"

$(IntermediateDirectory)/rdac$(PreprocessSuffix): rdac.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/rdac$(PreprocessSuffix) "rdac.c"

$(IntermediateDirectory)/smaster$(ObjectSuffix): smaster.c $(IntermediateDirectory)/smaster$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "smaster.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/smaster$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/smaster$(DependSuffix): smaster.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/smaster$(ObjectSuffix) -MF$(IntermediateDirectory)/smaster$(DependSuffix) -MM "smaster.c"

$(IntermediateDirectory)/smaster$(PreprocessSuffix): smaster.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/smaster$(PreprocessSuffix) "smaster.c"

$(IntermediateDirectory)/sqlite$(ObjectSuffix): sqlite.c $(IntermediateDirectory)/sqlite$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "sqlite.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/sqlite$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sqlite$(DependSuffix): sqlite.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/sqlite$(ObjectSuffix) -MF$(IntermediateDirectory)/sqlite$(DependSuffix) -MM "sqlite.c"

$(IntermediateDirectory)/sqlite$(PreprocessSuffix): sqlite.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sqlite$(PreprocessSuffix) "sqlite.c"

$(IntermediateDirectory)/webserv$(ObjectSuffix): webserv.c $(IntermediateDirectory)/webserv$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "webserv.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/webserv$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/webserv$(DependSuffix): webserv.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/webserv$(ObjectSuffix) -MF$(IntermediateDirectory)/webserv$(DependSuffix) -MM "webserv.c"

$(IntermediateDirectory)/webserv$(PreprocessSuffix): webserv.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/webserv$(PreprocessSuffix) "webserv.c"

$(IntermediateDirectory)/htmlreplace$(ObjectSuffix): htmlreplace.c $(IntermediateDirectory)/htmlreplace$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "htmlreplace.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/htmlreplace$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/htmlreplace$(DependSuffix): htmlreplace.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/htmlreplace$(ObjectSuffix) -MF$(IntermediateDirectory)/htmlreplace$(DependSuffix) -MM "htmlreplace.c"

$(IntermediateDirectory)/htmlreplace$(PreprocessSuffix): htmlreplace.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/htmlreplace$(PreprocessSuffix) "htmlreplace.c"

$(IntermediateDirectory)/htmlpost$(ObjectSuffix): htmlpost.c $(IntermediateDirectory)/htmlpost$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "htmlpost.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/htmlpost$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/htmlpost$(DependSuffix): htmlpost.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/htmlpost$(ObjectSuffix) -MF$(IntermediateDirectory)/htmlpost$(DependSuffix) -MM "htmlpost.c"

$(IntermediateDirectory)/htmlpost$(PreprocessSuffix): htmlpost.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/htmlpost$(PreprocessSuffix) "htmlpost.c"

$(IntermediateDirectory)/convbin$(ObjectSuffix): convbin.c $(IntermediateDirectory)/convbin$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "convbin.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/convbin$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/convbin$(DependSuffix): convbin.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/convbin$(ObjectSuffix) -MF$(IntermediateDirectory)/convbin$(DependSuffix) -MM "convbin.c"

$(IntermediateDirectory)/convbin$(PreprocessSuffix): convbin.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/convbin$(PreprocessSuffix) "convbin.c"

$(IntermediateDirectory)/BPTC1969$(ObjectSuffix): BPTC1969.c $(IntermediateDirectory)/BPTC1969$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "BPTC1969.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/BPTC1969$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/BPTC1969$(DependSuffix): BPTC1969.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/BPTC1969$(ObjectSuffix) -MF$(IntermediateDirectory)/BPTC1969$(DependSuffix) -MM "BPTC1969.c"

$(IntermediateDirectory)/BPTC1969$(PreprocessSuffix): BPTC1969.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/BPTC1969$(PreprocessSuffix) "BPTC1969.c"

$(IntermediateDirectory)/decode34Rate$(ObjectSuffix): decode34Rate.c $(IntermediateDirectory)/decode34Rate$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "decode34Rate.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/decode34Rate$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/decode34Rate$(DependSuffix): decode34Rate.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/decode34Rate$(ObjectSuffix) -MF$(IntermediateDirectory)/decode34Rate$(DependSuffix) -MM "decode34Rate.c"

$(IntermediateDirectory)/decode34Rate$(PreprocessSuffix): decode34Rate.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/decode34Rate$(PreprocessSuffix) "decode34Rate.c"

$(IntermediateDirectory)/hyteraDecode$(ObjectSuffix): hyteraDecode.c $(IntermediateDirectory)/hyteraDecode$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "hyteraDecode.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/hyteraDecode$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/hyteraDecode$(DependSuffix): hyteraDecode.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/hyteraDecode$(ObjectSuffix) -MF$(IntermediateDirectory)/hyteraDecode$(DependSuffix) -MM "hyteraDecode.c"

$(IntermediateDirectory)/hyteraDecode$(PreprocessSuffix): hyteraDecode.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/hyteraDecode$(PreprocessSuffix) "hyteraDecode.c"

$(IntermediateDirectory)/aprs$(ObjectSuffix): aprs.c $(IntermediateDirectory)/aprs$(DependSuffix)
	$(C_CompilerName) $(SourceSwitch) "aprs.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/aprs$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/aprs$(DependSuffix): aprs.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/aprs$(ObjectSuffix) -MF$(IntermediateDirectory)/aprs$(DependSuffix) -MM "aprs.c"

$(IntermediateDirectory)/aprs$(PreprocessSuffix): aprs.c
	@$(C_CompilerName) $(CmpOptions) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/aprs$(PreprocessSuffix) "aprs.c"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) $(IntermediateDirectory)/main$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/main$(DependSuffix)
	$(RM) $(IntermediateDirectory)/main$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/dmr$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/dmr$(DependSuffix)
	$(RM) $(IntermediateDirectory)/dmr$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/rdac$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/rdac$(DependSuffix)
	$(RM) $(IntermediateDirectory)/rdac$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/smaster$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/smaster$(DependSuffix)
	$(RM) $(IntermediateDirectory)/smaster$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/sqlite$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/sqlite$(DependSuffix)
	$(RM) $(IntermediateDirectory)/sqlite$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/webserv$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/webserv$(DependSuffix)
	$(RM) $(IntermediateDirectory)/webserv$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/htmlreplace$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/htmlreplace$(DependSuffix)
	$(RM) $(IntermediateDirectory)/htmlreplace$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/htmlpost$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/htmlpost$(DependSuffix)
	$(RM) $(IntermediateDirectory)/htmlpost$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/convbin$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/convbin$(DependSuffix)
	$(RM) $(IntermediateDirectory)/convbin$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/BPTC1969$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/BPTC1969$(DependSuffix)
	$(RM) $(IntermediateDirectory)/BPTC1969$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/decode34Rate$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/decode34Rate$(DependSuffix)
	$(RM) $(IntermediateDirectory)/decode34Rate$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/hyteraDecode$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/hyteraDecode$(DependSuffix)
	$(RM) $(IntermediateDirectory)/hyteraDecode$(PreprocessSuffix)
	$(RM) $(IntermediateDirectory)/aprs$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/aprs$(DependSuffix)
	$(RM) $(IntermediateDirectory)/aprs$(PreprocessSuffix)

	$(RM) $(OutputFile)


