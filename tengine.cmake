#genrate the makefile  for tenginx

#jemalloc
#SET(JEMALLOC_PATH ${CMAKE_SOURCE_DIR}/jemalloc)
#http://www.canonware.com/download/jemalloc/jemalloc-3.6.0.tar.bz2
#externalproject_add(jemalloc_build
#    URL jemalloc-3.6.0.tar.bz2
#    URL_MD5 e76665b63a8fddf4c9f26d2fa67afdf2
#    SOURCE_DIR ${JEMALLOC_PATH}
#    CONFIGURE_COMMAND ./configure --prefix=${TENGINX_INSTALL}
#    BUILD_COMMAND make -j4
#    BUILD_IN_SOURCE 1
#    INSTALL_DIR ${TENGINX_INSTALL}
#)

#openssl


#luajit2
#
SET(LUAJIT2 ${CMAKE_SOURCE_DIR}/thirdparty/luajit2)
IF(NOT EXISTS "${LUAJIT2}/lib")
    externalproject_add(luajit2_build
        PREFIX ${LUAJIT2}
        URL ${LUAJIT2}/luajit2-master.zip
        URL_MD5 81b4529a55efb6a6c97c7c62e1004858
        CONFIGURE_COMMAND echo start build luajit
        BINARY_DIR ${LUAJIT2}
        BUILD_COMMAND /bin/sh build.sh
        INSTALL_COMMAND echo build luajit done
    )
endif()

file(GLOB_RECURSE SRC_C FOLLOW_SYMLINKS "${LUAJIT2}/include/*")
add_custom_target(luajit2_src SOURCES ${SRC_C} )

#protobuf
#
SET(PROTOBUF ${CMAKE_SOURCE_DIR}/thirdparty/protobuf)
IF(NOT EXISTS "${PROTOBUF}/lib")
    externalproject_add(proto_build
        PREFIX ${PROTOBUF}
        URL ${PROTOBUF}/protobuf-2.6.1.tar.gz
        URL_MD5 f3916ce13b7fcb3072a1fa8cf02b2423
        CONFIGURE_COMMAND echo start build protobuf
        BINARY_DIR ${PROTOBUF}
        BUILD_COMMAND /bin/sh build.sh
        INSTALL_COMMAND echo build protobuf done
    )
endif()

file(GLOB_RECURSE SRC_C FOLLOW_SYMLINKS "${PROTOBUF}/include/*")
add_custom_target(luajit2_src SOURCES ${SRC_C})

#tengine
#git config --global http.proxy http://10.198.2.146:8080
#https://codeload.github.com/alibaba/tengine/zip/master
#GIT_REPOSITORY https://github.com/alibaba/tengine.git
SET(TENGINX_PATH ${CMAKE_SOURCE_DIR}/thirdparty/tengine)
SET(TENGINX_SRC ${CMAKE_SOURCE_DIR}/build/tengine-src)
SET(TENGINX_INSTALL ${CMAKE_SOURCE_DIR}/install)

SET(CONFIGURE_COMMAND CC=gcc ./configure --prefix=${TENGINX_INSTALL}
--with-debug
--with-backtrace_module
--with-link=g++
--with-cc-opt=-O0\ -g\ -ggdb\ -ggdb3\ -I.\ -I${PROTOBUF}/include
--with-cxx-opt=-std=c++11
--with-ld-opt=-L${PROTOBUF}/lib\ -lprotobuf\ -static-libgcc\ -static-libstdc++\ -L/usr/local/lib\ -lpcre
--with-http_lua_module
--with-luajit-inc=${LUAJIT2}/include/luajit-2.0
--with-luajit-lib=${LUAJIT2}/lib)

#add module
foreach(module ${NGX_MODULE})
    include_directories("${module}")
endforeach()

#tenginx header for qtcreator
include_directories("${TENGINX_SRC}/src/core")
include_directories("${TENGINX_SRC}/src/event")
include_directories("${TENGINX_SRC}/src/http")
include_directories("${TENGINX_SRC}/src/mail")
include_directories("${TENGINX_SRC}/src/misc")
include_directories("${TENGINX_SRC}/src/os")
include_directories("${TENGINX_SRC}/src/proc")


foreach(module ${NGX_MODULE})
    SET(CONFIGURE_COMMAND ${CONFIGURE_COMMAND} --add-module=${module})
    #add to qtcreator
    file(GLOB_RECURSE SRC_PP FOLLOW_SYMLINKS "${module}/*.[ch]pp")
    file(GLOB_RECURSE SRC_CC FOLLOW_SYMLINKS "${module}/*.cc")
    file(GLOB_RECURSE SRC_C  FOLLOW_SYMLINKS "${module}/*.c")
    file(GLOB_RECURSE SRC_CONFIG FOLLOW_SYMLINKS "${module}/config")
    file(GLOB_RECURSE HEADERS FOLLOW_SYMLINKS "${module}/*.h")

    get_filename_component(MODULE_NAME ${module} NAME)
    add_custom_target(${MODULE_NAME} SOURCES ${SRC_PP} ${SRC_CC} ${SRC_C} ${HEADERS} ${SRC_CONFIG})
    #message("add add_custom_target:${MODULE_NAME} from with ${SRC_PP} ${SRC_CC} ${SRC_C} ${HEADERS}")
    #add_dependencies(tengine_build ${MODULE_NAME})
endforeach()


externalproject_add(tengine_build
    URL ${TENGINX_PATH}/tengine-master.zip
    URL_MD5 31117c9a2664f8734b17e6f455260265
    PATCH_COMMAND patch -p1 -i ${TENGINX_PATH}/log_location.patch
          COMMAND patch -p1 -i ${TENGINX_PATH}/set_linker.patch
          COMMAND patch -p1 -i ${TENGINX_PATH}/set_cxxflags_in_addons.patch

    CONFIGURE_COMMAND ${CONFIGURE_COMMAND}
    SOURCE_DIR ${TENGINX_SRC}
    BUILD_COMMAND make -j4 install
    BUILD_IN_SOURCE 1
    INSTALL_DIR ${TENGINX_INSTALL}
)

add_dependencies(tengine_build proto_build luajit2_build)


include_directories(${module})
#add to qtcreator
file(GLOB_RECURSE SRC_C FOLLOW_SYMLINKS "${TENGINX_SRC}/*")
add_custom_target(tengine_src SOURCES ${SRC_C})

