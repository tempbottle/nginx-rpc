
boost = {
     "work_directory":"thirdparty/boost/",
     "prefix":"thirdparty/boost/", # replace by boost['work_directory']+gcc/

     "init":
       [
         "rm -fr boost_1_57_0",
         "tar zxf boost_1_57_0.tar.gz",
         "mkdir build"
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
         "rm -fr protobuf-master",
         "unzip -q protobuf-master.zip",
         "cp ../../patches/gtest-1.7.0.zip .",
         "patch -p0 -i ../../patches/use_local_gtest.patch",
         "mkdir build"
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

         "ls protobuf-master.zip || wget -c https://codeload.github.com/google/protobuf/zip/master -O protobuf-master.zip"
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
         "cd tengine-master; patch -p1 -i ../../../patches/set_linker.patch",
         "cd tengine-master; patch -p1 -i ../../../patches/set_cxxflags_in_addons.patch"
       ],

      "configure":
          "echo hehe",

      "install":[
          "echo hehe",
      ],

     "update":
      [
         "ls tengine-master.zip || wget -c https://codeload.github.com/alibaba/tengine/zip/master -O tengine-master.zip"
      ]
}

#tc malloc or jemalloc
depends = [ protobuf, tengine]
