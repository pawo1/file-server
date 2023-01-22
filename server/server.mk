##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=server
ConfigurationName      :=Debug
WorkspaceConfiguration := $(ConfigurationName)
WorkspacePath          :=/home/pawo/file-server
ProjectPath            :=/home/pawo/file-server/server
IntermediateDirectory  :=../build-$(ConfigurationName)/server
OutDir                 :=../build-$(ConfigurationName)/server
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=pawo
Date                   :=21/01/23
CodeLitePath           :=/home/pawo/.codelite
LinkerName             :=/usr/bin/g++
SharedObjectLinkerName :=/usr/bin/g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=../build-$(ConfigurationName)/bin/$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :=$(IntermediateDirectory)/ObjectsList.txt
PCHCompileFlags        :=
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)ssl $(LibrarySwitch)crypto 
ArLibs                 :=  "ssl" "crypto" 
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -g -O0 -std=c++17 -Wall $(Preprocessors)
CFLAGS   :=  -g -O0 -std=c++17 -Wall $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=../build-$(ConfigurationName)/server/FileInfo.cpp$(ObjectSuffix) ../build-$(ConfigurationName)/server/main.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: MakeIntermediateDirs $(OutputFile)

$(OutputFile): ../build-$(ConfigurationName)/server/.d $(Objects) 
	@mkdir -p "../build-$(ConfigurationName)/server"
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@mkdir -p "../build-$(ConfigurationName)/server"
	@mkdir -p ""../build-$(ConfigurationName)/bin""

../build-$(ConfigurationName)/server/.d:
	@mkdir -p "../build-$(ConfigurationName)/server"

PreBuild:


##
## Objects
##
../build-$(ConfigurationName)/server/FileInfo.cpp$(ObjectSuffix): FileInfo.cpp ../build-$(ConfigurationName)/server/FileInfo.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/pawo/file-server/server/FileInfo.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/FileInfo.cpp$(ObjectSuffix) $(IncludePath)
../build-$(ConfigurationName)/server/FileInfo.cpp$(DependSuffix): FileInfo.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT../build-$(ConfigurationName)/server/FileInfo.cpp$(ObjectSuffix) -MF../build-$(ConfigurationName)/server/FileInfo.cpp$(DependSuffix) -MM FileInfo.cpp

../build-$(ConfigurationName)/server/FileInfo.cpp$(PreprocessSuffix): FileInfo.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) ../build-$(ConfigurationName)/server/FileInfo.cpp$(PreprocessSuffix) FileInfo.cpp

../build-$(ConfigurationName)/server/main.cpp$(ObjectSuffix): main.cpp ../build-$(ConfigurationName)/server/main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/pawo/file-server/server/main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
../build-$(ConfigurationName)/server/main.cpp$(DependSuffix): main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT../build-$(ConfigurationName)/server/main.cpp$(ObjectSuffix) -MF../build-$(ConfigurationName)/server/main.cpp$(DependSuffix) -MM main.cpp

../build-$(ConfigurationName)/server/main.cpp$(PreprocessSuffix): main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) ../build-$(ConfigurationName)/server/main.cpp$(PreprocessSuffix) main.cpp


-include ../build-$(ConfigurationName)/server//*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r $(IntermediateDirectory)


