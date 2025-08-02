

#include <common.h>


#if 0

//#include <boost/interprocess/managed_shared_memory.hpp>
//#include <boost/interprocess/smart_ptr/shared_ptr.hpp>
//#include <boost/interprocess/smart_ptr/deleter.hpp>
//#include <boost/interprocess/allocators/allocator.hpp>
#include <iostream>

int main() {
    // 공유 메모리 생성
    bi::shared_memory_object::remove(SharedMemName);
    bi::managed_shared_memory segment(bi::create_only, SharedMemName, 65536);

    // Allocator와 deleter 설정
    //typedef bi::allocator<MyType, bi::managed_shared_memory::segment_manager> MyAllocator;
    //typedef bi::deleter<MyType, bi::managed_shared_memory::segment_manager> MyDeleter;

    //MyAllocator alloc_inst(segment.get_segment_manager());

    // shared_ptr 생성
    bi::shared_ptr<MyType, MyAllocator, MyDeleter> MySharedPtr(
        segment.construct<MyType>("MyTypeInstance")(42)
        , MyAllocator( segment.get_segment_manager() )
        , MyDeleter( segment.get_segment_manager() )
    );

    // 포인터 저장 (이름 등록)
    segment.construct<bi::shared_ptr<MyType, MyAllocator, MyDeleter>>(SharedPtrName)(MySharedPtr);

    use_count(&segment);

    segment.destroy<MyType>("MyTypeInstance");
    bi::shared_memory_object::remove(SharedMemName);
}

#else


void write();
int main()
{
    write();
}

#include <iostream>
#include <string>
#include <memory>

void write() {
    bi::shared_memory_object::remove(SharedMemName);

    try {
        bi::managed_shared_memory segment(bi::create_only, SharedMemName, 65536);
        std::cout << "공유 메모리 세그먼트 '" << SharedMemName << "' 생성됨." << std::endl;

        MyType* my_data_ptr = segment.construct<MyType>("MyType")(300);

        #if 0
        auto custom_deleter = [&](int* p) {
            std::cout << "Deleter가 호출되어 공유 메모리에서 int 객체를 파괴합니다." << std::endl;
            segment.destroy_ptr(p); // segment_manager를 통해 객체를 파괴합니다.
        };
        #endif

        MySharedPtr* my_shared_ptr = 
            segment.construct<MySharedPtr>(SharedPtrName)(
                my_data_ptr
                , MyAllocator( segment.get_segment_manager() )
                , MyDeleter( segment.get_segment_manager() )
            );

        std::cout << "Use count : " << my_shared_ptr->use_count() << std::endl;

        // 사용자 실행 후
        std::cout << "Use count : " << my_shared_ptr->use_count() << std::endl;

        my_shared_ptr->reset();

        std::cout << "Use count : " << my_shared_ptr->use_count() << std::endl;

        std::cout << "생산자 애플리케이션 종료 중..." << std::endl;
    } catch (const bi::interprocess_exception& ex) {
        std::cerr << "오류 발생: " << ex.what() << std::endl;
        std::cerr << "이전 실행에서 공유 메모리 또는 뮤텍스가 제대로 정리되지 않았을 수 있습니다. 수동으로 제거하거나 재시도하십시오." << std::endl;
    }

    // 공유 메모리 세그먼트와 뮤텍스 제거 (생산자가 종료될 때 정리)
    // 일반적으로 공유 메모리 세그먼트를 생성한 프로세스가 이를 제거합니다.
    bi::shared_memory_object::remove(SharedMemName);
    std::cout << "공유 메모리 세그먼트와 뮤텍스 제거됨." << std::endl;
}

#endif



#if 0
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp> // 올바른 헤더 경로
#include <boost/interprocess/sync/named_mutex.hpp>
#include <iostream>
#include <string>
#include <memory> // std::unique_ptr (선택 사항, 로컬 사용용)

// Boost 1.80 이상 버전에서 테스트되었으며, Visual Studio 2022와 호환됩니다.

// 공유 메모리 세그먼트 및 뮤텍스의 이름 정의
const char* ShmName = "MySharedMemory";
const char* MutexName = "MySharedMutex";
const char* SharedPtrName = "MySharedIntPtr";

// 공유 메모리 내에서 int 타입을 위한 할당자 정의
// 이 할당자는 managed_shared_memory의 세그먼트 관리자를 사용합니다.
typedef boost::interprocess::allocator<int, boost::interprocess::managed_shared_memory::segment_manager> ShmIntAllocator;

// 공유 메모리에서 사용할 boost::interprocess::shared_ptr 타입 정의
// 두 번째 템플릿 인자로 위에서 정의한 할당자(ShmIntAllocator)를 전달합니다.
// 이 Allocator 타입이 메모리 할당과 해제(Deallocation)를 모두 담당합니다.
// boost::interprocess::shared_ptr는 std::shared_ptr와 달리 별도의 Deallocator 템플릿 인자를 직접 받지 않습니다.
typedef boost::interprocess::shared_ptr<int, ShmIntAllocator> ShmIntSharedPtr;

int main() {
    std::cout << "생산자 애플리케이션 시작..." << std::endl;

    // 이전 실행에서 남은 공유 메모리 세그먼트와 뮤텍스를 제거합니다.
    // 이는 새로운 시작을 보장하기 위함입니다.
    boost::interprocess::shared_memory_object::remove(ShmName);
    boost::interprocess::named_mutex::remove(MutexName);

    try {
        // 공유 메모리 세그먼트 생성 (크기 65536 바이트)
        // managed_shared_memory는 공유 메모리 내에서 객체 생성을 관리합니다.
        boost::interprocess::managed_shared_memory segment(boost::interprocess::create_only, ShmName, 65536);
        std::cout << "공유 메모리 세그먼트 '" << ShmName << "' 생성됨." << std::endl;

        // 공유 메모리 내에 int 값을 생성합니다.
        // 이 int는 shared_ptr의 대상이 됩니다.
        // segment.construct<T>(name)(args...)는 이름으로 객체를 생성하고 초기화합니다.
        int* shared_int_data = segment.construct<int>("SharedIntData")(123);
        std::cout << "공유 메모리에 int 값 " << *shared_int_data << " 생성됨." << std::endl;

        // boost::interprocess::shared_ptr를 위한 사용자 정의 Deleter 정의
        // 이 Deleter는 shared_ptr의 참조 카운트가 0이 될 때 호출됩니다.
        // segment.destroy()를 사용하여 공유 메모리에서 객체를 명시적으로 파괴합니다.
        // 람다 함수는 segment_manager를 캡처하여 객체를 파괴할 때 사용합니다.
        auto custom_deleter = [&](int* p) {
            std::cout << "Deleter가 호출되어 공유 메모리에서 int 객체를 파괴합니다." << std::endl;
            segment.destroy_ptr(p); // segment_manager를 통해 객체를 파괴합니다.
        };

        // ShmIntSharedPtr를 사용하여 공유 메모리 내의 int를 가리킵니다.
        // 이 shared_ptr 자체도 공유 메모리 내에 생성됩니다.
        //
        // shm_int_ptr는 공유 메모리 내에 생성된 shared_ptr 객체 자체를 가리키는 포인터입니다.
        // 이 shared_ptr는 shared_int_data가 가리키는 공유 메모리 내의 int를 관리합니다.
        // 세 번째 인자로 사용자 정의 Deleter를 전달합니다.
        // 네 번째 인자 segment.get_segment_manager()는 shared_ptr가 참조 카운터와
        // 내부 메타데이터를 관리하기 위해 공유 메모리 세그먼트 관리자를 사용하도록 지시합니다.
        ShmIntSharedPtr* shm_int_ptr = 
            segment.construct<ShmIntSharedPtr>(SharedPtrName)(
                shared_int_data, 
                custom_deleter, // 사용자 정의 Deleter 전달
                segment.get_segment_manager()
            );
        std::cout << "공유 메모리에 boost::interprocess::shared_ptr 생성됨. 현재 값: " << *(*shm_int_ptr) << std::endl;

        // 소비자에게 공유 메모리가 준비되었음을 알리기 위한 named_mutex 생성
        // create_only 플래그는 뮤텍스가 존재하지 않을 때만 생성하도록 합니다.
        boost::interprocess::named_mutex mutex(boost::interprocess::create_only, MutexName);
        std::cout << "Named Mutex '" << MutexName << "' 생성됨." << std::endl;

        std::cout << "소비자 애플리케이션이 공유 메모리에 접근할 수 있도록 대기 중..." << std::endl;
        std::cout << "아무 키나 누르면 종료됩니다." << std::endl;
        std::cin.get(); // 사용자가 키를 누를 때까지 대기

        std::cout << "생산자 애플리케이션 종료 중..." << std::endl;
    } catch (const boost::interprocess::interprocess_exception& ex) {
        std::cerr << "오류 발생: " << ex.what() << std::endl;
        std::cerr << "이전 실행에서 공유 메모리 또는 뮤텍스가 제대로 정리되지 않았을 수 있습니다. 수동으로 제거하거나 재시도하십시오." << std::endl;
    }

    // 공유 메모리 세그먼트와 뮤텍스 제거 (생산자가 종료될 때 정리)
    // 일반적으로 공유 메모리 세그먼트를 생성한 프로세스가 이를 제거합니다.
    boost::interprocess::shared_memory_object::remove(ShmName);
    boost::interprocess::named_mutex::remove(MutexName);
    std::cout << "공유 메모리 세그먼트와 뮤텍스 제거됨." << std::endl;

    return 0;
}

#endif
