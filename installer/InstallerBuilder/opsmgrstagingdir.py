# coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
##
# Module containing classes to populate a staging directory
#
# Date:   2007-09-25 13:45:34
#

import filegenerator
import scxutil
import omfilegenerator
from os import path
from stagingdir import StagingDir

##
# Class containing logic for creating the staging directory
# structure.
#
class OpsMgrStagingDir(StagingDir):
    ##
    # Ctor.
    # \param[in] buildInfo OpsMgrBuildInformation instance that tells us about where certain folders are.
    # \param[in] platform  Tells us what platform we are building for.
    #
    def __init__(self, buildInfo, platform):
        self.targetDir = buildInfo.GetTargetDir()
        self.installerDir = path.join(buildInfo.GetBaseDir(), 'installer')
        self.intermediateDir = path.join(path.join(buildInfo.GetBaseDir(), 'intermediate'), buildInfo.CreatePlatformString())
        self.stagingDir = buildInfo.GetStagingDir()
        self.platform = platform
        self.buildType = buildInfo.GetBuildType()

        self.sharedLibrarySuffix = 'so'
        if self.platform.OS == 'HPUX' and self.platform.Architecture == "pa-risc":
            self.sharedLibrarySuffix = 'sl'
        elif self.platform.OS == 'MacOS':
            self.sharedLibrarySuffix = 'dylib'

        # Where is /etc, /opt, and /var?
        if self.platform.OS == 'MacOS':
            self.etcRoot = 'private/etc'
            self.optRoot = 'usr/libexec'
            self.varRoot = 'private/var'
        else:
            self.etcRoot = 'etc'
            self.optRoot = 'opt'
            self.varRoot = 'var'

        # Name of group with id 0
        self.rootGroupName = 'root'
        if self.platform.OS == 'AIX':
            self.rootGroupName = 'system'
        elif self.platform.OS == 'MacOS':
            self.rootGroupName = 'wheel'

    ##
    # Creates a file map mapping destination path to source path
    #
    def CreateStagingObjectList(self):

        stagingDirectories = [
            filegenerator.NewDirectory('',                                               700, 'root', self.rootGroupName,'sysdir'),
            ]

        # For MacOS, create /private (/private/etc, and /private/var)
        if self.platform.OS == 'MacOS':
            stagingDirectories = stagingDirectories + [
                filegenerator.NewDirectory('private',                                    755, 'root', self.rootGroupName, 'sysdir')
            ]

        stagingDirectories = stagingDirectories + [
            filegenerator.NewDirectory('usr',                                            755, 'root', self.rootGroupName, 'sysdir'),
            filegenerator.NewDirectory('usr/sbin',                                       755, 'root', self.rootGroupName, 'sysdir'),
            filegenerator.NewDirectory(self.optRoot,                                     755, 'root', self.rootGroupName, 'sysdir'),
            filegenerator.NewDirectory(self.etcRoot,                                     755, 'root', self.rootGroupName, 'sysdir'),
            filegenerator.NewDirectory(self.etcRoot + '/opt',                            755, 'root', self.rootGroupName, 'sysdir'),
            filegenerator.NewDirectory(self.varRoot,                                     755, 'root', self.rootGroupName, 'sysdir'),
            filegenerator.NewDirectory(self.varRoot + '/opt',                            755, 'root', self.rootGroupName, 'sysdir'),
            filegenerator.NewDirectory(self.optRoot + '/microsoft',                      755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.optRoot + '/microsoft/scx',                  755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.optRoot + '/microsoft/scx/bin',              755, 'root', self.rootGroupName)
            ]

        # Create an empty scxUninstall.sh script for now (bit of a chicken and egg thing)
        # The contents of this file will be filled in later on (after files are staged)
        # Note: Do this as early as possible so scxUninstall.sh is deleted near the very end!
        if self.platform.OS == 'MacOS':
            stagingDirectories = stagingDirectories + [
                filegenerator.EmptyFile(self.optRoot + '/microsoft/scx/bin/scxUninstall.sh',
                                    744, 'root', self.rootGroupName)
                ]

        stagingDirectories = stagingDirectories + [
            filegenerator.NewDirectory(self.optRoot + '/microsoft/scx/bin/tools',        755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.optRoot + '/microsoft/scx/lib',              755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.optRoot + '/microsoft/scx/lib/providers',    755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.optRoot + '/microsoft/scx/lib/providers/ext', 755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.etcRoot + '/opt/microsoft',                  755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.etcRoot + '/opt/microsoft/scx',              755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.etcRoot + '/opt/microsoft/scx/conf',         755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.etcRoot + '/opt/microsoft/scx/ssl',          755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.varRoot + '/opt/microsoft',                  755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.varRoot + '/opt/microsoft/scx',              755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.varRoot + '/opt/microsoft/scx/log',          755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.varRoot + '/opt/microsoft/scx/lib',          755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.varRoot + '/opt/microsoft/scx/lib/state',    755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.varRoot + '/opt/microsoft/scx/tmp',          755, 'root', self.rootGroupName),
            filegenerator.NewDirectory(self.varRoot + '/opt/microsoft/scx/tmp/localauth', 755, 'root', self.rootGroupName),
            ]


        # MacOS doesn't have /etc/init.d
        if self.platform.OS != 'MacOS':
            stagingDirectories = stagingDirectories + [
                filegenerator.NewDirectory(self.etcRoot + '/init.d',                     755, 'root', 'sys', 'sysdir')
                ]


        if self.platform.OS == 'SunOS':
            if self.platform.MajorVersion == 5 and self.platform.MinorVersion < 10:
                stagingDirectories = stagingDirectories + [
                    filegenerator.NewDirectory(self.etcRoot + '/rc2.d',  755, 'root', 'bin', 'sysdir')
                ]
            else:
                stagingDirectories = stagingDirectories + [
                    filegenerator.NewDirectory('var/svc/',                                   755, 'root', 'sys', 'sysdir'),
                    filegenerator.NewDirectory('var/svc/manifest',                           755, 'root', 'sys', 'sysdir'),
                    filegenerator.NewDirectory('var/svc/manifest/application',               755, 'root', 'sys', 'sysdir'),
                    filegenerator.NewDirectory('var/svc/manifest/application/management',    755, 'root', 'sys', 'sysdir')
                    ]
        elif self.platform.OS == 'HPUX':
            stagingDirectories = stagingDirectories + [
                filegenerator.NewDirectory('var/log',                                    755, 'root', 'sys', 'sysdir'),
                filegenerator.NewDirectory('sbin',                                       755, 'root', 'bin', 'sysdir'),
                filegenerator.NewDirectory('sbin/init.d',                                755, 'root', 'bin', 'sysdir'),
                filegenerator.NewDirectory('sbin/rc1.d',                                 755, 'root', 'bin', 'sysdir'),
                filegenerator.NewDirectory('sbin/rc2.d',                                 755, 'root', 'bin', 'sysdir')
                ]
        elif self.platform.OS == 'AIX':
            # These directories are actually only needed for the package format itself.
            stagingDirectories = stagingDirectories + [
                filegenerator.NewDirectory('usr/lpp',                                    755, 'root', self.rootGroupName, 'sysdir'),
                filegenerator.NewDirectory('usr/lpp/scx.rte',                            755, 'root', self.rootGroupName)
                ]
        elif self.platform.OS == 'MacOS':
            # These directories are needed for the launchd startup scripts
            stagingDirectories = stagingDirectories + [
                filegenerator.NewDirectory('Library',                                    775, 'root', 'admin', 'sysdir'),
                filegenerator.NewDirectory('Library/LaunchDaemons',                      755, 'root', self.rootGroupName, 'sysdir')
                ]

        installerStagingFiles = [
            # ToDo            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/README', 'docs/README'),
            # ToDo            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/CHANGELOG', 'docs/CHANGELOG'),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/LICENSE', 
                                   'docs/scx_license.txt',
                                   644, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/COPYRIGHT', 
                                   'docs/copyright.txt',
                                   644, 'root', self.rootGroupName)
            ]

        # For service start on solaris
        if self.platform.OS == 'SunOS':
            if self.platform.MajorVersion == 5 and self.platform.MinorVersion < 10:
                installerStagingFiles = installerStagingFiles + [
                    filegenerator.FileCopy(self.etcRoot + '/init.d/scx-cimd',
                                           'conf/init.d/scx-cimd.sun8',
                                           744, 'root', self.rootGroupName),
                    filegenerator.SoftLink('etc/rc2.d/S999scx-cimd',
                                          '../init.d/scx-cimd',
                                           744, 'root', self.rootGroupName),
                    ]
            else:
                installerStagingFiles = installerStagingFiles + [
                    filegenerator.FileCopy('opt/microsoft/scx/bin/tools/scx-cimd',
                                           'conf/svc-method/scx-cimd',
                                           555, 'root', 'bin'),
                    filegenerator.FileCopy('var/svc/manifest/application/management/scx-cimd.xml',
                                           'conf/svc-manifest/scx-cimd.xml',
                                           444, 'root', 'sys')
                    ]
        # For service start on linux
        elif self.platform.OS == 'Linux' and self.platform.Distribution == 'SUSE':
            installerStagingFiles = installerStagingFiles + [
                filegenerator.FileCopy(self.etcRoot + '/init.d/scx-cimd',
                                       'conf/init.d/scx-cimd.sles',
                                       744, 'root', self.rootGroupName)
                ]
        elif self.platform.OS == 'Linux' and self.platform.Distribution == 'REDHAT':
            installerStagingFiles = installerStagingFiles + [
                filegenerator.FileCopy(self.etcRoot + '/init.d/scx-cimd',
                                       'conf/init.d/scx-cimd.rhel',
                                       744, 'root', self.rootGroupName)
                ]
        elif self.platform.OS == 'Linux' and self.platform.Distribution == 'UBUNTU':
            installerStagingFiles = installerStagingFiles + [
                filegenerator.FileCopy(self.etcRoot + '/init.d/scx-cimd',
                                       'conf/init.d/scx-cimd.ubuntu',
                                       744, 'root', self.rootGroupName)
                ]
        # For service start on hpux
        elif self.platform.OS == 'HPUX':
            installerStagingFiles = installerStagingFiles + [
                filegenerator.FileCopy('sbin/init.d/scx-cimd',
                                       'conf/init.d/scx-cimd.hpux',
                                       744, 'root', self.rootGroupName),
                filegenerator.SoftLink('sbin/rc2.d/S999scx-cimd',
                                       '../init.d/scx-cimd',
                                       744, 'root', self.rootGroupName),
                filegenerator.SoftLink('sbin/rc1.d/K100scx-cimd',
                                       '../init.d/scx-cimd',
                                       744, 'root', self.rootGroupName),
                ]
        # For service start on Mac OS
        elif self.platform.OS == 'MacOS':
            installerStagingFiles = installerStagingFiles + [
                filegenerator.FileCopy('Library/LaunchDaemons/com.microsoft.scx-cimd.plist',
                                       'conf/launchd/com.microsoft.scx-cimd.plist',
                                       644, 'root', self.rootGroupName),
                ]

        for stagingFile in installerStagingFiles:
            stagingFile.SetSrcRootDir(self.installerDir)

        # These files are added here as empty files. In some cases they are filled
        # with information in the postinstall script. In some cases they are empty
        # configuration files that could be edited once installed.
        emptyStagingFiles = [
            filegenerator.EmptyFile(self.etcRoot + '/opt/microsoft/scx/conf/installinfo.txt',
                                    644, 'root', self.rootGroupName),
            filegenerator.EmptyFile(self.etcRoot + '/opt/microsoft/scx/conf/cimserver_planned.conf',
                                    644, 'root', self.rootGroupName, 'conffile'),
            filegenerator.EmptyFile(self.etcRoot + '/opt/microsoft/scx/conf/cimserver_current.conf',
                                    644, 'root', self.rootGroupName, 'conffile'),
            filegenerator.EmptyFile(self.etcRoot + '/opt/microsoft/scx/conf/scxlog.conf',
                                    644, 'root', self.rootGroupName, 'conffile'),
            filegenerator.EmptyFile(self.etcRoot + '/opt/microsoft/scx/conf/scxrunas.conf',
                                   644, 'root', self.rootGroupName, 'conffile')
            ]

        GeneratedFiles = [
            omfilegenerator.SCXSetup(self.optRoot + '/microsoft/scx/bin/setup.sh',
                                     self.platform, self.buildType,
                                     644, 'root', self.rootGroupName),
            omfilegenerator.SCXSetupTools(self.optRoot + '/microsoft/scx/bin/tools/setup.sh',
                                          self.platform, self.buildType,
                                          644, 'root', self.rootGroupName),
            # The scxadmin script is intentionally w:x - we want non-privileged functions to work
            omfilegenerator.SCXAdmin(self.optRoot + '/microsoft/scx/bin/tools/scxadmin',
                                     self.platform,
                                     755, 'root', self.rootGroupName),
            omfilegenerator.SCXSSLConfig(self.optRoot + '/microsoft/scx/bin/tools/scxsslconfig',
                                         self.platform,
                                         755, 'root', self.rootGroupName),
            ]

        # Create a link to make scxadmin easier to run ...
        #
        # On Solaris 5.10 and later, we deal with the /usr/sbin/scxadmin link via postinstall;
        # This is to allow zones to work properly for sparse root zones
        if self.platform.OS != 'SunOS' or (self.platform.MajorVersion == 5 and self.platform.MinorVersion < 10):
            GeneratedFiles = GeneratedFiles + [
                filegenerator.SoftLink('usr/sbin/scxadmin',
                                       '../../' + self.optRoot + '/microsoft/scx/bin/tools/scxadmin',
                                       755, 'root', self.rootGroupName)
                ]
                
        if self.buildType  == 'Bullseye':
            GeneratedFiles = GeneratedFiles + [
               filegenerator.FileCopy(self.varRoot + '/opt/microsoft/scx/log/OpsMgr.cov',
                                      'OpsMgr.cov',
                                      777, 'root', self.rootGroupName) ]
				
        for stagingFile in GeneratedFiles:
            stagingFile.SetSrcRootDir(self.intermediateDir)


        pegasusBinStagingFiles = [
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/bin/scxcimprovider',
                                   'bin/cimprovider',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/bin/scxcimprovagt',
                                   'bin/cimprovagt',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/bin/scxcimservera',
                                   'bin/cimservera',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/bin/scxcimserver',
                                   'bin/cimserver',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/bin/tools/scxcimmof',
                                   'bin/cimmof',
                                   744, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/bin/tools/scxcimmofl',
                                   'bin/cimmofl',
                                   744, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/bin/tools/scxcimconfig',
                                   'bin/cimconfig',
                                   744, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/bin/tools/scxwbemexec',
                                   'bin/wbemexec',
                                   744, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/bin/tools/scxcimcli',
                                   'bin/cimcli',
                                   744, 'root', self.rootGroupName)
        ]

        for stagingFile in pegasusBinStagingFiles:
            stagingFile.SetSrcRootDir(path.join(self.intermediateDir, 'pegasus'))

        # File name suffixes are added later.
        pegasusLibStagingFiles = [
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/providers/libCIMOMStatDataProvider.',
                                   'lib/libCIMOMStatDataProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/providers/libCIMOMStatDataProvider.',
                                   'libCIMOMStatDataProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libCIMxmlIndicationHandler.',
                                   'lib/libCIMxmlIndicationHandler.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libCIMxmlIndicationHandler.',
                                   'libCIMxmlIndicationHandler.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libcmpiCppImpl.',
                                   'lib/libcmpiCppImpl.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libcmpiCppImpl.',
                                   'libcmpiCppImpl.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libcmpiCWS_Util.',
                                   'lib/libcmpiCWS_Util.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libcmpiCWS_Util.',
                                   'libcmpiCWS_Util.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libCMPIProviderManager.',
                                   'lib/libCMPIProviderManager.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libCMPIProviderManager.',
                                   'libCMPIProviderManager.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libcmpiUtilLib.',
                                   'lib/libcmpiUtilLib.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libcmpiUtilLib.',
                                   'libcmpiUtilLib.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/providers/libConfigSettingProvider.',
                                   'lib/libConfigSettingProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/providers/libConfigSettingProvider.',
                                   'libConfigSettingProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/providers/libCertificateProvider.',
                                   'lib/libCertificateProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/providers/libCertificateProvider.',
                                   'libCertificateProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libDefaultProviderManager.',
                                   'lib/libDefaultProviderManager.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libDefaultProviderManager.',
                                   'libDefaultProviderManager.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/providers/libInteropProvider.',
                                   'lib/libInteropProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/providers/libInteropProvider.',
                                   'libInteropProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/providers/libNamespaceProvider.',
                                   'lib/libNamespaceProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/providers/libNamespaceProvider.',
                                   'libNamespaceProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegauthentication.',
                                   'lib/libpegauthentication.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegauthentication.',
                                   'libpegauthentication.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegclient.',
                                   'lib/libpegclient.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegclient.',
                                   'libpegclient.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegcliutils.',
                                   'lib/libpegcliutils.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegcliutils.',
                                   'libpegcliutils.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegcommon.',
                                   'lib/libpegcommon.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegcommon.',
                                   'libpegcommon.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegconfig.',
                                   'lib/libpegconfig.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegconfig.',
                                   'libpegconfig.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegwsmserver.',
                                   'lib/libpegwsmserver.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegwsmserver.',
                                   'libpegwsmserver.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegexportclient.',
                                   'lib/libpegexportclient.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegexportclient.',
                                   'libpegexportclient.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegexportserver.',
                                   'lib/libpegexportserver.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegexportserver.',
                                   'libpegexportserver.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpeggetoopt.',
                                   'lib/libpeggetoopt.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpeggetoopt.',
                                   'libpeggetoopt.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpeghandlerservice.',
                                   'lib/libpeghandlerservice.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpeghandlerservice.',
                                   'libpeghandlerservice.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegindicationservice.',
                                   'lib/libpegindicationservice.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegindicationservice.',
                                   'libpegindicationservice.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegpmservice.',
                                   'lib/libpegpmservice.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegpmservice.',
                                   'libpegpmservice.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegprm.',
                                   'lib/libpegprm.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegprm.',
                                   'libpegprm.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegprovidermanager.',
                                   'lib/libpegprovidermanager.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegprovidermanager.',
                                   'libpegprovidermanager.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/providers/libpegprovider.',
                                   'lib/libpegprovider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/providers/libpegprovider.',
                                   'libpegprovider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegquerycommon.',
                                   'lib/libpegquerycommon.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegquerycommon.',
                                   'libpegquerycommon.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegqueryexpression.',
                                   'lib/libpegqueryexpression.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegqueryexpression.',
                                   'libpegqueryexpression.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegrepository.',
                                   'lib/libpegrepository.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegrepository.',
                                   'libpegrepository.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegserver.',
                                   'lib/libpegserver.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegserver.',
                                   'libpegserver.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegservice.',
                                   'lib/libpegservice.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegservice.',
                                   'libpegservice.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpeguser.',
                                   'lib/libpeguser.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpeguser.',
                                   'libpeguser.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegwql.',
                                   'lib/libpegwql.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegwql.',
                                   'libpegwql.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/providers/libProviderRegistrationProvider.',
                                   'lib/libProviderRegistrationProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/providers/libProviderRegistrationProvider.',
                                   'libProviderRegistrationProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/providers/libUserAuthProvider.',
                                   'lib/libUserAuthProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/providers/libUserAuthProvider.',
                                   'libUserAuthProvider.',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/libpegcompiler.',
                                   'lib/libpegcompiler.',
                                   755, 'root', self.rootGroupName),
            filegenerator.SoftLink(self.optRoot + '/microsoft/scx/lib/libpegcompiler.',
                                   'libpegcompiler.',
                                   755, 'root', self.rootGroupName),
            ]

        for stagingFile in pegasusLibStagingFiles:
            stagingFile.SetSrcRootDir(path.join(self.intermediateDir, 'pegasus'))

            # Here we add the file suffixes.
            if stagingFile.GetFileType() == 'file':
                if self.platform.OS == 'HPUX':
                    stagingFile.SetPath(stagingFile.GetPath() + '1')
                    stagingFile.SetSrcPath(stagingFile.GetSrcPath() + '1')
                else:
                    stagingFile.SetPath(stagingFile.GetPath() + self.sharedLibrarySuffix + '.1')
                    stagingFile.SetSrcPath(stagingFile.GetSrcPath() + self.sharedLibrarySuffix)
            if stagingFile.GetFileType() == 'link':
                stagingFile.SetPath(stagingFile.GetPath() + self.sharedLibrarySuffix)
                if self.platform.OS == 'HPUX':
                    stagingFile.SetTarget(stagingFile.GetTarget() + '1')
                else:
                    stagingFile.SetTarget(stagingFile.GetTarget() + self.sharedLibrarySuffix + '.1')



        repositoryDirectory = filegenerator.DirectoryCopy(self.varRoot + '/opt/microsoft/scx/lib/repository',
                                                          path.join(self.intermediateDir, 'pegasus/repository'),
                                                          755, 'root', self.rootGroupName)

        repositoryDirectories = repositoryDirectory.GetDirectoryList()
        repositoryFiles = repositoryDirectory.GetFileList()
        # We add some empty files as placeholders for files that are written by
        # Pegasus at runtime.
        repositoryFiles = repositoryFiles + [
            filegenerator.EmptyFile(self.varRoot + '/opt/microsoft/scx/lib/repository/root#scx/classes/PG_ElementConformsToProfile.CIM_ElementConformsToProfile',
                                    644, 'root', self.rootGroupName),
            filegenerator.EmptyFile(self.varRoot + '/opt/microsoft/scx/lib/repository/root#scx/classes/PG_RegisteredProfile.CIM_RegisteredProfile',
                                    644, 'root', self.rootGroupName),
            filegenerator.EmptyFile(self.varRoot + '/opt/microsoft/scx/lib/repository/root#PG_InterOp/instances/associations',
                                    644, 'root', self.rootGroupName),
            filegenerator.EmptyFile(self.varRoot + '/opt/microsoft/scx/lib/repository/root#PG_InterOp/instances/PG_ObjectManager.instances',
                                    644, 'root', self.rootGroupName),
            filegenerator.EmptyFile(self.varRoot + '/opt/microsoft/scx/lib/repository/root#PG_InterOp/instances/PG_ObjectManager.idx',
                                    644, 'root', self.rootGroupName)
            ]

        scxCoreStagingFiles = [
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/lib/providers/libSCXCoreProviderModule.' + self.sharedLibrarySuffix,
                                   'libSCXCoreProviderModule.' + self.sharedLibrarySuffix,
                                   755, 'root', self.rootGroupName),
            # Make tool binaries "hidden" ('.' prefix) since we have scripts to run it
            #
            # Permissions are intentionally w:x.  We protect the resources (files, etc), not
            # the binary itself.  This also allows non-prived functions (i.e. -status) to work.
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/bin/tools/.scxsslconfig',
                                   'scxsslconfig',
                                   755, 'root', self.rootGroupName),
            filegenerator.FileCopy(self.optRoot + '/microsoft/scx/bin/tools/.scxadmin',
                                   'scxadmin',
                                   755, 'root', self.rootGroupName),
            ]

        for stagingFile in scxCoreStagingFiles:
            stagingFile.SetSrcRootDir(self.targetDir)

        self.stagingObjectList = stagingDirectories + \
                                 repositoryDirectories + \
                                 installerStagingFiles + \
                                 emptyStagingFiles + \
                                 GeneratedFiles + \
                                 pegasusBinStagingFiles + \
                                 pegasusLibStagingFiles + \
                                 repositoryFiles + \
                                 scxCoreStagingFiles

        for stagingObject in self.stagingObjectList:
            stagingObject.SetRootDir(self.stagingDir)

    ##
    # Return the list of staging objects.
    # \returns The list of staging objects.
    #
    def GetStagingObjectList(self):
        return self.stagingObjectList

    ##
    # Return the root path of the staging directory.
    # \returns The root path of the staging directory.
    #
    def GetRootPath(self):
        return self.stagingDir

    ##
    # Generate the staging directory.
    #
    def Generate(self):
        if not path.isdir(self.stagingDir):
            scxutil.MkAllDirs(self.stagingDir)

        scxutil.RmTree(self.stagingDir)

        self.CreateStagingObjectList()

        for stagingObject in self.stagingObjectList:
            stagingObject.DoCreate()
