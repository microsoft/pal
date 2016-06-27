/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Contains the public interface of the Log framework.

    \date        07-06-05 11:32:00

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXLOG_H
#define SCXLOG_H

#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxsingleton.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>
#include <string>


namespace SCXCoreLib
{
#if !defined(DISABLE_WIN_UNSUPPORTED)  
    extern "C" {
        typedef void (*SCXLogRotateHandlerPtr)(int); //!< Pointer to signal handler
    }
#endif //!defined(DISABLE_WIN_UNSUPPORTED)  

    /*----------------------------------------------------------------------------*/
    /**
        Defines severity levels used for logging in the SCX project.

    */
    enum SCXLogSeverity { eNotSet,
                          eHysterical,
                          eTrace,
                          eInfo,
                          eWarning,
                          eError,
                          eSuppress,
                          eSeverityMax };

    class SCXLogItem;

    /*----------------------------------------------------------------------------*/
    /**
        Defines interface for a log item consumer.

        This interface is implemented by the log mediator and the log backends.
    */
    class SCXLogItemConsumerIf
    {
    public:

        /*----------------------------------------------------------------------------*/
        /**
            Log a message.

            \param[in] item Log item to consume.
        */
        virtual void LogThisItem(const SCXLogItem& item) = 0;

        /*----------------------------------------------------------------------------*/
        /**
            Get the effective severity for a particular log module.

            This method enables us to make logging more efficient by giving us the
            possibility to do upstream filtering.

            \param[in] module Log module to retrieve severity for.
            \returns Effective severity for log module.

        */
        virtual SCXLogSeverity GetEffectiveSeverity(const std::wstring& module) const = 0;


        /*----------------------------------------------------------------------------*/
        /**
           Handle log rotations that have occurred
         */
        virtual void HandleLogRotate() { }

        /**
            Destructor.

        */
        virtual ~SCXLogItemConsumerIf() {};
    };

    /*----------------------------------------------------------------------------*/
    /**
        Defines interface for log configurator.

        This interface is implemented by the SCXLogFileConfigurator class.
    */
    class SCXLogConfiguratorIf
    {
    public:
        /**
            Set the effective severity per module
            \param[in] module Log module to set severity for.
            \param[in] newThreshold The new severity threshold to set.

        */
        virtual void SetSeverityThreshold(const std::wstring& module, SCXLogSeverity newThreshold) = 0;

        /**
            Unset the effective severity per module
            \param[in] module Log module to clear severity for.

        */
        virtual void ClearSeverityThreshold(const std::wstring& module) = 0;

        /**
            Get current config version
            \returns Current config version.
        */
        virtual unsigned int GetConfigVersion() const = 0;

        /**
            Restore configuration by rereading the configuration file.
        */
        virtual void RestoreConfiguration() = 0;

        /**
            Get the minimum log severity threshold active anywhere in the
            framework.
            \returns Severity threshold as string.
        */
        virtual std::wstring GetMinActiveSeverityThreshold() const = 0;

        /**
            Virtual destructor.
        */
        virtual ~SCXLogConfiguratorIf() {};
    };

    /*----------------------------------------------------------------------------*/
    /**
        Provides a handle to the logging facility.

        \date        07-06-05 11:32:00

        Used for logging in SCX project

        The SCXLogHandle class corresponds roughly to the module concept.
        You can retrieve a handle instance by calling the GetLogHandle method
        of the SCXLogHandleFactory and supplying a module name.
        Once you have an instance of the handle, you are free to use it and
        destroy it at will. If you need an instance of the LogHandle for the root
        module, use the empty string.

        The Logger instance holds an effective severity threshold which is the
        minimum of the severity thresholds for this module in each of the back
        ends in the system. This threshold has to be refreshed regularly since
        the configuration may change at run time. This is implemented using a
        configuration version number.

        If a specific log item is above the severity
        threshold, extra information like timestamp and thread id are appended
        and the log item is then sent to the log mediator.

    */
    class SCXLogHandle
    {
    public:
        void Log(SCXLogSeverity sev, const std::wstring& message, const SCXCodeLocation& location) const;

        void Log(SCXLogSeverity sev, const std::string& message, const SCXCodeLocation& location) const
        {
            Log(sev, SCXCoreLib::StrFromUTF8(message), location);
        }

        SCXLogSeverity GetSeverityThreshold() const;
        void SetSeverityThreshold(SCXLogSeverity newSeverity);
        void ClearSeverityThreshold();

        SCXLogHandle();
        SCXLogHandle(const std::wstring& module, SCXHandle<SCXLogItemConsumerIf> mediator, SCXHandle<SCXLogConfiguratorIf> configurator);
        SCXLogHandle(const SCXLogHandle& o);
        SCXLogHandle& operator=(const SCXLogHandle& o);
        virtual ~SCXLogHandle();

        const std::wstring DumpString() const;

    private:
        std::wstring m_module; //!< Module string for this handle.
        mutable unsigned char m_severityThreshold; //!< Effective severity threshold. Char to make class reentrant. Conceptually of type SCXLogSeverity.
        mutable unsigned int m_configVersion;      //!< Used to keep severity threshold in sync.
        SCXHandle<SCXLogItemConsumerIf> m_mediator; //!< Mediator to send log items to.
        SCXHandle<SCXLogConfiguratorIf> m_configurator; //!< Used to know when to check for new configuration.
    };

    /*----------------------------------------------------------------------------*/
    /**
        Factory class to create log handle instances.

        \date        07-06-05 14:32:00

        Used to retrieve log handles

        This is the entry point into the logging framework for the programmer.
        This class is a singleton instance and is responsible for creating
        SCXLogHandle objects.

    */
    class SCXLogHandleFactory : public SCXSingleton<SCXLogHandleFactory>
    {
        friend class SCXSingleton<SCXLogHandleFactory>;
    public:

        static SCXLogHandle GetLogHandle(const std::wstring& module);
        static const SCXHandle<const SCXLogConfiguratorIf> GetLogConfigurator();

        const std::wstring DumpString() const;

        virtual ~SCXLogHandleFactory();

    private:
        SCXLogHandleFactory();

        /** Private copy constructor */
        SCXLogHandleFactory(const SCXLogHandleFactory&) :
            SCXSingleton<SCXLogHandleFactory>(),
            m_LogMediator(0),
            m_LogConfigurator(0)
        {
            SCXASSERT(!"Do not copy SCXLogHandleFactory");
        }
        /** Private assignment operator */
        SCXLogHandleFactory& operator=(const SCXLogHandleFactory&)
        {
            SCXASSERT(!"Do not assign SCXLogHandleFactory");
            return *this;
        }
#if !defined(DISABLE_WIN_UNSUPPORTED)  
        static const int LOGROTATE_REACTION_SIGNAL;

        static void InstallLogRotateSupport();
        static void HandleLogRotate(int sig);

        static SCXLogRotateHandlerPtr s_nextSignalHandler;
#endif //!defined(DISABLE_WIN_UNSUPPORTED)  
        SCXHandle<SCXLogItemConsumerIf> m_LogMediator; //!< Handle to log mediator.
        SCXHandle<SCXLogConfiguratorIf> m_LogConfigurator; //!< Handle to log configurator.
    };

    SCXSingleton_Define(SCXLogHandleFactory);
}

#include <scxcorelib/scxsingleton-defs.h>

/** Log a message with severity. */
#define SCX_LOG(loghandle, severity, message) {                \
    SCXCoreLib::SCXLogSeverity sev = (severity);               \
    if (sev >= (loghandle).GetSeverityThreshold())             \
    {                                                          \
        (loghandle).Log(sev, (message), SCXSRCLOCATION);       \
    }                                                          \
}

/** Log an error */
#define SCX_LOGERROR(loghandle, message)      SCX_LOG((loghandle), SCXCoreLib::eError,      (message))
/** Log a warning */
#define SCX_LOGWARNING(loghandle, message)    SCX_LOG((loghandle), SCXCoreLib::eWarning,    (message))
/** Log an informative message */
#define SCX_LOGINFO(loghandle, message)       SCX_LOG((loghandle), SCXCoreLib::eInfo,       (message))
/** Log a trace message */
#define SCX_LOGTRACE(loghandle, message)      SCX_LOG((loghandle), SCXCoreLib::eTrace,      (message))
/** Log a hysterical message */
#define SCX_LOGHYSTERICAL(loghandle, message) SCX_LOG((loghandle), SCXCoreLib::eHysterical, (message))
/** Log a sensitive message of sensitive nature. This will be disabled in release builds */
#if defined(ENABLE_INTERNAL_LOGS)
#define SCX_LOGINTERNAL(loghandle, severity, message)   SCX_LOG((loghandle), (severity), (message))
#else
#define SCX_LOGINTERNAL(loghandle, severity, message)
#endif
#endif /* SCXLOG_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
