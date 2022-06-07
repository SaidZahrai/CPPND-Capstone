#pragma once

/*****************************************************************************
 * This file defines three template classes for defining the inner operators.
 * All three have the same base class and similar structure, but slightly 
 * different funtionality. 
 *      1. The source has no input and its operation is expected to acquire 
 *          data the way it should be. It can for example be capturing data 
 *          from camera or reading data from a port or disk.
 *      2. The sink has the inverse function of the source. It does not have
 *          any explicit output and is expected to be the end of local process.
 *          For example, a sink can be sending the data to a consumer or saving
 *          on the disk.
 *      3. The third element is an oerator that receive a data, modifies it and
 *          delivers it to the output.
 * 
 * Operators should implement the logic of the operation, necessary internal 
 * data structures, and a standard way of allowing connection between them and
 * the wrapper which will execute the operation.
 * 
 * Between two operators, there is always a unique pointer, so that the data
 * ownership will be clearly defined. However, it does not matter which one
 * of the two operators will manage the lifecycle of the resources. One operator
 * can request a resource or offer it in case the other is requesting.
 * 
 * Operators can build a chain without any linit. The wrapper will execute their
 * operation in a sequence in a thread and therefore there are no risks for 
 * data race.
 * 
 * A chain of operators will be hosted by the wrapper, which runs the thread.
 * Data should be acquired from the input of the wrapper by the first operator
 * and the result will be transfered to the output of the wrapper by the last
 * link in the chain.
 * 
 * Communication between the input of the first operator and the input buffer 
 * of the wrapper, as well as between the output of the last operator and 
 * output buffer of the wrapper is through a pointer-to-pointer and is managed
 * by the wrapper. The wrapper connects the input and output to right location
 * in the memory.
 * 
 * ****************************************************************************/

using namespace std;

namespace parallelOperators
{
    // Reporting the status of operation to the caller
    enum OperationStatus
    {
        running = 0,
        complete,
        error
    };

    // Base operator class, allowing to handle the operation in a generic way.
    class BaseOperator
    {
    public:
        BaseOperator(string opName)
        {
            _opName = opName;
        };

        virtual OperationStatus operation() = 0;  // Operation provided by the operator
    protected:
        string _opName; // A string so that the object can be recognized.
    };
    
    //-----------------------------------------------------------------------------------
    // An operator which receied data, operated on that and delivers to the next operator.   
    template <class T_IN, class T_OUT>
    class Operator : public BaseOperator
    {
    public:
        Operator(string opName) : BaseOperator(opName){};

        // Member function, deliveering the address to the input pointer so that the wrapper
        // can connects the pointer to right data location.
        T_IN ** inputAddress()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) The address of input pointer requested from  - " << _opName << "   \n";
#endif
            return & _input;
        };

        // Member function, creating and delivering a pointer to the input location. Used when this operator
        // manages the lifecycle of the interface between the two operators.
        T_IN * input()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Input reference requested from  - " << _opName << "   \n";
#endif
            if (_input == nullptr)
            {
                _inputBuffer = make_unique<T_IN>();
                _input = _inputBuffer.get();
            }
            return _input;
        };

        // Member function, receiving a pointer to the input location, managed by another operator.
        void input(T_IN * inp)
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Input pointer to be set for  - " << _opName << "   \n";
#endif
            _input = inp;
        };

        // Member function, deliveering the address to the output pointer so that the wrapper
        // can connects the pointer to right data location.
        T_OUT ** outputAddress()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) The address of output pointer requested from  - " << _opName << "   \n";
#endif
            return & _output;
        };

        // Member function, creating and delivering a pointer to the output location. Used when this operator
        // manages the lifecycle of the interface between the two operators.
        T_OUT * output()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Output reference requested from  - " << _opName << "   \n";
#endif
            if (_output == nullptr) 
            {
                _outputBuffer = make_unique<T_OUT>();
                _output = _outputBuffer.get();
            }
            return _output;
        };
        // Member function, receiving a pointer to the output location, managed by another operator.
        void output(T_OUT * outp)
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Output pointer to be set for  - " << _opName << "   \n";
#endif
            _output = outp;
        };
    protected:
        // Buffers in case this object is responsiblel for the lifecycle
        unique_ptr <T_IN> _inputBuffer;     
        unique_ptr <T_OUT> _outputBuffer;

        // Pointers that point to the location of the interface between the operator and outside
        T_IN * _input = nullptr;
        T_OUT * _output = nullptr;
    };

    //-----------------------------------------------------------------------------------
    // Source operator with same structure as atwo sided operator, but without any input.
    // Member variable and methods as same as the two sided version and will not be commented
    // again.
    template <class T_OUT>
    class SourceOperator : public BaseOperator
    {
    public:
        SourceOperator(string opName) : BaseOperator(opName){};
        T_OUT ** outputAddress()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) The address of output pointer requested from  - " << _opName << "   \n";
#endif
            return & _output;
        };
        T_OUT * output()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Output reference requested from  - " << _opName << "   \n";
#endif
            if (_output == nullptr) 
            {
                _outputBuffer = make_unique<T_OUT>();
                _output = _outputBuffer.get();
            }
            return _output;
        };
        void output(T_OUT * outp)
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Output pointer to be set for  - " << _opName << "   \n";
#endif
            _output = outp;
        };
    protected:
        unique_ptr <T_OUT> _outputBuffer;
        T_OUT * _output = nullptr;
    };
    
    //-----------------------------------------------------------------------------------
    // Sink operator with same structure as a two sided operator, but without any output.
    // Member variable and methods as same as the two sided version and will not be commented
    // again.
    template <class T_IN>
    class SinkOperator : public BaseOperator
    {
    public:
        SinkOperator(string opName) : BaseOperator(opName){};
        T_IN ** inputAddress()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) The address of input pointer requested from  - " << _opName << "   \n";
#endif
            return & _input;
        };
        T_IN * input()
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Input reference requested from  - " << _opName << "   \n";
#endif
            if (_input == nullptr)
            {
                _inputBuffer = make_unique<T_IN>();
                _input = _inputBuffer.get();
            }
            return _input;
        };
        void input(T_IN * inp)
        {
#ifdef DEBUG_PRINTOUT
            cout << " **) Input pointer to be set for  - " << _opName << "   \n";
#endif
            _input = inp;
        };
    protected:
        unique_ptr <T_IN> _inputBuffer;
        T_IN * _input = nullptr;
    };
}