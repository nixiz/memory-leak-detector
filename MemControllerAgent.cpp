#include "MemControllerAgent.h"
#include "MemLeakControllerService.h"
#include "malloc_allocator.hpp"
#include <boost/stacktrace/stacktrace.hpp>
#include <stdint.h>
#include <iostream>
#include <mutex>
#include <unordered_set>

#include <string>
#include <sstream>

namespace bs = boost::stacktrace;

using malloc_string = std::basic_string<char, std::char_traits<char>, malloc_allocator_t<char>>;
struct entry_t
{
	entry_t(void* allocated_ptr_)
		: allocated_ptr(allocated_ptr_)
	{ }
	entry_t(void* allocated_ptr_, 
					unsigned int thread_id_, 
					malloc_string stack_dump_)
		: allocated_ptr(allocated_ptr_)
		, thread_id(thread_id_)
		, stack_dump(stack_dump_)
	{ }
	void* allocated_ptr{};
	unsigned int thread_id{};
	malloc_string stack_dump{};
};

inline std::ostream& operator << (std::ostream& os, const entry_t& entry)
{
	os << "thrd: " << entry.thread_id << "\t"
		<< "ptr: " << entry.allocated_ptr << "\n"
		<< "-------stack trace-------\n"
		<< entry.stack_dump << "\n";
	return os;
}

inline bool operator == (const entry_t& lhs, const entry_t& rhs) { return lhs.allocated_ptr == rhs.allocated_ptr; }

struct new_entry_hash_t : std::hash<void*>
{
	using base = std::hash<void*>;
	std::size_t operator()(const entry_t& entry) const { return base::operator()(entry.allocated_ptr); }
};

class MemControllerAgentImpl
{
public:
	MemControllerAgentImpl()
	{
		entry_table = malloc_new<entry_set_t>();
		std::cout << "Hello!\n";
	}

	~MemControllerAgentImpl()
	{
		free_new(entry_table);
		std::cout << "Bye!\n";
	}
	
	void OnNewAllocation(void* ptr, unsigned int thread_id)
	{
		//auto bt = stacktrace(4, 10);
		//std::cout << bt << std::endl;
		auto res = entry_table->insert(entry_t{ptr, thread_id, ""});
		if (!res.second) {
			std::cerr << "this cannot ben possible!" << std::endl;
		}
	}

	void OnDelAllocation(void* ptr, unsigned int thread_id)
	{
		auto it = entry_table->find(entry_t{ptr});
		if (it != entry_table->end())
		{
			entry_table->erase(it);
		}
		else
		{
			std::cerr 
				<< "deallocation of ptr: " << ptr 
				<< " made in another thread[" << thread_id << "]" 
				<< std::endl;
		}
	}

	void DumpCurrentAllocations() const
	{
		for (const auto& e : *entry_table)
		{
			std::cout << e;
		}
	}

	auto getAllocationCount() const noexcept
	{
		return entry_table->size();
	}

private:
	using entry_set_t = std::unordered_set<
		entry_t,
		new_entry_hash_t,
		std::equal_to<entry_t>,
		malloc_allocator_t<entry_t>
	>;
	entry_set_t *entry_table;
	//frame sýnýfý allocator aware çalýþmadýðý için içerisindeki string yaratýmlarý patlýyor!
	//using stacktrace = bs::basic_stacktrace<malloc_allocator_t<bs::frame>>;
};

MemControllerAgent::MemControllerAgent() 
{
	p_impl = malloc_new<MemControllerAgentImpl>();

	auto thid = std::this_thread::get_id();
	working_thread = *reinterpret_cast<unsigned int*>(&thid);
	// these 3 lines of code for being sure that type casting thread::id to integer is successful
	//std::basic_stringstream<char, std::char_traits<char>, malloc_allocator_t<char>> ss;
	//ss << thid;
	//_ASSERT(ss.str() == std::to_string(working_thread));
	MemLeakControllerService::GetInstance()->RegisterNewAgent(this);
}

MemControllerAgent::~MemControllerAgent() {
	MemLeakControllerService::GetInstance()->UnRegisterNewAgent(this);
	if (p_impl) {
		p_impl->~MemControllerAgentImpl();
	}
}

void* MemControllerAgent::allocate(size_t n) noexcept
{
	void* ptr = std::malloc(n);
	if (ptr) 
		p_impl->OnNewAllocation(ptr, working_thread);
	return ptr;
}

void MemControllerAgent::deallocate(void* ptr) noexcept
{
	_ASSERT(ptr);
	p_impl->OnDelAllocation(ptr, working_thread);
	std::free(ptr);
}

unsigned int MemControllerAgent::GetWorkingThread() const
{
	return working_thread;
}

size_t MemControllerAgent::NumberOfAllocations() const
{
	return p_impl->getAllocationCount();
}

void MemControllerAgent::DumpCurrentAllocations() const
{
	p_impl->DumpCurrentAllocations();
}