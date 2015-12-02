#!/bin/sh

#set -x

# Example Input...
#-DPF_DISTRO_SUSE -DPF_MAJOR=10 -DPF_MINOR=0 -Dx64 -DINTERNAL_STRING_IS_NARROW -DNDEBUG

if [ $# -eq 0 ]; then
    echo "Usage: outputfilename <args to parse>"
    exit 1
fi

# Output file MUST be the FIRST argument.  No sense adding needless complexity...
outputfile=$1
shift;

for i in $@; do
    # Check for Name/Value Pair
    echo $i | grep "=" > /dev/null
    if [ $? = 0 ]; then
        defName=`echo $i | sed 's/=.*$//;s/^-D//'`
        defValue=`echo $i | sed 's/^.*=//'`
        echo "#if !defined($defName)" >> $outputfile
        echo "#define $defName $defValue" >> $outputfile
        echo "#endif" >> $outputfile
    else
        # Skip -I directives.
        echo $i | grep "^-I" > /dev/null
        if [ $? = 0 ]; then
            continue;
        fi
        defName=`echo $i | sed 's/^-D//'`
        echo "#if !defined($defName)" >> $outputfile
        echo "#define $defName" >> $outputfile
        echo "#endif" >> $outputfile
    fi

done
