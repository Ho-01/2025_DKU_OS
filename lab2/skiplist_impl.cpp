/*
*	DKU Operating System Lab
*	    Lab2 (Concurrent Data Structure : Skip List)
*	    Student id : 32190789
*	    Student name : 김승호
*	    Date : 2025-05-19
*/

#include "skiplist_impl.h"
#include <cstdlib>
#include <ctime>

// DefaultSkipList 생성자
DefaultSkipList::DefaultSkipList(int max_level, float prob) {
    this->max_level_ = max_level;
    this->prob_ = prob;
    
    header_ = new Node();
    header_->key = -1;  
    header_->value = -1;
    header_->upd_cnt = 0;
    header_->level = max_level_;
    
    header_->forward = new Node*[max_level_ + 1];
    for (int i = 0; i <= max_level_; i++) {
        header_->forward[i] = nullptr;
    }
    
    srand(time(nullptr));
}

// DefaultSkipList 소멸자
DefaultSkipList::~DefaultSkipList() {
    Node* current = header_;
    Node* temp;
    
    while(current) {
        temp = current->forward[0];
        delete[] current->forward;
        delete current;
        current = temp;
    }
}

// 랜덤 레벨 생성 함수
int DefaultSkipList::random_level() {
    int level = 0;
    while (((double)rand() / RAND_MAX) < prob_ && 
            level < max_level_) {
        level++;
    }
    return level;
}

// SkipList 생성자
SkipList::SkipList(int max_level, float prob) : DefaultSkipList(max_level, prob) {}

// SkipList 소멸자
SkipList::~SkipList() {}

void SkipList::insert(int key, int value) {
    // 삽입할 위치를 찾기 위해 각 레벨에서 이전 노드를 저장할 배열
    Node* update[max_level_ + 1];
    Node* current = header_;

    // 최고 레벨부터 시작해 삽입 위치를 찾는다
    for (int i = max_level_; i >= 0; i--) {
        // 현재 레벨에서 다음 노드의 key가 삽입할 key보다 작을 때 계속 오른쪽으로 이동
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        // 삽입 위치 직전 노드를 저장
        update[i] = current;
    }

    // 레벨 0에서 실제 삽입 위치의 다음 노드 확인
    current = current->forward[0];

    if (current && current->key == key) {
        // 이미 동일한 key가 존재하면 value를 더하고 upd_cnt를 증가시킴
        current->value += value;
        current->upd_cnt++;
    } else {
        // 새로운 노드가 삽입될 경우, 랜덤하게 노드의 레벨 결정
        int new_level = random_level();
        Node* new_node = new Node();
        new_node->key = key;
        new_node->value = value;
        new_node->upd_cnt = 0; // 새로 삽입된 노드는 아직 업데이트되지 않았으므로 0
        new_node->level = new_level;

        // 새 노드의 forward 배열 생성 및 초기화
        new_node->forward = new Node*[new_level + 1];

        // 각 레벨에서 new_node를 기존 노드들 사이에 연결
        for (int i = 0; i <= new_level; i++) {
            new_node->forward[i] = update[i]->forward[i];  // new_node의 forward[i]는 이전 노드의 forward[i]를 가리킴
            update[i]->forward[i] = new_node;  // 이전 노드의 forward[i]는 이제 new_node를 가리킴
        }
    }
}

int SkipList::lookup(int key) {
    // 헤더부터 탐색 시작
    Node* current = header_;

    // 최고 레벨부터 시작하여 key보다 작은 값들을 건너뛰며 이동
    for (int i = max_level_; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
    }

    // 레벨 0까지 내려온 후, 다음 노드가 찾는 key인지 확인
    current = current->forward[0];
    
    // key가 존재하면 해당 노드의 value를 반환, 없으면 0 반환
    return (current && current->key == key) ? current->value : 0;
}

void SkipList::remove(int key) {
    // 각 레벨에서 삭제할 노드 직전 노드를 저장
    Node* update[max_level_ + 1];
    Node* current = header_;

    // 최고 레벨부터 시작하여 삭제할 위치를 찾음
    for (int i = max_level_; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current; // 해당 레벨에서 삽입 지점 바로 앞 노드를 저장
    }

    // 레벨 0에서 실제 삭제 대상 노드 확인
    current = current->forward[0];

    if (current && current->key == key) {
        // 각 레벨에서 해당 노드를 건너뛰도록 포인터를 수정
        for (int i = 0; i <= max_level_; i++) {
            if (update[i]->forward[i] != current) break;
            update[i]->forward[i] = current->forward[i];
        }
        // 메모리 해제
        delete current;
    }
}

// CoarseSkipList 생성자
CoarseSkipList::CoarseSkipList(int max_level, float prob) : DefaultSkipList(max_level, prob) {
    pthread_mutex_init(&mutex_lock, nullptr);
}

// CoarseSkipList 소멸자
CoarseSkipList::~CoarseSkipList() {
    pthread_mutex_destroy(&mutex_lock);
}

void CoarseSkipList::insert(int key, int value) {
    // 전체 리스트에 대해 락을 걸어 다른 스레드의 접근을 차단
    pthread_mutex_lock(&mutex_lock);

    // 삽입 위치를 찾기 위해 이전 노드를 저장하는 배열
    Node* update[max_level_ + 1];
    Node* current = header_;

    // 최고 레벨부터 내려오며 삽입 지점 이전 노드를 찾음
    for (int i = max_level_; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    // 레벨 0에서 다음 노드를 확인하여 삽입 여부 결정
    current = current->forward[0];

    if (current && current->key == key) {
        // 동일한 키가 존재하면 값 누적 + 업데이트 횟수 증가
        current->value += value;
        current->upd_cnt++;
    } else {
        // 새 노드 삽입을 위해 랜덤 레벨 생성
        int new_level = random_level();
        Node* new_node = new Node();
        new_node->key = key;
        new_node->value = value;
        new_node->level = new_level;
        new_node->upd_cnt = 0;  // 새 노드는 아직 업데이트 이력이 없음

        // forward 배열 초기화 및 연결
        new_node->forward = new Node*[new_level + 1];
        for (int i = 0; i <= new_level; i++) {
            new_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = new_node;
        }
    }

    // 락 해제
    pthread_mutex_unlock(&mutex_lock);
}

int CoarseSkipList::lookup(int key) {
    // 읽기 도중 다른 스레드의 쓰기를 막기 위해 락
    pthread_mutex_lock(&mutex_lock);

    Node* current = header_;

    // 최고 레벨부터 시작하여 key보다 작은 노드를 건너뜀
    for (int i = max_level_; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
    }

    // 레벨 0에서 다음 노드가 찾는 키인지 확인
    current = current->forward[0];
    int value = (current && current->key == key) ? current->value : 0;

    pthread_mutex_unlock(&mutex_lock); // 락 해제 후 결과 반환
    return value;
}

void CoarseSkipList::remove(int key) {
    // 삭제 도중 다른 스레드의 접근을 막기 위해 락
    pthread_mutex_lock(&mutex_lock);

    Node* update[max_level_ + 1];
    Node* current = header_;

    // 삭제할 노드의 이전 노드를 각 레벨별로 저장
    for (int i = max_level_; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    // 삭제 대상 노드 접근
    current = current->forward[0];

    if (current && current->key == key) {
        // 각 레벨에서 해당 노드를 건너뛰도록 포인터를 재설정
        for (int i = 0; i <= max_level_; i++) {
            if (update[i]->forward[i] != current) break;
            update[i]->forward[i] = current->forward[i];
        }
        // 노드 메모리 해제
        delete current;
    }

    pthread_mutex_unlock(&mutex_lock); // 락 해제
}

// FineSkipList 생성자
FineSkipList::FineSkipList(int max_level, float prob) : DefaultSkipList(max_level, prob) {
    delete header_;
    header_ = new FineNode();
    header_->key = -1;
    header_->value = -1;
    header_->upd_cnt = 0;
    header_->level = max_level_;
    
    header_->forward = new Node*[max_level_ + 1];
    for (int level = 0; level <= max_level_; level++) {
        header_->forward[level] = nullptr;
    }
}

// FineSkipList 소멸자
FineSkipList::~FineSkipList() {
    FineNode* current = (FineNode*)header_;
    FineNode* temp;
    
    while(current) {
        temp = (FineNode*)current->forward[0];
        pthread_mutex_destroy(&((FineNode*)current)->lock);
        delete[] current->forward;
        delete current;
        current = temp;
    }
    
    header_ = nullptr;
}

void FineSkipList::insert(int key, int value) {
    FineNode* update[max_level_ + 1];
    FineNode* current = (FineNode*)header_;
    pthread_mutex_lock(&current->lock); // 헤더 노드 락 획득

    // 삽입 위치 탐색 (레벨 상위부터 아래로)
    for (int i = max_level_; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            FineNode* next = (FineNode*)current->forward[i];
            pthread_mutex_lock(&next->lock); // 다음 노드 락 먼저 획득
            pthread_mutex_unlock(&current->lock); // 현재 노드 락 해제
            current = next; // 다음 노드로 이동
        }
        update[i] = current; // 다음 노드로 이동
    }

    FineNode* target = (FineNode*)current->forward[0]; // 레벨 0의 삽입 위치

    if (target && target->key == key) {
        pthread_mutex_lock(&target->lock); // 기존 노드 락 획득
        target->value += value;
        target->upd_cnt++;
        pthread_mutex_unlock(&target->lock); // 락 해제
    } else {
        int new_level = random_level();
        FineNode* new_node = new FineNode();
        new_node->key = key;
        new_node->value = value;
        new_node->upd_cnt = 0;
        new_node->level = new_level;
        pthread_mutex_init(&new_node->lock, nullptr); // 노드마다 락 초기화
        new_node->forward = new Node*[new_level + 1];

        // 각 레벨별로 연결 설정
        for (int i = 0; i <= new_level; i++) {
            new_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = new_node;
        }
    }

    // 탐색 중 걸었던 락을 모두 해제
    for (int i = 0; i <= max_level_; i++) {
        if (update[i])
            pthread_mutex_unlock(&update[i]->lock);
    }
}

int FineSkipList::lookup(int key) {
    FineNode* current = (FineNode*)header_;
    pthread_mutex_lock(&current->lock); // 헤더 락 획득
    
    // 탐색 진행
    for (int i = max_level_; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            FineNode* next = (FineNode*)current->forward[i];
            pthread_mutex_lock(&next->lock); // 다음 노드 락 먼저 획득
            pthread_mutex_unlock(&current->lock); // 현재 락 해제
            current = next;
        }
    }

    FineNode* target = (FineNode*)current->forward[0];
    int result = 0;

    if (target && target->key == key) {
        pthread_mutex_lock(&target->lock);
        result = target->value;
        pthread_mutex_unlock(&target->lock);
    }

    pthread_mutex_unlock(&current->lock); // 마지막으로 사용한 락 해제
    return result;
}

void FineSkipList::remove(int key) {
    FineNode* update[max_level_ + 1];
    FineNode* current = (FineNode*)header_;
    pthread_mutex_lock(&current->lock);

    // 삭제할 위치 탐색
    for (int i = max_level_; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->key < key) {
            FineNode* next = (FineNode*)current->forward[i];
            pthread_mutex_lock(&next->lock);
            pthread_mutex_unlock(&current->lock);
            current = next;
        }
        update[i] = current;
    }

    FineNode* target = (FineNode*)current->forward[0];
    if (target && target->key == key) {
        pthread_mutex_lock(&target->lock);

        // 각 레벨에서 포인터 변경
        for (int i = 0; i <= max_level_; i++) {
            if (update[i]->forward[i] != target) break;
            update[i]->forward[i] = target->forward[i];
        }
        pthread_mutex_unlock(&target->lock);
        pthread_mutex_destroy(&target->lock); // 노드 락 파괴
        delete[] target->forward;
        delete target;
    }

    // 탐색 시 걸었던 락 모두 해제
    for (int i = 0; i <= max_level_; i++) {
        if (update[i])
            pthread_mutex_unlock(&update[i]->lock);
    }
}
