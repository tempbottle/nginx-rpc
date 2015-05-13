#!/bin/bash

BASE=$(dirname $(readlink -f $0))


PROTOC="$BASE/thirdparty/protobuf/bin/protoc"
NGX_RPC_PLUGIN="$BASE/release/sbin/protoc-gen-ngx_rpc"


if [ -z $1 ]; then

    echo "usage:$0 protofile"
    exit 1
fi

PBDIR=$(dirname $1)
FILENAME=$(basename -s .proto $1)
echo ${PBDIR}
echo ${FILENAME}

echo "$PROTOC --ngx_rpc_out=. --proto_path=${PBDIR} $1 --plugin=$NGX_RPC_PLUGIN"
$PROTOC --ngx_rpc_out=. --proto_path=${PBDIR} $1 --plugin=$NGX_RPC_PLUGIN

echo "cp -f $1 ngx_rpc_${FILENAME}_module"
cp -f $1 ngx_rpc_${FILENAME}_module/

echo "$PROTOC --cpp_out=. ngx_rpc_${FILENAME}_module/$(basename $1) --proto_path=${PBDIR} -Ingx_rpc_${FILENAME}_module"
$PROTOC --cpp_out=ngx_rpc_${FILENAME}_module $1 --proto_path=${PBDIR}
