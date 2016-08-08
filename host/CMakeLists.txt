cmake_minimum_required(VERSION 2.8.8)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake")
include(LinuxCrossCompile)

project(easynmc)
SET(PROJECT_VERSION 0.2.1)
SET(PROJECT_API_VERSION 2)

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(LIBELF libelf)

SET(LIB_SOURCES easynmc-core.c easynmc-filters.c)

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/include/
)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -DLIBEASYNMC_VERSION=\\\"${PROJECT_VERSION}\\\"")

ADD_LIBRARY(easynmc OBJECT ${LIB_SOURCES})

#Shared
ADD_LIBRARY(easynmcshared SHARED $<TARGET_OBJECTS:easynmc>)
SET_TARGET_PROPERTIES(easynmcshared PROPERTIES SOVERSION ${PROJECT_VERSION}
VERSION ${PROJECT_API_VERSION})
SET_TARGET_PROPERTIES(easynmcshared PROPERTIES OUTPUT_NAME easynmc)

target_link_libraries(easynmcshared
    ${LIBELF_LIBRARIES}
)

#Static
ADD_LIBRARY(easynmcstatic STATIC $<TARGET_OBJECTS:easynmc>)
SET_TARGET_PROPERTIES(easynmcstatic PROPERTIES OUTPUT_NAME easynmc)

#Executables
add_executable(nmrun nmrun.c)
target_link_libraries(nmrun easynmcshared)

add_executable(nmctl nmctl.c)
target_link_libraries(nmctl easynmcshared)

# Generate a pkg-config

#Finally, generate the pkg-config file
SET(PKG_CONFIG_LIBDIR
    "\${prefix}/lib/\${deb_host_multiarch}"
)
SET(PKG_CONFIG_INCLUDEDIR
    "\${prefix}/include/\${deb_host_multiarch}/easynmc-${PROJECT_VERSION}"
)
SET(PKG_CONFIG_LIBS
    "-L\${libdir} -l:libaura.so.${PROJECT_VERSION}"
)
SET(PKG_CONFIG_CFLAGS
    "-I\${includedir}"
)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
)

#Now, install everything
INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
       DESTINATION lib/${CMAKE_LIBRARY_PATH}/pkgconfig/)

INSTALL(TARGETS easynmcshared LIBRARY
        DESTINATION lib/${CMAKE_LIBRARY_PATH})

INSTALL(TARGETS easynmcstatic ARCHIVE
        DESTINATION lib/${CMAKE_LIBRARY_PATH})

#headers
file(GLOB_RECURSE EASYNMC_HEADERS
    "${CMAKE_SOURCE_DIR}/include/*"
)

file(GLOB_RECURSE EASYNMC_HEADERS_LINUX
    "${CMAKE_SOURCE_DIR}/include/linux/*"
)

INSTALL(FILES ${EASYNMC_HEADERS}
    DESTINATION include/${CMAKE_LIBRARY_PATH}/easynmc-${PROJECT_VERSION}/)

INSTALL(FILES ${EASYNMC_HEADERS_LINUX}
    DESTINATION include/${CMAKE_LIBRARY_PATH}/easynmc-${PROJECT_VERSION}/linux/)
