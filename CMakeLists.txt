cmake_minimum_required(VERSION 3.24)

project(
  particle_methods
  VERSION 0.1.0
  DESCRIPTION "Particle-based methods (DEM, SPH, MPM) with C++/CUDA and Python bindings"
  LANGUAGES CXX)

option(PM_ENABLE_OPENMP "Enable OpenMP parallel loops" ON)
option(PM_ENABLE_CUDA "Enable CUDA acceleration" ON)
option(PM_ENABLE_PYTHON "Build Python bindings" ON)
option(PM_BUILD_EXAMPLES "Build C++ examples" ON)
option(PM_BUILD_TESTS "Build unit/regression tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
  particle_methods
  src/core/particle_system.cpp
  src/scenes/dem_scenes.cpp
  src/scenes/sph_scenes.cpp
  src/solvers/dem_solver.cpp
  src/solvers/sph_solver.cpp
  src/solvers/mpm_solver.cpp)

set_target_properties(
  particle_methods
  PROPERTIES
    POSITION_INDEPENDENT_CODE ON)

target_include_directories(
  particle_methods
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include)

target_compile_options(
  particle_methods
  PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>)

if(PM_ENABLE_OPENMP)
  find_package(OpenMP)
  if(OpenMP_CXX_FOUND)
    target_link_libraries(particle_methods PUBLIC OpenMP::OpenMP_CXX)
    target_compile_definitions(particle_methods PUBLIC PM_HAS_OPENMP=1)
  else()
    message(STATUS "OpenMP not found. Building without OpenMP.")
    target_compile_definitions(particle_methods PUBLIC PM_HAS_OPENMP=0)
  endif()
else()
  target_compile_definitions(particle_methods PUBLIC PM_HAS_OPENMP=0)
endif()

set(PM_CUDA_AVAILABLE OFF)
if(PM_ENABLE_CUDA)
  include(CheckLanguage)
  check_language(CUDA)
  if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)

    add_library(
      particle_methods_cuda
      src/cuda/dem_cuda.cu)

    target_include_directories(
      particle_methods_cuda
      PUBLIC
        ${PROJECT_SOURCE_DIR}/include)

    target_link_libraries(
      particle_methods_cuda
      PUBLIC
        particle_methods
        CUDA::cudart)

    set_target_properties(
      particle_methods_cuda
      PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CUDA_STANDARD 17
        CUDA_STANDARD_REQUIRED ON
        CUDA_SEPARABLE_COMPILATION ON)

    target_compile_definitions(particle_methods PUBLIC PM_ENABLE_CUDA=1)
    target_compile_definitions(particle_methods_cuda PUBLIC PM_ENABLE_CUDA=1)
    set(PM_CUDA_AVAILABLE ON)
  else()
    message(WARNING "CUDA compiler not found. Building without CUDA acceleration.")
    target_compile_definitions(particle_methods PUBLIC PM_ENABLE_CUDA=0)
  endif()
else()
  target_compile_definitions(particle_methods PUBLIC PM_ENABLE_CUDA=0)
endif()

if(PM_BUILD_EXAMPLES)
  add_executable(
    falling_particles_2d
    src/examples/falling_particles_demo.cpp)

  target_link_libraries(falling_particles_2d PRIVATE particle_methods)

  if(PM_CUDA_AVAILABLE)
    target_link_libraries(falling_particles_2d PRIVATE particle_methods_cuda)
  endif()
endif()

if(PM_ENABLE_PYTHON)
  find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)

  include(FetchContent)
  FetchContent_Declare(
    pybind11
    URL https://github.com/pybind/pybind11/archive/refs/tags/v2.13.6.tar.gz)
  FetchContent_MakeAvailable(pybind11)

  pybind11_add_module(particle_methods_py MODULE bindings/python_module.cpp)
  target_link_libraries(particle_methods_py PRIVATE particle_methods)

  if(PM_CUDA_AVAILABLE)
    target_link_libraries(particle_methods_py PRIVATE particle_methods_cuda)
  endif()

  set_target_properties(
    particle_methods_py
    PROPERTIES
      OUTPUT_NAME _core)
endif()

if(PM_BUILD_TESTS)
  include(CTest)
  if(BUILD_TESTING)
    add_executable(
      dem_phase1_tests
      tests/dem_phase1_tests.cpp)

    target_link_libraries(dem_phase1_tests PRIVATE particle_methods)
    add_test(NAME dem_phase1_tests COMMAND dem_phase1_tests)

    add_executable(
      sph_phase2_tests
      tests/sph_phase2_tests.cpp)

    target_link_libraries(sph_phase2_tests PRIVATE particle_methods)
    add_test(NAME sph_phase2_tests COMMAND sph_phase2_tests)
  endif()
endif()
