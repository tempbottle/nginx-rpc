import sys
import os
import time

#../thirdparty/protobuf/bin/protoc --python_out=.  --proto_path=../thirdparty/protobuf/include/google/protobuf  ../thirdparty/protobuf/include/google/protobuf/descriptor.proto


sys.path.append(os.path.abspath(os.path.dirname(__file__)) + '/../thirdparty/protobuf/protobuf-2.6.1/python')
#import google
#import google.protobuf

sys.path.append(os.path.abspath(os.path.dirname(__file__)) + '/../ngx_rpc_inspect_module/')
from inspect_pb2 import *

import urllib2
urllib2.getproxies = lambda: {}

if __name__ == '__main__':

    url = "http://10.25.66.77:8082/ngxrpc/inspect/application/requeststatus"
    #url = "http://10.25.66.77:8082/ngxrpc/inspect/application/interface"
    reqpb = Request()
    reqpb.json = "dasfgsgsdfsdfhehe2"
    data = reqpb.SerializeToString()

    opener = urllib2.build_opener()
    opener.addheaders = {'Content-Type':'application/json'}.items()
    start  = time.time()
    
    response = opener.open(url, data);
    print "header:%s" % response.headers
    res = Response()
    res.ParseFromString(response.read())
    
    print "time:%f, res:%s" % ((time.time() - start ), res.json)


