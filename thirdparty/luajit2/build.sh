#get from https://github.com/openresty/luajit2/archive/master.zip

rm -fr luajit2-master
unzip luajit2-master.zip
cd luajit2-master
sed -i 's|^export PREFIX= /usr/local|#export PREFIX|g' Makefile 
PREFIX=`pwd`/../ make install
cd ..
#rm -fr luajit2-master

