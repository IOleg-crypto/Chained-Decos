# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.29

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = "C:/Program Files/CMake/bin/cmake.exe"

# The command to remove a file.
RM = "C:/Program Files/CMake/bin/cmake.exe" -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild"

# Utility rule file for nlohmann_json-populate.

# Include any custom commands dependencies for this target.
include CMakeFiles/nlohmann_json-populate.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/nlohmann_json-populate.dir/progress.make

CMakeFiles/nlohmann_json-populate: CMakeFiles/nlohmann_json-populate-complete

CMakeFiles/nlohmann_json-populate-complete: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-install
CMakeFiles/nlohmann_json-populate-complete: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-mkdir
CMakeFiles/nlohmann_json-populate-complete: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-download
CMakeFiles/nlohmann_json-populate-complete: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-update
CMakeFiles/nlohmann_json-populate-complete: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-patch
CMakeFiles/nlohmann_json-populate-complete: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-configure
CMakeFiles/nlohmann_json-populate-complete: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-build
CMakeFiles/nlohmann_json-populate-complete: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-install
CMakeFiles/nlohmann_json-populate-complete: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-test
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir="D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Completed 'nlohmann_json-populate'"
	"C:/Program Files/CMake/bin/cmake.exe" -E make_directory "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles"
	"C:/Program Files/CMake/bin/cmake.exe" -E touch "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles/nlohmann_json-populate-complete"
	"C:/Program Files/CMake/bin/cmake.exe" -E touch "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-done"

nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-update:
.PHONY : nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-update

nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-build: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-configure
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir="D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "No build step for 'nlohmann_json-populate'"
	cd "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-build" && "C:/Program Files/CMake/bin/cmake.exe" -E echo_append
	cd "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-build" && "C:/Program Files/CMake/bin/cmake.exe" -E touch "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-build"

nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-configure: nlohmann_json-populate-prefix/tmp/nlohmann_json-populate-cfgcmd.txt
nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-configure: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-patch
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir="D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_3) "No configure step for 'nlohmann_json-populate'"
	cd "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-build" && "C:/Program Files/CMake/bin/cmake.exe" -E echo_append
	cd "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-build" && "C:/Program Files/CMake/bin/cmake.exe" -E touch "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-configure"

nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-download: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-gitinfo.txt
nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-download: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-mkdir
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir="D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_4) "Performing download step (git clone) for 'nlohmann_json-populate'"
	cd "D:/gitnext/Chained Decos/build/_deps" && "C:/Program Files/CMake/bin/cmake.exe" -P "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/tmp/nlohmann_json-populate-gitclone.cmake"
	cd "D:/gitnext/Chained Decos/build/_deps" && "C:/Program Files/CMake/bin/cmake.exe" -E touch "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-download"

nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-install: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-build
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir="D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_5) "No install step for 'nlohmann_json-populate'"
	cd "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-build" && "C:/Program Files/CMake/bin/cmake.exe" -E echo_append
	cd "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-build" && "C:/Program Files/CMake/bin/cmake.exe" -E touch "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-install"

nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-mkdir:
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir="D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_6) "Creating directories for 'nlohmann_json-populate'"
	"C:/Program Files/CMake/bin/cmake.exe" -Dcfgdir= -P "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/tmp/nlohmann_json-populate-mkdirs.cmake"
	"C:/Program Files/CMake/bin/cmake.exe" -E touch "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-mkdir"

nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-patch: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-patch-info.txt
nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-patch: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-update
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir="D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_7) "No patch step for 'nlohmann_json-populate'"
	"C:/Program Files/CMake/bin/cmake.exe" -E echo_append
	"C:/Program Files/CMake/bin/cmake.exe" -E touch "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-patch"

nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-update:
.PHONY : nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-update

nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-test: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-install
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir="D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_8) "No test step for 'nlohmann_json-populate'"
	cd "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-build" && "C:/Program Files/CMake/bin/cmake.exe" -E echo_append
	cd "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-build" && "C:/Program Files/CMake/bin/cmake.exe" -E touch "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-test"

nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-update: nlohmann_json-populate-prefix/tmp/nlohmann_json-populate-gitupdate.cmake
nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-update: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-update-info.txt
nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-update: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-download
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir="D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_9) "Performing update step for 'nlohmann_json-populate'"
	cd "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-src" && "C:/Program Files/CMake/bin/cmake.exe" -Dcan_fetch=YES -P "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/nlohmann_json-populate-prefix/tmp/nlohmann_json-populate-gitupdate.cmake"

nlohmann_json-populate: CMakeFiles/nlohmann_json-populate
nlohmann_json-populate: CMakeFiles/nlohmann_json-populate-complete
nlohmann_json-populate: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-build
nlohmann_json-populate: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-configure
nlohmann_json-populate: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-download
nlohmann_json-populate: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-install
nlohmann_json-populate: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-mkdir
nlohmann_json-populate: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-patch
nlohmann_json-populate: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-test
nlohmann_json-populate: nlohmann_json-populate-prefix/src/nlohmann_json-populate-stamp/nlohmann_json-populate-update
nlohmann_json-populate: CMakeFiles/nlohmann_json-populate.dir/build.make
.PHONY : nlohmann_json-populate

# Rule to build all files generated by this target.
CMakeFiles/nlohmann_json-populate.dir/build: nlohmann_json-populate
.PHONY : CMakeFiles/nlohmann_json-populate.dir/build

CMakeFiles/nlohmann_json-populate.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/nlohmann_json-populate.dir/cmake_clean.cmake
.PHONY : CMakeFiles/nlohmann_json-populate.dir/clean

CMakeFiles/nlohmann_json-populate.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild" "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild" "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild" "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild" "D:/gitnext/Chained Decos/build/_deps/nlohmann_json-subbuild/CMakeFiles/nlohmann_json-populate.dir/DependInfo.cmake" "--color=$(COLOR)"
.PHONY : CMakeFiles/nlohmann_json-populate.dir/depend

