/******************************************************************************************
 * In this file, 8 simple classes are defined to be used for testing of the system
 *      1. CounterSource: Start at a given number. Add 1 at each call 5 times.
 *          After that, the operation status is changed to 'complete' and
 *          a constant value of start + 5 is delivered.
 *          With this source, we can test the ending process.
 *      2. CounterSink: A very simple sink that only receives the input and makes it
 *          visible through a member function getValue(). It allows to test that a chain
 *          from source to final sink functions properly.
 *      3. Mult2: output = input * 2.1
 *      4. Div2Round: utput = floor(input/2)
 *      5. Mult3: output = input * 3.1
 *      6. Div2Round: output = floor(input/3)
 *      7. Add5: output = input + 5
 *      8. Div2: output = input / 2
 *****************************************************************************************/

#include <operator.hpp>
#include <opsexecuter.hpp>

using namespace parallelOperators;

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
class CounterSource : public SourceOperator<int>
{
public:
    CounterSource(std::string opName, int start): SourceOperator(opName) 
    {
        _counter = start;
        _endLimit = start + 5;
    };
    OperationStatus operation() override;
private:
    int _counter {0};
    int _endLimit;
};

OperationStatus CounterSource::operation()
{
    if (_counter < _endLimit)
    {
        *_output = _counter++;
        return OperationStatus::running;
    }
    else
    {
        *_output = _counter;
        return OperationStatus::complete;
    }
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
class CounterSink : public SinkOperator<float>
{
public:
    CounterSink(std::string opName): SinkOperator(opName) {};
    OperationStatus operation() override;
    float getValue();
private:
    float _sinkVariable {0};
};

OperationStatus CounterSink::operation()
{
    _sinkVariable = *_input;
    return OperationStatus::running;
}

float CounterSink::getValue()
{
    return _sinkVariable;
}


//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
class  Mult2 : public Operator<int, float>
{    
public:
    Mult2(std::string opName): Operator(opName) {};
    OperationStatus operation() override;
};

OperationStatus Mult2::operation(){
    *_output = 2.1*(*_input);
    return OperationStatus::running;
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
class  Div2Round : public Operator<float, float>
{
public:
    Div2Round(std::string opName): Operator(opName) {};
    OperationStatus operation() override;
};

OperationStatus Div2Round::operation(){
    *_output = std::floor((*_input)/2);
    return OperationStatus::running;
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
class  Mult3 : public Operator<int, float>
{
private:
    
public:
    Mult3(std::string opName): Operator(opName) {};
    OperationStatus operation() override;
};

OperationStatus Mult3::operation(){
    *_output = 3.1*(*_input);
    return OperationStatus::running;
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
class  Div3Round : public Operator<float, float>
{
private:
    
public:
    Div3Round(std::string opName): Operator(opName) {};
    OperationStatus operation() override;
};

OperationStatus Div3Round::operation(){
    *_output = std::floor((*_input)/3);
    return OperationStatus::running;
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
class  Add5 : public Operator<float, float>
{
private:
    
public:
    Add5(std::string opName): Operator(opName) {};
    OperationStatus operation() override;
};

OperationStatus Add5::operation(){
    *_output = 5.0 + *_input;
    return OperationStatus::running;
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
class  Div2 : public Operator<float, float>
{
private:
    
public:
    Div2(std::string opName): Operator(opName) {};
    OperationStatus operation() override;
};

OperationStatus Div2::operation(){
    *_output = *_input/2.0;
    return OperationStatus::running;
};
