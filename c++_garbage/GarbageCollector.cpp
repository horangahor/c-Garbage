#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
using namespace std;

class refptr { // 새로 생긴 동적 메모리마다 있는 매니저

    void* p;  // heap 메모리의 주소
    vector<void*> hi; // heap의 ( heap을 참조하는 애들 리스트)

public:
    refptr(void* p) :p(p) {}
    void track(void* p) { hi.push_back(p); } // 참조하는 애들을 추가
    size_t getCnt() { return hi.size(); } // 애들 수 보기
    void* getPtr() { return p; } // 동적 메모리 주소 보기
    void update() { // 참조 안하는 애들 제거
        auto it = hi.begin();
        while (it != hi.end()) { // 참조하는 애들 순회
            if (*(void**)(*it) != p) { // 참조하는 애들이 동적메모리를 더이상 참조하지 않으면
                it = hi.erase(it); // 제거 후 다음 요소로 이동
            }
            else {
                ++it; // 참조하면 다음 애 이동
            }
        }
        if (hi.empty()) { // 참조하는 애들이 남은게 없다면 동적 메모리 해제
            cout << "쓰레기 해제: " << p << endl;
            free(p);
            p = nullptr;
        }
    }
    void check() { for (auto it : hi) cout << it << " "; } // 참조하는 애들 보기
};

template<typename T>
class smartptr {
    T* p;

public:
    smartptr(T* p = 0) : p(p) {}
    ~smartptr() {
        cout << "스마트 포인터 메모리 해제" << endl;
        delete p;
    }
    T& operator*() { return *p; }
    T* operator->() { return p; }
    smartptr& operator= (T* p) {  // 대입연산자 행위를 지정안해주면 동적 메모리인데도 참조를 벗어나니까 자동으로 파괴자 호출?
        this->p = p;
        return *this;
    }
};

// Skeleton for GC with a Sigleton design pattern
class GarbageCollector {
private:

    vector<refptr> a;
    const int period = 5;
    thread th;
    bool flag;
    bool flag2;

    //Thread function
    void timer(int period) {
        while (flag) {
            std::this_thread::sleep_for(std::chrono::seconds(period)); // period만큼 기다리고
            flag2 = true;  // 기다리게 바꿈
            collectGarbage();  // 이것만 넣으면, 되긴 하는데 main이 0초일 때를 못 쫓아감
            flag2 = false;
        }
    }

    void join() {
        th.join();
    }

    // Disable constructors 
      // Default constructor. You can modifiy it if you want
    GarbageCollector() :th(&GarbageCollector::timer, this, period), flag(true), flag2(false) {}
    GarbageCollector(const GarbageCollector&) = delete;
    GarbageCollector& operator=(const GarbageCollector&) = delete;
    GarbageCollector(GarbageCollector&&) = delete;
    GarbageCollector& operator=(GarbageCollector&&) = delete;

public:
    static GarbageCollector& GetInstance() {
        static GarbageCollector instance;
        return instance;
    }

    void createObject(refptr p) {
        a.push_back(p);
    }

    void collectGarbage() {
        cout << "collecting Garbage~~~~~~" << endl;
        int count = 0;
        for (auto it = a.begin(); it != a.end();) {
            it->update(); // 각 동적메모리 매니저 업데이트
            if (it->getPtr() == nullptr) { // nullptr이라는 것은 메모리가 해제되었단 뜻이므로 매니저 관리 벡터에서 제거
                it = a.erase(it);
                count++;
            }
            else { it++; } // 아니면 다음 매니저로 넘어가기
        }
        cout << "제거한 메모리 수: " << count << endl;
        cout << "Active objects: " << a.size() << endl;
    }
    void gget() {
        flag = false;
        join();
    } // timer 스레드 비활성화

    void waiting() {  // 컬렉팅 끝날때까지 main 대기
        while (flag2) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

};
//Global variable 'GC' for the garbage collector class. 
GarbageCollector& GC = GarbageCollector::GetInstance();


// operator new overloading
void* operator new (size_t size, void* dest, int)
{
    void* p = malloc(size);
    refptr k(p);
    k.track(dest);
    GC.createObject(k);
    return p;
}

struct member {
    int x;
    int y;
};

int main() {
    // For common pointers
    int* ptr;
    ptr = new(&ptr, 0) int;
    ptr = new(&ptr, 0)  int;
    ptr = new(&ptr, 0)  int;
    ptr = new(&ptr, 0)  int;
    ptr = new(&ptr, 0)  int;
    ptr = new(&ptr, 0)  int;
    ptr = new(&ptr, 0)  int;
    GC.collectGarbage();

    // For class/structure pointers
    member* qtr;
    qtr = new(&qtr, 0) member;
    qtr = new(&qtr, 0) member;
    qtr = new(&qtr, 0) member;
    qtr = new(&qtr, 0) member;
    qtr = new(&qtr, 0) member;
    qtr = new(&qtr, 0) member;
    qtr = new(&qtr, 0) member;
    GC.collectGarbage();

    // For SmartPointers
    smartptr<member> rtr;  // free() 하면 파괴자 호출 없이 해제되니까
    rtr = new(&rtr, 0) member;
    rtr = new(&rtr, 0) member;
    rtr = new(&rtr, 0) member;
    rtr = new(&rtr, 0) member;
    rtr = new(&rtr, 0) member;
    rtr = new(&rtr, 0) member;
    GC.collectGarbage();


    for (;;) {
        ptr = new(&ptr, 0) int;
        qtr = new(&qtr, 0) member;
        rtr = new(&rtr, 0) member;
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 쓰레기 생성 속도 제한
        GC.waiting(); // 컬렉팅 시작했으면 끝날 때까지 기다리기
    }

    GC.gget(); // 스레드 끝내기 ( 자동 가비지 컬렉터 종료)

    /*
    delete ptr; // 끝나기 전에 얘들은 해제해야함 (ptr,qtr에 연결되어있는 메모리들, 아직 연결되어있어서 쓰레기 취급 x, rtr은 스마트포인터라 자동 해제)
    delete qtr;
    */

    ptr = nullptr;
    qtr = nullptr;
    GC.collectGarbage();

    //이렇게 하면 delete 호출할 필요 없음

}