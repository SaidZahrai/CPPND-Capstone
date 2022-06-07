/*************************************************************************************
 * Operator executers are wrappers that manage a chain of operators and execute their
 * operation in a thread. Within the thread, the execution is sequential and managed
 * by the executer, which calls operation() method of each operator.
 * 
 * Similar to the operators, wrappers also have 3 types, with everyone to correspond to 
 * an operator, i.e. source type, sink type and operator type.
 * 
 * We can hypothetically have many wrappers and many threads. Each thread will run 
 * a numeber of operators, but there will not by any automatic synchronization between
 * two wrappers. Synchronization is managed by message passig, but in a special way to
 * reduce the need for memory allocation and inefficiencies related to allocation and
 * deallocation of memory.
 * 
 * Input and output of Executers are shared pointers of a structure called UniqueBuffer.
 * Shared pointer allows two wrappers connect to the same data structure, and the
 * unique buffer offers the possibility to separate the two threads in an efficient
 * manner. The data transfer mechanism is as follows:
 * 
 * Internally, two unique pointers are set up and managed by the Executer, on on the input
 * side and one on the output side. The memory resources in these two buffers is used
 * by the operators inside the wrapper.
 * 
 * Another structure, a unique buffer, wrapped in a shared pointer is created and managed 
 * by one of two Executesrs. It is wrapped n a shared pointer to allow both of the 
 * wrapper access the data. Once created, it is there to allow exchange of data through
 * data-swapping between the internal unique pointer and the one inside the unique buffer.
 * 
 * Data will be sent to the unique buffer, and will wait until the buffer is available.
 * After availability, the data with be swapped, without any copying or reallocation.
 * New data will block the buffer until it is consumed by the other thread, which opens
 * the buffer for a new exchange. 
 * 
 * The unique buffer is both offers means for exchange of data as well as a means for
 * synchronization between two threads in a sequence.
 * 
*************************************************************************************/

#include <thread>
#include <vector>
#include <operator.hpp>
#include <uniquebuffer.hpp>

#include <deque>
#include <mutex>
#include <condition_variable>
#include <future>

#include <atomic>

#include <iostream>

using namespace std;

namespace parallelOperators
{
    // Commands that can be sent to an Executer. In step mode, the thread will wait
    // for a new signal, while in the continuous mode, it will execute as soon as data
    // is available.
    enum ExecutionMode
    {
        Step = 0,
        Continuous
    };

    // Base class containing the common part of the three types of executer.
    class BaseExecuter
    {
    public:
        BaseExecuter(string tname): _tname (tname), _executionMode(ExecutionMode::Step), 
                                _newMessage(false), _futureExit(_exitPromise.get_future()),
                                _opStatus(OperationStatus::running) {};
        ~BaseExecuter()
        {
            joinThreads();
        };

        // A static vector keeps all threads that are created and allow joining them at a suitable time.
        static void joinThreads()
        {
            for (thread & t : _allThreads)
            {
                if (t.joinable()) t.join();
            }
        }

        // Commands to the Executer. It can switch between Step and Continuous.
        // Only one message at the time is stored.
        // A new message can both change the execution mode, and become a signal
        // for stepping forward.
        void send(ExecutionMode && msg)
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Execution mode command sent to - " << _tname << "   \n";
#endif
            lock_guard<mutex> uLock(_mutex);
            _message = move(msg);
            _executionMode = _message;
            _newMessage = true;
            _condition.notify_one();    // This is used when the execution is blocked by Step request.
        }

        // Stopping requestion by external unit. it sets the _ending variable so that the 
        // signal will be observed, and also signals to unique buffers to release their locks.
        void stop()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) stop() called  - " << _tname << "   \n";
#endif
            _ending.store(true);
            _mutex.unlock();
            _condition.notify_all();    
            _terminateInputOutput();
        }

        // Adds a new operator in the vector. Operators will be executed in order.
        void addOperator(BaseOperator * op)
        {
            operators.emplace_back(op);
        }

        // After initialization, the thread is started and kept track of by the static 
        // vector of the thread.
        void startThread()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Starting the thread  - " << _tname << "   \n";
#endif
            _allThreads.emplace_back(thread(&BaseExecuter::_execute, this, move(_exitPromise)));
        }

        // A promise is used to be able to follow the process and wait until the thread
        // has completed its task.
        void waitToEnd()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Waiting for ending  - " << _tname << "   \n";
#endif
            _futureExit.wait();
        }

    protected:
        string _tname;                      // A name to allow following the process
        vector<BaseOperator *> operators;   // Collection of all operators to be executed serially
        static vector<thread> _allThreads;  // Static collection for all threads
        ExecutionMode _executionMode;       // Tracking the requested execution mode (Continuous or step-wise)
        ExecutionMode _message;             // Command to the Executer
        condition_variable _condition;      // Condition variable for waiting in step-mode
        mutex _mutex;                       // Mutex for protection of data and used for condition.wait()
        atomic_bool _ending = false;        // Boolean variable to stop the infinite loop.
        bool _newMessage;                   // Indicator that new message has arrived.
        OperationStatus _opStatus;          // Recording the status of operations. 
        promise<void> _exitPromise;         // Promise to follow up that the task is complete
        future<void> _futureExit;           // To be checked for exit.

        virtual void _execute(promise<void> && exitPromise) = 0;        // The task manager that will be executed in the thread
        virtual void _terminateInputOutput() = 0;                       // A routine for termination of inputs and outputs to be 
                                                                        // implemented by child classes
    };

    vector<thread> BaseExecuter::_allThreads{};       // The static vector variable collecting all threads.

    //---------------------------------------------------------------------------------
    // The complete implementation of an executer with both input and output. 
    // It receives an input data and passes it through its operators one by one and finally
    // makes the result available for the next Executer in line, with additional operators.
    template <class T_IN, class T_OUT>
    class OperatorExecuter : public BaseExecuter
    {
    public:
        OperatorExecuter(string tname): BaseExecuter(tname), _inputBuffer(make_unique<T_IN>()), 
                        _outputBuffer(make_unique<T_OUT>()) {};
        ~OperatorExecuter(){};

        // A shared pointer to a unique buffer will be created and shared with the other executer that
        // is expected to provide the input. The transfer of data will be through swapping of resources
        // between the local unique pointer and the unique buffer.
        shared_ptr <UniqueBuffer<T_IN>> input()
        {
            if (_inputPort == nullptr) _inputPort = make_shared<UniqueBuffer<T_IN>>(_tname + "_input_buffer");
            return _inputPort;
        };

        // This is the alternative connection where the lifecycle of the unique buffer will be managed
        // by the other Executer, but the access is garanted to this one.
        void input(shared_ptr <UniqueBuffer<T_IN>> inp)
        {
            if (inp != nullptr)
            {
                if (_inputPort != nullptr) _inputPort.reset();
                _inputPort = inp;
            }
        };

        // Similar to above resource is allocated and is offered to the input of the neighboring executer.
        shared_ptr <UniqueBuffer<T_OUT>> output()
        {
            if (_outputPort == nullptr) _outputPort = make_shared<UniqueBuffer<T_OUT>>(_tname + "_output_buffer");
            return _outputPort;
        };

        // Similar to above resource is allocated by the neighboring executer and will be offered here.
        void output(shared_ptr <UniqueBuffer<T_OUT>> outp)
        {
            if (outp != nullptr)
            {
                if (_outputPort != nullptr) _outputPort.reset();
                _outputPort = outp;
            }
        };

        // Here, a pointer to pointer is saved so that the input of the first operator and output of the
        // last operator can be dynamically connected to the memory locations after swapping with the 
        // unique buffers.
        void opInput(T_IN ** inp)
        {
            _opInput = inp;
        };
        void opOutput(T_OUT ** outp)
        {
            _opOutput = outp;
        };

    private:
        T_IN ** _opInput;           // pointer to the input pointer of first operator
        T_OUT ** _opOutput;         // pointer to the output pointer of last operator
        shared_ptr<UniqueBuffer<T_IN>> _inputPort = nullptr;        // One buffer is needed between two Executers but
        shared_ptr<UniqueBuffer<T_OUT>> _outputPort = nullptr;      // it does not matter which one manages the lifetime
        unique_ptr<T_IN> _inputBuffer;              // Internal input buffer to store data locally in the thread 
        unique_ptr<T_OUT> _outputBuffer;            // Internal output buffer to store data locally in the thread 

        // implementation of the termination functino for the buffers. 
        void _terminateInputOutput()
        {
            input()->releaseAll();
            output()->releaseAll();
        }

        // This is the main task executer, which organizes and executes all tasks defined 
        // by operators.
        void _execute(promise<void> && exitPromise) override
        {
            OperationStatus opStat;             // Saves the intermediate status of the execution. 
            while (!_ending.load())             // Loop as long as no ending request appears.
            {
                unique_lock<mutex> uLock(_mutex);
#ifdef DEBUG_PRINTOUT
                cout << " 01) Loop starts  - " << _tname << "   \n";
#endif
                if ((_executionMode == ExecutionMode::Step) && (!_ending.load()))
                {
#ifdef DEBUG_PRINTOUT
                    cout << " 02) Waiting for command  - " << _tname << "   \n";
#endif
                    // Messages change the mode, or signal to step forward. The change of
                    // mode is already affected in the 'send' method. Here, we check
                    // that the _condition is notified AND that actually a message has arrived.
                    // If there is an ending request, we do not wait for a new message.
                    _condition.wait(uLock, [this] { return (_newMessage || _ending.load()); }); 
                    _newMessage = false;
                }
                uLock.unlock();
#ifdef DEBUG_PRINTOUT
                cout << " 03) Loop resumed  - " << _tname << "   \n";
#endif
                // Operation is moving forward unless there is an ending request.
                if (!_ending.load())
                {
#ifdef DEBUG_PRINTOUT
                   cout << " 04) Reading the input  - " << _tname << "   \n";
#endif
                    input()->receive(_inputBuffer);         // Wait until there is input data
                    *_opInput = _inputBuffer.get();         // Get the address of the input data and set to the input of the first operator
                    *_opOutput = _outputBuffer.get();       // Get the address of the input data and set to the output of the last operator
                    for ( auto op : operators)
                    {
                        opStat = op->operation();           // Perform the operation and take necessary actions if the process is finished
                        if (opStat == OperationStatus::complete) 
                        {
                            _opStatus = OperationStatus::complete;
                        }
                    }
                    if (_opStatus == OperationStatus::complete)
                    {
                        _ending.store(true);                // Set the ending signal to terminate
#ifdef DEBUG_PRINTOUT
                        cout << " 05) Operation completed  - " << _tname << "   \n";
#endif
                    }
#ifdef DEBUG_PRINTOUT
                    cout << " 06) Setting the output  - " << _tname << "   \n";
#endif
                    output()->send(_outputBuffer);          // Set the output buffer and wait until it is consumed. Note that othereise
                }                                           // setting the pointer to the address becomes partial.
            }
#ifdef DEBUG_PRINTOUT
            cout << " 07) Loop completed  - " << _tname << "   \n";
#endif
            exitPromise.set_value();                        // Signal that the promise is fulfilled
        }
   };

    //---------------------------------------------------------------------------------
    // This is the Source version of the above operator, with exactly same structure but 
    // without input. Explanations will not be repeated.
    template <class T_OUT>
    class SourceExecuter : public BaseExecuter
    {
    public:
        SourceExecuter(string tname): BaseExecuter(tname), _outputBuffer(make_unique<T_OUT>()) {};
        ~SourceExecuter(){};

        shared_ptr <UniqueBuffer<T_OUT>> output()
        {
            if (_outputPort == nullptr) _outputPort = make_shared<UniqueBuffer<T_OUT>>(_tname + "_output_buffer");
            return _outputPort;
        };
        void output(shared_ptr <UniqueBuffer<T_OUT>> outp)
        {
            if (outp != nullptr)
            {
                if (_outputPort != nullptr) _outputPort.reset();
                _outputPort = outp;
            }
        };
        void opOutput(T_OUT ** outp)
        {
            _opOutput = outp;
        };

    private:
        T_OUT ** _opOutput;
        shared_ptr<UniqueBuffer<T_OUT>> _outputPort = nullptr;
        unique_ptr<T_OUT> _outputBuffer;

        void _execute(promise<void> && exitPromise) override
        {
            OperationStatus opStat;
            while (!_ending.load())
            {
                unique_lock<mutex> uLock(_mutex);
#ifdef DEBUG_PRINTOUT
                cout << " 01) Loop starts  - " << _tname << "   \n";
#endif
                if ((_executionMode == ExecutionMode::Step) && (!_ending.load()))
                {
#ifdef DEBUG_PRINTOUT
                    cout << " 02) Waiting for command  - " << _tname << "   \n";
#endif
                    _condition.wait(uLock, [this] { return (_newMessage || _ending.load()); }); 
                    _newMessage = false;
                }
                uLock.unlock();
#ifdef DEBUG_PRINTOUT
                cout << " 03) Loop resumed  - " << _tname << "   \n";
#endif
                if (!_ending.load())
                {
                    *_opOutput = _outputBuffer.get();
                    for ( auto op : operators)
                    {
                        opStat = op->operation();
                        if (opStat == OperationStatus::complete) 
                        {
                            _opStatus = OperationStatus::complete;
                        }
                    }
                    if (_opStatus == OperationStatus::complete)
                    {
                        _ending.store(true);
#ifdef DEBUG_PRINTOUT
                        cout << " 05) Operation completed  - " << _tname << "   \n";
#endif
                    }
#ifdef DEBUG_PRINTOUT
                    cout << " 06) Setting the output  - " << _tname << "   \n";
#endif
                    output()->send(_outputBuffer);
                }
            }
#ifdef DEBUG_PRINTOUT
            cout << " 07) Loop completed  - " << _tname << "   \n";
#endif
            exitPromise.set_value();
        }

        void _terminateInputOutput()
        {
            output()->releaseAll();
        }
    };

    //---------------------------------------------------------------------------------
    // This is the Sink version of the above operator, with exactly same structure but 
    // without output. Explanations will not be repeated.

    template <class T_IN>
    class SinkExecuter : public BaseExecuter
    {
    public:
        SinkExecuter(string tname): BaseExecuter(tname), _inputBuffer(make_unique<T_IN>()) {};
        ~SinkExecuter(){};

        shared_ptr <UniqueBuffer<T_IN>> input()
        {
            if (_inputPort == nullptr) _inputPort = make_shared<UniqueBuffer<T_IN>>(_tname + "_input_buffer");
            return _inputPort;
        };
        void input(shared_ptr <UniqueBuffer<T_IN>> inp)
        {
            if (inp != nullptr)
            {
                if (_inputPort != nullptr) _inputPort.reset();
                _inputPort = inp;
            }
        };
        void opInput(T_IN ** inp)
        {
            _opInput = inp;
        };
        void addOperator(BaseOperator * op)
        {
            operators.emplace_back(op);
        };
    private:
        T_IN ** _opInput;
        shared_ptr<UniqueBuffer<T_IN>> _inputPort = nullptr;
        unique_ptr<T_IN> _inputBuffer;

        void _execute(promise<void> && exitPromise) override
        {
            OperationStatus opStat;
            while (!_ending.load())
            {
                unique_lock<mutex> uLock(_mutex);
#ifdef DEBUG_PRINTOUT
                cout << " 01) Loop starts  - " << _tname << "   \n";
#endif
                if ((_executionMode == ExecutionMode::Step) && (!_ending.load()))
                {
#ifdef DEBUG_PRINTOUT
                    cout << " 02) Waiting for command  - " << _tname << "   \n";
#endif
                    _condition.wait(uLock, [this] { return (_newMessage || _ending.load()); }); 
                    _newMessage = false;
                }
                uLock.unlock();
#ifdef DEBUG_PRINTOUT
                cout << " 03) Loop resumed  - " << _tname << "   \n";
#endif
                if (!_ending.load())
                {
#ifdef DEBUG_PRINTOUT
                   cout << " 04) Reading the input  - " << _tname << "   \n";
#endif
                     input()->receive(_inputBuffer);
                    *_opInput = _inputBuffer.get();
                    for ( auto op : operators)
                    {
                        opStat = op->operation();
                        if (opStat == OperationStatus::complete) 
                        {
                            _opStatus = OperationStatus::complete;
                        }
                    }
                    if (_opStatus == OperationStatus::complete)
                    {
                        _ending.store(true);
#ifdef DEBUG_PRINTOUT
                        cout << " 05) Operation completed  - " << _tname << "   \n";
#endif
                    }
                }
            }
#ifdef DEBUG_PRINTOUT
            cout << " 07) Loop completed  - " << _tname << "   \n";
#endif
            exitPromise.set_value();
        }
        void _terminateInputOutput()
        {
            input()->releaseAll();
        }
    };

}