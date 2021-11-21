cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(ClusterRows VERSION 1.0.3)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(LO_PATH "/usr/lib/libreoffice" CACHE FILEPATH "LibreOffice install directory")
set(LO_SDK_PATH "/usr/lib/libreoffice/sdk" CACHE FILEPATH "LibreOffice SDK directory")

set(LO_IDLC "${LO_SDK_PATH}/bin/idlc")
set(LO_REGMERGE "${LO_PATH}/program/regmerge")
set(LO_CPPUMAKER "${LO_SDK_PATH}/bin/cppumaker")
set(LO_IDL_DIR "${LO_SDK_PATH}/idl")
set(LO_INC_DIR "${LO_SDK_PATH}/include")

if(APPLE)
    set(URE_MISC ${LO_PATH}/Contents/Resources/ure/share/misc)
else()
    set(URE_MISC ${LO_PATH}/program)
endif()

set(URE_TYPES "${URE_MISC}/types.rdb")

if(APPLE)
    set(OFFICE_TYPES "${LO_PATH}/Contents/Resources/types/offapi.rdb")
else()
    set(OFFICE_TYPES "${LO_PATH}/program/types/offapi.rdb")
endif()

file(GLOB MY_IDL_FILES "${CMAKE_SOURCE_DIR}/idl/*.idl")

set(MY_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/build")

set(MY_URD_DIR "${MY_BUILD_DIR}/urd")
set(MY_REG_DIR "${MY_BUILD_DIR}/rdb")
set(MY_INC_DIR "${MY_BUILD_DIR}/inc")
set(MY_META_DIR "${MY_BUILD_DIR}/meta")
set(MY_REG_FILE "${MY_REG_DIR}/GMMCluster.uno.rdb")
list(APPEND MY_TYPES "-Tcom.github.dennisfrancis.XGMMCluster" "-Tcom.github.dennisfrancis.GMMCluster")

list(APPEND MY_TMPL_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/tmpl/manifest.xml.tmpl
    ${CMAKE_CURRENT_SOURCE_DIR}/tmpl/ClusterRows.components.tmpl
    ${CMAKE_CURRENT_SOURCE_DIR}/tmpl/description.xml.tmpl
    )

# IDL -> URD
add_custom_command(
    OUTPUT ${MY_BUILD_DIR}/urd.done
    COMMAND rm -rf ${MY_URD_DIR}
    COMMAND mkdir -p ${MY_URD_DIR}
    COMMAND ${LO_IDLC} -I${LO_IDL_DIR} -O${MY_URD_DIR} ${MY_IDL_FILES}
    COMMAND ${CMAKE_COMMAND} -E touch ${MY_BUILD_DIR}/urd.done
    COMMENT "Generating URD files from IDL files"
    DEPENDS ${MY_IDL_FILES}
    )

# URD -> single RDB
add_custom_command(
    OUTPUT ${MY_BUILD_DIR}/rdb.done
    COMMAND rm -rf ${MY_REG_DIR}
    COMMAND mkdir -p ${MY_REG_DIR}
    COMMAND ${LO_REGMERGE} ${MY_REG_FILE} /UCR ${MY_URD_DIR}/*.urd
    COMMAND ${CMAKE_COMMAND} -E touch ${MY_BUILD_DIR}/rdb.done
    COMMENT "Generating RDB file from URD files"
    DEPENDS ${MY_BUILD_DIR}/urd.done
    )

# header file generation
add_custom_command(
    OUTPUT ${MY_BUILD_DIR}/headers.done
    COMMAND rm -rf ${MY_INC_DIR}
    COMMAND mkdir -p ${MY_INC_DIR}
    COMMAND ${LO_CPPUMAKER} -Gc -O${MY_INC_DIR} ${URE_TYPES} ${OFFICE_TYPES}
    COMMAND ${LO_CPPUMAKER} -Gc -O${MY_INC_DIR} ${MY_TYPES} ${MY_REG_FILE} -X${URE_TYPES} -X${OFFICE_TYPES}
    COMMAND ${CMAKE_COMMAND} -E touch ${MY_BUILD_DIR}/headers.done
    COMMENT "Generating headers for all types"
    DEPENDS ${MY_BUILD_DIR}/rdb.done
    )

add_custom_command(
    OUTPUT ${MY_BUILD_DIR}/tmpl.done
    COMMAND COMP_NAME=ClusterRows UNOPKG_PLATFORM=Linux_x86_64 COMP_RDB_NAME=GMMCluster.uno.rdb envsubst < ${CMAKE_CURRENT_SOURCE_DIR}/tmpl/manifest.xml.tmpl > ${CMAKE_CURRENT_BINARY_DIR}/manifest.xml
    COMMAND UNOPKG_PLATFORM=Linux_x86_64 COMP_IMPL_NAME=libClusterRows.so envsubst < ${CMAKE_CURRENT_SOURCE_DIR}/tmpl/ClusterRows.components.tmpl > ${CMAKE_CURRENT_BINARY_DIR}/ClusterRows.components
    COMMAND CLUSTERROWS_VERSION=${CMAKE_PROJECT_VERSION} envsubst < ${CMAKE_CURRENT_SOURCE_DIR}/tmpl/description.xml.tmpl > ${CMAKE_CURRENT_BINARY_DIR}/description.xml
    COMMAND ${CMAKE_COMMAND} -E touch ${MY_BUILD_DIR}/tmpl.done
    COMMENT "Generating manifest, components and description files from templates"
    DEPENDS ${MY_TMPL_FILES}
    )

add_custom_target(
    allHeaders
    ALL
    DEPENDS ${MY_BUILD_DIR}/headers.done ${MY_BUILD_DIR}/tmpl.done
    )

file(GLOB MY_SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/*.cxx")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -std=c++17")

add_library(ClusterRows ${MY_SOURCE_FILES})

target_link_options(ClusterRows PUBLIC LINKER:-z,origin LINKER:-rpath,$ORIGIN)
target_link_libraries(ClusterRows -L"${LO_SDK_PATH}/lib" -L"${LO_PATH}/program")
# FIXME: make this conditional on OS.
target_compile_definitions(ClusterRows PUBLIC UNX GCC LINUX CPPU_ENV=gcc3 LOGGING_ENABLED)
target_include_directories(ClusterRows PUBLIC ${CMAKE_SOURCE_DIR}/inc ${MY_INC_DIR} ${LO_INC_DIR})

target_link_libraries(ClusterRows -luno_cppuhelpergcc3 -luno_cppu -luno_sal -lm)

add_dependencies(ClusterRows allHeaders)

file(GLOB MY_XCU_FILES "${CMAKE_SOURCE_DIR}/xcu/*.xcu")
file(GLOB MY_XDL_FILES "${CMAKE_SOURCE_DIR}/xdl/*.xdl")
file(GLOB MY_XDL_FILES "${CMAKE_SOURCE_DIR}/xdl/*.xdl")
file(GLOB MY_IMG_FILES "${CMAKE_SOURCE_DIR}/img/*.png")

install(TARGETS ClusterRows DESTINATION Linux_x86_64/)
install(
    FILES
    ${MY_XCU_FILES}
    ${MY_XDL_FILES}
    ${CMAKE_CURRENT_BINARY_DIR}/ClusterRows.components
    ${MY_REG_FILE}
    ${CMAKE_CURRENT_SOURCE_DIR}/description-en-US.txt
    ${CMAKE_CURRENT_BINARY_DIR}/description.xml
    DESTINATION
    .
    )

install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/manifest.xml
    DESTINATION
    META-INF
    )

install(
    FILES
    ${MY_IMG_FILES}
    DESTINATION
    img
    )

include(CPack)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

get_filename_component(CPACK_COMMAND ${CMAKE_COMMAND} PATH)
set(CPACK_COMMAND ${CPACK_COMMAND}/cpack)

add_custom_target(genext
    COMMAND ${CPACK_COMMAND} -G ZIP -D CPACK_PACKAGE_FILE_NAME=ClusterRows -D CPACK_INCLUDE_TOPLEVEL_DIRECTORY=OFF
    COMMAND mv ClusterRows.zip ClusterRows.oxt
    WORKING_DIRECTORY ${CMAKE_BUILD_DIR}
    DEPENDS ClusterRows
    )
