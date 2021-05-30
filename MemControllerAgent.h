#pragma once

#include <memory>
//#include <thread>

class MemControllerAgent
{
public:
	MemControllerAgent();
	~MemControllerAgent();

	void* allocate(size_t n) noexcept;
	void  deallocate(void* ptr) noexcept;

	size_t NumberOfAllocations() const;

protected:
	unsigned int GetWorkingThread() const;
	void DumpCurrentAllocations() const;
	
private:
	friend class MemLeakControllerServiceImpl;
	class MemControllerAgentImpl* p_impl;
	unsigned int working_thread;
};

