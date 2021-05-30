// MemControllerService.cpp : Defines the entry point for the application.
//

#include <string>
#include <random>
#include <thread>
#include "MemControllerService.h"
#include "new.hpp"
#include "MemLeakControllerService.h"

using namespace std;

struct test_s
{
	test_s(string name_)
		: data{} 
		, name{std::move(name_)}
	{ }
	int data;
	std::string name;
};

inline test_s* create_test_s()
{
	std::srand(0);
	const auto size = 1 + (rand() % 100);
	const char c = 'a' + (rand() % 29);
	return new test_s(std::string(size, c));
}

template<typename T, int N> void good()
{
	delete (new T);
	delete (new(std::nothrow) T);
	delete[](new T[N]);

	struct Boom { Boom() { throw 1; } };

	try { new Boom; }
	catch (...) {} // not a leak!
	try { new Boom[N]; }
	catch (...) {} // same!
}

template<typename T, int N> void mismatch()
{
	delete[](new T);
	delete (new T[N]);
}

template<typename T, int N> void leak()
{
	/*delete*/ new T;
	/*delete []*/ new T[N];
	/*delete []*/ new T[N][N+2];
}

auto leak_vec()
{
	std::vector<test_s*> ivec;
	ivec.reserve(5);

	for (size_t i = 0; i < 15; i++)
	{
		ivec.push_back(create_test_s());
	}
	return ivec;
}

constexpr	auto ss = sizeof(void*);

MemLeakControllerService* glMemLeakControllerService = MemLeakControllerService::GetInstance();

int main()
{
	//std::set_new_handler(::operator new);

	//good<char, 100>();
	//mismatch<char, 100>();
	//leak<char, 100>();

	//auto x = new int[10];
	//delete[] x;
	//
	auto xnt  = new(std::nothrow) int(13);
	delete xnt;

	std::thread th([] 
	{
		new double(3.14);
	}); 
	//std::this_thread::sleep_for(std::chrono::seconds(3));
	getchar();
	th.join();

	//auto ivec = leak_vec();

	//ndt::dump_all();

	//for (auto& s : ivec)
	//{
	//	delete s;
	//}
	//ivec.clear();

	//ndt::dump_all();
	return 0;
}
