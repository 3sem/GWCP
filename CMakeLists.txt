cmake_minimum_required(VERSION 3.15)
project(TwoSidedContextParser VERSION 0.6 LANGUAGES CXX)

set(TARGET_NAME "obparser")
set(CMAKE_CXX_STANDARD 14)

add_executable(${TARGET_NAME} src/inlined.cpp src/grammar.h src/grammar.cpp)
target_compile_definitions(${TARGET_NAME} PRIVATE NAME="${TARGET_NAME}" PRIVATE VERSION="${PROJECT_VERSION}")

FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${TARGET_NAME} ${Boost_LIBRARIES})
