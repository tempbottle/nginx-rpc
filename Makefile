# build for nginx_app

PWD :=$(shell pwd)
INSTALL:=$(PWD)/release
BUILD:=$(PWD)/build

NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)

PROTO_INC="$(PWD)/thirdparty/protobuf/include"
PROTO_LIB="$(PWD)/thirdparty/protobuf/lib"


install: thirdparty/tengine/tengine-master/Makefile protoc-gen-ngx_rpc
	rm -f release/logs/*
	make -j$(NPROCS) -C thirdparty/tengine/tengine-master
	make -j$(NPROCS) -C thirdparty/tengine/tengine-master install


clean:
	make -C thirdparty/tengine/tengine-master clean
	rm -fr build/*
	rm -f ngx_rpc_plugin/*.o ngx_rpc_plugin/protoc-gen-ngx_rpc

rebuild:
	rm -fr jemalloc
	rm -fr tengine

thirdparty/tengine/tengine-master/Makefile: thirdparty/tengine/tengine-master/configure
	mkdir -p build
	cd thirdparty/tengine/tengine-master; \
	./configure  --prefix=$(INSTALL) \
        --with-debug \
        --with-backtrace_module \
        --with-link=g++ \
        --with-cc-opt=" -O0 -g -ggdb -ggdb3 " \
        --with-cxx-opt=" -std=c++11 "\
        --with-http_lua_module \
        --add-module=$(PWD)/inspect_server \
        --add-module=$(PWD)/ngx_rpc_module



plugin_header:= ngx_rpc_plugin/ngx_rpc_defines.h \
                ngx_rpc_plugin/ngx_rpc_generator.h \
                ngx_rpc_plugin/ngx_rpc_module_template.h \
                ngx_rpc_plugin/ngx_rpc_server_template.h




protoc-gen-ngx_rpc: ngx_rpc_plugin/ngx_rpc_plugin.o  ngx_rpc_plugin/ngx_rpc_generator.o
	g++ -o $(INSTALL)/sbin/protoc-gen-ngx_rpc ngx_rpc_plugin/ngx_rpc_generator.o ngx_rpc_plugin/ngx_rpc_plugin.o \
		-L$(PROTO_LIB)  -lprotoc -lprotobuf -lpthread -lrt
#	./thirdparty/protobuf/bin/protoc  ./inspect_server/inspect.proto  --ngx_rpc_out=. --plugin=./ngx_rpc_plugin/protoc-gen-ngx_rpc


ngx_rpc_plugin/ngx_rpc_generator.o : ngx_rpc_plugin/ngx_rpc_generator.cpp $(plugin_header)
	g++ -c -std=c++11 -o ngx_rpc_plugin/ngx_rpc_generator.o -I$(PROTO_INC) ngx_rpc_plugin/ngx_rpc_generator.cpp

ngx_rpc_plugin/ngx_rpc_plugin.o : ngx_rpc_plugin/ngx_rpc_plugin.cpp $(plugin_header)
	g++ -c -o ngx_rpc_plugin/ngx_rpc_plugin.o -I$(PROTO_INC) ngx_rpc_plugin/ngx_rpc_plugin.cpp

#        --add-module=$(PWD)/test \
#        --add-module=$(PWD)/rpc_proc

#         --with-jemalloc="$(PWD)/thirdparty/jemalloc" \
#         --add-module=$(PWD)/rpc_proc
#         --add-module=$(PWD)/rpc_module \
#         --add-module=$(PWD)/js_module \
#         --add-module=$(PWD)/proxy_module \
#         --with-ld-opt="-lstdc++" \
#         --with-google_perftools_module \
#         --enable-mods-shared=all \
#         --with-http_sub_module=shared

# OBJS_BUILD_DIR:=$(PWD)/soap_sub_module/
# SRC_DIR:=$(PWD)/soap_sub_module
# CLIENT_FLAGS=-D__CLIENT_TEST__

# CLIENT_INC:=-I$(PWD)/soap_sub_module -I$(PWD)/../thirdparty/protobuf -I$(PWD)/../thirdparty/mtrpc/include 
# CLIENT_LIB:= -L$(PWD)/../thirdparty/mtrpc/lib -lmtrpc \
#              -L$(PWD)/../thirdparty/protobuf/ \
#              -lprotobuf -static-libgcc -static-libstdc++ -Wl,--no-undefined -lm -lpthread

# OBJS=$(OBJS_BUILD_DIR)/youtucheck_base.pb.o \
#        $(OBJS_BUILD_DIR)/youtu_facecheck.pb.o \
#       $(OBJS_BUILD_DIR)/ngx_base64_code.o \
 #      $(OBJS_BUILD_DIR)/pugixml.o \
 #      $(OBJS_BUILD_DIR)/soap_pack_helper.o \
 #      $(OBJS_BUILD_DIR)/proxy_message_code.o \
 #      $(OBJS_BUILD_DIR)/soap_client.o 
#
#
#
#soap_client : $(OBJS)
#	g++  -o soap_client $(CLIENT_INC) $(OBJS) $(CLIENT_LIB)
#	echo "hehe"
#soap_client_clean:
#	rm -fr $(OBJS_BUILD_DIR)/*.o
#	rm -f soap_client
#	
#$(OBJS_BUILD_DIR)/youtucheck_base.pb.o:$(SRC_DIR)/youtucheck_base.pb.cc
#	gcc $(CLIENT_FLAGS) -c $(CLIENT_INC) -o $@  $^
#
#$(OBJS_BUILD_DIR)/youtu_facecheck.pb.o:$(SRC_DIR)/youtu_facecheck.pb.cc
#	gcc $(CLIENT_FLAGS) -c $(CLIENT_INC) -o $@  $^
#	
#$(OBJS_BUILD_DIR)/ngx_base64_code.o:$(SRC_DIR)/ngx_base64_code.cpp
#	gcc $(CLIENT_FLAGS) -c $(CLIENT_INC) -o $@  $^
#		
#$(OBJS_BUILD_DIR)/pugixml.o:$(SRC_DIR)/pugixml.cpp
#	gcc $(CLIENT_FLAGS) -c $(CLIENT_INC) -o $@  $^
#	
#$(OBJS_BUILD_DIR)/soap_pack_helper.o:$(SRC_DIR)/soap_pack_helper.cpp
#	gcc $(CLIENT_FLAGS) -c $(CLIENT_INC) -o $@  $^
#	
#$(OBJS_BUILD_DIR)/proxy_message_code.o:$(SRC_DIR)/proxy_message_code.cpp
#	gcc $(CLIENT_FLAGS) -c $(CLIENT_INC) -o $@  $^
#		
#$(OBJS_BUILD_DIR)/soap_client.o:$(SRC_DIR)/soap_client.cpp
#	gcc $(CLIENT_FLAGS) -c $(CLIENT_INC) -o $@  $^
#	

