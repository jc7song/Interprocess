
#pragma once

const char* ShmName = "MySharedMemory";
const char* MutexName = "MySharedMutex";
const char* SharedPtrName = "MySharedIntPtr";

#include <iostream>

class MyType {
public:
  int data_{100};
  MyType(int data) {
    data_ = data;
  }
  virtual ~MyType() {
    std::cout << data_ << "Destructed" << std::endl;
  }
};
