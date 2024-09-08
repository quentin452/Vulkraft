cmake_minimum_required(VERSION 3.5)

project("CG_Project")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED)

add_subdirectory(headers)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# Ajouter les fichiers sources avec GLOB_RECURSE
file(GLOB_RECURSE SOURCES_GLOB "src/*.cpp" "src/*.hpp")

# Ajouter les fichiers sources manuellement
set(SOURCES_MANUAL
    "${CMAKE_SOURCE_DIR}/headers/ThreadedLoggerForCPPV0.5.2/src/LoggerThread.cpp"
)

# Combiner les deux listes de sources
set(SOURCES ${SOURCES_GLOB} ${SOURCES_MANUAL})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    add_definitions(-DPERFORMANCE_MONITOR)
else()
    remove_definitions(-DPERFORMANCE_MONITOR)
endif()
#add_definitions(-DPERFORMANCE_MONITOR)

add_executable(main ${SOURCES})

add_custom_command(TARGET main PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/shaders $<TARGET_FILE_DIR:main>/shaders)
add_custom_command(TARGET main PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/models $<TARGET_FILE_DIR:main>/models)
add_custom_command(TARGET main PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/textures $<TARGET_FILE_DIR:main>/textures)

include_directories(
    headers
    "${CMAKE_SOURCE_DIR}/headers/game_performance_profiler"
    "${CMAKE_SOURCE_DIR}/headers/ThreadedLoggerForCPPV0.5.2/libs/include"
)

make_directory(${CMAKE_CURRENT_BINARY_DIR}/bin/shaders)

target_include_directories(main PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(main glm glfw)

target_link_libraries(main Vulkan::Vulkan)

# Inclure le script de génération des commandes de compilation
include(${CMAKE_SOURCE_DIR}/GenerateCompileCommands.cmake)
