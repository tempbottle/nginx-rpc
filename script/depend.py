
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
          "cd boost_1_57_0; sh bootstrap.sh --with-libraries=all --prefix=%s",
      
      "install":[
         "cd boost_1_57_0;./b2 variant=release link=static runtime-link=static --build-type=minimal"
         "cd boost_1_57_0;./b2 install"
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
         "patch -p0 -i p.patch" 
       ],
      
      "configure":
          "cd protobuf-master; sh autogen.sh;./configure --enable-shared=false --prefix=%s",
      
      "install":
       [
         "cd protobuf-master; make -j8; make install"
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
         "unzip -q tengine-master.zip"
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


depends = [ boost, protobuf, tengine]
