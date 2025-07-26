네, `boost::interprocess::shared_ptr`는 `template<class T, class VoidAllocator, class Deleter>` 형식을 가집니다. 이 형식에 맞춰 `typedef`를 정의해달라고 하셨으니, 세 번째 인자인 `Deleter`를 포함하여 `boost::interprocess::shared_ptr`를 정의하는 방법을 알려드릴게요.

-----

### `Deleter`를 포함한 `shared_ptr` `typedef` 정의

기본적으로 `boost::interprocess::shared_ptr`는 두 번째 템플릿 인자로 제공된 **`VoidAllocator`를 사용하여 객체를 소멸**시킵니다. 즉, 별도의 `Deleter`를 명시적으로 지정하지 않으면 `VoidAllocator`가 `T` 타입 객체의 할당 해제 및 소멸을 담당합니다.

만약 특별한 소멸 로직이 필요 없다면, 앞서 사용했던 두 인자 형식(`template<class T, class VoidAllocator>`)으로 충분합니다. 왜냐하면 `VoidAllocator`가 내부적으로 기본 소멸자 역할을 겸하기 때문입니다.

그러나 **사용자 정의 소멸자(custom deleter)를 제공해야 하는 특별한 경우**에는 세 번째 인자인 `Deleter`를 명시적으로 지정할 수 있습니다.

예시를 통해 설명해 드릴게요.

```cpp
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp> // Boost 1.80.0 이상
#include <iostream>
#include <string>

// 공유 메모리에 저장할 데이터 구조체
struct MyData {
    int value;
    char message[100];

    MyData(int v, const char* msg) : value(v) {
        strncpy(message, msg, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
        std::cout << "MyData constructor called. Value: " << value << std::endl;
    }

    ~MyData() {
        std::cout << "MyData destructor called. Value: " << value << std::endl;
    }
};

// 사용자 정의 소멸자 (Deleter) 예시
// 이 소멸자는 boost::interprocess::shared_ptr에 의해 호출됩니다.
template<class T, class VoidAllocator>
struct MyCustomDeleter {
    VoidAllocator allocator_; // 할당자를 저장하여 소멸 시 사용

    MyCustomDeleter(const VoidAllocator& allocator) : allocator_(allocator) {}

    // delete 연산자 오버로드. shared_ptr가 객체를 소멸할 때 호출됩니다.
    void operator()(T* p) {
        if (p) {
            std::cout << "MyCustomDeleter called for MyData with value: " << p->value << std::endl;
            // 할당자를 사용하여 객체 소멸 및 메모리 해제
            p->~T(); // 객체의 소멸자 호출
            allocator_.deallocate((void*)p, 1); // 메모리 해제
        }
    }
};

void demonstrate_shared_ptr_with_deleter() {
    // 공유 메모리 세그먼트 이름과 크기 설정
    const char* segment_name = "MySharedMemorySegmentWithDeleter";
    const size_t segment_size = 65536;

    // 공유 메모리 객체 제거 (이전 실행 흔적 제거)
    struct shm_remove {
        shm_remove() { boost::interprocess::shared_memory_object::remove(segment_name); }
        ~shm_remove() { boost::interprocess::shared_memory_object::remove(segment_name); }
    } remover;

    try {
        // 1. 관리되는 공유 메모리 세그먼트 생성
        boost::interprocess::managed_shared_memory segment(
            boost::interprocess::create_only,
            segment_name,
            segment_size
        );

        // 2. MyData에 대한 공유 메모리 할당자 타입 정의
        // boost::interprocess::managed_shared_memory::allocator는 타입 인자로 일반적인 T를 받습니다.
        typedef boost::interprocess::managed_shared_memory::allocator<MyData>::type MyDataShmemAllocator;

        // 3. VoidAllocator 타입 정의 (Deleter의 템플릿 인자로 필요)
        // boost::interprocess::managed_shared_memory::void_allocator는 void*를 할당할 수 있는 할당자입니다.
        // 이것은 shared_ptr가 Deleter에 전달하는 할당자의 일반적인 형태입니다.
        typedef boost::interprocess::managed_shared_memory::void_allocator<boost::interprocess::managed_shared_memory>::type VoidShmemAllocator;


        // 4. Deleter 타입 정의
        typedef MyCustomDeleter<MyData, VoidShmemAllocator> MyDeleter;


        // 5. boost::interprocess::shared_ptr 타입 정의 (세 가지 템플릿 인자 사용)
        typedef boost::interprocess::shared_ptr<MyData, VoidShmemAllocator, MyDeleter> MySharedDataPtrWithDeleter;


        // 이제 실제로 shared_ptr를 생성하고 사용합니다.

        // MyData 객체를 할당할 할당자 인스턴스
        MyDataShmemAllocator my_data_alloc(segment.get_segment_manager());

        // VoidAllocator 인스턴스 (Deleter에 전달할 용도)
        VoidShmemAllocator void_alloc(segment.get_segment_manager());

        // MyData 객체 생성 (placement new 사용)
        MyData* raw_ptr = my_data_alloc.allocate(1); // MyData를 위한 메모리 할당
        new (raw_ptr) MyData(77, "Data with custom deleter!"); // 객체 생성자 호출

        // MySharedDataPtrWithDeleter 생성
        // shared_ptr의 생성자에 raw_ptr, void_allocator, custom_deleter를 전달합니다.
        MySharedDataPtrWithDeleter* ptr_in_shm = segment.construct<MySharedDataPtrWithDeleter>("NamedSharedDataPtr")(
            raw_ptr, void_alloc, MyDeleter(void_alloc));

        std::cout << "Writer: Shared data constructed. Value: " << (*ptr_in_shm)->value
                  << ", Message: " << (*ptr_in_shm)->message << std::endl;

        std::cout << "Writer: Shared pointer reference count: " << ptr_in_shm->use_count() << std::endl;

        // 잠시 대기하여 다른 프로세스가 접근할 기회를 줍니다 (실제 시나리오에서는 IPC 동기화 필요)
        std::cout << "Writer: Pausing for 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // *ptr_in_shm의 스코프를 벗어나거나 reset()을 호출하면 Deleter가 호출됩니다.
        // 여기서는 main 함수 종료 시 segment가 소멸되면 자동으로 해제됩니다.
        // 또는 명시적으로 segment->destroy<MySharedDataPtrWithDeleter>("NamedSharedDataPtr")를 호출할 수도 있습니다.
        std::cout << "Writer: Exiting scope, shared_ptr will be destroyed." << std::endl;

    } catch (const boost::interprocess::interprocess_exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
}

#include <chrono>
#include <thread>
int main() {
    demonstrate_shared_ptr_with_deleter();
    return 0;
}
```

-----

### 코드 설명 및 중요 사항

1.  **`MyData` 구조체:** `MyData` 구조체는 생성자와 소멸자를 명시적으로 정의하여 객체 생성 및 소멸 시점을 확인할 수 있게 했습니다.

2.  **`MyCustomDeleter`:**

      * `template<class T, class VoidAllocator>`로 정의되어 `shared_ptr`가 관리할 타입 `T`와 내부적으로 사용할 할당자 `VoidAllocator`를 받습니다.
      * 생성자에서 `VoidAllocator` 인스턴스를 저장합니다. 이는 객체를 소멸할 때 올바른 할당자를 사용하여 메모리를 해제하기 위함입니다.
      * **`void operator()(T* p)`:** 이 함수가 `boost::interprocess::shared_ptr`가 가리키던 객체의 소멸자로 호출됩니다.
          * `p->~T();`: `T` 타입 객체의 소멸자를 명시적으로 호출합니다.
          * `allocator_.deallocate((void*)p, 1);`: 저장된 `VoidAllocator`를 사용하여 `p`가 가리키는 메모리를 해제합니다. `void*`로 캐스팅하는 것은 `deallocate`가 `void*`를 받기 때문입니다.

3.  **`VoidShmemAllocator`:**

      * `typedef boost::interprocess::managed_shared_memory::void_allocator<boost::interprocess::managed_shared_memory>::type VoidShmemAllocator;`
      * 이 타입은 `boost::interprocess`에서 범용적인 `void*` 포인터를 다루기 위해 제공되는 특수한 할당자입니다. `Deleter`가 `void*`를 해제해야 할 때 이 할당자 타입을 사용합니다.
      * `MyCustomDeleter`의 템플릿 인자로 전달됩니다.

4.  **`MyDeleter` `typedef`:**

      * `typedef MyCustomDeleter<MyData, VoidShmemAllocator> MyDeleter;`
      * `MyData` 타입과 `VoidShmemAllocator`를 사용하는 `MyCustomDeleter`의 특정 인스턴스를 `MyDeleter`로 별칭을 지정합니다.

5.  **`MySharedDataPtrWithDeleter` `typedef`:**

      * `typedef boost::interprocess::shared_ptr<MyData, VoidShmemAllocator, MyDeleter> MySharedDataPtrWithDeleter;`
      * 이제 `shared_ptr`에 `MyData` (데이터 타입), `VoidShmemAllocator` (내부 할당자), `MyDeleter` (사용자 정의 소멸자) 세 가지 템플릿 인자를 모두 명시하여 정의했습니다.

6.  **`shared_ptr` 생성 시 주의사항:**

      * `segment.construct<MySharedDataPtrWithDeleter>("NamedSharedDataPtr")(...)` 부분을 보세요. `shared_ptr`의 생성자에 `(raw_ptr, void_alloc, MyDeleter(void_alloc))` 이렇게 세 가지 인자를 전달하고 있습니다.
          * `raw_ptr`: `new (my_data_alloc) MyData(...)`를 통해 할당된 원시 포인터입니다.
          * `void_alloc`: `shared_ptr`가 내부적으로 참조 카운팅 등을 관리하는 데 사용할 할당자입니다.
          * `MyDeleter(void_alloc)`: `shared_ptr`가 소멸 시 호출할 사용자 정의 소멸자 객체입니다. 소멸자 객체도 `VoidAllocator` 인스턴스를 인자로 받아서 생성합니다.

-----

### 언제 사용자 정의 `Deleter`가 필요한가요?

대부분의 경우, `boost::interprocess::shared_ptr<T, ShmemAllocator>` 형식만으로도 충분합니다. 이 경우 `ShmemAllocator`가 내부적으로 `T` 객체의 소멸과 메모리 해제를 담당합니다.

하지만 다음과 같은 상황에서는 사용자 정의 `Deleter`가 유용합니다:

  * **복잡한 정리 작업:** 객체가 소멸될 때 단순한 `delete` 이상의 복잡한 정리 작업(예: 파일 핸들 닫기, 네트워크 연결 해제, 다른 공유 메모리 자원 해제)이 필요한 경우.
  * **리소스 풀 관리:** 객체가 풀에서 할당되었고, `shared_ptr`가 소멸될 때 객체를 풀로 반환해야 하는 경우.
  * **디버깅/로깅:** 객체가 소멸될 때 특정 로깅 메시지를 남기고자 하는 경우 (위 예시처럼).

이 예시를 통해 `boost::interprocess::shared_ptr`의 세 번째 템플릿 인자인 `Deleter`를 어떻게 활용할 수 있는지 이해하시는 데 도움이 되었으면 합니다.