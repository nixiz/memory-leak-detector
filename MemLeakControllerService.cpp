#include "MemLeakControllerService.h"
#include "MemControllerAgent.h"
#include "malloc_allocator.hpp"
#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <set>
#include <unordered_set>
#include <boost/thread.hpp>

class MemLeakControllerServiceImpl
{
public:
  MemLeakControllerServiceImpl() 
  {
    malloc_create(&guard);
    malloc_create(&is_active, true);
    malloc_create(&agents);
    worker_thread = malloc_new<boost::thread>(boost::bind(&MemLeakControllerServiceImpl::ServiceThreadMain, this));
    //malloc_create(&worker_thread, 
    //              std::bind(&MemLeakControllerServiceImpl::ServiceThreadMain, this));
  }

  ~MemLeakControllerServiceImpl()
  {
    is_active->store(false);
    worker_thread->join();
    agents->clear();

    free_new(worker_thread);
    free_new(agents);
    free_new(guard);
    free_new(is_active);
  }

  void RegisterNewAgent(MemControllerAgent* agent)
  {
    std::lock_guard<std::mutex> lock(*guard);
    auto id = agent->GetWorkingThread();
    auto res = agents->insert(agent);
    if (!res.second)
    {
      throw;
    }
  }

  void UnRegisterNewAgent(MemControllerAgent* agent)
  {
    std::lock_guard<std::mutex> lock(*guard);
    auto res = agents->find(agent);
    if (res != agents->end())
    {
      agents->erase(agent);
    }

    if (agent->NumberOfAllocations())
    {
      std::cerr << "------ Mem Leak! -------\n\n";
      agent->DumpCurrentAllocations();
      std::cerr << "------ Mem Leak! [End]-------\n\n" << std::endl;
    }
  }

  void ServiceThreadMain()
  {
    while (is_active->load())
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      {
        std::lock_guard<std::mutex> lock(*guard);
        for (const auto& agent : *agents)
        {
          std::cout << "start  dump for thread[" << agent->GetWorkingThread() << "]\n";
          agent->DumpCurrentAllocations();
          std::cout << "stoped dump for thread[" << agent->GetWorkingThread() << "]\n";
        }
      }
    }
  }

private:
  using unordered_set_t = std::unordered_set<
    MemControllerAgent*,
    std::hash< MemControllerAgent*>,
    std::equal_to< MemControllerAgent*>,
    malloc_allocator_t< MemControllerAgent*>
  >;

  std::mutex *guard;
  std::atomic_bool *is_active;
  unordered_set_t *agents;
  boost::thread *worker_thread;
};

MemLeakControllerService* MemLeakControllerService::GetInstance()
{
  static MemLeakControllerService instance;
  return &instance;
}

MemLeakControllerService::MemLeakControllerService()
{
  malloc_create(&impl);
  //impl = new MemLeakControllerServiceImpl;
}

MemLeakControllerService::~MemLeakControllerService()
{
  free_new(impl);
}

void MemLeakControllerService::RegisterNewAgent(MemControllerAgent* agent)
{
  impl->RegisterNewAgent(agent);
}

void MemLeakControllerService::UnRegisterNewAgent(MemControllerAgent* agent)
{
  impl->UnRegisterNewAgent(agent);
}