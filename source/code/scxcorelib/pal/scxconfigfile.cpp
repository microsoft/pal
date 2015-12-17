/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Contains a configuration dictionnary that can be persisted on disk

   \date        2014-09-29 11:55:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxconfigfile.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfile.h>
#include <errno.h>
#include <fstream>
#include <vector>

namespace SCXCoreLib
{
    using std::string;
    using std::wstring;

    /*----------------------------------------------------------------------------*/
    /**
       Constructor

       \param[in] configpath : Path object to the configuration file.
    */
    SCXConfigFile::SCXConfigFile(const SCXFilePath& configpath) : m_configLoaded(false), m_configFilePath(configpath)
    {}

    /*----------------------------------------------------------------------------*/
    /**
       Loads the configuration file into memory.
       This must be called after the constructor and before any other operation.
       The internal error exception will be thrown after parsing all the configuration file.

       \throws     SCXUnauthorizedFileSystemAccessException
       \throws     SCXLineStreamPartialReadException       Line to long to be stored in a std::wstring
       \throws     SCXLineStreamContentException           Invalid byte sequence according to UTF8
       \throws     SCXInternalErrorException               If there was an error in the config file
       \throws     SCXFilePathNotFoundException
    */
    void SCXConfigFile::LoadConfig()
    {
        m_configLoaded = true;

        if (!SCXFile::Exists(m_configFilePath))
        {
            throw SCXFilePathNotFoundException(m_configFilePath, SCXSRCLOCATION);
        }

        std::vector<wstring> lines;
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLinesAsUTF8(m_configFilePath, lines, nlfs);
        std::wostringstream error_reasons;

        bool internalError = false;

        for (std::vector<wstring>::const_iterator line=lines.begin(); line!=lines.end(); ++line)
        {
            string::size_type pos = line->find(L"=");
            if (pos != string::npos)
            {
                wstring key = StrTrim( line->substr(0, pos) );
                wstring value = StrTrim( line->substr(pos + 1, line->size()) );

                if (key.empty())
                {
                    error_reasons << L"Empty key: \"" << *line << "\"" << std::endl;
                    internalError = true;
                }
                else if (KeyExists(key))
                {
                    error_reasons << L"Duplicate key: \"" << key << "\"" << std::endl;
                    internalError = true;
                }
                else
                {
                    m_config[key] = value;
                }
            }
        }

        if (internalError)
        {
            throw SCXInvalidConfigurationFile( m_configFilePath.Get() + L"\n" + error_reasons.str(), SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       \Persist the current state of the configuration on disk overwritting the loaded one
       \throws    SCXInvalidStateException if the config file is not loaded
    */
    void SCXConfigFile::SaveConfig() const
    {
        ThrowExceptionIfNotLoaded();
        std::vector<wstring> lines(m_config.size(), L"");
        int i=0;

        for (std::map<wstring, wstring>::const_iterator it=m_config.begin(); it!=m_config.end(); ++it)
        {
            lines[i++] = it->first +  L"=" + it->second;
        }

        // By default, SCX code won't write a newline at the very end; we want that in a .INI-style file
        if ( m_config.size() )
        {
            lines[m_config.size() - 1] += L"\n";
        }

        SCXFile::WriteAllLinesAsUTF8(m_configFilePath, lines, std::ios_base::out);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get the value for a specified key.
       \throws    SCXInvalidStateException if the config file is not loaded
       \param[in] Key.
       \param[out] Reference to set
       \return Whether the output param was set
    */
    bool SCXConfigFile::GetValue(const wstring& key, wstring& value) const
    {
        ThrowExceptionIfNotLoaded();
        std::map<wstring, wstring>::const_iterator it = m_config.find(key);
        if (it == m_config.end())
        {
            return false;
        }
        else
        {
            value = it->second;
            return true;
        }
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Set the value for a specified key.
       Creates the key if non existent.
       \throws    SCXInvalidStateException if the config file is not loaded
       \param[in] Key
       \param[in] Value
    */
    void SCXConfigFile::SetValue(const wstring& key, const wstring& value)
    {
        ThrowExceptionIfNotLoaded();
        m_config[ StrTrim(key) ] = StrTrim(value);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Deletes the key/value configuration entry
       \throws    SCXInvalidStateException if the config file is not loaded
       \throws    SCXInvalidArgumentException if the key to delete is not found
       \param[in] Key
    */
    void SCXConfigFile::DeleteEntry(const wstring& key)
    {
        ThrowExceptionIfNotLoaded();
        std::map<wstring, wstring>::iterator it = m_config.find(key);
        if (it != m_config.end())
        {
            m_config.erase(it);
        }
        else
        {
            throw SCXInvalidArgumentException(L"key", L"Key not found : " + key, SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       \return Wheter the key exists in the configuration
       \throws    SCXInvalidStateException if the config file is not loaded
       \param[in] Key
    */
    bool SCXConfigFile::KeyExists(const wstring& key) const
    {
        ThrowExceptionIfNotLoaded();
        std::map<wstring, wstring>::const_iterator it = m_config.find(key);

        return (it != m_config.end());
    }

    void SCXConfigFile::ThrowExceptionIfNotLoaded() const
    {
        if (!m_configLoaded)
        {
            throw SCXInvalidStateException(L"The configuration file must be loaded beforehand", SCXSRCLOCATION);
        }
    }

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
