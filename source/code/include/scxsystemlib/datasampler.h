/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief      Contains the definition of the DataSampler template class.
    

    \date       2007-07-10 13:57:48

*/
/*----------------------------------------------------------------------------*/
#ifndef DATASAMPLER_H
#define DATASAMPLER_H

#include <scxcorelib/scxthreadlock.h>
#include <deque>

namespace SCXSystemLib  
{
    // Note: This class can only be used with native data types (i.e. not a class or struct)
    template<class T>
    class FixedSizeVector
    {
    public:
        typedef T* iterator;
        typedef const T* const_iterator;

        FixedSizeVector(size_t maxsize) : _maxSize(maxsize), _size(0)
        {
            _data = new T[_maxSize];
        }

        FixedSizeVector(const FixedSizeVector& x) : _maxSize(x._maxSize), _size(0)
        {
            _data = new T[_maxSize];
            for (const_iterator p = x.begin(); p != x.end(); p++)
                push_back(*p);
        }

        ~FixedSizeVector()
        {
            clear();
            delete [] _data;
        }

        FixedSizeVector& operator=(const FixedSizeVector& x)
        {
            if (this != &x)
            {
                clear();

                for (const_iterator p = x.begin(); p != x.end(); p++)
                    push_back(*p);
            }

            return *this;
        }

        size_t size() const 
        {
            return _size; 
        }

        T& operator[](size_t i) 
        {
            return  _data[i];
        }

        T operator[](size_t i) const
        {
            return _data[i];
        }

        T* begin() 
        { 
            return _data;
        }

        const T* begin() const 
        {
            return _data;
        }

        iterator end() 
        { 
            return begin() + _size;
        }

        const_iterator end() const 
        { 
            return begin() + _size;
        }

        void clear()
        {
            _size = 0;
        }

        void push_front(const T& x)
        {
            if (_size != _maxSize)
            {
                memmove(begin() + 1, begin(), sizeof(T) * _size);
                _data[0] = x;
                _size++;
            }
        }

        void push_back(const T& x)
        {
            if (_size != _maxSize)
            {
                _data[_size] = x;
                _size++;
            }
        }

        void pop_back()
        {
            _size--;
        }

    private:
        // Data elements.
        T* _data;                       //!< Data elements

        size_t _maxSize;                //!< Maximum size
        size_t _size;                   //!< Current size (not to exceed _maxSize)

    };

    /*----------------------------------------------------------------------------*/
    /**
        Template Class that represents a series of measurements of a particular
        value over time. 
        
        \param T Sample type. This type has to be of a numerical type, or
                 more speciffically, it has to support the operato - and
                 must be devidable by an integer.
        \param maxSamples Maximum number of samples represented.
        
        Could for example be used to collect statistics about how
        a counter value changes over time.

    */
    template<class T> class DataSampler
    {
    public:
        typedef FixedSizeVector<T> Samples;

        /*----------------------------------------------------------------------------*/
        /**
            Constructor.
            
        */
        DataSampler(size_t numElements) : m_lock(SCXCoreLib::ThreadLockHandleGet()),
                                          m_samples(numElements),
                                          m_numElements(numElements)
        {
        }

        /*----------------------------------------------------------------------------*/
        /**
            Add a new sample.
        
            \param  sample New sample.
            
        */
        void AddSample(T sample)
        {
            SCXCoreLib::SCXThreadLock lock(m_lock);

            if ( m_samples.size() == m_numElements)
                m_samples.pop_back();

            m_samples.push_front(sample);
        }

        /*----------------------------------------------------------------------------*/
        /**
            Check if the latest value added is smaller then an earlier value
        
            \param  samples Number of samples to go back.
            
            \returns  true if smaller, else false
            
        */
        bool HasWrapped(size_t samples)
        {
            if (m_samples.size() < 2)
            {
                return false;
            }
            SCXCoreLib::SCXThreadLock lock(m_lock);
            size_t index = samples - 1;
            if (samples > m_samples.size())
            {
                index = m_samples.size() - 1;
            }
            return m_samples[0] < m_samples[index];
        }

        /*----------------------------------------------------------------------------*/
        /**
           Get the average value of all samples.

           \returns  average value of all samples.

        */
        template <class V> V GetAverage() const 
        {
            V sum = 0;
            if (m_samples.size() == 0)
            {
                return sum;
            }

            SCXCoreLib::SCXThreadLock lock(m_lock);
            for (typename Samples::const_iterator it = m_samples.begin(); it != m_samples.end(); ++it) 
            {
                sum += static_cast<V>(*it);
            } 
            return sum / static_cast<V>(m_samples.size());
        }

        /*----------------------------------------------------------------------------*/
        /**
            Get the average change in value in the latest samples.
        
            \param  samples Number of samples to go back.
            
            \returns  Average change in value in the last samples.
            
            If the number of collected samples is less than the samples parameter, a
            best effort average is returned using all the samples collected.
            
        */
        T GetAverageDelta(size_t samples) const
        {
            return GetAverageDeltaFactored(samples, 1);
        }

        /*----------------------------------------------------------------------------*/
        /**
            Get the average change in value in the latest samples factored by a given factor.
    
            \param       samples number of samples to go back.
            \param       factor factor to increase the average
            \returns     average change in value in the last samples factored by the factor.
    
            \date        07-08-22 15:10:00
    
            Used to get better values for integer averages which should be factored.
    
        */
        T GetAverageDeltaFactored(size_t samples, T factor) const
        {
            if (samples < 2 || m_samples.size() < 2 || 0 == factor)
            {
                // Too few samples to produce a valid output (or zero factor).
                return T();
            }
            SCXCoreLib::SCXThreadLock lock(m_lock);
            size_t index = samples - 1;
            if (samples > m_samples.size())
            {
                index = m_samples.size() - 1;
            }
            return ((m_samples[0] - m_samples[index])*factor) / static_cast<T>(index);
        }

        /*----------------------------------------------------------------------------*/
        /**
            Get the change in value in the latest samples.
    
            \param       samples Number of samples to go back.
            \returns     change in value in the last samples factored by the factor.
    
            \date        07-08-22 15:10:00
    
        */
        T GetDelta(size_t samples) const
        {
            if (samples < 2 || m_samples.size() < 2)
            {
          
                // Too few samples to produce a valid output.
                return T();
            }
            SCXCoreLib::SCXThreadLock lock(m_lock);
            size_t index = samples - 1;
            if (samples > m_samples.size())
            {
                index = m_samples.size() - 1;
            }

            return m_samples[0] - m_samples[index];
        }

        /*----------------------------------------------------------------------------*/
        /**
            Get a specific sample value.
    
            \param       index sample index to retrieve.
            \returns     The sample value ate given index.
            \throws      SCXIllegalIndexExceptionUInt if index is larger than sample history.
    
            \date        07-08-22 15:10:00
    
        */
        T operator [] (size_t index) const
        {
            SCXCoreLib::SCXThreadLock lock(m_lock);
            if (index >= m_samples.size())
            {
                throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"index", index, SCXSRCLOCATION);
            }
            return m_samples[index];
        }

        /*----------------------------------------------------------------------------*/
        /**
            Erase all samples.
    
            \date        07-08-22 15:10:00
    
        */
        void Clear()
        {
            SCXCoreLib::SCXThreadLock lock(m_lock);
            m_samples.clear();
        }

        /*----------------------------------------------------------------------------*/
        /**
            Retrieve the number of samples.
        
            \returns      Number of samples saved.
            
        */
        size_t GetNumberOfSamples() const
        {
            SCXCoreLib::SCXThreadLock lock(m_lock);
            return m_samples.size();
        }

    private:
        SCXCoreLib::SCXThreadLockHandle m_lock;  //!< Makes the datasampler thread safe.
        Samples m_samples;                 //!< Contains the samples.
        size_t m_numElements;              //!< Maximum number of elements
    };
}

#endif /* DATASAMPLER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
