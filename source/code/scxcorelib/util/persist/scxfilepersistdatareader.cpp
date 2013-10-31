/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implementation of the SCXFilePersistDataReader class.
    
    \date        2008-08-21 14:55:48

*/
/*----------------------------------------------------------------------------*/

#define _SOLARIS_11_NO_MACROS_
#include <scxcorelib/scxcmn.h>

#include <scxcorelib/scxstream.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxregex.h>
#include "scxfilepersistdatareader.h"
#include <string>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Constructor.

        \param[in]  path        Path where persisted data should be read.
    */
    SCXFilePersistDataReader::SCXFilePersistDataReader(const SCXFilePath& path) :
        m_StartedGroups()
    {
        m_Stream = SCXFile::OpenFstream(path, std::ios_base::in);

        std::wstreampos pos = m_Stream->tellg();

        try
        {
            Consume(L"<?xml");
            Consume(L"version");
            Consume(L"=");
            ConsumeString(L"1.0");
            Consume(L"encoding");
            Consume(L"=");
            Consume(L"'UTF-8'");
            Consume(L"standalone");
            Consume(L"=");
            Consume(L"'yes'");
            Consume(L"?>");

            Consume(L"<");
            Consume(L"SCXPersistedData");
            Consume(L"Version");
            Consume(L"=");
            std::wstring versionString = ConsumeString();
            Consume(L">");
            m_Version = StrToUInt(versionString);
        }
        catch (PersistUnexpectedDataException& e)
        {
            m_Stream->seekg(pos);
            throw e;
        }
        catch (SCXCoreLib::SCXNotSupportedException&)
        {
            std::wstreampos errorpos = m_Stream->tellg();
            m_Stream->seekg(pos);
            throw PersistUnexpectedDataException(L"The Version attribute should have an unsigned integer value",
                                                 errorpos, SCXSRCLOCATION);
        }
    }
    
    /*----------------------------------------------------------------------------*/
    /**
        Destructor
    */
    SCXFilePersistDataReader::~SCXFilePersistDataReader()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get persistence data version

        \returns   persistence data version
        Retrive version stored by data writer.
    */
    unsigned int SCXFilePersistDataReader::GetVersion()
    {
        return m_Version;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Check if current item is a "start group" tag with the given name and if so
        consumes that item.

        \param[in] name     Name of start group tag to check for.
        \param[in] dothrow  Should this function throw an exception if current
                            item is not a start group tag with the given name.
        
        \returns   true if current item is a start group tag with the given name,
                   else false.
        \throws    PersistUnexpectedDataException if next input is not the start of
                   expected group.
    */
    bool SCXFilePersistDataReader::ConsumeStartGroup(const std::wstring& name, bool dothrow /*= false*/)
    {
        std::wstreampos pos = m_Stream->tellg();
        try
        {
            Consume(L"<");
            Consume(L"Group");
            Consume(L"Name");
            Consume(L"=");
            ConsumeString(name);
            Consume(L">");
        }
        catch (PersistUnexpectedDataException& e)
        {
            m_Stream->seekg(pos);
            if (dothrow)
            {
                throw e;
            }
            return false;
        }
        
        m_StartedGroups.push_front(name);

        return true;
        
    }

    /*----------------------------------------------------------------------------*/
    /**
        Check if current item is an "end group" tag with the given name and if so
        consumes that item.

        \param[in] dothrow  Should this function throw an exception if current
                            item is not an end group tag with expected name.

        \returns   true if current item is an end group tag with the given name,
                   else false.
        \throws SCXInvalidStateException if no group is opened.
        \throws PersistUnexpectedDataException if next tag is not a close tag
                of the currently open group and dothrow is true.
    */
    bool SCXFilePersistDataReader::ConsumeEndGroup(bool dothrow /*= false*/)
    {
        if (m_StartedGroups.empty())
        {
            throw SCXInvalidStateException(L"No open group when calling ConsumeEndGroup.", SCXSRCLOCATION);
        }

        std::wstreampos pos = m_Stream->tellg();
        try
        {
            Consume(L"</");
            Consume(L"Group");
            Consume(L">");
        }
        catch (PersistUnexpectedDataException& e)
        {
            m_Stream->seekg(pos);
            if (dothrow)
            {
                throw e;
            }
            return false;
        }
        
        m_StartedGroups.pop_front();

        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Check if current item is a "value" tag with the given name and if so
        consumes that item and retrive the value.

        \param[in]  name     Name of value tag to check for.
        \param[out] value    Return value.
        \param[in]  dothrow  Should this function throw an exception if current
                             item is not a value tag with the given name.

        \returns   true if current item is a value tag with the given name, else
                   false.
    */
    bool SCXFilePersistDataReader::ConsumeValue(const std::wstring& name, std::wstring& value, bool dothrow /*= false*/)
    {
        std::wstreampos pos = m_Stream->tellg();
        try
        {
            Consume(L"<");
            Consume(L"Value");
            Consume(L"Name");
            Consume(L"=");
            ConsumeString(name);
            Consume(L"Value");
            Consume(L"=");
            value = ConsumeString();
            Consume(L"/>");
        }
        catch (PersistUnexpectedDataException& e)
        {
            m_Stream->seekg(pos);
            if (dothrow)
            {
                throw e;
            }
            return false;
        }
        
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Check if current item is a "value" tag with the given name and if so
        consumes that item and returns the value. Throws an exception if current 
        item is not a value tag with the given name.

        \param[in]  name     Name of value tag to check for.
        \returns    value of the value tag.
        \throws InternalErrorException if current item is not a value tag with the
                given name.
    */
    std::wstring SCXFilePersistDataReader::ConsumeValue(const std::wstring& name)
    {
        std::wstring retval = L"";
        ConsumeValue(name, retval, true); 
        return retval;
    }

    
    /*----------------------------------------------------------------------------*/
    /**
        Consumes an XML-encoded character.
        
        XML-encoded characters are the characters starting with & and ending with ;
        Example &lt; &quot; &#24;.
        
        This method is called after the initial & has been encountered to consume
        everything after the '&' character and return the encoded character.

        \returns Consumed character.

        \throws PersistUnexpectedDataException if data is not the next data in the
                stream.
    */
    wchar_t SCXFilePersistDataReader::ConsumeEncodedChar()
    {
        std::wstring entity(L"");
        for (wchar_t ch = GetUTF8Char(); ch != ';'; ch = GetUTF8Char())
        {
            entity.push_back(ch);
        }

        wchar_t c = 0;
        if (L"lt" == entity)
        {
            c = L'<';
        }
        else if (L"amp" == entity)
        {
            c = L'&';
        }
        else if (L"apos" == entity)
        {
            c = L'\'';
        }
        else if (L"quot" == entity)
        {
            c = L'\"';
        }
        else if (entity.size() > 0 && entity[0] == L'#')
        {
            c = static_cast<wchar_t>(StrToUInt(std::wstring(entity, 1)));
        }
        else
        {
            throw PersistUnexpectedDataException(L"XML encoded character.",
                                                 m_Stream->tellg(), SCXSRCLOCATION);
        }
        return c;
    }
    
    /*----------------------------------------------------------------------------*/
    /**
        Consumes any whitespace in the stream until data is encountered. It then
        consumes data and returns.

        \param[in] data Data to consume.

        \throws PersistUnexpectedDataException if data is not the next data in the
                stream.
    */
    void SCXFilePersistDataReader::Consume(const std::wstring& data)
    {
        std::wstring::const_iterator iter = data.begin();
        if (iter == data.end()) return;

        try {
            wchar_t ch = GetUTF8CharSkipLeadingWS();

            for (;;)
            {
                if (ch != *iter)
                {
                    throw PersistUnexpectedDataException(data, m_Stream->tellg(), SCXSRCLOCATION);
                }
                
                ++iter;
                if (iter == data.end()) break;

                ch = GetUTF8Char();
            }

            return;

        } catch (SCXLineStreamContentException& e) {
            // If an invalid UTF-8 sequnce is encountered, including bad
            // file state or EOF.
            throw PersistUnexpectedDataException(data, m_Stream->tellg(), SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Consumes any whitespace in the stream until data is encountered. It then
        consumes data enclosed in double quotes returns.

        \param[in] data Data to consume.

        \throws PersistUnexpectedDataException if data is not the next data in the
                stream.

        In the stream, data between the double quote characters is expected to be
        xml-encoded so that all special characters are encoded with \&...;
        e.g. double quote character is encoded as \&quote;
    */
    void SCXFilePersistDataReader::ConsumeString(const std::wstring& data)
    {
        try {

            wchar_t ch = GetUTF8CharSkipLeadingWS();

            // Expect a '"' to start the string
            if (ch != L'\"')
            {
                throw PersistUnexpectedDataException(L"\"",
                                                     m_Stream->tellg(), SCXSRCLOCATION);
            }


            // Read stream char-by-char and match it to the argument-string
            for (std::wstring::const_iterator iter = data.begin();
                 iter != data.end();
                 ++iter)
            {
                ch = GetUTF8Char();
                if (L'&' == ch)
                {
                    ch = ConsumeEncodedChar();
                }
                if (ch != *iter)
                {
                    throw PersistUnexpectedDataException(data, m_Stream->tellg(), SCXSRCLOCATION);
                }
            }

            // Expect a '"' to end the string
            if (GetUTF8Char() != L'\"')
            {
                throw PersistUnexpectedDataException(L"\"",
                                                     m_Stream->tellg(), SCXSRCLOCATION);
            }

        } catch (SCXLineStreamContentException& e) {
            // If an invalid UTF-8 sequnce is encountered, including bad
            // file state or EOF.
            throw PersistUnexpectedDataException(L"\".*\"",
                                                 m_Stream->tellg(), SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Consumes any whitespace in the stream until a double quote character is
        encountered. It then consumes all characters until another double quote
        character is encountered and returns those characters as a string.

        \returns String of characters

        \throws PersistUnexpectedDataException if a double quote character is not 
                the next data in the stream or there is no matching double quote
                character to end the string.

        In the stream, data between the double quote characters is expected to
        be xml-encoded so that all special characters are encoded with \&...;
        e.g. double quote character is encoded as \&quote;
    */
    std::wstring SCXFilePersistDataReader::ConsumeString()
    {
        try {
            std::wstring data;
            wchar_t ch = GetUTF8CharSkipLeadingWS();

            // Expect a '"' to start the string
            if (ch != L'\"')
            {
                throw PersistUnexpectedDataException(L"\"",
                                                     m_Stream->tellg(), SCXSRCLOCATION);
            }
            
            for (ch = GetUTF8Char(); ch != L'\"'; ch = GetUTF8Char())
            {
                if (ch == L'&')
                {
                    ch = ConsumeEncodedChar();
                }
                data.push_back(ch);
            }
            return data;

        } catch (SCXLineStreamContentException& e) {
            // If an invalid UTF-8 sequnce is encountered, including bad
            // file state or EOF.
            throw PersistUnexpectedDataException(L"\".*\"",
                                                 m_Stream->tellg(), SCXSRCLOCATION);
        }

        // Not reached
        return std::wstring();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Consumes any whitespace at the beginning of the stream and returns the first
        non-whitespace character.

        \returns Next non white space character in stream as UTF8.

       \throws PersistUnexpectedDataException if EOF or a bad file state is encountered
    */
    wchar_t SCXFilePersistDataReader::GetUTF8CharSkipLeadingWS()
    {
        wchar_t ch(0);

        do {
            ch = GetUTF8Char();

        } while (std::isspace(ch, m_Locale)); // Check WI9509 before changing the locale parameter!

        return ch;
    }

    /**
       Gets next UTF-8 character from the stream.
       \returns Next character in stream as UTF8.
       \throws PersistUnexpectedDataException if EOF or a bad file state is encountered

       This metod should only be called to get an expected character. 
       This is mostly an insulation layer to avoid that SCXInvalidArgumentException
       is thrown which would make negative test cases impossible since it asserts 
       and aborts if thrown in a debug-built binary.
    */
    wchar_t SCXFilePersistDataReader::GetUTF8Char()
    {
        if (m_Stream->peek() == EOF || !m_Stream->good())
        {
            throw PersistUnexpectedDataException(L"UTF8 character", m_Stream->tellg(), SCXSRCLOCATION);
        }
        
        return SCXStream::ReadCharAsUTF8(*m_Stream);
    }

}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
