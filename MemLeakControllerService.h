#pragma once

#include "MemControllerAgent.h"
#include "malloc_allocator.hpp"
#include "MemoryEntry.h"
#include <thread>
#include <unordered_set>

class MemLeakControllerService
{
public:
  class Client
  {
  public:
    virtual ~Client() = default;
    virtual size_t NumberOfAllocations() const = 0;
    virtual const std::thread::id& GetWorkingThread() const = 0;
    virtual void DumpCurrentAllocations(entry_set_t *global_entry_table) = 0;
  };

public: 
  ~MemLeakControllerService();
  static MemLeakControllerService* GetInstance();

  void RegisterNewAgent(Client* agent);
  void UnRegisterNewAgent(Client* agent);

private:
  MemLeakControllerService();
  class MemLeakControllerServiceImpl* impl;
};

