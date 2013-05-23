#!/bin/sh

#####################################################################
## Copyright (c) Microsoft Corporation.  All rights reserved.
## Name: GetLinuxOS.sh 
## Date: 7/1/2012
## Version: 1.0
## Description: Release file parsing for Linux Operating Systems
#####################################################################

Hostname=`uname -n`
OSName=`uname -s`
Version=`uname -r`      # Version
Arch=`uname -m`         # Overridden as uname -p on some platforms
ReleaseFile=""
EtcPath="/etc"

# Create destination directory if it does not exist
RelDir="<RelDir>"
RelFile="$RelDir/scx-release"
DisableFile="$RelDir/disablereleasefileupdates"

if [ ! -e ${RelDir} ]; then 
    mkdir -p $RelDir
fi

## Function used to get Linux Distro and Version
######################################################################
GetKitType() {
    # The os-release file doesn't tell us the type of kit, so try to determine
    if [ `rpm -q rpm 2> /dev/null | /bin/egrep '^rpm-[0-9].*' | wc -l` = 1 ]; then
        OSAlias="UniversalR"
    elif [ `dpkg -l dpkg 2> /dev/null | egrep "^ii.*"| wc -l` = 1 ]; then
        OSAlias="UniversalD"
    else
        OSAlias="Universal?"
    fi
}

GetLinuxInfo() {
    IsLinux="true"

    # Return kernel version as OS version - will be updated for specific cases below
    Version=`uname -r | cut -d. -f1,2`

    # Determine release file

    # Try to find -release file
    if [ -z ${ReleaseFile} ]; then 
        ReleaseFile=`ls -F ${EtcPath}/*-release 2>/dev/null | grep -v lsb-release |grep -v release@| grep -v scx-release| sed -n '1p'`
    fi

    # Fall back to lsb-release (e.g. Ubunutu)
    if [ -z ${ReleaseFile} ]; then 
        if [ -e "${EtcPath}/lsb-release" ]; then ReleaseFile="${EtcPath}/lsb-release"; fi
    fi

    # Debian (no lsb-release, but debian_version exists)
    if [ -z ${ReleaseFile} ]; then 
        TestFile="${EtcPath}/debian_version"
        if [ -e $TestFile ]; then ReleaseFile=$TestFile; fi
    fi

    # Try RHEL/CentOS
    TestFile="${EtcPath}/redhat-release"
    if [ ! -h $TestFile -a -e $TestFile ]; then ReleaseFile=$TestFile; fi

    # Try OEL
    TestFile="${EtcPath}/oracle-release"
    if [ ! -h $TestFile -a -e $TestFile ]; then ReleaseFile=$TestFile; fi

    # Try NeoKylin
    TestFile="${EtcPath}/neokylin-release"
    if [ ! -h $TestFile -a -e $TestFile ]; then ReleaseFile=$TestFile; fi

    # Try SLES
    TestFile="${EtcPath}/SUSE-release"
    if [ ! -h $TestFile -a -e $TestFile ]; then ReleaseFile=$TestFile; fi


    # Get OS Name
    if [ ! -z $ReleaseFile ]; then
      OSName=`/bin/egrep -o 'Red Hat Enterprise Linux|SUSE Linux Enterprise Server|SUSE LINUX Enterprise Server' $ReleaseFile`
    fi
    ## Extract Version from file
    ## Could also use /etc/*release (not Ubuntu)
    if [ "${OSName}" = "Red Hat Enterprise Linux" ]; then
        Version=`grep 'Red Hat Enterprise' $ReleaseFile | sed s/.*release\ // | sed s/\ .*//`
        if [ "${Version}" != "" ]; then
            OSAlias="RHEL"
            OSManufacturer="Red Hat, Inc."
        fi
    elif [ "${OSName}" = "SUSE Linux Enterprise Server" ]; then
        # SLES 10 uses "Linux". Need to parse the minor version as SLES 10.0 is not supported, only 10.1 and up
        Version=`grep 'SUSE Linux Enterprise Server' $ReleaseFile | sed s/.*Server\ // | sed s/\ \(.*//`
        # Discovery Wizard wants "10.X" not "10 SPX"
        if [ `echo ${Version} | grep 'SP' | wc -l` -eq 1 ]; then
            Version=`echo ${Version} | awk '{print $1"."$2}'| sed s/SP//`
        else
            VersionPL=`grep PATCHLEVEL $ReleaseFile | sed s/.*PATCHLEVEL\ =\ //`
            if [ "${VersionPL}" !=  "" ]; then
                Version=`echo ${Version}.${VersionPL}`
            fi
        fi
        if [ "${Version}" != "" ]; then
            OSAlias="SLES"
            OSManufacturer="SUSE GmbH"
        fi
    elif [ "${OSName}" = "SUSE LINUX Enterprise Server" ]; then
        # SLES 9 uses "LINUX". No need to parse minor version as Agent supports 9.0 and up.
        Version=`grep 'SUSE LINUX Enterprise Server' $ReleaseFile | sed s/.*Server\ // | sed s/\ \(.*//`
        if [ "${Version}" != "" ]; then
            OSAlias="SLES"
            OSManufacturer="SUSE GmbH"
        fi
    else
        OSAlias="Universal"
        OSName="Linux"
        Version=`uname -r | cut -d. -f1,2`

        # Do we have the (newer) os-release standard file?
        # If so, that trumps everything else
        if [ -e "${EtcPath}/os-release" ]; then
            GetKitType

            # The os-release files contain TAG=VALUE pairs; just read it in
            . $ReleaseFile

            # Some fields are optional, for details see the WWW site:
            #   http://www.freedesktop.org/software/systemd/man/os-release.html
            [ ! -z "$NAME" ] && OSName="$NAME"
            [ ! -z "$VERSION_ID" ] && Version="$VERSION_ID"

            # Set the manufacturer if we know this ID
            # (Set OSAlias for unit test purposes; test injection won't inject that)
            [ -z "$ID" ] && ID="linux"
            case $ID in
                debian)
                    OSManufacturer="Softare in the Public Interest, Inc."
       	            OSAlias="UniversalD"
                    ;;
            esac

        elif [ ! -z $ReleaseFile ]; then
            # Set OSName to release file contents for evaluation.  If parsing logic is not known, Release File contents will be used as OSName.
            OSName=`sed '/^$/d' ${ReleaseFile} | head -1`

            # Try known cases for OSName/Version

            # ALT Linux
            if [ `echo $OSName | grep "ALT Linux" | wc -l` -gt 0 ]; then
                OSName="ALT Linux"
                OSAlias="UniversalR"
                OSManufacturer="ALT Linux Ltd"
                Version=`grep 'ALT Linux' $ReleaseFile | sed s/.*Linux\ // | sed s/\ \.*//`
            fi

            # Enterprise Linux Server
            if [ `echo $OSName | grep "Enterprise Linux Enterprise Linux Server" | wc -l` -gt 0 ]; then
                OSName="Enterprise Linux Server"
                OSAlias="UniversalR"
                OSManufacturer="Oracle Corporation"
                Version=`grep 'Enterprise Linux Enterprise Linux Server' $ReleaseFile | sed s/.*release\ // | sed s/\ \(.*//`
            fi

            # Oracle Enterprise Linux Server
            if [ `echo $OSName | grep "Oracle Linux Server" | wc -l` -gt 0 ]; then
                OSName="Oracle Linux Server"
                OSAlias="UniversalR"
                OSManufacturer="Oracle Corporation"
                Version=`grep 'Oracle Linux Server release' $ReleaseFile | sed s/.*release\ // | sed s/\ \(.*//`
            fi

            # NeoKylin Linux Advanced Server
            if [ `echo $OSName | grep "NeoKylin Linux Advanced Server" | wc -l` -gt 0 ]; then
                OSName="NeoKylin Linux Server"
                OSAlias="UniversalR"
                OSManufacturer="China Standard Software Co., Ltd."
                Version=`grep 'NeoKylin Linux Advanced Server release' $ReleaseFile | sed s/.*release\ // | sed s/\ \(.*//`
            fi

            # OpenSUSE
            if [ `echo $OSName | grep -i "openSUSE" | wc -l` -gt 0 ]; then
                Version=`echo $OSName | awk '{print $2}'`
                OSName="openSUSE"
                OSAlias="UniversalR"
                OSManufacturer="SUSE GmbH"
            fi

            # Debian
            if [ "$ReleaseFile" = "${EtcPath}/debian_version" ]; then
                OSName="Debian"
                OSAlias="UniversalD"
                OSManufacturer="Softare in the Public Interest, Inc."
                Version=`cat ${EtcPath}/debian_version`
            fi
                        
            # Ubuntu
            if [ `echo $OSName | grep "Ubuntu" | wc -l` -gt 0 ]; then
                OSName="Ubuntu"
                OSAlias="UniversalD"
                OSManufacturer="Canonical Group Limited "
                Version=`grep 'DISTRIB_RELEASE' $ReleaseFile | cut -d'=' -f2`
            fi

            # Fedora
            if [ `echo $OSName | grep "Fedora" | wc -l` -gt 0 ]; then
                OSName="Fedora"
                OSAlias="UniversalR"
                OSManufacturer="Red Hat, Inc."
                Version=`grep 'Fedora' $ReleaseFile | sed s/.*release\ // | sed s/\ .*//`
            fi

            # CentOS
            if [ `echo $OSName | grep "CentOS" | wc -l` -gt 0 ]; then
                OSName="CentOS"
                OSAlias="UniversalR"
                OSManufacturer="Central Logistics GmbH"
                Version=`grep 'CentOS' $ReleaseFile | sed s/.*release\ // | sed s/\ .*//`
            fi

            # If distro is not known, determine whether RPM or DPKG is installed
            if [ "${OSAlias}" = "Universal" ]; then
                # Identify package manager
                GetKitType
            fi

            # If Version is null, something went wrong in release file parsing, reset to kernel version
            if [ "$Version" = "" ]; then
                Version=`uname -r`
            fi

            # If OSName is null, something went wrong in release file parsing, reset to Linux
            if [ "$OSName" = "" ]; then
                OSName="Linux"
                OSManufacturer="Universal"
            fi

        else
            GetKitType

            Version=`uname -r`
            OSName="Linux"
            OSManufacturer="Universal"
        fi
    fi

    if [ -z `echo ${Version} | grep '\.'` ]; then
        Version="$Version.0"
    fi

    # Construct OSFullName string
    OSFullName="$OSName $Version ($Arch)"
}

## End Linux distro function
######################################################################

GetLinuxInfo

# If the touch file does not exist or RelFile does not exist, write the rel file
if [ ! -e $DisableFile ] || [ ! -e $RelFile ]; then
    # Update scx-release
    printf "OSName=$OSName\n" > $RelFile
    printf "OSVersion=$Version\n" >> $RelFile
    printf "OSFullName=$OSFullName\n" >>$RelFile
    printf "OSAlias=$OSAlias\n" >>$RelFile
    printf "OSManufacturer=$OSManufacturer\n" >>$RelFile

    # Verify that it's W:R so non-priv'ed users can read
    chmod 644 $RelFile
fi

