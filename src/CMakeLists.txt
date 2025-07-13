cmake_minimum_required(VERSION 3.16)
project(richy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recherche des packages requis
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# Pour macOS, utilisation de pkgconfig pour jsoncpp
if(APPLE)
    pkg_check_modules(JSONCPP jsoncpp)
    if(NOT JSONCPP_FOUND)
        find_package(jsoncpp REQUIRED)
    endif()
else()
    pkg_check_modules(JSONCPP REQUIRED jsoncpp)
endif()

# Génération de la version
add_custom_target(generate_version
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_version.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating version information..."
)

# Ajout des sous-répertoires
add_subdirectory(core)
add_subdirectory(net)

# Exécutable principal
add_executable(richy main.cpp)

# Dépendances pour la génération de version
add_dependencies(richy generate_version)
add_dependencies(core generate_version)
add_dependencies(net generate_version)

# Liaison avec les bibliothèques
target_link_libraries(richy 
    core 
    net
    OpenSSL::SSL 
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
)

# Configuration des includes
target_include_directories(richy PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIRS}
)

# Flags de compilation
target_compile_options(richy PRIVATE 
    ${CURL_CFLAGS_OTHER}
    ${JSONCPP_CFLAGS_OTHER}
)