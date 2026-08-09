cmake_minimum_required(VERSION 3.16)
project(RL_Gomoku CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

# ---------------------------------------------------------------------------
# Core: czysta logika gry jako biblioteka (bez PyTorch/pybind).
# ---------------------------------------------------------------------------
add_library(gomoku_core STATIC core/src/game.cpp)
target_include_directories(gomoku_core PUBLIC "${CMAKE_SOURCE_DIR}/core/include")
set_target_properties(gomoku_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# Binarka gry (playground w C++). Zostaje jak byla: globuje src/*.cpp.
# Buduje sie tylko gdy budujemy standalone (nie przez pip/scikit-build).
# ---------------------------------------------------------------------------
if(NOT DEFINED SKBUILD)
  file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
  if(GAME_SOURCES)
    add_executable(gomoku ${GAME_SOURCES})
    target_include_directories(gomoku PRIVATE "${CMAKE_SOURCE_DIR}/src")
  endif()
endif()

# ---------------------------------------------------------------------------
# Modul Pythona (_core) przez pybind11. Budowany, gdy pybind11 jest dostepny
# (zawsze przy `pip install` dzieki scikit-build-core; przy czystym cmake tylko
# gdy find_package go znajdzie).
# ---------------------------------------------------------------------------
# Wymus wybor interpretera (pybind11 v3 uzywa FindPython). Bez tego moze
# trafic na inny Python niz aktywny i zbudowac .so z niepasujacym ABI.
find_package(Python COMPONENTS Interpreter Development.Module QUIET)
find_package(pybind11 CONFIG QUIET)
if(pybind11_FOUND)
  pybind11_add_module(_core bindings/module.cpp)
  target_link_libraries(_core PRIVATE gomoku_core)

  # Instalacja modulu do pakietu gomoku (uzywane przez scikit-build-core).
  if(DEFINED SKBUILD)
    install(TARGETS _core LIBRARY DESTINATION gomoku)
  endif()
else()
  message(STATUS "pybind11 nie znaleziony — pomijam modul _core "
                 "(zbuduj przez `pip install -e .`).")
endif()
