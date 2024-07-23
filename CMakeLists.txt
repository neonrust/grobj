cmake_minimum_required(VERSION 3.5)

project(grobj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(grobj
	src/main.cpp
	src/grimrock.cpp
	src/dump.cpp

	include/grobj/grimrock.h
	include/grobj/dump.h
)

target_include_directories(grobj
	PRIVATE
	include
)

target_compile_options(grobj
	PRIVATE
	-Wextra
	-Wall
	-Wpedantic
	-Werror
	--pedantic-errors
	-Wconversion
	-Wmissing-declarations
	-Wold-style-cast
	-Wno-padded
)
