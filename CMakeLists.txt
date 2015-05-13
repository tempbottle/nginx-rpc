cmake_minimum_required(VERSION 2.6)

#set project info
project(NgxRpc CXX)

set(VERSION_MAJOR "1")
set(VERSION_MINOR "1")
set(VERSION_PATCH "0")

set(VERSION_FULL "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(PKGVERSION_FULL "${VERSION_MAJOR}_${VERSION_MINOR}_${VERSION_PATCH}")




set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -msse -msse2 -msse3 -msse4.1   -g3  -fPIC -Wall -Wextra -Wno-unused-variable -Wno-unused-local-typedefs -Wno-unused-parameter -rdynamic -fpermissive -std=c++11 ")


set(CMAKE_BUILD_TYPE debug)
set(CMAKE_CXX_FLAGS_ALL "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")

if(CMAKE_BUILD_RELEASE)
    add_definitions(-DRELEASE)
    set(CMAKE_BUILD_TYPE relwithdebinfo)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")
    set(CMAKE_CXX_FLAGS_ALL "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()


message(status,"${CMAKE_CXX_FLAGS}")
## set instal info
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/release")


include(thirdparty/CMakeLists.txt)



set(NGX_MODULE ${CMAKE_SOURCE_DIR}/ngx_rpc_module)
set(NGX_MODULE ${NGX_MODULE} ${CMAKE_SOURCE_DIR}/ngx_rpc_inspect_module)

include(ExternalProject)
include(tengine.cmake)

## proto modules
add_subdirectory(ngx_rpc_plugin)
