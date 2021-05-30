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

static const std::thread::id main_thdr_id = std::this_thread::get_id();

class MemLeakControllerServiceImpl
{
public:
  MemLeakControllerServiceImpl() 
    : main_thread_agent{nullptr}
  {
    malloc_create(&guard);
    malloc_create(&is_active, true);
    malloc_create(&agents);
    malloc_create(&global_entry_table);
    worker_thread = malloc_new<boost::thread>(boost::bind(&MemLeakControllerServiceImpl::ServiceThreadMain, this));
    //malloc_create(&worker_thread, 
    //              std::bind(&MemLeakControllerServiceImpl::ServiceThreadMain, this));
  }

  ~MemLeakControllerServiceImpl()
  {
    is_active->store(false);
    worker_thread->join();
    agents->clear();

    if (main_thread_agent)
    {
      free_new(main_thread_agent);
    }

    free_new(worker_thread);
    free_new(agents);
    free_new(global_entry_table);
    free_new(guard);
    free_new(is_active);
  }

  void RegisterNewAgent(MemLeakControllerService::Client* agent)
  {
    std::lock_guard<std::mutex> lock(*guard);
    // get main thread agent and store for delete later.
    if (!main_thread_agent && 
        (agent->GetWorkingThread() == main_thdr_id)) {
      main_thread_agent = agent;
    }
    auto id = agent->GetWorkingThread();
    auto res = agents->insert(agent);
    if (!res.second)
    {
      throw;
    }
  }

  void UnRegisterNewAgent(MemLeakControllerService::Client* agent)
  {
    std::lock_guard<std::mutex> lock(*guard);
    auto res = agents->find(agent);
    if (res != agents->end())
    {
      agents->erase(agent);
    }

    if (agent->NumberOfAllocations())
    {
      entry_set_t* entry_table = malloc_new<entry_set_t>();
      agent->DumpCurrentAllocations(entry_table);

      std::cerr << "------ Possible Mem Leak! -------\n";
      std::cerr << "thread[" << agent->GetWorkingThread() << "] destroyed without cleaned these allocations:\n";
      for (auto& enrty : *entry_table) {
        std::cerr << enrty << "\n";
      }
      std::cerr << std::endl;
    }
  }

  void ServiceThreadMain()
  {
    static unsigned long counter = 0;
    while (is_active->load())
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      {
        std::lock_guard<std::mutex> lock(*guard);
        for (const auto& agent : *agents)
        {
          agent->DumpCurrentAllocations(global_entry_table);
        }
      }
      // ----- dont dump every xx seconds.. ----
      if (false && ++counter % 10 == 0)
      {
        for (auto& enrty : *global_entry_table) {
          std::cout << enrty << std::endl;
        }
      }
    }
  }

private:
  using unordered_set_t = std::unordered_set<
    MemLeakControllerService::Client*,
    std::hash<MemLeakControllerService::Client*>,
    std::equal_to<MemLeakControllerService::Client*>,
    malloc_allocator_t<MemLeakControllerService::Client>
  >;

  std::mutex *guard;
  std::atomic_bool *is_active;
  unordered_set_t *agents;
  entry_set_t* global_entry_table;
  boost::thread *worker_thread;
  MemLeakControllerService::Client* main_thread_agent;
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

void MemLeakControllerService::RegisterNewAgent(MemLeakControllerService::Client* agent)
{
  impl->RegisterNewAgent(agent);
}

void MemLeakControllerService::UnRegisterNewAgent(MemLeakControllerService::Client* agent)
{
  impl->UnRegisterNewAgent(agent);
}