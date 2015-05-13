set(CMAKE_CXX_COMPILER "g++")

set(TARGET protoc-gen-ngx_rpc)

include_directories("${CMAKE_SOURCE_DIR}")

file(GLOB_RECURSE SRC "*.[ch]pp")
file(GLOB_RECURSE HEADERS "*.h")


add_executable(${TARGET} ${SRC} ${HEADERS})
target_link_libraries(${TARGET}  protoc protobuf pthread )

set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-static-libgcc -static-libstdc++ -Wl,--no-undefined")
set_target_properties(${TARGET} PROPERTIES INSTALL_RPATH "./")

INSTALL(TARGETS ${TARGET} DESTINATION "sbin/")

