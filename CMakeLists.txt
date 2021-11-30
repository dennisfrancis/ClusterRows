cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(ClusterRows VERSION 1.0.3)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
set(EXE_SUFFIX "")
set(EXEC_CALL_PREFIX "")
set(COMP_INC_DIR_PREFIX "")
if (WIN32 OR CYGWIN)
    set(LO_PATH "${CMAKE_SOURCE_DIR}/cross/win64/install" CACHE FILEPATH "LibreOffice install directory")
    set(LO_SDK_PATH "${CMAKE_SOURCE_DIR}/cross/win64/sdk" CACHE FILEPATH "LibreOffice SDK directory")
    set(LO_DEPDLL_PATHS "${CMAKE_SOURCE_DIR}/cross/win64/install/program\;${CMAKE_SOURCE_DIR}/cross/win64/System64")
    set(EXE_SUFFIX ".exe")
    set(EXEC_CALL_PREFIX "WINEPATH=${LO_DEPDLL_PATHS}\ wine64\ ")
    set(COMP_INC_DIR_PREFIX "z:")
endif()

if (APPLE)
    #FIXME
    set(LO_PATH "/usr/lib/libreoffice" CACHE FILEPATH "LibreOffice install directory")
    set(LO_SDK_PATH "/usr/lib/libreoffice/sdk" CACHE FILEPATH "LibreOffice SDK directory")
endif()

if (UNIX AND NOT APPLE)
    set(LO_PATH "/usr/lib/libreoffice" CACHE FILEPATH "LibreOffice install directory")
    set(LO_SDK_PATH "/usr/lib/libreoffice/sdk" CACHE FILEPATH "LibreOffice SDK directory")
endif()

option(LOGGING_ENABLED "Enable logging" OFF)

set(LO_IDLC "${LO_SDK_PATH}/bin/idlc")
set(LO_REGMERGE "${LO_PATH}/program/regmerge")
set(LO_CPPUMAKER "${LO_SDK_PATH}/bin/cppumaker")
set(LO_IDL_DIR "${LO_SDK_PATH}/idl")
set(LO_INC_DIR "${LO_SDK_PATH}/include")

file(GLOB COMP_SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/*.cxx")
add_library(ClusterRows SHARED ${COMP_SOURCE_FILES})

set(COMP_NAME ${CMAKE_PROJECT_NAME})
if (WIN32 OR CYGWIN)
    set(URE_MISC ${LO_PATH}/program)
    set(OFFICE_TYPES "${LO_PATH}/program/types/offapi.rdb")
    set(UNOPKG_PLATFORM "Windows")
    set(COMP_IMPL_NAME "${COMP_NAME}.dll")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32 -DWNT -D_DLL -DCPPU_ENV=mscx -D_MSC_VER=1700")
    if (LOGGING_ENABLED)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLOGGING_ENABLED")
    endif()
    #set(CMAKE_LINKER "/usr/bin/ld")
    #set(CMAKE_LINKER_FLAGS "-DEBUGTYPE:cv -DEF:${LO_SDK_PATH}/settings/component.uno.def -L${LO_SDK_PATH}/lib -L${LO_PATH}/program -licppuhelper -licppu -lisal -lmsvcrt -lkernel32 -shared")
    #set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_LINKER} ${CMAKE_LINKER_FLAGS} -o ${COMP_IMPL_NAME} CMakeFiles/ClusterRows.dir/objects.a -Wl,--no-whole-archive @CMakeFiles/ClusterRows.dir/linklibs.rsp")

    #target_link_libraries(ClusterRows -L"${LO_SDK_PATH}/lib" -L"${LO_PATH}/program" -licppuhelper -licppu -lisal -lmsvcrt -lkernel32)
    #target_link_options(ClusterRows PUBLIC -DEBUGTYPE:cv -DEF:${LO_SDK_PATH}/settings/component.uno.def -licppuhelper -licppu -lisal -lmsvcrt -lkernel32 -lcppu3 -lreglo -lunoidllo -lsal3 -lcli_cppuhelper)
    target_link_options(ClusterRows PUBLIC -licppuhelper -lcppuhelper3MSC -lcppu3 -lsal3 -lsalhelper3MSC -lmsvcrt -lunoidllo -lreglo -lsal_textenclo -DEBUGTYPE:cv -DEF:${LO_SDK_PATH}/settings/component.uno.def)
    target_link_directories(ClusterRows PUBLIC ${LO_SDK_PATH}/lib ${LO_PATH}/program)
    #set(CMAKE_EXE_LINKER_FLAGS "-Wl,--no-whole-archive" CACHE INTERNAL "" FORCE)
    #target_link_options(ClusterRows PRIVATE -licppuhelper -licppu -lisal -lmsvcrt -lkernel32)
    #target_link_libraries(ClusterRows -L"${LO_SDK_PATH}/lib" -L"${LO_PATH}/program" icppuhelper.lib icppu.lib isal.lib msvcrt.lib kernel32.lib)
endif()

if (APPLE)
    set(URE_MISC ${LO_PATH}/Contents/Resources/ure/share/misc)
    set(OFFICE_TYPES "${LO_PATH}/Contents/Resources/types/offapi.rdb")
    set(UNOPKG_PLATFORM "MacOSX_x86_64")
    set(COMP_IMPL_NAME "lib${COMP_NAME}.dylib")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common -fvisibility=hidden")
    # FIXME
    #target_link_options(ClusterRows PUBLIC -single_module -Wl,-multiply_defined,suppress)
    #target_compile_definitions
    #target_link_libraries
endif()

if (UNIX AND NOT APPLE)
    set(URE_MISC ${LO_PATH}/program)
    set(OFFICE_TYPES "${LO_PATH}/program/types/offapi.rdb")
    set(UNOPKG_PLATFORM "Linux_x86_64")
    set(COMP_IMPL_NAME "lib${COMP_NAME}.so")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -Wall -Wextra")
    list(APPEND ClusterRowsDefinitions UNX GCC LINUX CPPU_ENV=gcc3)
    if (LOGGING_ENABLED)
        list(APPEND ClusterRowsDefinitions LOGGING_ENABLED)
    endif()
    target_link_options(ClusterRows PUBLIC LINKER:-z,origin LINKER:-rpath,$ORIGIN)
    target_link_libraries(ClusterRows -L"${LO_SDK_PATH}/lib" -L"${LO_PATH}/program")
    target_compile_definitions(ClusterRows PUBLIC ${ClusterRowsDefinitions})
    target_link_libraries(ClusterRows -luno_cppuhelpergcc3 -luno_cppu -luno_sal -lm)
endif()

set(COMP_RDB_NAME "GMMCluster.uno.rdb")

set(URE_TYPES "${URE_MISC}/types.rdb")

file(GLOB COMP_IDL_FILES "${CMAKE_SOURCE_DIR}/idl/*.idl")

set(COMP_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/build")

set(COMP_SRCGEN_DIR "${CMAKE_SOURCE_DIR}/generators")
set(COMP_GEN_DIR "${COMP_BUILD_DIR}/generators")
set(COMP_URD_DIR "${COMP_BUILD_DIR}/urd")
set(COMP_REG_DIR "${COMP_BUILD_DIR}/rdb")
set(COMP_INC_DIR "${COMP_BUILD_DIR}/inc")
set(COMP_META_DIR "${COMP_BUILD_DIR}/meta")
set(COMP_REG_FILE "${COMP_REG_DIR}/${COMP_RDB_NAME}")
set(COMP_TYPES "-Tcom.github.dennisfrancis.XGMMCluster\;com.github.dennisfrancis.GMMCluster")

list(APPEND COMP_TMPL_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/tmpl/manifest.xml.tmpl
    ${CMAKE_CURRENT_SOURCE_DIR}/tmpl/ClusterRows.components.tmpl
    ${CMAKE_CURRENT_SOURCE_DIR}/tmpl/description.xml.tmpl
    )

configure_file(${COMP_SRCGEN_DIR}/genurd.sh.in ${COMP_GEN_DIR}/genurd.sh)
configure_file(${COMP_SRCGEN_DIR}/genrdb.sh.in ${COMP_GEN_DIR}/genrdb.sh)
configure_file(${COMP_SRCGEN_DIR}/genheaders.sh.in ${COMP_GEN_DIR}/genheaders.sh)

# IDL -> URD
add_custom_command(
    OUTPUT ${COMP_BUILD_DIR}/urd.done
    COMMAND rm -rf ${COMP_URD_DIR}
    COMMAND mkdir -p ${COMP_URD_DIR}
    #COMMAND ${LO_IDLC} -I${LO_IDL_DIR} -O${COMP_URD_DIR} ${COMP_IDL_FILES}
    COMMAND bash -x ${COMP_GEN_DIR}/genurd.sh
    COMMAND ${CMAKE_COMMAND} -E touch ${COMP_BUILD_DIR}/urd.done
    COMMENT "Generating URD files from IDL files"
    DEPENDS ${COMP_IDL_FILES} ${COMP_GEN_DIR}/genurd.sh
    )

# URD -> single RDB
add_custom_command(
    OUTPUT ${COMP_BUILD_DIR}/rdb.done
    COMMAND rm -rf ${COMP_REG_DIR}
    COMMAND mkdir -p ${COMP_REG_DIR}
    #COMMAND ${LO_REGMERGE} ${COMP_REG_FILE} /UCR ${COMP_URD_DIR}/*.urd
    COMMAND bash -x ${COMP_GEN_DIR}/genrdb.sh
    COMMAND ${CMAKE_COMMAND} -E touch ${COMP_BUILD_DIR}/rdb.done
    COMMENT "Generating RDB file from URD files"
    DEPENDS ${COMP_BUILD_DIR}/urd.done ${COMP_GEN_DIR}/genrdb.sh
    )

# header file generation
add_custom_command(
    OUTPUT ${COMP_BUILD_DIR}/headers.done
    COMMAND rm -rf ${COMP_INC_DIR}
    COMMAND mkdir -p ${COMP_INC_DIR}
    #COMMAND ${LO_CPPUMAKER} -Gc -O${COMP_INC_DIR} ${URE_TYPES} ${OFFICE_TYPES}
    #COMMAND ${LO_CPPUMAKER} -Gc -O${COMP_INC_DIR} ${COMP_TYPES} ${COMP_REG_FILE} -X${URE_TYPES} -X${OFFICE_TYPES}
    COMMAND bash -x ${COMP_GEN_DIR}/genheaders.sh
    COMMAND ${CMAKE_COMMAND} -E touch ${COMP_BUILD_DIR}/headers.done
    COMMENT "Generating headers for all types"
    DEPENDS ${COMP_BUILD_DIR}/rdb.done ${COMP_GEN_DIR}/genheaders.sh
    )

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tmpl/manifest.xml.in ${COMP_META_DIR}/manifest.xml)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tmpl/ClusterRows.components.in ${COMP_META_DIR}/ClusterRows.components)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tmpl/description.xml.in ${COMP_META_DIR}/description.xml)

add_custom_target(
    allHeaders
    ALL
    DEPENDS
        ${COMP_BUILD_DIR}/headers.done
        ${COMP_META_DIR}/manifest.xml
        ${COMP_META_DIR}/ClusterRows.components
        ${COMP_META_DIR}/description.xml
    )

target_include_directories(ClusterRows PUBLIC ${CMAKE_SOURCE_DIR}/inc ${COMP_INC_DIR} ${LO_INC_DIR})

add_dependencies(ClusterRows allHeaders)

file(GLOB COMP_XCU_FILES "${CMAKE_SOURCE_DIR}/xcu/*.xcu")
file(GLOB COMP_XDL_FILES "${CMAKE_SOURCE_DIR}/xdl/*.xdl")
file(GLOB COMP_XDL_FILES "${CMAKE_SOURCE_DIR}/xdl/*.xdl")
file(GLOB COMP_IMG_FILES "${CMAKE_SOURCE_DIR}/img/*.png")

install(TARGETS ClusterRows DESTINATION Linux_x86_64/)
install(
    FILES
    ${COMP_XCU_FILES}
    ${COMP_XDL_FILES}
    ${COMP_META_DIR}/ClusterRows.components
    ${COMP_REG_FILE}
    ${CMAKE_CURRENT_SOURCE_DIR}/description-en-US.txt
    ${COMP_META_DIR}/description.xml
    DESTINATION
    .
    )

install(
    FILES
    ${COMP_META_DIR}/manifest.xml
    DESTINATION
    META-INF
    )

install(
    FILES
    ${COMP_IMG_FILES}
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
