nginx-rpc 
   use nginx as a network frame, support Google Proto Rpc.
   but use c++11 and boost 

The Protocal:
    1 now use HTTP 1.0  POST + data,further more use http 2.0
    
design and thinking:
    1 why nginx
    
    2 why G company rpc ?

    3 why http?
    
    4 why c++11 and boost:
    
    5 what interface or api?
    
hello world example:
   
   
build:


test:
    
    
    
the road map of nginx-rpc:
1 use ngx-content-handler do all process:
      1 interface adjuest
          1  support google rpc cpp
          2  warpper the nginx-native layer
      2 build tools
          1 use cmake
          2 with patch
      3 test  and bench mark
          gtest
          perf
          
      4 add client:
          1 base on curl
          2 base on nginx
          
      5 montor helper and state
          

2 add async compute handler with procs and upstream

3 support the custem encode and decode such as json.
      

3 add async fiber handler with procs and upstream

4 add replace  upstream with shmemory 

