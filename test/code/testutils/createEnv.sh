#!/bin/sh

# We need to set up some environment for the testrunner; do so here

ENV_FILE=`dirname $0`/env.sh

echo "#!/bin/sh" > $ENV_FILE
echo >> $ENV_FILE

# Use the en_US locale for tests sensitive to date/time formatting
[ -z "$LANG" ] && LANG=`locale -a | grep -i en_us | grep -i utf`
echo "LANG=\"$LANG\"; export LANG" >> $ENV_FILE

if [ "`uname`" = "SunOS" ]; then
    echo "LD_LIBRARY_PATH=/usr/local/ssl/lib; export LD_LIBRARY_PATH" >> $ENV_FILE
elif [ "`uname`" = "AIX" ]; then
    echo "ODMDIR=/etc/objrepos; export ODMDIR" >> $ENV_FILE
fi

# Export a variable solely so that we can see we're running under test
# This is currently used by the logfile provider
#
# Also, add logfilereader executable location to path (use '' to evaluate in child)
# This avoids failing unit tests by having variable paths on invocation line
echo "SCX_TESTRUN_ACTIVE=1; export SCX_TESTRUN_ACTIVE" >> $ENV_FILE
echo 'PATH=$1:$PATH:/sbin:/usr/sbin; export PATH' >> $ENV_FILE

# Testrunner arguments are sent using environment variables...
[ -n "$SCX_TESTRUN_NAMES" ] && echo "SCX_TESTRUN_NAMES=\"$SCX_TESTRUN_NAMES\"; export SCX_TESTRUN_NAMES" >> $ENV_FILE
[ -n "$SCX_TESTRUN_ATTRS" ] && echo "SCX_TESTRUN_ATTRS=\"$SCX_TESTRUN_ATTRS\"; export SCX_TESTRUN_ATTRS" >> $ENV_FILE

# Code coverage (BullsEye) environment
[ -n "$COVFILE" ] && echo "COVFILE=\"$COVFILE\"; export COVFILE" >> $ENV_FILE

exit 0;
