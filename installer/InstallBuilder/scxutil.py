# coding: utf-8
import shutil
import os
import fnmatch

##
# Copy all files in srcDir matching pattern to dstDir.
# If dstDir does not exist it is created.
# \param[in] srcDir Absolute path to source directory.
# \param[in] pattern File pattern to match.
# \param[in] dstDir Absolute path to destination directory.
#
def CopyPattern(srcDir, pattern, dstDir):
    
    names = fnmatch.filter(os.listdir(srcDir), pattern)

    for name in names:
        srcname = os.path.join(srcDir, name)
        dstname = os.path.join(dstDir, name)
        if os.path.islink(srcname):
            print("Symbolic link: " + srcname + " -> " + dstname)
            self.copy(srcname, dstname)
        elif os.path.isfile(srcname):
            print("Regular file:  " + srcname + " -> " + dstname)
            self.copy(srcname, dstname)
        elif os.path.isdir(srcname):
            print("Directory:     " + srcname + " Ignored")
        else:
            print("Unknown:       " + srcname + " Ignored")


##
# Utility method to copy files with all permissions retained
# \param[in] srcPath Absolute source path.
# \param[in] dstPath Absolute destination path.
#
def Copy(srcPath, dstPath):
    retval = os.system("cp -p \"" + srcPath + "\" \"" + dstPath + "\"")
    if retval != 0:
        print("Unable to copy from %s to %s." % (srcPath, dstPath))
        exit(1)
    
##
# Utility method to move files with all permissions retained
# \param[in] srcPath Absolute source path.
# \param[in] dstPath Absolute destination path.
#
def Move(srcPath, dstPath):
    retval = os.system("mv \"" + srcPath + "\" \"" + dstPath + "\"")
    if retval != 0:
        print("Unable to move from %s to %s." % (srcPath, dstPath))
        exit(1)
    
##
# Utility method to create a soft link
# \param[in] path path to link destination.
# \param[in] dstPath Absolute destination path.
#
def Link(path, dstPath):
    retval = os.system("ln -s \"" + path + "\" \"" + dstPath + "\"")
    if retval != 0:
        print("Unable to ln -s %s %s." % (path, dstPath))
        exit(1)
    
##
# Utility method to touch a file
# \param[in] path Absolute path.
#
def Touch(path):
    retval = os.system("touch \"" + path + "\"")
    if retval != 0:
        print("Unable to touch %s." % path)
        exit(1)

##
# Utility method to change owner/group on a file.
#
# \param[in] path Absolute path
# \param[in] uid New owner (passed as string)
# \param[in] gid New group (passed as string)
#
def ChOwn(path,uid,gid):
    # Must use 'os.system' rather than os.chown because 'root' will be passed
    # (and we use 'sudo' to allow things like uid=root)
    retval = os.system('sudo chown %s:%s %s' % (uid,gid,path))
    if retval != 0:
        print("Unable to chown %s." % path)
        exit(1)

##
# Utility method to change permissions on a file
# \param[in] path Absolute path
# \param[in] mode Numeric mode to set the file to
#
def ChMod(path,mode):
    # Muse use 'os.system' rather than os.chmod because we may need 'sudo' ...
    retval = os.system('sudo chmod %s %s' % (mode, path))
    if retval != 0:
        print("Unable to chmod %s." % path)
        exit(1)

##
# Utility method to create a new directory
# \param[in] path Absolute path to directory to create.
#
def MkDir(path):
    os.mkdir(path)

##
# Utility method to create a new directory and all parent directories
# needed.
# \param[in] path Absolute path to directory to create.
#
def MkAllDirs(path):
    if not os.path.isdir(path):
        os.makedirs(path)

##
# Utility method to remove a directory ignoring errors
# \param[in] path Absolute path to directory to remove.
#
def RmTree(path):
    shutil.rmtree(path, 1)

