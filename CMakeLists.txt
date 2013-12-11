cmake_minimum_required(VERSION 2.8.7)

project(webos-application)

include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC4)
webos_component(0 1 0)

find_package(PkgConfig "0.22" REQUIRED)

pkg_check_modules(GLIB2 glib-2.0 REQUIRED)
pkg_check_modules(LS2 luna-service2 REQUIRED)
pkg_check_modules(PBNJSON_C pbnjson_c REQUIRED)

webos_build_pkgconfig()

include_directories(include/public)
include_directories(${GLIB2_INCLUDE_DIRS} ${LS2_INCLUDE_DIRS} ${PBNJSON_C_INCLUDE_DIRS})
webos_add_compiler_flags(ALL -Wall)

set(SOURCES src/webos_application.c src/luna_service_utils.h)
add_library(webos-application SHARED ${SOURCES})
target_link_libraries(webos-application ${GLIB2_LDFLAGS} ${LS_LDFLAGS} ${PBNJSON_C_LDFLAGS})

webos_build_library()
