#include "MemControllerAgent.h"
#include "MemLeakControllerService.h"
#include "MemoryEntry.h"
#include "malloc_allocator.hpp"
#include "stack_helper.h"
#include <stdint.h>
#include <iostream>
#include <atomic>
#include <mutex>
#include <unordered_set>
#include <algorithm>
#include <string>
#include <sstream>


class MemControllerAgentImpl final
	: public MemLeakControllerService::Client
{
public:
	MemControllerAgentImpl(const std::thread::id& tid_)
		: tid(tid_)
	{
		local_entry_table = malloc_new<entry_set_t>();
		malloc_create(&should_lock, false);
		malloc_create(&guard);

		std::cout << "Hello! from thread: " << tid << "\n";
		MemLeakControllerService::GetInstance()->RegisterNewAgent(this);
	}

	~MemControllerAgentImpl()
	{
		std::cout << "Bye! from thread: " << tid << "\n";
		MemLeakControllerService::GetInstance()->UnRegisterNewAgent(this);
		free_new(local_entry_table);
		free_new(guard);
		free_new(should_lock);
	}
	
	void OnNewAllocation(void* ptr, size_t n)
	{
		std::unique_lock<std::mutex> lock(*guard);
		auto res = local_entry_table->insert(entry_t{ptr, n, tid, ""});
		if (!res.second) {
			std::cerr << "this cannot ben possible!" << std::endl;
		}
	}

	void OnDelAllocation(void* ptr)
	{
		std::unique_lock<std::mutex> lock(*guard);
		auto it = local_entry_table->find(entry_t{ptr});
		if (it != local_entry_table->end())
		{
			local_entry_table->erase(it);
		}
		else
		{
			std::cerr 
				<< "deallocation of ptr: " << ptr 
				<< " made in another thread[" << tid << "]" 
				<< std::endl;
		}
	}

	size_t NumberOfAllocations() const override
	{
		std::unique_lock<std::mutex> lock(*guard);
		return local_entry_table->size();
	}

	const std::thread::id& GetWorkingThread() const override
	{
		return tid;
	}

	void DumpCurrentAllocations(entry_set_t* global_entry_table)
	{
		std::unique_lock<std::mutex> lock(*guard);
		global_entry_table->insert(local_entry_table->begin(), 
															 local_entry_table->end());
	}
	
private:
	entry_set_t *local_entry_table;
	std::atomic_bool* should_lock;
	std::mutex* guard;
	std::thread::id tid;
};

MemControllerAgent::MemControllerAgent() 
{
	auto thid = std::this_thread::get_id();
	//working_thread = *reinterpret_cast<unsigned int*>(&thid);
	p_impl = malloc_new<MemControllerAgentImpl>(thid);
}

MemControllerAgent::~MemControllerAgent() {
	if (p_impl) {
		p_impl->~MemControllerAgentImpl();
	}
}

void MemControllerAgent::allocated(void* ptr, size_t n) noexcept
{
	p_impl->OnNewAllocation(ptr, n);
}

void MemControllerAgent::deallocated(void* ptr) noexcept
{
	p_impl->OnDelAllocation(ptr);
}
