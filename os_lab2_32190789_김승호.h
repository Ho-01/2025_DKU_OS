#include "skiplist.h"

#ifndef SKIPLIST_IMPL_H
#define SKIPLIST_IMPL_H

/**
 * @brief SkipList without lock
 * DefaultSkipList의 함수를 오버라이드하여, 클래스를 완성한다.
 * 구현에 필요한 멤버 변수/함수를 추가하여 구현한다.
 */
class SkipList : public DefaultSkipList {
    private:
        // 멤버 변수 추가 선언 가능

    public:
        // 생성자
        SkipList(int max_level = 16, float prob = 0.5);
        // 소멸자
        ~SkipList();
        
        // 멤버 함수 추가 선언 가능
        void insert(int key, int value) override;
        int lookup(int key) override;
        void remove(int key) override;
        void traversal(KVC* arr);
};

/**
 * @brief SkipList with coarse-grained lock
 * SkipList 전체를 critical section으로 가정하여, 하나의 lock으로 이를 관리한다.
 * DefaultSkipList의 함수를 오버라이드하여, 클래스를 완성한다.
 * 구현에 필요한 멤버 변수/함수를 추가하여 구현한다.
 */
class CoarseSkipList : public DefaultSkipList {
    private:
        // 멤버 변수 추가 선언 가능
        pthread_mutex_t mutex_lock;
        
    public:
        // 생성자
        CoarseSkipList(int max_level = 16, float prob = 0.5);
        // 소멸자
        ~CoarseSkipList();
        
        // 멤버 함수 추가 선언 가능
        void insert(int key, int value) override;
        int lookup(int key) override;
        void remove(int key) override;
        void traversal(KVC* arr);
};

/// @brief FineSkipList는 FineNode를 정의하여 사용하길 권장한다.
struct FineNode : public Node {
    // 멤버 변수 추가 가능
    pthread_mutex_t lock;
};

/**
 * @brief SkipList with fine-grained lock
 * SkipList 내부의 critical section을 Node 단위로 lock으로 관리한다.
 * Node를 상속받은 FineNode를 정의하여 사용하길 권장한다.
 * DefaultSkipList의 함수를 오버라이드하여, 클래스를 완성한다.
 * 구현에 필요한 멤버 변수/함수를 추가하여 구현한다.
 */
class FineSkipList : public DefaultSkipList {
    private:
        // 멤버 변수 추가 선언 가능
        
    public:
        // 생성자
        FineSkipList(int max_level = 16, float prob = 0.5);
        // 소멸자
        ~FineSkipList();
        
        // 멤버 함수 추가 선언 가능
        void insert(int key, int value) override;
        int lookup(int key) override;
        void remove(int key) override;
        void traversal(KVC* arr);
};
#endif