rm -fr protobuf-2.6.1
tar -zvxf protobuf-2.6.1.tar.gz
PD=`pwd`
echo $PD
cd protobuf-2.6.1
./configure --enable-shared=no --prefix="$PD"
make
make install

