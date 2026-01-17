cmake_minimum_required(VERSION 3.16)
project(ModApacheDeny_15Watt C)

find_program(APXS_EXECUTABLE apxs)
if(NOT APXS_EXECUTABLE)
    message(FATAL_ERROR "apxs wurde nicht gefunden. Bitte Apache httpd installieren.")
endif()

execute_process(
        COMMAND ${APXS_EXECUTABLE} -q INCLUDEDIR
        OUTPUT_VARIABLE APACHE_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
        COMMAND ${APXS_EXECUTABLE} -q CFLAGS
        OUTPUT_VARIABLE APACHE_CFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
        COMMAND ${APXS_EXECUTABLE} -q LIBEXECDIR
        OUTPUT_VARIABLE APACHE_MODULE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

# ENTWEDER dynamisch über brew:
execute_process(
        COMMAND brew --prefix apr
        OUTPUT_VARIABLE APR_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "APR_PREFIX = ${APR_PREFIX}")

add_library(${PROJECT_NAME} SHARED
        src/modApacheDeny_15Watt.c
        src/CheckIpAddr.h
        src/checkIpAddr.c
        src/test.c
        src/loadUserAgents.c
        src/loadUserAgents.h
        src/loadIpNetworks.c
        src/loadIpNetworks.h
        src/functionsString.c
        src/functionsString.h
        src/shouldUserAgentBeBlocked.c
        src/shouldUserAgentBeBlocked.h)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_include_directories(${PROJECT_NAME} PRIVATE
        ${APACHE_INCLUDE_DIR}
        ${APR_PREFIX}/include/apr-1      # <--- WICHTIG: Hier liegt apr_hooks.h
        /opt/homebrew/include
)

target_compile_options(${PROJECT_NAME} PRIVATE ${APACHE_CFLAGS})

install(TARGETS ${PROJECT_NAME} DESTINATION ${APACHE_MODULE_DIR})