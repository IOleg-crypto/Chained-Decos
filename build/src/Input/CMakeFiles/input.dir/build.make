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
CMAKE_SOURCE_DIR = "D:/gitnext/Chained Decos"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "D:/gitnext/Chained Decos/build"

# Include any dependencies generated for this target.
include src/Input/CMakeFiles/input.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/Input/CMakeFiles/input.dir/compiler_depend.make

# Include the progress variables for this target.
include src/Input/CMakeFiles/input.dir/progress.make

# Include the compile flags for this target's objects.
include src/Input/CMakeFiles/input.dir/flags.make

src/Input/CMakeFiles/input.dir/InputManager.cpp.obj: src/Input/CMakeFiles/input.dir/flags.make
src/Input/CMakeFiles/input.dir/InputManager.cpp.obj: src/Input/CMakeFiles/input.dir/includes_CXX.rsp
src/Input/CMakeFiles/input.dir/InputManager.cpp.obj: D:/gitnext/Chained\ Decos/src/Input/InputManager.cpp
src/Input/CMakeFiles/input.dir/InputManager.cpp.obj: src/Input/CMakeFiles/input.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir="D:/gitnext/Chained Decos/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/Input/CMakeFiles/input.dir/InputManager.cpp.obj"
	cd "D:/gitnext/Chained Decos/build/src/Input" && E:/LLVM/bin/clang++.exe $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/Input/CMakeFiles/input.dir/InputManager.cpp.obj -MF CMakeFiles/input.dir/InputManager.cpp.obj.d -o CMakeFiles/input.dir/InputManager.cpp.obj -c "D:/gitnext/Chained Decos/src/Input/InputManager.cpp"

src/Input/CMakeFiles/input.dir/InputManager.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/input.dir/InputManager.cpp.i"
	$(CMAKE_COMMAND) -E cmake_unimplemented_variable CMAKE_CXX_CREATE_PREPROCESSED_SOURCE

src/Input/CMakeFiles/input.dir/InputManager.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/input.dir/InputManager.cpp.s"
	$(CMAKE_COMMAND) -E cmake_unimplemented_variable CMAKE_CXX_CREATE_ASSEMBLY_SOURCE

# Object files for target input
input_OBJECTS = \
"CMakeFiles/input.dir/InputManager.cpp.obj"

# External object files for target input
input_EXTERNAL_OBJECTS =

src/Input/input.lib: src/Input/CMakeFiles/input.dir/InputManager.cpp.obj
src/Input/input.lib: src/Input/CMakeFiles/input.dir/build.make
src/Input/input.lib: src/Input/CMakeFiles/input.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir="D:/gitnext/Chained Decos/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library input.lib"
	cd "D:/gitnext/Chained Decos/build/src/Input" && $(CMAKE_COMMAND) -P CMakeFiles/input.dir/cmake_clean_target.cmake
	cd "D:/gitnext/Chained Decos/build/src/Input" && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/input.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/Input/CMakeFiles/input.dir/build: src/Input/input.lib
.PHONY : src/Input/CMakeFiles/input.dir/build

src/Input/CMakeFiles/input.dir/clean:
	cd "D:/gitnext/Chained Decos/build/src/Input" && $(CMAKE_COMMAND) -P CMakeFiles/input.dir/cmake_clean.cmake
.PHONY : src/Input/CMakeFiles/input.dir/clean

src/Input/CMakeFiles/input.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "D:/gitnext/Chained Decos" "D:/gitnext/Chained Decos/src/Input" "D:/gitnext/Chained Decos/build" "D:/gitnext/Chained Decos/build/src/Input" "D:/gitnext/Chained Decos/build/src/Input/CMakeFiles/input.dir/DependInfo.cmake" "--color=$(COLOR)"
.PHONY : src/Input/CMakeFiles/input.dir/depend

