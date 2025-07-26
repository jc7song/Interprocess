��, `boost::interprocess::shared_ptr`�� `template<class T, class VoidAllocator, class Deleter>` ������ �����ϴ�. �� ���Ŀ� ���� `typedef`�� �����ش޶�� �ϼ�����, �� ��° ������ `Deleter`�� �����Ͽ� `boost::interprocess::shared_ptr`�� �����ϴ� ����� �˷��帱�Կ�.

-----

### `Deleter`�� ������ `shared_ptr` `typedef` ����

�⺻������ `boost::interprocess::shared_ptr`�� �� ��° ���ø� ���ڷ� ������ **`VoidAllocator`�� ����Ͽ� ��ü�� �Ҹ�**��ŵ�ϴ�. ��, ������ `Deleter`�� ��������� �������� ������ `VoidAllocator`�� `T` Ÿ�� ��ü�� �Ҵ� ���� �� �Ҹ��� ����մϴ�.

���� Ư���� �Ҹ� ������ �ʿ� ���ٸ�, �ռ� ����ߴ� �� ���� ����(`template<class T, class VoidAllocator>`)���� ����մϴ�. �ֳ��ϸ� `VoidAllocator`�� ���������� �⺻ �Ҹ��� ������ ���ϱ� �����Դϴ�.

�׷��� **����� ���� �Ҹ���(custom deleter)�� �����ؾ� �ϴ� Ư���� ���**���� �� ��° ������ `Deleter`�� ��������� ������ �� �ֽ��ϴ�.

���ø� ���� ������ �帱�Կ�.

```cpp
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp> // Boost 1.80.0 �̻�
#include <iostream>
#include <string>

// ���� �޸𸮿� ������ ������ ����ü
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

// ����� ���� �Ҹ��� (Deleter) ����
// �� �Ҹ��ڴ� boost::interprocess::shared_ptr�� ���� ȣ��˴ϴ�.
template<class T, class VoidAllocator>
struct MyCustomDeleter {
    VoidAllocator allocator_; // �Ҵ��ڸ� �����Ͽ� �Ҹ� �� ���

    MyCustomDeleter(const VoidAllocator& allocator) : allocator_(allocator) {}

    // delete ������ �����ε�. shared_ptr�� ��ü�� �Ҹ��� �� ȣ��˴ϴ�.
    void operator()(T* p) {
        if (p) {
            std::cout << "MyCustomDeleter called for MyData with value: " << p->value << std::endl;
            // �Ҵ��ڸ� ����Ͽ� ��ü �Ҹ� �� �޸� ����
            p->~T(); // ��ü�� �Ҹ��� ȣ��
            allocator_.deallocate((void*)p, 1); // �޸� ����
        }
    }
};

void demonstrate_shared_ptr_with_deleter() {
    // ���� �޸� ���׸�Ʈ �̸��� ũ�� ����
    const char* segment_name = "MySharedMemorySegmentWithDeleter";
    const size_t segment_size = 65536;

    // ���� �޸� ��ü ���� (���� ���� ���� ����)
    struct shm_remove {
        shm_remove() { boost::interprocess::shared_memory_object::remove(segment_name); }
        ~shm_remove() { boost::interprocess::shared_memory_object::remove(segment_name); }
    } remover;

    try {
        // 1. �����Ǵ� ���� �޸� ���׸�Ʈ ����
        boost::interprocess::managed_shared_memory segment(
            boost::interprocess::create_only,
            segment_name,
            segment_size
        );

        // 2. MyData�� ���� ���� �޸� �Ҵ��� Ÿ�� ����
        // boost::interprocess::managed_shared_memory::allocator�� Ÿ�� ���ڷ� �Ϲ����� T�� �޽��ϴ�.
        typedef boost::interprocess::managed_shared_memory::allocator<MyData>::type MyDataShmemAllocator;

        // 3. VoidAllocator Ÿ�� ���� (Deleter�� ���ø� ���ڷ� �ʿ�)
        // boost::interprocess::managed_shared_memory::void_allocator�� void*�� �Ҵ��� �� �ִ� �Ҵ����Դϴ�.
        // �̰��� shared_ptr�� Deleter�� �����ϴ� �Ҵ����� �Ϲ����� �����Դϴ�.
        typedef boost::interprocess::managed_shared_memory::void_allocator<boost::interprocess::managed_shared_memory>::type VoidShmemAllocator;


        // 4. Deleter Ÿ�� ����
        typedef MyCustomDeleter<MyData, VoidShmemAllocator> MyDeleter;


        // 5. boost::interprocess::shared_ptr Ÿ�� ���� (�� ���� ���ø� ���� ���)
        typedef boost::interprocess::shared_ptr<MyData, VoidShmemAllocator, MyDeleter> MySharedDataPtrWithDeleter;


        // ���� ������ shared_ptr�� �����ϰ� ����մϴ�.

        // MyData ��ü�� �Ҵ��� �Ҵ��� �ν��Ͻ�
        MyDataShmemAllocator my_data_alloc(segment.get_segment_manager());

        // VoidAllocator �ν��Ͻ� (Deleter�� ������ �뵵)
        VoidShmemAllocator void_alloc(segment.get_segment_manager());

        // MyData ��ü ���� (placement new ���)
        MyData* raw_ptr = my_data_alloc.allocate(1); // MyData�� ���� �޸� �Ҵ�
        new (raw_ptr) MyData(77, "Data with custom deleter!"); // ��ü ������ ȣ��

        // MySharedDataPtrWithDeleter ����
        // shared_ptr�� �����ڿ� raw_ptr, void_allocator, custom_deleter�� �����մϴ�.
        MySharedDataPtrWithDeleter* ptr_in_shm = segment.construct<MySharedDataPtrWithDeleter>("NamedSharedDataPtr")(
            raw_ptr, void_alloc, MyDeleter(void_alloc));

        std::cout << "Writer: Shared data constructed. Value: " << (*ptr_in_shm)->value
                  << ", Message: " << (*ptr_in_shm)->message << std::endl;

        std::cout << "Writer: Shared pointer reference count: " << ptr_in_shm->use_count() << std::endl;

        // ��� ����Ͽ� �ٸ� ���μ����� ������ ��ȸ�� �ݴϴ� (���� �ó����������� IPC ����ȭ �ʿ�)
        std::cout << "Writer: Pausing for 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // *ptr_in_shm�� �������� ����ų� reset()�� ȣ���ϸ� Deleter�� ȣ��˴ϴ�.
        // ���⼭�� main �Լ� ���� �� segment�� �Ҹ�Ǹ� �ڵ����� �����˴ϴ�.
        // �Ǵ� ��������� segment->destroy<MySharedDataPtrWithDeleter>("NamedSharedDataPtr")�� ȣ���� ���� �ֽ��ϴ�.
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

### �ڵ� ���� �� �߿� ����

1.  **`MyData` ����ü:** `MyData` ����ü�� �����ڿ� �Ҹ��ڸ� ��������� �����Ͽ� ��ü ���� �� �Ҹ� ������ Ȯ���� �� �ְ� �߽��ϴ�.

2.  **`MyCustomDeleter`:**

      * `template<class T, class VoidAllocator>`�� ���ǵǾ� `shared_ptr`�� ������ Ÿ�� `T`�� ���������� ����� �Ҵ��� `VoidAllocator`�� �޽��ϴ�.
      * �����ڿ��� `VoidAllocator` �ν��Ͻ��� �����մϴ�. �̴� ��ü�� �Ҹ��� �� �ùٸ� �Ҵ��ڸ� ����Ͽ� �޸𸮸� �����ϱ� �����Դϴ�.
      * **`void operator()(T* p)`:** �� �Լ��� `boost::interprocess::shared_ptr`�� ����Ű�� ��ü�� �Ҹ��ڷ� ȣ��˴ϴ�.
          * `p->~T();`: `T` Ÿ�� ��ü�� �Ҹ��ڸ� ��������� ȣ���մϴ�.
          * `allocator_.deallocate((void*)p, 1);`: ����� `VoidAllocator`�� ����Ͽ� `p`�� ����Ű�� �޸𸮸� �����մϴ�. `void*`�� ĳ�����ϴ� ���� `deallocate`�� `void*`�� �ޱ� �����Դϴ�.

3.  **`VoidShmemAllocator`:**

      * `typedef boost::interprocess::managed_shared_memory::void_allocator<boost::interprocess::managed_shared_memory>::type VoidShmemAllocator;`
      * �� Ÿ���� `boost::interprocess`���� �������� `void*` �����͸� �ٷ�� ���� �����Ǵ� Ư���� �Ҵ����Դϴ�. `Deleter`�� `void*`�� �����ؾ� �� �� �� �Ҵ��� Ÿ���� ����մϴ�.
      * `MyCustomDeleter`�� ���ø� ���ڷ� ���޵˴ϴ�.

4.  **`MyDeleter` `typedef`:**

      * `typedef MyCustomDeleter<MyData, VoidShmemAllocator> MyDeleter;`
      * `MyData` Ÿ�԰� `VoidShmemAllocator`�� ����ϴ� `MyCustomDeleter`�� Ư�� �ν��Ͻ��� `MyDeleter`�� ��Ī�� �����մϴ�.

5.  **`MySharedDataPtrWithDeleter` `typedef`:**

      * `typedef boost::interprocess::shared_ptr<MyData, VoidShmemAllocator, MyDeleter> MySharedDataPtrWithDeleter;`
      * ���� `shared_ptr`�� `MyData` (������ Ÿ��), `VoidShmemAllocator` (���� �Ҵ���), `MyDeleter` (����� ���� �Ҹ���) �� ���� ���ø� ���ڸ� ��� ����Ͽ� �����߽��ϴ�.

6.  **`shared_ptr` ���� �� ���ǻ���:**

      * `segment.construct<MySharedDataPtrWithDeleter>("NamedSharedDataPtr")(...)` �κ��� ������. `shared_ptr`�� �����ڿ� `(raw_ptr, void_alloc, MyDeleter(void_alloc))` �̷��� �� ���� ���ڸ� �����ϰ� �ֽ��ϴ�.
          * `raw_ptr`: `new (my_data_alloc) MyData(...)`�� ���� �Ҵ�� ���� �������Դϴ�.
          * `void_alloc`: `shared_ptr`�� ���������� ���� ī���� ���� �����ϴ� �� ����� �Ҵ����Դϴ�.
          * `MyDeleter(void_alloc)`: `shared_ptr`�� �Ҹ� �� ȣ���� ����� ���� �Ҹ��� ��ü�Դϴ�. �Ҹ��� ��ü�� `VoidAllocator` �ν��Ͻ��� ���ڷ� �޾Ƽ� �����մϴ�.

-----

### ���� ����� ���� `Deleter`�� �ʿ��Ѱ���?

��κ��� ���, `boost::interprocess::shared_ptr<T, ShmemAllocator>` ���ĸ����ε� ����մϴ�. �� ��� `ShmemAllocator`�� ���������� `T` ��ü�� �Ҹ�� �޸� ������ ����մϴ�.

������ ������ ���� ��Ȳ������ ����� ���� `Deleter`�� �����մϴ�:

  * **������ ���� �۾�:** ��ü�� �Ҹ�� �� �ܼ��� `delete` �̻��� ������ ���� �۾�(��: ���� �ڵ� �ݱ�, ��Ʈ��ũ ���� ����, �ٸ� ���� �޸� �ڿ� ����)�� �ʿ��� ���.
  * **���ҽ� Ǯ ����:** ��ü�� Ǯ���� �Ҵ�Ǿ���, `shared_ptr`�� �Ҹ�� �� ��ü�� Ǯ�� ��ȯ�ؾ� �ϴ� ���.
  * **�����/�α�:** ��ü�� �Ҹ�� �� Ư�� �α� �޽����� ������� �ϴ� ��� (�� ����ó��).

�� ���ø� ���� `boost::interprocess::shared_ptr`�� �� ��° ���ø� ������ `Deleter`�� ��� Ȱ���� �� �ִ��� �����Ͻô� �� ������ �Ǿ����� �մϴ�.