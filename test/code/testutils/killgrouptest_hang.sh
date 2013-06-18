#!/bin/bash

# This script, along with killgrouptest.sh should both be killed in the TestKillGroup unit test.

# Write out the PID of this process
echo $$ > ./testfiles/killgrouptest_hang.pid

# Hang around for awhile so that we get killed.
sleep 30
