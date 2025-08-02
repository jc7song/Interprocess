

#include <common.h>

void read();
int main()
{
    read();

    std::cout << "Hello World!\n";
}

#include <iostream>
#include <string>
#include <memory> // std::unique_ptr (선택 사항, 로컬 사용용)

void read() {
    try {
        bi::managed_shared_memory segment(bi::open_only, SharedMemName);
        std::cout << "공유 메모리 세그먼트 '" << SharedMemName << "' 열림." << std::endl;

        MySharedPtr* my_shared_ptr = segment.find<MySharedPtr>(SharedPtrName).first;

        if (!my_shared_ptr) {
            std::cerr << "오류: 공유 메모리에서 shared_ptr를 찾을 수 없습니다." << std::endl;
            return;
        }

        MySharedPtr my_data = *my_shared_ptr;

        std::cout << "data : " << my_data->data_ << std::endl;
        std::cout << "Use count : " << my_shared_ptr->use_count() << std::endl;
        std::cout << "소비자 애플리케이션 종료 중..." << std::endl;
#if 0
        MySharedPtr my_shared = *(my_shared_ptr);
        std::cout << "data : " << my_shared->data_ << std::endl;
        std::cout << "Use count : " << my_shared.use_count() << std::endl;
#endif

    } catch (const boost::interprocess::interprocess_exception& ex) {
        std::cerr << "오류 발생: " << ex.what() << std::endl;
        std::cerr << "공유 메모리를 열 수 없습니다." << std::endl;
    }

    return;
}

#if 0
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp> // 올바른 헤더 경로
#include <boost/interprocess/sync/named_mutex.hpp>
#include <iostream>
#include <string>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::milliseconds

// Boost 1.80 이상 버전에서 테스트되었으며, Visual Studio 2022와 호환됩니다.

// 공유 메모리 세그먼트 및 뮤텍스의 이름 정의 (생산자와 동일해야 함)
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
    std::cout << "소비자 애플리케이션 시작..." << std::endl;

    try {
        // 생산자가 공유 메모리 세그먼트와 뮤텍스를 생성할 때까지 대기합니다.
        // open_or_create 플래그는 뮤텍스가 존재하면 열고, 없으면 생성합니다.
        // 여기서는 생산자가 뮤텍스를 생성했는지 확인하는 용도로 사용됩니다.
        std::cout << "생산자가 공유 메모리를 생성할 때까지 대기 중..." << std::endl;
        boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, MutexName);
        // 뮤텍스 생성 또는 열기 시도 후, 생산자가 완전히 준비되었는지 확인하기 위해
        // 잠시 대기하는 것이 좋습니다 (생산자가 shared_ptr를 construct할 시간을 줍니다).
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "생산자가 공유 메모리 준비 완료." << std::endl;

        // 기존 공유 메모리 세그먼트 열기
        // open_only 플래그는 세그먼트가 이미 존재해야 함을 의미합니다.
        boost::interprocess::managed_shared_memory segment(boost::interprocess::open_only, ShmName);
        std::cout << "공유 메모리 세그먼트 '" << ShmName << "' 열림." << std::endl;

        // 공유 메모리 내에서 ShmIntSharedPtr 찾기
        // find는 std::pair<T*, size_type>를 반환합니다.
        std::pair<ShmIntSharedPtr*, std::size_t> res;
        
        // shared_ptr가 생성될 때까지 기다립니다.
        // 생산자가 shared_ptr를 construct하기 전에 소비자가 find를 시도할 수 있으므로,
        // 일정 시간 대기하면서 재시도하는 로직이 필요합니다.
        int attempts = 0;
        const int max_attempts = 20; // 최대 20초 대기
        while (attempts < max_attempts) {
            res = segment.find<ShmIntSharedPtr>(SharedPtrName);
            if (res.first) {
                break; // 찾았으면 루프 종료
            }
            std::cout << "shared_ptr를 찾을 수 없음. " << attempts + 1 << "초 후 재시도..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            attempts++;
        }

        if (!res.first) {
            std::cerr << "오류: 공유 메모리에서 shared_ptr를 찾을 수 없습니다. 생산자가 실행 중이고 shared_ptr를 생성했는지 확인하세요." << std::endl;
            return 1;
        }

        ShmIntSharedPtr& shm_int_ptr = *(res.first);
        std::cout << "공유 메모리에서 boost::interprocess::shared_ptr 찾음." << std::endl;

        // shared_ptr를 통해 공유된 데이터에 접근
        std::cout << "현재 공유된 int 값: " << *shm_int_ptr << std::endl;
        std::cout << "참조 카운트: " << shm_int_ptr.use_count() << std::endl;

        // 값 수정
        *shm_int_ptr = 456;
        std::cout << "공유된 int 값을 " << *shm_int_ptr << "로 수정함." << std::endl;
        std::cout << "참조 카운트: " << shm_int_ptr.use_count() << std::endl;

        // 생산자가 종료될 때까지 대기
        std::cout << "생산자가 공유 메모리를 제거할 때까지 대기 중..." << std::endl;
        // 생산자가 named_mutex를 제거할 때까지 기다립니다.
        // open_only로 named_mutex를 열려고 시도하면,
        // 생산자가 뮤텍스를 제거했을 때 예외가 발생합니다.
        try {
            // 이 뮤텍스는 생산자가 제거할 때까지 존재해야 합니다.
            boost::interprocess::named_mutex wait_for_producer_exit(boost::interprocess::open_only, MutexName);
            // 이 지점에 도달하면 생산자가 뮤텍스를 아직 제거하지 않았다는 의미입니다.
            // 무한 루프를 돌며 뮤텍스 제거를 기다립니다.
            while(true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } catch (const boost::interprocess::interprocess_exception& ex) {
            // 생산자가 뮤텍스를 제거하면 이곳으로 들어옵니다.
            std::cout << "생산자가 종료되어 공유 메모리가 제거되었습니다." << std::endl;
        }

    } catch (const boost::interprocess::interprocess_exception& ex) {
        std::cerr << "오류 발생: " << ex.what() << std::endl;
        std::cerr << "생산자 애플리케이션이 먼저 실행되었는지 확인하고, 공유 메모리/뮤텍스 이름이 일치하는지 확인하세요." << std::endl;
    }

    std::cout << "소비자 애플리케이션 종료." << std::endl;
    return 0;
}

}

#endif
