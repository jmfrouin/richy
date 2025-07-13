file(GLOB SOURCES "*.cpp")
add_library(net ${SOURCES})

# Configuration spécifique pour macOS
if(APPLE)
    set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl")
    set(OPENSSL_LIBRARIES "/opt/homebrew/opt/openssl/lib")
    set(OPENSSL_INCLUDE_DIR "/opt/homebrew/opt/openssl/include")
endif()

# Recherche des dépendances
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)

# Configuration jsoncpp
pkg_check_modules(JSONCPP jsoncpp)
if(NOT JSONCPP_FOUND)
    find_package(jsoncpp REQUIRED)
endif()

# Configuration des includes AVANT la liaison
target_include_directories(net PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
)

# Liaison avec les bibliothèques
target_link_libraries(net 
    OpenSSL::SSL 
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
)

# Liaison jsoncpp
if(JSONCPP_FOUND)
    target_link_libraries(net ${JSONCPP_LIBRARIES})
    target_compile_options(net PRIVATE ${JSONCPP_CFLAGS_OTHER})
else()
    target_link_libraries(net jsoncpp_lib)
endif()

# Flags de compilation
target_compile_options(net PRIVATE 
    ${CURL_CFLAGS_OTHER}
)

# Debug des chemins
message(STATUS "net module - OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "net module - OpenSSL libraries: ${OPENSSL_LIBRARIES}")