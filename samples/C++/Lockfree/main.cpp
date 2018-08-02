// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cstdint>
#include "lockfree.h"

template<typename T>
void run_test(  int producers,    // Number of producer threads
				int consumers)    // Number of consumer threads
{
	using namespace std;
	boost::thread_group producer_threads, consumer_threads;

	// Initiate a timer to measure performance
	boost::timer::cpu_timer timer;

	cout << T::get_name() << "\t" << producers << "\t" << consumers << "\t";

	// Reset the counters after the previous test
	producer_count = consumer_count = 0;
	done = false;
	ResetEvent(hEvtDone);

	// Start all the producer threads with a given thread proc
	for (int i = 0; i != producers; ++i)
		producer_threads.create_thread(T::producer);

	// Start all the consumer threads with a given thread proc
	for (int i = 0; i != consumers; ++i)
		consumer_threads.create_thread(T::consumer);

	// Waiting for the producers to complete
	producer_threads.join_all();
	done = true;
	SetEvent(hEvtDone);

	// Waiting for the consumers to complete
	consumer_threads.join_all();

	// Report the time of execution
	auto nanoseconds = boost::chrono::nanoseconds(timer.elapsed().user + timer.elapsed().system);
	auto seconds = boost::chrono::duration_cast<boost::chrono::milliseconds>(nanoseconds);
	auto time_per_item = nanoseconds.count() / producer_count;
	cout << time_per_item << "\t" << seconds.count() << endl;
}

class StackTest
{
public:
	static const char* get_name() { return "MS SList"; }
	static void producer(void)
	{
		for (int i = 0; i != iterations; ++i)
		{
			if (!stack.push(++producer_count))
				return;
		}
	}
	static void consumer(void)
	{
		int64_t value;
		while (WaitForSingleObject(hEvtDone, 10) != WAIT_OBJECT_0)
		{
			while (stack.pop(value))
			{
				++consumer_count;
			}
		}
		while (stack.pop(value))
		{
			++consumer_count;
		}
	}
private:
	static SList<int64_t> stack;
	static uint32_t iterations;
	static uint32_t producer_count;
	static uint32_t consumer_count;
	static HANDLE hEvtDone;
};

int main()
{
	run_test<StackTest>(10, 10);
	return 0;
}

