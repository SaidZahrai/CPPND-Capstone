#include <gtest/gtest.h>
#include <iostream>
#include <cmath>

/******************************************************************************************
 */
#include "classdefs.hpp"
 /*
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

class OperatorTest : public ::testing::Test
{
protected:
    Mult2 op1 = Mult2("multiply_2.1");
    Div2Round op2 = Div2Round("divide_2.1_floor");

    CounterSource cSrc = CounterSource ("counter_37", 37);
    CounterSink cSnk = CounterSink ("sink_37");

    // virtual void SetUp() override
    // {      
    // }

    // virtual void TearDown()
    // {
    // }
};

TEST_F(OperatorTest, ChechOneOperator)
{
    std::cout << "[ INFO     ] " << "Test of operator 1 int -> float.\n";
    ASSERT_TRUE(op1.input());
    ASSERT_TRUE(op1.output());
    *op1.input() = 3;
    ASSERT_EQ(*op1.input(), 3);
    *op1.output() = 2.3;
    ASSERT_TRUE(op1.output());
    op1.operation();
    ASSERT_NEAR(*op1.output(), 3*2.1, 1e-3);
}

TEST_F(OperatorTest, CheckTwoOperators)
{
    std::cout << "[ INFO     ] " << "Test of int -> float -> floor.\n";

    *op1.input() = 3;
    *op1.output() = 2.3;
    op1.operation();
    ASSERT_NEAR(*op1.output(), 3*2.1, 1e-3);
    *op2.input() = *op1.output();
    *op2.output() = 0.0;
    ASSERT_NEAR(*op2.input(), 3*2.1, 1e-3);
    op2.operation();
    ASSERT_NEAR(*op2.output(), std::floor(3*2.1/2), 1e-5);

}

TEST_F(OperatorTest, LinkedTwoOperators)
{
    std::cout << "[ INFO     ] " << "Test of linked operators.\n";
    *op1.input() = 16;
    op2.input(op1.output());
    *op2.output() = 0.0;
    op1.operation();
    ASSERT_NEAR(*op1.output(), 16*2.1, 1e-3);
    op2.operation();
    ASSERT_NEAR(*op2.output(), std::floor(16*2.1/2), 1e-5);
}

TEST_F(OperatorTest, SourceAndSink)
{
    std::cout << "[ INFO     ] " << "Test of linked operators with source and sink.\n";
    OperationStatus opStat;

    op1.input(cSrc.output());
    op2.input(op1.output());
    op2.output(cSnk.input());

    int sourceOutput = 37; // First call

    opStat = cSrc.operation();
    op1.operation();
    ASSERT_NEAR(*op1.output(), sourceOutput*2.1, 1e-3);
    op2.operation();
    ASSERT_NEAR(*op2.output(), std::floor(sourceOutput*2.1/2), 1e-5);
    cSnk.operation();
    ASSERT_NEAR(cSnk.getValue(), std::floor(sourceOutput*2.1/2), 1e-5);

    sourceOutput = 38; // Second call
    opStat = cSrc.operation();
    op1.operation();
    op2.operation();
    cSnk.operation();
    ASSERT_EQ(opStat, OperationStatus::running);
    ASSERT_NEAR(cSnk.getValue(), std::floor(sourceOutput*2.1/2), 1e-5);

    sourceOutput = 39; // Third call
    opStat = cSrc.operation();
    op1.operation();
    op2.operation();
    cSnk.operation();
    ASSERT_EQ(opStat, OperationStatus::running);
    ASSERT_NEAR(cSnk.getValue(), std::floor(sourceOutput*2.1/2), 1e-5);

    sourceOutput = 40; // Forth call
    opStat = cSrc.operation();
    op1.operation();
    op2.operation();
    cSnk.operation();
    ASSERT_EQ(opStat, OperationStatus::running);
    ASSERT_NEAR(cSnk.getValue(), std::floor(sourceOutput*2.1/2), 1e-5);

    sourceOutput = 41; // Fifth call
    opStat = cSrc.operation();
    op1.operation();
    op2.operation();
    cSnk.operation();
    ASSERT_EQ(opStat, OperationStatus::running);
    ASSERT_NEAR(cSnk.getValue(), std::floor(sourceOutput*2.1/2), 1e-5);

    sourceOutput = 42; // 6th call - Now, 42 - 37 is not smaller than 5, so the status should change.
    opStat = cSrc.operation();
    op1.operation();
    op2.operation();
    cSnk.operation();
    ASSERT_EQ(opStat, OperationStatus::complete);
    ASSERT_NEAR(cSnk.getValue(), std::floor(sourceOutput*2.1/2), 1e-5);

}

class ExecutionTest : public ::testing::Test
{
protected:
    Mult3 op1 = Mult3("multiply_3.1");
    Div3Round op2 = Div3Round("divide_3_floor");

    Add5 op3 = Add5("add_5");
    Div2 op4 = Div2("divide_2");

    OperatorExecuter<int,float> exec1 = OperatorExecuter<int,float>("Exec_1");
    OperatorExecuter<float,float> exec2 = OperatorExecuter<float,float>("Exec_2");

    SourceExecuter<int> source = SourceExecuter<int>("Source");
    SinkExecuter<float> sink = SinkExecuter<float>("Sink");

    CounterSource cSrc = CounterSource ("counter_37", 37);
    CounterSink cSnk = CounterSink ("sink_37");
    
    // virtual void SetUp() override
    // {      
    // }

    // virtual void TearDown()
    // {
    // }
};

TEST_F(ExecutionTest, OneThreadTest)
{   
    std::cout << "[ INFO     ] " << "Test of linked operators run in a thread.\n";

    op2.input(op1.output());
    exec1.opInput(op1.inputAddress());
    exec1.opOutput(op2.outputAddress());
    exec1.addOperator(&op1);
    exec1.addOperator(&op2);

    auto input = make_unique<int>();
    auto output = make_unique<float>();

    exec1.startThread();
    exec1.send(ExecutionMode::Continuous);

    *input = 16;
    exec1.input()->send(input);
    exec1.output()->receive(output);
    ASSERT_NEAR(*output, std::floor(16*3.1/3), 1e-5);
    *input = 15;
    exec1.input()->send(input);
    exec1.output()->receive(output);
    ASSERT_NEAR(*output, std::floor(15*3.1/3), 1e-5);

    exec1.send(ExecutionMode::Step);

    *input = 13;
    exec1.input()->send(input);
    exec1.output()->receive(output);
    ASSERT_NEAR(*output, std::floor(13*3.1/3), 1e-5);
    *input = 12;
    exec1.send(ExecutionMode::Step);
    exec1.input()->send(input);
    exec1.output()->receive(output);
    ASSERT_NEAR(*output, std::floor(12*3.1/3), 1e-5);

    exec1.stop();
    exec1.waitToEnd();
    
}

TEST_F(ExecutionTest, TwoThreadsTest)
{
    std::cout << "[ INFO     ] " << "Test of linked operators run in two threads.\n";

    op2.input(op1.output());
    exec1.opInput(op1.inputAddress());
    exec1.opOutput(op2.outputAddress());
    exec1.addOperator(&op1);
    exec1.addOperator(&op2);

    op4.input(op3.output());
    exec2.opInput(op3.inputAddress());
    exec2.opOutput(op4.outputAddress());
    exec2.addOperator(&op3);
    exec2.addOperator(&op4);

    exec2.input(exec1.output());

    exec1.send(ExecutionMode::Continuous);
    exec2.send(ExecutionMode::Continuous);
    exec1.startThread();
    exec2.startThread();

    std::this_thread::sleep_for (std::chrono::milliseconds(100));

    auto input = make_unique<int>();
    auto output = make_unique<float>();

    *input = 16;
    exec1.input()->send(input);
    exec2.output()->receive(output);
    ASSERT_NEAR(*output, (std::floor(16*3.1/3)+5.0)/2.0, 1e-5);

    *input = 15;
    exec1.input()->send(input);
    exec2.output()->receive(output);
    ASSERT_NEAR(*output, (std::floor(15*3.1/3)+5.0)/2.0, 1e-5);

    std::this_thread::sleep_for (std::chrono::milliseconds(100));
    exec1.stop();
    exec2.stop();

    exec1.waitToEnd();
    exec2.waitToEnd();

}

TEST_F(ExecutionTest, FourThreadsCompleteTest)
{
    std::cout << "[ INFO     ] " << "Test of linked operators run in two threads with souce and sink.\n";

    op2.input(op1.output());
    exec1.opInput(op1.inputAddress());
    exec1.opOutput(op2.outputAddress());
    exec1.addOperator(&op1);
    exec1.addOperator(&op2);

    op4.input(op3.output());
    exec2.opInput(op3.inputAddress());
    exec2.opOutput(op4.outputAddress());
    exec2.addOperator(&op3);
    exec2.addOperator(&op4);


    source.addOperator(&cSrc);
    source.opOutput(cSrc.outputAddress());

    sink.addOperator(&cSnk);
    sink.opInput(cSnk.inputAddress());

    exec1.input(source.output());
    exec2.input(exec1.output());
    sink.input(exec2.output());

    source.send(ExecutionMode::Step);
    exec1.send(ExecutionMode::Continuous);
    exec2.send(ExecutionMode::Continuous);
    sink.send(ExecutionMode::Continuous);

    source.startThread();
    exec1.startThread();
    exec2.startThread();
    sink.startThread();

    auto input = make_unique<int>();
    auto output = make_unique<float>();

    std::this_thread::sleep_for (std::chrono::milliseconds(100));

    int inputValue = 37;
    source.send(ExecutionMode::Step);
    std::this_thread::sleep_for (std::chrono::milliseconds(100));
    ASSERT_NEAR(cSnk.getValue(), (std::floor((++inputValue)*3.1/3)+5.0)/2.0, 1e-5);

    source.send(ExecutionMode::Step);
    std::this_thread::sleep_for (std::chrono::milliseconds(100));
    ASSERT_NEAR(cSnk.getValue(), (std::floor((++inputValue)*3.1/3)+5.0)/2.0, 1e-5);

    source.send(ExecutionMode::Step);
    std::this_thread::sleep_for (std::chrono::milliseconds(100));
    ASSERT_NEAR(cSnk.getValue(), (std::floor((++inputValue)*3.1/3)+5.0)/2.0, 1e-5);

    std::this_thread::sleep_for (std::chrono::milliseconds(100));

    source.stop();
    exec1.stop();
    exec2.stop();
    sink.stop();

    exec1.waitToEnd();
    exec2.waitToEnd();
    source.waitToEnd();
    sink.waitToEnd();

}

TEST_F(ExecutionTest, FourThreadsCompleteTestRun5times)
{
    std::cout << "[ INFO     ] " << "Test of linked operators run in two threads with souce and sink should run 5 times only.\n";

    op2.input(op1.output());
    exec1.opInput(op1.inputAddress());
    exec1.opOutput(op2.outputAddress());
    exec1.addOperator(&op1);
    exec1.addOperator(&op2);

    op4.input(op3.output());
    exec2.opInput(op3.inputAddress());
    exec2.opOutput(op4.outputAddress());
    exec2.addOperator(&op3);
    exec2.addOperator(&op4);


    source.addOperator(&cSrc);
    source.opOutput(cSrc.outputAddress());

    sink.addOperator(&cSnk);
    sink.opInput(cSnk.inputAddress());

    exec1.input(source.output());
    exec2.input(exec1.output());
    sink.input(exec2.output());

    source.send(ExecutionMode::Step);
    exec1.send(ExecutionMode::Continuous);
    exec2.send(ExecutionMode::Continuous);
    sink.send(ExecutionMode::Continuous);

    source.startThread();
    exec1.startThread();
    exec2.startThread();
    sink.startThread();

    auto input = make_unique<int>();
    auto output = make_unique<float>();

    std::this_thread::sleep_for (std::chrono::milliseconds(100));

    int inputValue = 37;
    source.send(ExecutionMode::Step);
    std::this_thread::sleep_for (std::chrono::milliseconds(100));
    ASSERT_NEAR(cSnk.getValue(), (std::floor((++inputValue)*3.1/3)+5.0)/2.0, 1e-5);

    source.send(ExecutionMode::Step);
    std::this_thread::sleep_for (std::chrono::milliseconds(100));
    ASSERT_NEAR(cSnk.getValue(), (std::floor((++inputValue)*3.1/3)+5.0)/2.0, 1e-5);

    source.send(ExecutionMode::Step);
    std::this_thread::sleep_for (std::chrono::milliseconds(100));
    ASSERT_NEAR(cSnk.getValue(), (std::floor((++inputValue)*3.1/3)+5.0)/2.0, 1e-5);

    source.send(ExecutionMode::Step);
    std::this_thread::sleep_for (std::chrono::milliseconds(100));
    ASSERT_NEAR(cSnk.getValue(), (std::floor((++inputValue)*3.1/3)+5.0)/2.0, 1e-5);

    source.send(ExecutionMode::Step);
    std::this_thread::sleep_for (std::chrono::milliseconds(100));
    ASSERT_NEAR(cSnk.getValue(), (std::floor((++inputValue)*3.1/3)+5.0)/2.0, 1e-5);

    std::cout << "[ INFO     ] " << "All results obtained.\n";

    std::this_thread::sleep_for (std::chrono::milliseconds(100));
 

    sink.stop();
    exec2.stop();
    exec1.stop();
    source.stop();
    std::cout << "[ INFO     ] " << "Wait for Source to end .\n";
    source.waitToEnd();
    std::cout << "[ INFO     ] " << "Source ended.\n"; 

    std::cout << "[ INFO     ] " << "Wait for Exec1 to end.\n";
    exec1.waitToEnd();
    std::cout << "[ INFO     ] " << "Exec1 ended.\n";

    std::cout << "[ INFO     ] " << "Wait for Exec2 to end.\n";
    exec2.waitToEnd();
    std::cout << "[ INFO     ] " << "Exec2 ended.\n";

    std::cout << "[ INFO     ] " << "Wait for Sink to end.\n";
    sink.waitToEnd();
    std::cout << "[ INFO     ] " << "Sink ended.\n";

}
