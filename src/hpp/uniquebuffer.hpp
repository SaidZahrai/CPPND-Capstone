/**************************************************************************************
 * Unique Buffer is a kind of message queue between two Executers, i.e. between two threads.
 * it offers both means for exchange of data without risk for collisions and a machanism for
 * synchronization between two threads.
 * 
 * It contains a buffer, where the data can be store. The delivery of data is simply by 
 * exchange of addresses and no copying will take place. 
 * 
 * The buffer can be occupied with yet not used data, the data can be old or invalid, or it 
 * can be available for storing new data.
 * 
 * In case of invalid data, the receive request should wait, and in case of being occpied 
 * with unused data, the send request has to wait.
 * 
 * **************************************************************************************/
#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include <atomic>

#include <iostream>

using namespace std;

namespace parallelOperators
{
    template <class T>
    class UniqueBuffer
    {
    public:
        UniqueBuffer(string bname): _buffer(make_unique<T>()), _bname (bname) {};

        // Send and receive implement the process explained above, with waiting for available buffer
        // at send and waiting for new data at receive.
        void receive(unique_ptr<T> & data_ptr)
        {
            unique_lock<mutex> uLock(_mutex);
#ifdef DEBUG_PRINTOUT
            cout << " **) Waiting for refreshed data from - " << _bname << "   \n";
#endif
            _condition.wait(uLock, [this] { return (_dataRefreshed || _ending); });
#ifdef DEBUG_PRINTOUT
            cout << " **) New data has arrived and now, the data can now be swaped at - " << _bname << "   \n";
#endif
            if (_dataRefreshed) _buffer.swap(data_ptr);
            _bufferAvailable = true;
            _dataRefreshed = false;
            _condition.notify_one();
        }; 
        void send(unique_ptr<T> & data_ptr)
        {
            unique_lock<mutex> uLock(_mutex);
#ifdef DEBUG_PRINTOUT
            cout << " **) Waiting for the buffer to become available - " << _bname << "   \n";
#endif
            _condition.wait(uLock, [this] { return (_bufferAvailable || _ending); });
#ifdef DEBUG_PRINTOUT
            cout << " **) Buffer is available and data can now be swaped at - " << _bname << "   \n";
#endif
            if (_bufferAvailable) _buffer.swap(data_ptr);
            _bufferAvailable = false;
            _dataRefreshed = true;
            _condition.notify_one();
        }

        // When an ending reques has come, the locks need to be release.
        void releaseAll()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Request to end and release mutex - " << _bname << "   \n";
#endif
            _ending = true;
            _mutex.unlock();
            _condition.notify_all();
        }

    private:
        string _bname;                              // A name to allow following the process
        mutex _mutex;                               // Data protection
        condition_variable _condition;              // Condition variable for waiting
        unique_ptr<T> _buffer;                      // Data storage
        atomic_bool _bufferAvailable = true;        // States that the buffer can be in
        atomic_bool _dataRefreshed = false;  
        atomic_bool _ending = false;
    };
}