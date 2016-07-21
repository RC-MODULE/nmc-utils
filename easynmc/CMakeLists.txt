CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../host/cmake)
include(NeuromatrixToolchain)
NMC_TARGET(soc)

PROJECT(easynmc C ASM)
SET(PROJECT_VERSION   0.1)


SET(PROJECT_FULL_NAME
    ${PROJECT_NAME}-${PROJECT_VERSION}${CMAKE_BUILD_TYPE})

SET(SRCS platform.asm easynmc-io.c aura.c printf.cpp)

include_directories(./include)
SET(CMAKE_EXE_LINKER_FLAGS -c${CMAKE_SOURCE_DIR}/conf/K1879.cfg )

ADD_LIBRARY(easynmc ${SRCS})

#installation stuff
install(TARGETS easynmc DESTINATION lib/${PROJECT_FULL_NAME})
file(GLOB HEADERS ${CMAKE_SOURCE_DIR}/include/easynmc/*)

install(FILES ${HEADERS} DESTINATION include/${PROJECT_FULL_NAME}/easynmc)

#pkg-config stuff
SET(PKG_CONFIG_LIBDIR
    "\${prefix}/lib/${PROJECT_FULL_NAME}"
)

SET(PKG_CONFIG_INCLUDEDIR
    "\${prefix}/include/${PROJECT_FULL_NAME}"
)

SET(PKG_CONFIG_LIBS
    "-L\${libdir} easynmc.lib"
)
SET(PKG_CONFIG_CFLAGS
    "-I\${includedir}"
)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_FULL_NAME}.pc"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_FULL_NAME}.pc" DESTINATION pkg-config)
