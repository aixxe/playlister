cmake_minimum_required(VERSION 3.24)

project(playlister3 VERSION 3.1.0.0 LANGUAGES CXX)

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
    NAME              bddisasm
    GIT_TAG           61382e95f0029a51bbde12c6bd98c65a419b3abf
    GITHUB_REPOSITORY bitdefender/bddisasm
)

CPMAddPackage(
    NAME              safetyhook
    GIT_TAG           25629ca8320a7fe3df5ea37f1c07c1291d43a0d6
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

configure_file(src/version.h.in macros/version.h @ONLY)

target_link_libraries(${PROJECT_NAME} spdlog safetyhook yaml-cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/macros)