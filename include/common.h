
#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp> // 올바른 헤더 경로
#include <boost/interprocess/sync/named_mutex.hpp>

const char* SharedMemName = "SharedMemName";
const char* SharedPtrName = "SharedPtrName";

#include <iostream>

class MyType {
public:
  int data_{100};
  MyType(int data) {
    data_ = data;
    std::cout << data_ << " MyData 생성됨" << std::endl;
  }
  virtual ~MyType() {
    std::cout << data_ << " MyData 소멸됨" << std::endl;
  }
};

namespace bi = boost::interprocess;
typedef bi::managed_shared_memory::segment_manager segment_manager_type;
typedef bi::allocator<void, segment_manager_type> void_allocator_type;
typedef bi::allocator<MyType, segment_manager_type> MyAllocator;
typedef bi::deleter<MyType, segment_manager_type> MyDeleter;
typedef bi::shared_ptr<MyType, MyAllocator, MyDeleter> MySharedPtr;
