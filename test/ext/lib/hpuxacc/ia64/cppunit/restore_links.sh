# This script restores the symlinks the same way they were set up by "make install".
# And no, simply renaming libcppunit-1.12.so.0.0 to libcppunit.so does not work.
ln -s libcppunit-1.12.so.0.0 libcppunit-1.12.so.0
ln -s libcppunit-1.12.so.0.0 libcppunit.so
