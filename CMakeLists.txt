cmake_minimum_required(VERSION 2.8.11)
# Name of the project
set(PROJECT_NAME TextureDemos)
project(${PROJECT_NAME})

add_subdirectory(${PROJECT_SOURCE_DIR}/DivisorInstancing/ )
add_subdirectory(${PROJECT_SOURCE_DIR}/TBOInstancing/ )
add_subdirectory(${PROJECT_SOURCE_DIR}/UBOInstancing/ )
