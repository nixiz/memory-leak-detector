#pragma once

#include "MemControllerAgent.h"

class MemLeakControllerService
{
public: 
  ~MemLeakControllerService();
  static MemLeakControllerService* GetInstance();

  void RegisterNewAgent(MemControllerAgent* agent);
  void UnRegisterNewAgent(MemControllerAgent* agent);

private:
  MemLeakControllerService();
  class MemLeakControllerServiceImpl* impl;
};

