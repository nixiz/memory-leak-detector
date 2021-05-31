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

static const std::thread::id main_thdr_id = std::this_thread::get_id();

class MemControllerAgentImpl final
	: public MemLeakControllerService::Client
{
public:
	MemControllerAgentImpl(const std::thread::id& tid_)
		: tid(tid_)
	{
		malloc_create(&alloc_entry_table);
		malloc_create(&dealloc_entry_table);
		malloc_create(&should_lock, false);
		malloc_create(&guard);

		std::cout << "Hello! from thread: " << tid << "\n";
		MemLeakControllerService::GetInstance()->RegisterNewAgent(this);
	}

	~MemControllerAgentImpl()
	{
		std::cout << "Bye! from thread: " << tid << "\n";
		MemLeakControllerService::GetInstance()->UnRegisterNewAgent(this);
		free_new(alloc_entry_table);
		free_new(dealloc_entry_table);
		free_new(guard);
		free_new(should_lock);
	}
	
	void OnNewAllocation(void* ptr, size_t n)
	{
		std::unique_lock<std::mutex> lock(*guard);
		auto res = alloc_entry_table->insert(entry_t{ptr, n, tid, ""});
		if (!res.second) {
			std::cerr << "this cannot ben possible!" << std::endl;
		}
	}

	void OnDelAllocation(void* ptr)
	{
		std::unique_lock<std::mutex> lock(*guard);
		auto res = dealloc_entry_table->insert(entry_t{ptr, tid });
		if (!res.second) {
			std::cerr << "this cannot ben possible!" << std::endl;
		}
	}

	size_t NumberOfAllocations() const override
	{
		std::unique_lock<std::mutex> lock(*guard);
		return alloc_entry_table->size();
	}

	const std::thread::id& GetWorkingThread() const override
	{
		return tid;
	}

	void DumpCurrentAllocations(entry_set_t* g_alloc_entry_table,
															entry_set_t* g_dealloc_entry_table) override
	{
		std::unique_lock<std::mutex> lock(*guard);
		g_alloc_entry_table->insert(alloc_entry_table->begin(),
																alloc_entry_table->end());
		g_dealloc_entry_table->insert(dealloc_entry_table->begin(),
																	dealloc_entry_table->end());
	}
	
private:
	entry_set_t *alloc_entry_table;
	entry_set_t *dealloc_entry_table;
	std::atomic_bool* should_lock;
	std::mutex* guard;
	std::thread::id tid;
};

MemControllerAgent::MemControllerAgent() 
{
	auto thid = std::this_thread::get_id();
	malloc_create(&p_impl, thid);
}

MemControllerAgent::~MemControllerAgent() {
	// do not release main thread memory controller agent due to seg faults on static dtors.
	if (main_thdr_id != p_impl->GetWorkingThread())
	{
		free_new(p_impl);
	}
	//else
	//{
	//	std::cerr << "Upps I leaked [" << sizeof(MemControllerAgentImpl) << "] more bytes but it's okay!\n";
	//}
}

void MemControllerAgent::allocated(void* ptr, size_t n) noexcept
{
	p_impl->OnNewAllocation(ptr, n);
}

void MemControllerAgent::deallocated(void* ptr) noexcept
{
	p_impl->OnDelAllocation(ptr);
}
