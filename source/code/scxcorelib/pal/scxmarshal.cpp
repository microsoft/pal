/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief     Helper class for handling std::wstring marshaling

   \date      2011-04-18 11:14:14


*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <scxcorelib/scxmarshal.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxstream.h>

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <wchar.h>

using namespace std;

#define CHECK_WRITE_ERROR(x) \
    if (x.fail()) throw SCXLineStreamContentWriteException(SCXSRCLOCATION)

#define CHECK_READ_ERROR(x) \
    if (x.fail()) throw SCXLineStreamPartialReadException(SCXSRCLOCATION)


namespace SCXCoreLib {
    /******************************************************************************
     *  
     *  SCXMarshalFormatException Implementation 
     *  
     ******************************************************************************/
    
    /*----------------------------------------------------------------------------*/
    /* (overload)
       Format reason why there was a format exception
    */
    wstring SCXMarshalFormatException::What() const 
    {
        wostringstream msg;
        msg << L"Expected argument type '"
            << (int) m_expectedType
            << L"', received argument type '"
            << (int) m_actualType
            << L"'";

        // Example: Expected argument type '10', received argument type '20'
        return msg.str();
    }


    /*----------------------------------------------------------------------------*/
    /* Write an integer to the Marshal object 
    */
    void Marshal::Write(int val)
    {
        writeDataType(MTYPE_INT);

        // check for stream write error 
        CHECK_WRITE_ERROR(m_stream);

        writeInteger(val);

        // check for stream write error 
        CHECK_WRITE_ERROR(m_stream);
    }

    /*----------------------------------------------------------------------------*/
    /* Write a wstring to the Marshal object 
    */
    void Marshal::Write(const wstring& ws)
    {
        writeDataType(MTYPE_WSTRING);

        // check for stream write error 
        CHECK_WRITE_ERROR(m_stream);

        int strSize = (int) (ws.length() * sizeof(wchar_t));
        writeInteger(strSize);

        CHECK_WRITE_ERROR(m_stream);

        vector<char> buf(strSize, '\0');
        memcpy(&buf[0], ws.c_str(), strSize);
        m_stream.write(&buf[0], strSize);
    
        // check for stream write error 
        CHECK_WRITE_ERROR(m_stream);
     }

    /*----------------------------------------------------------------------------*/
    /* Write a vector of wstring to the Marshal object 
    */
    void Marshal::Write(const vector<wstring>& vws)
    {
        writeDataType(MTYPE_VECTOR_WSTRING);

        // check for stream write error 
        CHECK_WRITE_ERROR(m_stream);

        int vecSize = (int) vws.size();
        writeInteger(vecSize);

        CHECK_WRITE_ERROR(m_stream);

        for(int i = 0; i < vecSize; i++)
        {
            Write(vws[i]);
        }
    }

    /*----------------------------------------------------------------------------*/
    /* Write a SCXRegexWithIndex object to the Marshal object 
    */
    void Marshal::Write(const SCXRegexWithIndex& ri)
    {
        writeDataType(MTYPE_REGEX_INDEX);
        
        // check for stream write error 
        CHECK_WRITE_ERROR(m_stream);

        writeInteger((int) ri.index);

        CHECK_WRITE_ERROR(m_stream);

        Write(ri.regex->Get());
    }

    /*----------------------------------------------------------------------------*/
    /* Write a vector of SCXRegexWithIndex object to the Marshal object 
    */
    void Marshal::Write(const vector<SCXRegexWithIndex>& vri)
    {
        writeDataType(MTYPE_VECTOR_REGEX_INDEX);

        // check for stream write error 
        CHECK_WRITE_ERROR(m_stream);

        int vecSize = (int) vri.size();
        writeInteger(vecSize);

        CHECK_WRITE_ERROR(m_stream);

        for(int i  = 0; i < vecSize; i++)
        {
           Write(vri[i]);
        }
    }

    /*----------------------------------------------------------------------------*/
    /* Flush the stream buffer of the Marshal object 
    */
    void Marshal::Flush()
    {
        m_stream.flush();
    }

    /*----------------------------------------------------------------------------*/
    /* Note: the following private methods serve as helper functions 
    */
    void Marshal::writeDataType(MarshalDataType val)
    {
        writeInteger((int) val);
    }

    void Marshal::writeInteger(int val)
    {
        m_stream.write((char *) &val, sizeof(int));
    }


    /*----------------------------------------------------------------------------*/
    /* UnMarshal class takes an istream object, unmarshals the stream,
       and reads out the original (marshaled) data. 
    */
    UnMarshal::UnMarshal(istream& iostrm) : m_strm (iostrm)
    {
    }

    /*----------------------------------------------------------------------------*/
    /* read MarshalDataType from the UnMarshal object  
    */
    MarshalDataType UnMarshal::readDataType()
    {
        return (MarshalDataType) readInteger();
    }

    /*----------------------------------------------------------------------------*/
    /* read an integer from the UnMarshal object  
    */
    int UnMarshal::readInteger()
    {
        int val;
        m_strm.read((char *) &val, sizeof(int));
        return val;
    }

    /*----------------------------------------------------------------------------*/
    /* read an integer from the UnMarshal object  
    */
    void UnMarshal::Read(int& val)
    {
        MarshalDataType dt = readDataType();

        // check for stream read error 
        CHECK_READ_ERROR(m_strm);

        if (dt != MTYPE_INT)
        {
            throw SCXMarshalFormatException(MTYPE_INT, dt, SCXSRCLOCATION);
        }

        val = readInteger();
        
        CHECK_READ_ERROR(m_strm);
    }
        
    /*----------------------------------------------------------------------------*/
    /* read a wstring from the UnMarshal object  
    */
    void UnMarshal::Read(wstring& ws)
    {
        MarshalDataType dt = readDataType();

        // check for stream read error 
        CHECK_READ_ERROR(m_strm);
        
        if (dt != MTYPE_WSTRING)
        {
            throw SCXMarshalFormatException(MTYPE_WSTRING, dt, SCXSRCLOCATION);
        }

        int strSize = readInteger();

        CHECK_READ_ERROR(m_strm);

        // Read in the wstring as a list of bytes
        vector<char> buf(strSize, '\0');
        m_strm.read(&buf[0], strSize);

        CHECK_READ_ERROR(m_strm);

        // Create a real wstring of the appropriate size
        size_t nr = strSize / sizeof(wchar_t);
        vector<wchar_t> wbuf(nr + 1 /* Null byte */, L'\0');
        memcpy(&wbuf[0], (void*) &buf[0], strSize);

        // Return the final wstring
        ws = wstring(&wbuf[0]);
    }

    /*----------------------------------------------------------------------------*/
    /* read a vector of wstring objects from the UnMarshal object  
    */
    void UnMarshal::Read(vector<wstring>& vws)
    {
        MarshalDataType dt = readDataType();

        // check for stream read error 
        CHECK_READ_ERROR(m_strm);

        if (dt != MTYPE_VECTOR_WSTRING)
        {
            throw SCXMarshalFormatException(MTYPE_VECTOR_WSTRING, dt, SCXSRCLOCATION);
        }

        int vecSize = readInteger();

        CHECK_READ_ERROR(m_strm);

        vws.clear();

        for(int i = 0; i < vecSize; i++)
        {
            wstring ws;
            Read(ws);
            vws.push_back(ws);
        }
    }

    /*----------------------------------------------------------------------------*/
    /* read a SCXRegexWithIndex object from the UnMarshal object  
    */
    void UnMarshal::Read(SCXRegexWithIndex& ri)
    {
        MarshalDataType dt = readDataType();

        // check for stream read error 
        CHECK_READ_ERROR(m_strm);
        
        if (dt != MTYPE_REGEX_INDEX)
        {
            throw SCXMarshalFormatException(MTYPE_REGEX_INDEX, dt, SCXSRCLOCATION);
        }

        int index = readInteger();
        
        CHECK_READ_ERROR(m_strm);

        ri.index = (size_t) index;

        wstring ws;
        Read(ws);
        ri.regex = new SCXRegex(ws);
    }

    /*----------------------------------------------------------------------------*/
    /* read a vector of SCXRegexWithIndex objects from the UnMarshal object  
    */
    void UnMarshal::Read(vector<SCXRegexWithIndex>& vri)
    {
        MarshalDataType dt = readDataType();

        // check for stream read error 
        CHECK_READ_ERROR(m_strm);
        
        if (dt != MTYPE_VECTOR_REGEX_INDEX)
        {
            throw SCXMarshalFormatException(MTYPE_VECTOR_REGEX_INDEX, dt, SCXSRCLOCATION);
        }

        int vecSize = readInteger();

        CHECK_READ_ERROR(m_strm);

        vri.clear();

        for(int i = 0; i < vecSize; i++)
        {
           SCXRegexWithIndex ri;
           Read(ri);
           vri.push_back(ri);
        }
    }
}
