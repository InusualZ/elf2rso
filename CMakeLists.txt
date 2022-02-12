cmake_minimum_required(VERSION 3.18)
project(elf2rso)

set(CMAKE_CXX_STANDARD 17)

# Source: Dolphin Emulator https://github.com/dolphin-emu/dolphin
if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  add_compile_options(/EHsc)
  add_compile_options(/Gy)
  add_compile_options(/Oi)
  add_compile_options(/GS-)
  add_compile_options(/permissive-)
  add_compile_options(/Zc:inline)
  add_compile_options(/Zc:throwingNew)
  add_compile_options(/volatile:iso)
  add_compile_options(/experimental:newLambdaProcessor)
  add_compile_options(/Zc:externConstexpr,lambda,preprocessor)

  # Temporarily disable warnings to enable /Zc:preprocessor compatibility with WinSDK headers.
  add_compile_options(
    /wd5105 # macro expansion producing 'defined' has undefined behavior
  )

  string(APPEND CMAKE_EXE_LINKER_FLAGS " /NXCOMPAT")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " /OPT:REF /OPT:ICF")
else()
    add_compile_options(-Ofast)
    add_compile_options(-fno-strict-aliasing)
    add_compile_options(-fno-exceptions)
    add_compile_options(-fvisibility-inlines-hidden)
    add_compile_options(-fvisibility=hidden)
    add_compile_options(-fomit-frame-pointer)
endif()

include_directories(.)
include_directories(elfio)

add_executable(elf2rso elf2rso.cpp FileWriter.h optparser.h RSO.h swap.h types.h)
