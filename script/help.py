import subprocess
import os

realpath = os.path.split(os.path.realpath(__file__))[0]
realpath = "/".join(realpath.split('/')[0:-1])

def get_gcc_version():
    cmd = "gcc --version |grep gcc |awk '{print $4}'"
    res = subprocess.Popen(cmd,shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,close_fds=True)  
    result = res.stdout.readline().strip().split('.')
    
    return result, '.'.join([result[0],result[1]])
    
    
def do_sub_cmd(project, key):
    swith_path = "cd " + realpath + "/" + project["work_directory"]
    ret = 0
    if type(project[key]) !=  type([]):
        cmd = project[key]
        do_cmd = swith_path + ";" + cmd
        print "do:" + cmd 
        ret = os.system(do_cmd)
        print "ret:" + ("ok" if ret == 0 else "failed")
        return ret 
        
    for cmd in project[key]:
        do_cmd = swith_path + ";" + cmd
        print "do:" + cmd 
        ret = os.system(do_cmd)
        print "ret:" + ("ok" if ret == 0 else "failed")
        if 0 != ret :
           break   
    return ret
    

def init_third_party(project, gcc_path):

    thirdparty = realpath +"/thirdparty"
    
    
    if not os.access(realpath + "/" +project['work_directory'], os.R_OK):
        os.system("mkdir -p "+ realpath + "/" +project['work_directory'] )
    
    
    # do update 
    if 0 != do_sub_cmd(project, "update"):
       return 0
    
    # do init
    if 0 != do_sub_cmd(project, "init"):
       return 0
    
    #set up prefix install
    project['prefix'] = project['prefix'] + "/" + gcc_path    
    project['configure'] =  project['configure'] + realpath + "/" + project['prefix']
    
    # do confirgure
    if 0 != do_sub_cmd(project, "configure"):
       return -1
    
    # do build and install
    if 0 != do_sub_cmd(project, "install"):
       return -1
    
    return 0
