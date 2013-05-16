from filegenerator import StagedObject
from os import path
import scxutil

##
# Shell script for loading environment variables needed by OM
#
class SCXSetup(StagedObject):
    ##
    # Ctor.
    # \param[in] path Path of target file relative to target root directory.
    # \param[in] platform Platform to build file for
    # \param[in] buildType Debug, Release, Bullseye.
    # \param[in] permissions File permissions.
    # \param[in] owner File owner.
    # \param[in] group File group.
    #
    def __init__(self, path, platform, buildType, permissions, owner, group):
        StagedObject.__init__(self, path, permissions, owner, group, 'file')
        self.platform = platform
        self.buildType = buildType
        
    ##
    # Set source directory root.
    # \param[in] rootDir Absolute path to source directory root.
    #
    def SetSrcRootDir(self, rootDir):
        pass

    ##
    # Create the file. Copies the file from source to dest.
    #
    def DoCreate(self) :
        shfile = open(path.join(self.rootDir, self.path), 'w')
        shfile.write('# Copyright (c) Microsoft Corporation.  All rights reserved.\n')
        # Configure script to not complain if environment variable isn't currently set
        shfile.write('set +u\n')
        if self.platform.OS == 'MacOS':
            shfile.write('PATH=/usr/libexec/microsoft/scx/bin:$PATH' + '\n')
        else:
            shfile.write('PATH=/opt/microsoft/scx/bin:$PATH' + '\n')
        shfile.write('export PATH' + '\n')
        if self.platform.OS == 'MacOS':
            shfile.write('DYLD_LIBRARY_PATH=/usr/libexec/microsoft/scx/lib:/usr/libexec/microsoft/scx/lib/providers:$DYLD_LIBRARY_PATH' + '\n')
            shfile.write('export DYLD_LIBRARY_PATH' + '\n')
        elif self.platform.OS == 'HPUX' and self.platform.Architecture == "pa-risc":
            shfile.write('SHLIB_PATH=/opt/microsoft/scx/lib:/opt/microsoft/scx/lib/providers:$SHLIB_PATH' + '\n')
            shfile.write('export SHLIB_PATH' + '\n')
        elif self.platform.OS == "SunOS" and self.platform.MajorVersion == 5 and self.platform.MinorVersion <= 9:
            shfile.write('LD_LIBRARY_PATH=/opt/microsoft/scx/lib:/opt/microsoft/scx/lib/providers:/usr/local/ssl/lib:$LD_LIBRARY_PATH' + '\n')
            shfile.write('export LD_LIBRARY_PATH' + '\n')
        else:
            shfile.write('LD_LIBRARY_PATH=/opt/microsoft/scx/lib:/opt/microsoft/scx/lib/providers:$LD_LIBRARY_PATH' + '\n')
            shfile.write('export LD_LIBRARY_PATH' + '\n')
        if self.buildType == 'Bullseye':
            shfile.write('COVFILE=/var/opt/microsoft/scx/log/OpsMgr.cov' + '\n')
            shfile.write('export COVFILE' + '\n')

##
# Shell script for loading environment variables needed by OM tools
#
class SCXSetupTools(StagedObject):
    ##
    # Ctor.
    # \param[in] path Path of target file relative to target root directory.
    # \param[in] platform Platform to build file for
    # \param[in] buildType Debug, Release, Bullseye.
    # \param[in] permissions File permissions.
    # \param[in] owner File owner.
    # \param[in] group File group.
    #
    def __init__(self, path, platform, buildType, permissions, owner, group):
        StagedObject.__init__(self, path, permissions, owner, group, 'file')
        self.platform = platform
        self.buildType = buildType
        
    ##
    # Set source directory root.
    # \param[in] rootDir Absolute path to source directory root.
    #
    def SetSrcRootDir(self, rootDir):
        pass

    ##
    # Create the file. Copies the file from source to dest.
    #
    def DoCreate(self) :
        shfile = open(path.join(self.rootDir, self.path), 'w')
        shfile.write('# Copyright (c) Microsoft Corporation.  All rights reserved.\n')
        # Configure script to not complain if environment variable isn't currently set
        shfile.write('set +u\n')
        if self.platform.OS == 'MacOS':
            shfile.write('PATH=/usr/libexec/microsoft/scx/bin/tools:$PATH' + '\n')
        else:
            shfile.write('PATH=/opt/microsoft/scx/bin/tools:$PATH' + '\n')
        shfile.write('export PATH' + '\n')
        if self.platform.OS == 'MacOS':
            shfile.write('DYLD_LIBRARY_PATH=/usr/libexec/microsoft/scx/lib:/usr/libexec/microsoft/scx/lib/providers:$DYLD_LIBRARY_PATH' + '\n')
            shfile.write('export DYLD_LIBRARY_PATH' + '\n')
        elif self.platform.OS == 'HPUX' and self.platform.Architecture == "pa-risc":
            shfile.write('SHLIB_PATH=/opt/microsoft/scx/lib:/opt/microsoft/scx/lib/providers:$SHLIB_PATH' + '\n')
            shfile.write('export SHLIB_PATH' + '\n')
        elif self.platform.OS == "SunOS" and self.platform.MajorVersion == 5 and self.platform.MinorVersion <= 9:
            shfile.write('LD_LIBRARY_PATH=/opt/microsoft/scx/lib:/opt/microsoft/scx/lib/providers:/usr/local/ssl/lib:$LD_LIBRARY_PATH' + '\n')
            shfile.write('export LD_LIBRARY_PATH' + '\n')
        else:
            shfile.write('LD_LIBRARY_PATH=/opt/microsoft/scx/lib:/opt/microsoft/scx/lib/providers:$LD_LIBRARY_PATH' + '\n')
            shfile.write('export LD_LIBRARY_PATH' + '\n')
        if self.buildType == 'Bullseye':
            shfile.write('COVFILE=/var/opt/microsoft/scx/log/OpsMgr.cov' + '\n')
            shfile.write('export COVFILE' + '\n')

##
# Shell script for loading environment variables needed by OM tools
#
class SCXAdmin(StagedObject):
    ##
    # Ctor.
    # \param[in] path Path of target file relative to target root directory.
    # \param[in] platform Platform to build file for
    # \param[in] permissions File permissions.
    # \param[in] owner File owner.
    # \param[in] group File group.
    #
    def __init__(self, path, platform, permissions, owner, group):
        StagedObject.__init__(self, path, permissions, owner, group, 'file')
        self.platform = platform
        
    ##
    # Set source directory root.
    # \param[in] rootDir Absolute path to source directory root.
    #
    def SetSrcRootDir(self, rootDir):
        pass

    ##
    # Create the file.
    #
    def DoCreate(self) :
        shfile = open(path.join(self.rootDir, self.path), 'w')
        shfile.write( scxutil.Get_sh_path(self.platform.OS) );
        shfile.write( '\n\n' );
        shfile.write('# Copyright (c) Microsoft Corporation.  All rights reserved.\n\n')

        if self.platform.OS == 'MacOS':
            scxpath = "/usr/libexec"
        else:
            scxpath = "/opt"

        shfile.write('. ' + scxpath + '/microsoft/scx/bin/tools/setup.sh\n')
        shfile.write('exec ' + scxpath + '/microsoft/scx/bin/tools/.scxadmin "$@"\n')


##
# Shell script for configuring SSL
#
class SCXSSLConfig(StagedObject):
    ##
    # Ctor.
    # \param[in] path Path of target file relative to target root directory.
    # \param[in] platform Platform to build file for
    # \param[in] permissions File permissions.
    # \param[in] owner File owner.
    # \param[in] group File group.
    #
    def __init__(self, path, platform, permissions, owner, group):
        StagedObject.__init__(self, path, permissions, owner, group, 'file')
        self.platform = platform
        
    ##
    # Set source directory root.
    # \param[in] rootDir Absolute path to source directory root.
    #
    def SetSrcRootDir(self, rootDir):
        pass

    ##
    # Create the file.
    #
    def DoCreate(self) :
        shfile = open(path.join(self.rootDir, self.path), 'w')
        shfile.write( scxutil.Get_sh_path(self.platform.OS) );
        shfile.write( '\n\n' );
        shfile.write('# Copyright (c) Microsoft Corporation.  All rights reserved.\n\n')

        if self.platform.OS == 'MacOS':
            scxpath = "/usr/libexec"
        else:
            scxpath = "/opt"

        shfile.write('. ' + scxpath + '/microsoft/scx/bin/tools/setup.sh\n')
        shfile.write('exec ' + scxpath + '/microsoft/scx/bin/tools/.scxsslconfig "$@"\n')
