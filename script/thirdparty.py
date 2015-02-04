
boost = {
     "work_directory":"thirdparty/boost/",
     "prefix":"thirdparty/boost/", # replace by boost['work_directory']+gcc/
     
     "init":
       [
         "mkdir build",
         "rm -fr boost_1_57_0",
         "tar zxf boost_1_57_0.tar.gz"
         
       ],
      
      "configure":
          "cd boost_1_57_0; sh bootstrap.sh --with-libraries=all --prefix=",
      
      "install":[
         "cd boost_1_57_0;./b2 -j8 variant=release link=static runtime-link=static --build-type=minimal",
         "cd boost_1_57_0;./b2 -j8 install",
         "rm -fr boost_1_57_0"
      ],
      
     "update":
      [
         "wget -c http://jaist.dl.sourceforge.net/project/boost/boost/1.57.0/boost_1_57_0.tar.gz"
      ]
}

## genrate patch :
##    diff -Narpu origin modify > p.patch
##    patch -p0 -i p.patch

protobuf = {
     "work_directory":"thirdparty/protobuf/",
     "prefix":"thirdparty/protobuf/",
      
     "init":
       [
         "mkdir build",
         "rm -fr protobuf-master",
         "unzip -q protobuf-master.zip",
         "cp ../../patches/gtest-1.7.0.zip .",
         "patch -p0 -i ../../patches/use_local_gtest.patch" 
       ],
      
      "configure":
          "cd protobuf-master; sh autogen.sh;./configure --enable-shared=false --prefix=",
      
      "install":
       [
         "cd protobuf-master; make -j8; make install",
         "rm -fr protobuf-master"
       ],
      
     "update":
       [
         "wget -c https://codeload.github.com/google/protobuf/zip/master -o protobuf-master.zip"
       ]
}

tengine = {
     "work_directory":"thirdparty/tengine/",
     "prefix":"thirdparty/tengine/",
     
     "init":
       [
         "mkdir build",
         "rm -fr tengine-master",
         "unzip -q tengine-master.zip",
         "cd tengine-master; patch -p1 -i ../../../patches/set_linker.patch" 
       ],
      
      "configure":
          "echo hehe",
      
      "install":[
          "echo hehe",
      ],
      
     "update":
      [
         "wget -c https://codeload.github.com/alibaba/tengine/zip/master -o tengine-master.zip"
      ]
}

#tc malloc or jemalloc
depends = [ boost, protobuf, tengine]
