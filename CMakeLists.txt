cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(ClusterRows VERSION 2.0.0)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(NATIVE_BUILD OFF)

# No cross-compile toolchain in use
if (NOT DEFINED EXTENSION_PLATFORM)
    # Native build is currently linux only.
    set(EXTENSION_PLATFORM "linux_x86_64")
    set(NATIVE_BUILD ON)
endif ()

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(LO_PATH "/usr/lib/libreoffice" CACHE FILEPATH "LibreOffice install directory")
set(LO_SDK_PATH "/usr/lib/libreoffice/sdk" CACHE FILEPATH "LibreOffice SDK directory")
option(LOGGING_ENABLED "Enable logging" OFF)

set(LO_IDLWRITE "${LO_SDK_PATH}/bin/unoidl-write")
set(LO_IDL_DIR "${LO_SDK_PATH}/idl")
set(LO_UNOPKG "${LO_PATH}/program/unopkg")

add_library(gmm
        src/cxx/em.cxx
        src/cxx/logging.cxx
        src/cxx/matrix.cxx
        src/cxx/diagonal.cxx
        src/cxx/svd.cxx
        src/cxx/gmm/cluster.cxx
        src/cxx/gmm/model.cxx)

set(COMP_NAME ${CMAKE_PROJECT_NAME})
set(COMP_IMPL_NAME ${COMP_NAME}.py)
set(COMP_IDENTIFIER com.github.dennisfrancis.ClusterRows)
set(COMP_PYDIR ${CMAKE_SOURCE_DIR}/src/py)
set(COMP_PYFILES
        ${COMP_PYDIR}/ClusterRows.py
        ${COMP_PYDIR}/DataCluster.py
        ${COMP_PYDIR}/CRJob.py
        ${COMP_PYDIR}/crlogger.py
        ${COMP_PYDIR}/crplatform.py
        ${COMP_PYDIR}/crrange.py
        ${COMP_PYDIR}/crcolors.py
        ${COMP_PYDIR}/perf.py)

set(ClusterRowsDefinitions "")
if (LOGGING_ENABLED)
    list(APPEND ClusterRowsDefinitions LOGGING_ENABLED)
endif ()
target_compile_definitions(gmm PUBLIC ${ClusterRowsDefinitions})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -Wall -Wextra -Werror")

if (WIN32 OR CYGWIN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -Wl,-Bstatic -lstdc++")
endif ()

set(COMP_RDB_NAME "DataCluster.uno.rdb")

file(GLOB COMP_IDL_FILES "${CMAKE_SOURCE_DIR}/idl/*.idl")

set(COMP_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/build")
set(COMP_REG_DIR "${COMP_BUILD_DIR}/rdb")
set(COMP_META_DIR "${COMP_BUILD_DIR}/meta")
set(COMP_REG_FILE "${COMP_REG_DIR}/${COMP_RDB_NAME}")
set(EXTENSION_DIR ${CMAKE_SOURCE_DIR}/extension)

# IDL files -> single RDB
add_custom_command(
        OUTPUT ${COMP_BUILD_DIR}/rdb.done
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${COMP_REG_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${COMP_REG_DIR}
        COMMAND ${LO_IDLWRITE} ${LO_PATH}/program/types.rdb ${LO_PATH}/program/types/offapi.rdb ${COMP_IDL_FILES} ${COMP_REG_FILE}
        COMMAND ${CMAKE_COMMAND} -E touch ${COMP_BUILD_DIR}/rdb.done
        COMMENT "Generating RDB file from IDL files"
        DEPENDS ${COMP_IDL_FILES}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tmpl/manifest.xml.in ${COMP_META_DIR}/manifest.xml)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tmpl/description.xml.in ${COMP_META_DIR}/description.xml)

add_custom_target(
        loext
        ALL
        COMMENT "Setting up extension files"
        DEPENDS
        ${COMP_BUILD_DIR}/rdb.done
        ${COMP_META_DIR}/manifest.xml
        ${COMP_META_DIR}/description.xml
)

target_include_directories(gmm PUBLIC ${CMAKE_SOURCE_DIR}/src/inc)

set(COMP_XCU_FILES
        ${CMAKE_SOURCE_DIR}/xcu/DataCluster.xcu
        ${CMAKE_SOURCE_DIR}/xcu/Jobs.xcu
        ${CMAKE_SOURCE_DIR}/xcu/Addons.xcu)

set(COMP_XDL_FILES ${CMAKE_SOURCE_DIR}/xdl/ClusterRows.xdl)

set(COMP_IMG_DIR ${CMAKE_SOURCE_DIR}/img)
set(COMP_IMG_FILES ${COMP_IMG_DIR}/icon_hc.png ${COMP_IMG_DIR}/icon.png)

install(TARGETS gmm DESTINATION .)
install(
        FILES
        ${COMP_PYFILES}
        ${COMP_XCU_FILES}
        ${COMP_XDL_FILES}
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
set(COMP_PACKAGE_NAME ${COMP_NAME}-${CMAKE_SYSTEM_NAME})

add_custom_target(ClusterRows
        COMMAND ${CPACK_COMMAND} -G ZIP -D CPACK_PACKAGE_FILE_NAME=${COMP_PACKAGE_NAME} -D CPACK_INCLUDE_TOPLEVEL_DIRECTORY=OFF
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EXTENSION_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${COMP_PACKAGE_NAME}.zip ${EXTENSION_DIR}/${COMP_PACKAGE_NAME}.oxt
        WORKING_DIRECTORY ${CMAKE_BUILD_DIR}
        COMMENT "Building extension package"
        DEPENDS gmm loext
        )

add_custom_target(deploy
        COMMAND ${LO_UNOPKG} remove ${COMP_IDENTIFIER} > /dev/null 2>&1 || (exit 0)
        COMMAND ${LO_UNOPKG} add ${EXTENSION_DIR}/${COMP_PACKAGE_NAME}.oxt
        COMMENT "Deploying extension package to system LibreOffice"
        DEPENDS ClusterRows
        )

add_custom_target(deployrun
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/testdocs/no-results.ods ${CMAKE_BINARY_DIR}/
        COMMAND soffice --calc ${CMAKE_BINARY_DIR}/no-results.ods
        COMMENT "Running system soffice with the built extension"
        DEPENDS deploy
        )

add_custom_target(showlogs
        COMMAND python3 ${CMAKE_SOURCE_DIR}/src/py/showlogs.py
        )

if (NATIVE_BUILD)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)

    enable_testing()

    add_executable(
            gmmTests
            ${CMAKE_SOURCE_DIR}/test/gmmTests.cxx
            ${CMAKE_SOURCE_DIR}/src/cxx/matrix.cxx
            ${CMAKE_SOURCE_DIR}/src/cxx/diagonal.cxx
            ${CMAKE_SOURCE_DIR}/src/cxx/svd.cxx
            ${CMAKE_SOURCE_DIR}/src/cxx/gmm/cluster.cxx
            ${CMAKE_SOURCE_DIR}/src/cxx/gmm/model.cxx
    )
    target_link_libraries(
            gmmTests
            gmm
            gtest_main
    )

    add_executable(
            utilTests
            ${CMAKE_SOURCE_DIR}/test/utilTests.cxx
            ${CMAKE_SOURCE_DIR}/src/cxx/matrix.cxx
            ${CMAKE_SOURCE_DIR}/src/cxx/diagonal.cxx
            ${CMAKE_SOURCE_DIR}/src/cxx/svd.cxx
            ${CMAKE_SOURCE_DIR}/src/cxx/gmm/cluster.cxx
            ${CMAKE_SOURCE_DIR}/src/cxx/gmm/model.cxx
    )
    target_link_libraries(
            utilTests
            gtest_main
    )
    target_include_directories(utilTests PUBLIC ${CMAKE_SOURCE_DIR}/src/inc)

    include(GoogleTest)
    gtest_discover_tests(gmmTests)
    gtest_discover_tests(utilTests)

endif ()

add_custom_target(
        copy-compile-commands ALL
        ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}
)
