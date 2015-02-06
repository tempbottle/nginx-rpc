nginx-rpc
====
    Use nginx as a network framework, Implement Google Proto Rpc.
    Use c++11 modern c++ sytle to develop nginx modules.
    

Install and Try
====
    
    1 write a proto
    
    2 init the server base code 
    
    3 write your implement
    
    4 build with nginx


Design and Thinking:
====

The Protocal:
-------
    1 now use HTTP 1.0  POST + data
    2 to support HTTP 2.0/SPDY

Some choice:
-------
    1 why nginx:
    
    2 why G company rpc ?

    3 why http ?
    
    4 why c++11?
    
    
    
the road map of nginx-rpc:
====

1 use ngx-content-handler do all process:
-------
      1 interface adjuest
          
          1  support google rpc cpp 
          2  warpper the nginx-native layer logger
          3  add init handler master or process
          4  rpc client with subrequest
      2 build tools
          1 write a python script
          2 with patch
                    
      3 test and bench mark
          perf
          bench
      5 montor helper and state
          

2 add async compute handler with procs and upstream
-------
3 support the custem encode and decode such as json.
-------
4 add async fiber handler with procs and upstream
-------
5 add replace  upstream with shmemory
-------

