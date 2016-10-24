/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file
 
    \brief     Definition of the standard SCX exceptions
 
    \date      07-05-24 14:32:12
 
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXEXCEPTION_H
#define SCXEXCEPTION_H

#include <string>
#include <sstream>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxassert.h>
#include <scxcorelib/strerror.h>

#include <string.h>

namespace SCXCoreLib
{
    
    /*----------------------------------------------------------------------------*/
    /**
       Source code location abstraction
       
       Provides a unit for file location information. Normally an instance of this
       class is always created using the SRCLOCATION macro. 

    */
    class SCXCodeLocation 
    {
    public:
        //! Default constructor
        SCXCodeLocation() {}; 
        /*----------------------------------------------------------------------------*/
        /**
           The ctor normally used
           
           \param[in]   file  Source code file as indicated by the preprocessor
           \param[in]   line  Source code line in file
           
        */
        SCXCodeLocation(std::wstring file, unsigned int line) : m_File(file), m_Line(line) {};

        //! Returns true if information is available
        bool         GotInfo() const { return m_File.length() ? true : false; };
        //! Returns a formatted string with code location
        std::wstring Where() const;

        //! Returns the line number where the exception occured if known, else returns "unknown"
        std::wstring WhichLine() const;

        //! Returns which file the exception occured if known, else returns "unknown"
        std::wstring WhichFile() const;

    private:
        //! String carrying information about source file line, from __FILE__ macro
        std::wstring m_File;
        //! The line number in file m_File
        unsigned int m_Line;
    };

    //! Internal macro, CPP trick to convert char[] to wchar_t[]
    //! There might be a general variant of this in scxcmn.h - this def should only be used in this context
    #define SCXEXCEPTIONTOWSTRING2(x) L ## x
    //! Internal macro, not for use outside this file
    #define SCXEXCEPTIONTOWSTRING(x) SCXEXCEPTIONTOWSTRING2(x)
    //! std::wstring variant of the __FILE__ standard macro
    #define __SCXEXCEPTIONWFILE__ SCXEXCEPTIONTOWSTRING(__FILE__)

    //! Anonymous object instance, used in logging calls in most user logs
    #define SCXSRCLOCATION SCXCoreLib::SCXCodeLocation(__SCXEXCEPTIONWFILE__, __LINE__)

    /*----------------------------------------------------------------------------*/
    /**
       Abastract base class for all exceptions in Aurora project
    
       All exceptions shall inherit from this class. The What() method must be overridden 
       by sublcasses. 

    */ 
    class SCXException {
    public:
        //! Dtor
        virtual ~SCXException() {}
        //! Return a formatted string explaining what happened, for human readers
        virtual std::wstring What() const = 0;
        //! Return a formatted string expressing where exception occured and 
        //! optionally details of call stack passed on the way. 
        virtual std::wstring Where() const;

        //! Predicate informing if there is any location information available
        bool    GotLocationInfo() const { return m_OriginatingLocation.GotInfo(); }

        //! Add any context you find relevant when re-throwing 
        //! Normally used only via SCXRETHROW() macro
        void    AddStackContext(const std::wstring& context, const SCXCodeLocation& location);
        /** 
            \overload 
        */
        void    AddStackContext(const std::wstring& context);
        /** 
            \overload 
        */
        void    AddStackContext(const SCXCodeLocation& location);

    protected:
        //! Default ctor, used only by subclasses 
        SCXException() {};
        //! Ctor used by most subclasses, carries source code location information 
        SCXException(const SCXCodeLocation& location) : m_OriginatingLocation(location) {};

    private:
        //! Indicates where the first throw was made
        SCXCodeLocation   m_OriginatingLocation;

        //! "Dynamic" information accumulating as an exception propagates up the call stack
        std::wstring      m_StackContext;
    };

    //! Macro to accomplish a re-throw with automatic context add
    #define SCXRETHROW(e, context) {                          \
        e.AddStackContext(context, SCXSRCLOCATION);           \
        throw;                                                \
    }
    
    /*----------------------------------------------------------------------------*/
    /**
       Generic exception for Invalid Argument
    
       Defines an exception for violation of precondition.

       @note This exception assert()s internally and should not be used when the 
       invalid argument is from some external source. It should only be used 
       when a contract is broken due to programming error. 

    */ 
    class SCXInvalidArgumentException : public SCXException {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
           Ctor 
          \param[in] formalArgument Name of the formal argument receiving invalid actual argument
          \param[in] reason         Details on in which way the argument is invalid
          \param[in] l              Source code location object
        */
        SCXInvalidArgumentException(std::wstring formalArgument, 
                                    std::wstring reason,
                                    const SCXCodeLocation& l) : SCXException(l), 
                                                                m_FormalArg(formalArgument),
                                                                m_Reason(reason) {       
            SCXASSERTFAIL((l.Where() + What()).c_str());
        };

        std::wstring What() const;

    protected:
        //! Indicate which of the arguments (source code name) was invalid
        std::wstring   m_FormalArg;
        //! Details on in which way the argument was invalid
        std::wstring   m_Reason;
    };        


    /*----------------------------------------------------------------------------*/
    /**
       Generic exception for NULL Pointer
    
       Defines an exception for when a NULL pointer is encountered in a place 
       where it may not be.

    */ 
    class SCXNULLPointerException : public SCXException {
    public: 
        
        /*----------------------------------------------------------------------------*/
        /**
           Ctor
           \param[in] pointerName  Name of the pointer containing NULL value
           \param[in] l            Source code location object
        */
        SCXNULLPointerException(std::wstring pointerName, 
                                const SCXCodeLocation& l) : SCXException(l), 
                                                            m_PointerName(pointerName) 
        { SCXASSERTFAIL((l.Where() + What()).c_str()); };
        
        std::wstring What() const;
        
    protected:
        //! The source code name of the violating pointer
        std::wstring   m_PointerName;
    };        


    /*----------------------------------------------------------------------------*/
    /**
       Generic exception for index out of bounds for a certain type
    
       Provides a central definition of a general exception to use for logical 
       error when an index is out of bounds.

       The class is implemented as a template with parametrized index type. Typedefs
       exsist for shorthand typing.
                   
    */ 
    template <class T, T emptyValue = 0>
    class SCXIllegalIndexException : public SCXException {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Ctor for exceptions without information of limits
           \param[in] indexName            Formal argument name the violating index
           \param[in] valueOfIllegalIndex  Violating value
           \param[in] l                    Source code location object
        */
        SCXIllegalIndexException(std::wstring           indexName, 
                                 T                      valueOfIllegalIndex,
                                 const SCXCodeLocation& l) : SCXException(l), 
                                                             m_IndexName(indexName),
                                                             m_IllegalIndex(valueOfIllegalIndex),
                                                             m_MinBoundary(emptyValue),
                                                             m_IsMinBoundarySupplied(false),
                                                             m_MaxBoundary(emptyValue),
                                                             m_IsMaxBoundarySupplied(false)
        { 
            SCXASSERTFAIL((l.Where() + What()).c_str());
        }

        /*----------------------------------------------------------------------------*/
        /**
          Ctor for exceptions with min and max boundary information 
          
          \param[in] indexName            Formal argument name the violating index
          \param[in] valueOfIllegalIndex  Violating value
          \param[in] minBoundary          Lower boundary, regarded only if isMinBoundarySupplied is true
          \param[in] isMinBoundarySupplied Indicates whether a minBoundary value is supplied
          \param[in] maxBoundary          Upper boundary, regarded only if isMaxBoundarySupplied is true
          \param[in] isMaxBoundarySupplied Indicates whether a maxBoundary value is supplied
          \param[in] l                    Source code location object

        */
        SCXIllegalIndexException(std::wstring           indexName, 
                                 T                      valueOfIllegalIndex,
                                 T                      minBoundary,
                                 bool                   isMinBoundarySupplied,
                                 T                      maxBoundary,
                                 bool                   isMaxBoundarySupplied,
                                 const SCXCodeLocation& l) : SCXException(l), 
                                                             m_IndexName(indexName),
                                                             m_IllegalIndex(valueOfIllegalIndex),
                                                             m_MinBoundary(minBoundary),
                                                             m_IsMinBoundarySupplied(isMinBoundarySupplied),
                                                             m_MaxBoundary(maxBoundary),
                                                             m_IsMaxBoundarySupplied(isMaxBoundarySupplied)
        {           
            SCXASSERTFAIL((l.Where() + What()).c_str());
        }

        //! Return information about this specific instance, for later printing 
        std::wstring What() const
        { 
            std::wstringstream s; 

            if (!m_IsMinBoundarySupplied && !m_IsMaxBoundarySupplied)
            {
                // Example
                // Index configArray has illegal value 34
                s <<  L"Index '" << m_IndexName << L"' has illegal value " << m_IllegalIndex;
                return s.str();
            }
            
            if (!m_IsMinBoundarySupplied && m_IsMaxBoundarySupplied)
            {
                // Example
                // Index configArray has illegal value 34 - upper boundary is 30
                s << L"Index '" << m_IndexName << L"' has illegal value " << m_IllegalIndex << 
                    L" - upper boundary is " << m_MaxBoundary;
                return s.str();
            }
            if (m_IsMinBoundarySupplied && !m_IsMaxBoundarySupplied)
            {
                // Example
                // Index configArray has illegal value 34 - lower boundary is 40
                s << L"Index '" << m_IndexName << L"' has illegal value " << m_IllegalIndex << 
                    L" - lower boundary is " << m_MinBoundary;
                return s.str();
            }

            
            // Both m_IsMinBoundarySupplied && m_IsMaxBoundarySupplied supplied
            // Example
            // Index configArray has illegal value 34 - boundaries are 20 and 30
            s << L"Index '" << m_IndexName << L"' has illegal value " << m_IllegalIndex << 
                L" - boundaries are " << m_MinBoundary << L" and " << m_MaxBoundary;
            return s.str();
        }
        

    protected: 
        //! Name of violating index (source code)
        std::wstring   m_IndexName;

        //! Violating index value
        T              m_IllegalIndex;

        // Used for numerical indices
        T              m_MinBoundary;           //!< Lower boundary value
        bool           m_IsMinBoundarySupplied; //!< Predicate whether there is min boundary information
        T              m_MaxBoundary;           //!< Upper boundary value
        bool           m_IsMaxBoundarySupplied; //!< Predicate whether there is max boundary information
    };        


    //! Convenience shorthand for illegal exception with unsigned int type
    typedef SCXIllegalIndexException<unsigned int> SCXIllegalIndexExceptionUInt;

    //! Convenience shorthand for illegal exception with int type
    typedef SCXIllegalIndexException<int> SCXIllegalIndexExceptionInt;


    /*----------------------------------------------------------------------------*/
    /**
       Generic exception for "Functionality not implemented" or Not Supported
    
       Provides a central definition of a general exception to use 
       when a Not Yet Implemented (NYI) situation is encountered.

    */ 
    class SCXNotSupportedException : public SCXException {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
           Ctor 
           \param[in] functionality  Description of the missing functionality
           \param[in] l              Source code location object

        */
        SCXNotSupportedException(std::wstring functionality, 
                                 const SCXCodeLocation& l) : SCXException(l),
                                                             m_Functionality(functionality)
        { };
                                                         
        std::wstring What() const;

    protected:
        //! Which functionality that's not in place (yet, or will never be) 
        std::wstring   m_Functionality;
    };        


    /*----------------------------------------------------------------------------*/
    /**
       Generic exception for Internal Error
    
       Provide a central definition of a general exception to use 
       when a non-recoverable internal error situation is encountered.
       
       @note This exception assert()s internally and should only be used for programming
       error situations! 

    */ 
    class SCXInternalErrorException : public SCXException {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
           Ctor
           \param[in] reason Description of the internal error
           \param[in] l      Source code location object

        */
        SCXInternalErrorException(std::wstring reason, 
                                 const SCXCodeLocation& l) : SCXException(l),
                                                             m_Reason(reason)
        { SCXASSERTFAIL((l.Where() + What()).c_str()); };

        std::wstring What() const;

    protected:
        //! Description of internal error
        std::wstring   m_Reason;
    };        


    /*----------------------------------------------------------------------------*/
    /**
       Generic exception for "Resource Exhausted" 
    
       Provide a central definition of a general exception to use when the requested 
       resource (memory or whatever) cannot be allocated.

    */ 
    class SCXResourceExhaustedException : public SCXException {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
          Ctor
          \param[in] resourceType     Descriptive name of resource type
          \param[in] resourceDetails  Details of allocation problem
          \param[in] l                Source code location object
        */
        SCXResourceExhaustedException(std::wstring resourceType, 
                                      std::wstring resourceDetails,
                                      const SCXCodeLocation& l) : SCXException(l), 
                                                                  m_ResourceType(resourceType),
                                                                  m_ResourceDetails(resourceDetails)
        { };
                                                         
        std::wstring What() const;

    protected:
        //! Type of resource (memory, disk, thread...)
        std::wstring   m_ResourceType;
        //! Used for numerical indices
        std::wstring   m_ResourceDetails;
    };        

    /*----------------------------------------------------------------------------*/
    /**
       Exeception for 'errno' conditions.

       Provides an exception for internal errors where the error code is delivered
       in form of an 'errno' variable. 

       The user is advised to check the 'errno' and first see if it can be
       matched against a different exception, before resorting to use this
       exception. For example, if 'errno' is ENOENT a SCXFileNotFound
       exception with the proper filename would be more appropriate.  On the
       other hand, some calls may return 20 different errors and checking
       for each and every one would only clutter up your code.

       Note that the descriptive text that perror() would print is not
       returned by What(). The reason is that that text is returned in 8-bit
       platform specific encoding while we internally use UNICODE UCS-4 for
       all text. An automatic conversion would require a call to conversion
       routines that could generate error by themseleves. Instead call
       ErrorText() and do the conversion yourself.
    */ 
    class SCXErrnoException : public SCXException {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
           Ctor
           \param[in] fkncall Description of the internal error
           \param[in] errno_  System error code with local interpretation
           \param[in] l       Source code location object

        */
        SCXErrnoException(std::wstring fkncall, int errno_, const SCXCodeLocation& l) 
            : SCXException(l), m_fkncall(fkncall), m_errno(errno_)
        { 
            char buf[80];
#ifdef WIN32
            strerror_s(buf, sizeof(buf), errno_);
            m_errtext = buf;
#elif (defined(hpux) && (PF_MINOR==23)) || (defined(sun) && (PF_MAJOR==5) && (PF_MINOR<=9))
            // WI7938
            m_errtext = strerror(errno_);
#elif (defined(hpux) && (PF_MINOR>23)) || (defined(sun) && (PF_MAJOR==5) && (PF_MINOR>9)) || (defined(aix)) || (defined(macos))
            int r = strerror_r(errno_, buf, sizeof(buf));
            if (0 != r) {
                snprintf(buf, sizeof(buf), "Unknown error %d", errno_);
            }
            m_errtext = buf;
#else
            // Do not assign to m_errtext directly to get compiler error when strerror_r is declared to return an int.
            char* r = strerror_r(errno_, buf, sizeof(buf));
            m_errtext = r;
#endif
        };

        std::wstring What() const {
            std::wostringstream txt;
            txt << L"Calling " << m_fkncall << L"() returned an error with errno = " << m_errno << L" (" << m_errtext.c_str() << L")";
            return txt.str();
        }

        /** Returns text describing the error.
            \returns error text in 8-bit platform specific encoding
        */
        std::string ErrorText() const { return m_errtext; }

        /** Returns errno number.
            \returns errno
        */
        int ErrorNumber() const { return m_errno; }

    protected:
        //! Text describing what system call that generated this error
        std::wstring m_fkncall;
        //! The 'errno' number that were reported
        int          m_errno;
        //! System-generated text that describes the meaning of the correspondning 'errno'
        std::string  m_errtext;
    };        

    /*----------------------------------------------------------------------------*/
    /**
       Specific errno exception for file errors (see SCXErrnoException).

       This makes it possible to see the exact filename that's causing a problem.
    */
    class SCXErrnoFileException : public SCXErrnoException {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
           Ctor
           \param[in] fkncall Function call for file-related operation
           \param[in] path    Path or filename parameter causing internal error
           \param[in] errno_  System error code with local interpretation
           \param[in] l       Source code location object

        */
        SCXErrnoFileException(std::wstring fkncall, std::wstring path, int errno_, const SCXCodeLocation& l) 
            : SCXErrnoException(fkncall, errno_, l), m_fkncall(fkncall), m_path(path)
        { };

        std::wstring What() const {
            std::wostringstream txt;
            txt << L"Calling " << m_fkncall << "() with file \"" << m_path
                << "\" returned an error with errno = " << m_errno << L" (" << m_errtext.c_str() << L")";
            return txt.str();
        }

        /** Returns function call for the file operation falure
            \returns file-operation in std::wstring encoding
        */
        std::wstring GetFnkcall() const { return m_fkncall; }

        /** Returns path for file open failure
            \returns path text in std::wstring encoding
        */
        std::wstring GetPath() const { return m_path; }
    protected:
        //! Text of file-related function call
        std::wstring m_fkncall;
        //! Text describing the significant parameter to the system call
        std::wstring m_path;
    };        

    /*----------------------------------------------------------------------------*/
    /**
       Specific errno exception for open errors (see SCXErrnoException).

       This makes it possible to see the exact filename that's causing a problem.
    */
    class SCXErrnoOpenException : public SCXErrnoFileException {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
           Ctor
           \param[in] path    Path or filename parameter causing internal error
           \param[in] errno_  System error code with local interpretation
           \param[in] l       Source code location object

        */
        SCXErrnoOpenException(std::wstring path, int errno_, const SCXCodeLocation& l)
            : SCXErrnoFileException(L"open", path, errno_, l)
        { };
    };

    /*----------------------------------------------------------------------------*/
    /**
       Specific errno exception for ERANGE errors (see SCXErrnoException).

       This gives remedial information on how to perhaps solve a system
       configuration problem
    */
    class SCXErrnoERANGE_Exception : public SCXErrnoException {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
           Ctor
           \param[in] fkncall Function call for file-related operation
           \param[in] text    Remedial text that might be helpful to the user
           \param[in] errno_  System error code with local interpretation
           \param[in] l       Source code location object

        */
        SCXErrnoERANGE_Exception(std::wstring fkncall, std::wstring text, int errno_, const SCXCodeLocation& l) 
            : SCXErrnoException(fkncall, errno_, l), m_fkncall(fkncall), m_recoveryText(text)
        { };

        std::wstring What() const {
            std::wostringstream txt;
            txt << L"Calling " << m_fkncall << L"()"
                << L" returned an error with errno = " << m_errno << L" (" << m_errtext.c_str() << L"). "
                << m_recoveryText;
            return txt.str();
        }

        /** Returns function call for the file operation falure
            \returns file-operation in std::wstring encoding
        */
        std::wstring GetFnkcall() const { return m_fkncall; }

        /** Returns path for file open failure
            \returns path text in std::wstring encoding
        */
        std::wstring GetRecoveryText() const { return m_recoveryText; }
    protected:
        //! Text of file-related function call
        std::wstring m_fkncall;
        //! Text describing the significant parameter to the system call
        std::wstring m_recoveryText;
    };        

    /*----------------------------------------------------------------------------*/
    /**
       Exeception for access violations.

       Provides a way to report that the program attempted a procedure for which
       it has insufficient privileges.
    */
    class SCXAccessViolationException : public SCXException {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
           Ctor
           \param[in] reason Description of the internal error
           \param[in] l      Source code location object

        */
        SCXAccessViolationException(std::wstring reason, const SCXCodeLocation& l) : 
            SCXException(l), m_Reason(reason)
        { };

        std::wstring What() const {
            return L"Access violation exception was thrown because: " + m_Reason;
        }

    protected:
        //! Description of internal error
        std::wstring   m_Reason;
    };        

    /*----------------------------------------------------------------------------*/
    /**
       Generic exception for Invalid State
    
       Defines an exception for violation of precondition. Use this for example
       when a method is called on an object that has not been initialized.

       @note This exception assert()s internally and should only be used 
       when a contract is broken due to programming error. 

    */ 
    class SCXInvalidStateException : public SCXException {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
           Ctor 
           \param[in] reason         Details on in which way the state is invalid
           \param[in] l              Source code location object
        */
        SCXInvalidStateException(std::wstring reason,
                                 const SCXCodeLocation& l) : SCXException(l), 
                                                             m_Reason(reason) {       
            SCXASSERTFAIL((l.Where() + What()).c_str());
        };

        std::wstring What() const;

    protected:
        //! Details on in which way the state was invalid
        std::wstring   m_Reason;
    };        

}

#endif /* SCXEXCEPTION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
