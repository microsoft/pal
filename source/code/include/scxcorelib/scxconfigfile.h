/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Contains a configuration dictionnary that can be persisted on disk

   \date        2014-09-29 11:55:00

*/
/*----------------------------------------------------------------------------*/

#ifndef SCXCONFIGFILE_H
#define SCXCONFIGFILE_H

#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>
#include <string>
#include <map>

namespace SCXCoreLib
{
    using std::wstring;

    /** Exception for an Invalid configuration file loaded */
    class SCXInvalidConfigurationFile : public SCXException
    {
    public:
        //! Ctor
        SCXInvalidConfigurationFile(std::wstring reason, 
                                const SCXCodeLocation& l) : SCXException(l),
        m_Reason(reason)
        {};

        std::wstring What() const { return L"Errors were detected in the configuration file : " + m_Reason;};
    protected:
        //! Description of internal error
        std::wstring   m_Reason;
    };

    /*----------------------------------------------------------------------------*/
    /**
       Defines interface for the configuration key value pairs.
        
       A configuration file is formatted as a single key and value per line separated by a colon.
    */
    class SCXConfigFile
    {
    public:
        //! Ctor
        SCXConfigFile(const SCXFilePath& configpath);

        void LoadConfig();
        void SaveConfig() const;
        bool GetValue(const wstring& key, wstring& value) const;
        void SetValue(const wstring& key, const wstring& value);
        void DeleteEntry(const wstring& key);
        bool KeyExists(const wstring& key) const;

        // Iterators
        std::map<wstring, wstring>::const_iterator begin() const
        {
            return m_config.begin();
        }

        std::map<wstring, wstring>::const_iterator end() const
        {
            return m_config.end();
        }

    protected:
        bool m_configLoaded;
        SCXFilePath m_configFilePath; 
        std::map<wstring, wstring> m_config;

    private:
        void ThrowExceptionIfNotLoaded() const;
    };
}
#endif /* SCXCONFIGFILE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
