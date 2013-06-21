#ifndef SCXASSERTEXCEPTION_H
#define SCXASSERTEXCEPTION_H

#include <cppunit/extensions/HelperMacros.h>
#include <exception>
#include <sstream>

/*----------------------------------------------------------------------------*/
/**
    Exception thrown for Unit tests for SCXASSERT Fails
*/
class SCXAssertException : public std::exception
{
public:
    /*----------------------------------------------------------------------------*/
    /**
        Default constructor
    */
    SCXAssertException() {}

    virtual const char* what() const throw()
    {
        return "SCXASSERT thrown!";
    }
};


#define CHECK_EXCEPTION(EXPRESSION, EXCEPTION) { \
                                                    try { \
                                                        EXPRESSION; \
                                                        std::stringstream message; message << "Exception failed at "; \
                                                        message << __FILE__ << ":" << __LINE__ ; \
                                                        CPPUNIT_ASSERT_MESSAGE(message.str(), false); \
                                                    } catch (EXCEPTION&) { \
                                                        CPPUNIT_ASSERT(true); \
                                                    } \
                                                    catch (SCXAssertException&) { \
                                                        CPPUNIT_ASSERT(true); \
                                                    } \
                                               } \




#endif /* SCXASSERTEXCEPTION_H */

