import sys
import os
import time

#../thirdparty/protobuf/bin/protoc --python_out=.  --proto_path=../thirdparty/protobuf/include/google/protobuf  ../thirdparty/protobuf/include/google/protobuf/descriptor.proto


sys.path.append(os.path.abspath(os.path.dirname(__file__)) + '/../thirdparty/protobuf/python')
#import google
#import google.protobuf

sys.path.append(os.path.abspath(os.path.dirname(__file__)) + '/../inspect_server/')
from inspect_pb2 import *

import urllib2

if __name__ == '__main__':

    url = "http://127.0.0.1:8081/ngxrpc/inspect/application/requeststatus"
    #url = "http://127.0.0.1:8081/ngxrpc/inspect/application/interface"
    reqpb = Request()
    reqpb.json = "hehe2"
    data = reqpb.SerializeToString()

    req = urllib2.Request(url)
    req.add_header('Content-Type', 'application/json')

    start  = time.time()
    response = urllib2.urlopen(url, data)

    res = Response()
    res.ParseFromString(response.read())
    print "time:%f, res:%s" % ((time.time() - start ), res.json)


