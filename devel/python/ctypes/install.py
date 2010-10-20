#!/prog/sdpsoft/python2.4/bin/python
import os
import os.path
import stat
import shutil
import sys
import py_compile

script_mode = 0775
data_mode   = 0664
dir_mode    = 0775
uid         = os.getuid()

def get_SDP_ROOT():
    cpu = os.uname()[4]
    RH  = open('/etc/redhat-release').read().split()[6]
    res_target = "%s_RH_%s" % (cpu , RH)
    sdp_root = "/project/res/%s_RH_%s" % (cpu , RH)
    return (sdp_root , float(RH))



(SDP_ROOT , RH) = get_SDP_ROOT()
target = "%s/lib/python" % SDP_ROOT
res_guid    = os.stat("/project/res")[stat.ST_GID]



def chgrp(path , guid ):
    os.chown( path , -1 , guid )


def install_link( target_file , link_name):
    if os.path.exists( link_name ):
        os.unlink( link_name )
    print "Linking: %s -> %s" % (link_name , target_file )
    os.symlink( target_file , link_name )
    

def install_file( src_file , target_file):
    if not os.path.exists( src_file ):
        raise Exception("Copying %s -> %s" % (src_file , target_file))
        sys.exit("The source file:%s does not exist?? " % src_file )
    shutil.copyfile( src_file , target_file )
    print "Updating file: %s" % target_file
    if os.stat( target_file )[stat.ST_UID] == uid:
        chgrp( target_file , res_guid )

        st = os.stat( src_file )
        if st[stat.ST_MODE] & stat.S_IXUSR: 
            os.chmod( target_file , script_mode )
        else:
            os.chmod( target_file , data_mode )


def make_dir( target_dir ):
    if not os.path.exists( target_dir ):
        os.makedirs( target_dir )
        print "Creating directory: %s" % target_dir

    if os.stat( target_dir )[stat.ST_UID] == uid:
        chgrp( target_dir , res_guid )
        os.chmod( target_dir , dir_mode )


def install_path( src_path , target_root , extensions = None):
    target_dir = "%s/%s" % (target_root , src_path)
    
    make_dir( target_dir )
    dir_entries  = []
    file_entries = []

    for entry in os.listdir( src_path ):
        if entry == ".svn":
            continue
        
        if entry[-1] == "~":
            continue

        full_path = "%s/%s" % (src_path , entry)
        if os.path.isdir( full_path ):
            dir_entries.append( full_path )
        else:
            if extensions:
                (base, ext) = os.path.splitext( full_path )
                if ext[1:] in extensions:
                    file_entries.append( full_path )
            else:
                file_entries.append( full_path )

    for file in file_entries:
        target_file = "%s/%s" % (target_root , file )
        (base, ext) = os.path.splitext( file )
        install_file( file , target_file )
        if ext == ".py":
            print "Byte compiling: %s" % target_file
            py_compile.compile( target_file )
            pyc_file = target_file + "c"
            chgrp( pyc_file , res_guid )
            os.chmod( pyc_file , data_mode )
    
    #Recursive"
    for dir in dir_entries:
        install_path( dir , target_root , extensions = extensions)
    

os.umask( 2 )
make_dir("%s/lib/python"  % SDP_ROOT)
make_dir("%s/lib/python/lib"  % SDP_ROOT)
cwd = os.getcwd()
install_path( "ert" , target , extensions = ["py"])
install_file( "C/libpycfile/slib/libpycfile.so"         , "%s/lib/python/lib/libpycfile.so"       % SDP_ROOT)
install_file( "../../libutil/slib/libutil.so"           , "%s/lib/python/lib/libutil.so"      % SDP_ROOT)
install_file( "../../libconfig/slib/libconfig.so"       , "%s/lib/python/lib/libconfig.so"    % SDP_ROOT)
install_file( "../../libecl/slib/libecl.so"             , "%s/lib/python/lib/libecl.so"       % SDP_ROOT)
install_file( "../../libjob_queue/slib/libjob_queue.so" , "%s/lib/python/lib/libjob_queue.so" % SDP_ROOT)
install_link( "%s/lib/python/lib/libpycfile.so" % SDP_ROOT , "%s/lib/python/ert/util/pycfile.so" % SDP_ROOT)
install_link( "%s/C/libpycfile/slib/libpycfile.so"  % cwd       , "%s/ert/util/pycfile.so" % cwd)
