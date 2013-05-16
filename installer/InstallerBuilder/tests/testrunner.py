import unittest
import scriptgenerator_test
import macospackage_test
import sunospkg_test
import linuxrpm_test
import linuxdeb_test
import aixlpp_test
import hpuxpackage_test
import opsmgr_test
import opsmgrbuildinfo_test
import sys

##
# Entry point to the test suite.
#
if __name__ == '__main__':
    suite = unittest.TestSuite()
    suite.addTest(unittest.TestLoader().loadTestsFromModule(scriptgenerator_test))
    suite.addTest(unittest.TestLoader().loadTestsFromModule(macospackage_test))
    suite.addTest(unittest.TestLoader().loadTestsFromModule(sunospkg_test))
    suite.addTest(unittest.TestLoader().loadTestsFromModule(linuxrpm_test))
    suite.addTest(unittest.TestLoader().loadTestsFromModule(linuxdeb_test))
    suite.addTest(unittest.TestLoader().loadTestsFromModule(aixlpp_test))
    suite.addTest(unittest.TestLoader().loadTestsFromModule(hpuxpackage_test))
    suite.addTest(unittest.TestLoader().loadTestsFromModule(opsmgr_test))
    suite.addTest(unittest.TestLoader().loadTestsFromModule(opsmgrbuildinfo_test))
    
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():
        sys.exit(1)
