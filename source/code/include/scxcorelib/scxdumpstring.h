/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Utilities to implement DumpString methods 
    
    \date        2007-12-04 14:20:00

    Every class should have a DumpString method for debugging purposes. This file offers
    functionality to ease the implementation of those, as well as making the return values coherent. 
    
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXDUMPSTRING_H
#define SCXDUMPSTRING_H
 
#include <string>
#include <sstream>
#include <vector>
/*----------------------------------------------------------------------------*/
/**
    Constructs a return value for DumpString        
*/
class SCXDumpStringBuilder {
public:
    SCXDumpStringBuilder(std::string classname);

    SCXDumpStringBuilder &Text(const std::string &name, const std::wstring &value);

    /*----------------------------------------------------------------------------*/
    //! Appends a scalar value
    //! \param[in]  name       Name (role) of the value
    //! \param[in]  value      Value to be appended
    //! \returns    *this
    template <class T>
    inline SCXDumpStringBuilder &Scalar(const std::string &name, const T &value) {
        m_stream << L" " << name.c_str() << L"=" << value;
        return *this;
    }
    
    /*----------------------------------------------------------------------------*/
    //! Appends the DumpString value of an instance
    //! \param[in]  name       Name (role) of the instance
    //! \param[in]  instance   Instance whose DumpString is to be appended
    //! \returns    *this
    template <class T>
    inline SCXDumpStringBuilder &Instance(const std::string &name, const T &instance) {
        m_stream << L" " << name.c_str() << L"=[" << instance.DumpString() << L"]";
        return *this;
    }

    /*----------------------------------------------------------------------------*/
    //! Appends values
    //! \param[in]  name        Name (role) of the values
    //! \param[in]  values      Values to be appended
    //! \returns    *this
    template <class T>
    SCXDumpStringBuilder &Scalars(const std::string &name, const std::vector<T> &values) {
        m_stream << L" " << name.c_str() << L"={";
        typename std::vector<T>::const_iterator i = values.begin();
        if (i != values.end()) {
            m_stream << *i;
            ++i;
            while (i != values.end()) {
                m_stream << L" " << *i;            
                ++i;
            }
        }
        m_stream << L"}";
        return *this;
        
    }

    /*----------------------------------------------------------------------------*/
    //! Appends the DumpString value of instances
    //! \param[in]  name        Name (role) of the value
    //! \param[in]  instances   Instances whose DumpStrings are to be appended
    //! \returns    *this
    template <class T>
    SCXDumpStringBuilder &Instances(const std::string &name, const std::vector<T> &instances) {
        m_stream << L" " << name.c_str() << L"={";
        typename std::vector<T>::const_iterator i = instances.begin();
        if (i != instances.end()) {
            m_stream << L"[" << i->DumpString() << L"]";
            ++i;
            while (i != instances.end()) {
                m_stream << L" [" << i->DumpString() << L"]";            
                ++i;
            }
        }
        m_stream << L"}";
        return *this;
        
    }

    std::wstring Str() const;
    operator std::wstring () const;

private:
    //! String stream to hold string
    std::wostringstream m_stream;
};


#endif /* SCXDUMPSTRING_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
