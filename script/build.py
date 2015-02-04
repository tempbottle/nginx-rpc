import sys
import getopt

#thirdparty 
from thirdparty import *
from help import *


def usage():
    print "usage:%s -m rpc_1 -m rpc_2 ..." % sys.argv[0]
    sys.exit(-1)


def get_rpc_modules():
    
    rpc_module = []
    proto_files = []
    
    try:
        opts, args = getopt.getopt(sys.argv[1:],
                               'm:p:h',['add_rpc_module=','add_proto=','help'])
        
        for o, a in opts:
            if o in ("-h", "--help"):
                usage()
                sys.exit()
            elif o in ("-m", "--add_rpc_module"):
                rpc_module.append(a)
            elif o in ("-p", "--add_proto"):
                proto_files.append(a)
            
        return rpc_module, proto_files
    
    except getopt.GetoptError:
        usage()
        
if __name__ == "__main__":

    # 0 check envirment
    gcc,gcc_path = get_gcc_version()
    
    if len(gcc) < 3 or gcc[0] != '4' or ( gcc[1] not in ['8','9']):
        print "please update your gcc to 4.8 or later,now is:" + str(gcc)
        sys.exit(-1)
    
    # 1 init thirdparty    
    result = map(lambda x :init_third_party(x, gcc_path), depends)


    
    rpc,proto = get_rpc_modules()
    
    if len(rpc) == 0 or len(proto) == 0:
        print "please assign your rpcs modules and proto files"
        usage()
    
    # 3 gen configure
    #map(lambda x: update_the_proto(x, gcc), proto)
    #update_rpc_config(rpc, proto)
    
    # 4 make
    #   configure
   
    # 6 test 
   


