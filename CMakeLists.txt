cmake_minimum_required(VERSION 3.24)

project(playlister3 VERSION 4.0.0.0 LANGUAGES CXX)

include(cmake/get_cpm.cmake)
include(cmake/GetGitRevisionDescription.cmake)

get_git_head_revision(GIT_REFSPEC GIT_COMMIT_HASH)

string(REPLACE "refs/heads/" "" GIT_BRANCH_NAME ${GIT_REFSPEC})
string(SUBSTRING ${GIT_COMMIT_HASH} 0 8 GIT_COMMIT_HASH_SHORT)

CPMAddPackage(
    NAME              spdlog
    GIT_TAG           v1.11.0
    GITHUB_REPOSITORY gabime/spdlog
)

CPMAddPackage(
    NAME               zydis
    GIT_TAG            v4.0.0
    GITHUB_REPOSITORY  zyantific/zydis
    OPTIONS            "ZYDIS_MINIMAL_MODE ON" "ZYDIS_FEATURE_ENCODER OFF" "ZYDIS_FEATURE_FORMATTER OFF"
                       "ZYDIS_FEATURE_AVX512 OFF" "ZYDIS_FEATURE_KNC OFF" "ZYDIS_FEATURE_SEGMENT OFF"
)

CPMAddPackage(
    NAME              safetyhook
    GIT_TAG           v0.4.1
    GITHUB_REPOSITORY cursey/safetyhook
)

CPMAddPackage(
    NAME              yaml-cpp
    GIT_TAG           yaml-cpp-0.7.0
    GITHUB_REPOSITORY jbeder/yaml-cpp
    OPTIONS           "YAML_CPP_BUILD_CONTRIB OFF" "YAML_CPP_BUILD_TOOLS OFF"
)

add_library(${PROJECT_NAME} SHARED
    src/init.cc
    src/playlists.cc
    res/playlister.rc
)

file(GLOB OFFSETS src/offsets/*.cc)

foreach (OFFSET_PATH ${OFFSETS})
    message(STATUS "Including version support file: ${OFFSET_PATH}...")

    file(READ "${OFFSET_PATH}" CONTENT)
    set(ALL_OFFSETS "${ALL_OFFSETS}\n${CONTENT}")
endforeach ()

configure_file(src/version.h.in macros/version.h @ONLY)
configure_file(src/offsets.h.in macros/offsets.h @ONLY)

target_link_libraries(${PROJECT_NAME} spdlog safetyhook yaml-cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/macros)