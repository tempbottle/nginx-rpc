set(THIRD_PATH ${CMAKE_SOURCE_DIR}/thirdparty)

macro(add_thirdparty_library DIR)

    MESSAGE("include: ${DIR}/include")

    INCLUDE_DIRECTORIES(${DIR}/include)

    file(GLOB_RECURSE HEADERS_H   "${DIR}/include/*.h")
    #file(GLOB_RECURSE HEADERS_HPP "${DIR}/include/*.hpp")

    foreach(header ${HEADERS_H})
        set_source_files_properties(${header} PROPERTY HEADER_FILE_ONLY TRUE)
    endforeach()

    foreach(libname ${ARGN})
        add_library(${libname} STATIC IMPORTED)
        #add_custom_target("${libname}_include" SOURCES ${HEADERS_H} ${HEADERS_HPP})
        set_property(TARGET ${libname} PROPERTY
                            IMPORTED_LOCATION ${DIR}/lib/lib${libname}.a)
        MESSAGE("add thirdparty lib ${libname}")
    endforeach()

endmacro(add_thirdparty_library)


##set protobuf
set(PROTOBUF ${THIRD_PATH}/protobuf)
add_thirdparty_library(${PROTOBUF} protobuf protoc)

