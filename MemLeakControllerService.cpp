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
    malloc_create(&table_guard);
    malloc_create(&is_active, true);
    malloc_create(&agents);
    malloc_create(&global_alloc_table);
    malloc_create(&global_dealloc_table);
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
    // 
    MergeTables(global_alloc_table, global_dealloc_table);
    if (global_alloc_table->size() ||
        global_dealloc_table->size())
    {
      std::cerr << "\n-----dumping mem leaks-----\n";
      PrintTables(global_alloc_table, global_dealloc_table);
    }
    else
    {
      std::cout << "you have no mem leaks! have a good day!" << std::endl;
    }

    free_new(global_alloc_table);
    free_new(global_dealloc_table);
    free_new(guard);
    free_new(table_guard);
    free_new(is_active);
  }

  inline void PrintTables(entry_set_t* alloc_entry_table,
                          entry_set_t* dealloc_entry_table)
  {
    if (alloc_entry_table->size())
    {
      std::cerr << "------ Possible Mem Leak! -------\n";
      //std::cerr << "thread[" << agent->GetWorkingThread() << "] destroyed without cleaned these allocations:\n";
      std::cerr << "agent destroyed without cleaned these allocations:\n";
      for (auto& enrty : *alloc_entry_table) {
        std::cerr << enrty << "\n";
      }
      std::cerr << std::endl;
    }

    if (dealloc_entry_table->size())
    {
      std::cerr << "------ Possible Mem Missmatch! -------\n";
      std::cerr << "agent released allocations which are not allocated by itself:\n";
      for (auto& enrty : *dealloc_entry_table) {
        std::cerr << enrty << "\n";
      }
      std::cerr << std::endl;
    }
  }

  inline void MergeTables(entry_set_t* alloc_entry_table,
                          entry_set_t* dealloc_entry_table)
  {
    // merge tables and erase deallocated memory entries from both lists
    //std::vector<entry_set_t::const_iterator, malloc_allocator_t<entry_set_t::const_iterator>> dealloc_entry_list;
    //dealloc_entry_list.reserve(dealloc_entry_table->size());
    for (auto it = dealloc_entry_table->begin();
         it != dealloc_entry_table->end(); /*++it*/)
    {
      auto erased = alloc_entry_table->erase(*it);
      if (erased) {
        it = dealloc_entry_table->erase(it);
      }
      else {
        it++;
      }
    }
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
    {
      std::lock_guard<std::mutex> lock(*guard);
      auto res = agents->find(agent);
      if (res != agents->end())
      {
        agents->erase(agent);
      }
    }

    entry_set_t* alloc_entry_table = malloc_new<entry_set_t>();
    entry_set_t* dealloc_entry_table = malloc_new<entry_set_t>();
    agent->DumpCurrentAllocations(alloc_entry_table, dealloc_entry_table);
    MergeTables(alloc_entry_table, dealloc_entry_table);
    PrintTables(alloc_entry_table, dealloc_entry_table);

    std::lock_guard<std::mutex> lock(*table_guard);
    global_alloc_table->insert(alloc_entry_table->begin(),
                                alloc_entry_table->end());
    global_dealloc_table->insert(dealloc_entry_table->begin(),
                                 dealloc_entry_table->end());
  }

  void ServiceThreadMain()
  {
    static unsigned long counter = 0;
    while (is_active->load())
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      {
        std::lock_guard<std::mutex> lock(*guard);
        std::lock_guard<std::mutex> tlock(*table_guard);
        for (const auto& agent : *agents)
        {
          agent->DumpCurrentAllocations(global_alloc_table, global_dealloc_table);
        }
        //if (++counter % 10 == 0)
        //{
        //  MergeTables(global_alloc_table, global_dealloc_table);
        //  PrintTables(global_alloc_table, global_dealloc_table);
        //}
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
  std::mutex *table_guard;
  std::atomic_bool *is_active;
  unordered_set_t *agents;
  entry_set_t* global_alloc_table;
  entry_set_t* global_dealloc_table;
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
