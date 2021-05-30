#pragma once

#include <memory>
//#include <thread>

class MemControllerAgent
{
public:
	MemControllerAgent();
	~MemControllerAgent();

	void allocated(void* ptr, size_t n) noexcept;
	void deallocated(void* ptr) noexcept;

private:
	friend class MemLeakControllerServiceImpl;
	class MemControllerAgentImpl* p_impl;
	//unsigned int working_thread;
};

