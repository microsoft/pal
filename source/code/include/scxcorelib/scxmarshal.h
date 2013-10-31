/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/ 
/**
   \file
 
   \brief     helper class for handling std::wstring marshaling
 
   \date      2011-04-18 11:14:14
 

*/
/*----------------------------------------------------------------------------*/

#ifndef SCXMARSHAL_H
#define SCXMARSHAL_H

#include <scxcorelib/scxregex.h>
#include <scxcorelib/scxexception.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;

namespace SCXCoreLib
{
    enum MarshalDataType
    {
        MTYPE_UNKNOWN = 0,
        MTYPE_INT = 10,
        MTYPE_WSTRING = 20,
        MTYPE_VECTOR_WSTRING = 30,
        MTYPE_REGEX_INDEX = 40,
        MTYPE_VECTOR_REGEX_INDEX = 50
    };

    /*----------------------------------------------------------------------------*/
    /**
       Exception for "MarshalFormatException"
    
       This exception is raised when the data type being read does not match
       the data type found in the stream
    */ 
    class SCXMarshalFormatException : public SCXException
    {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
           Ctor 
           \param[in] expectedType   Expected data type to be found
           \param[in] actualType     Actual data type that was found
           \param[in] l              Source code location object

        */
        SCXMarshalFormatException(MarshalDataType expected,
                                  MarshalDataType actual,
                                  const SCXCodeLocation& l) : SCXException(l),
                                                              m_expectedType(expected),
                                                              m_actualType(actual)
        { };

        wstring What() const;

    protected:
        //! Expected data type that we tried to read
        MarshalDataType m_expectedType;

        //! Actual data type that we read
        MarshalDataType m_actualType;
    };

    class Marshal
    {
    private:
        //! The ostream object that we marshal data to
        ostream& m_stream;

        void writeDataType(MarshalDataType val);
        void writeInteger(int val);

    public:
        Marshal(ostream& stream) : m_stream(stream) {};
        void Write(int val);
        void Write(const wstring& ws);
        void Write(const vector<wstring>& vws);
        void Write(const SCXRegexWithIndex& ri);
        void Write(const vector<SCXRegexWithIndex>& vri);
        void Flush();
    };

    class UnMarshal
    {
    private:
        //! The istream object that we unmarshal data from
        istream& m_strm;

        MarshalDataType readDataType();
        int readInteger();

    public:
        UnMarshal(istream& strm);

        void Read(int& val);
        void Read(wstring& ws);
        void Read(vector<wstring>& vws);
        void Read(SCXRegexWithIndex& ri);
        void Read(vector<SCXRegexWithIndex>& vri);
    };
}

#endif /* SCXMARSHAL_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
