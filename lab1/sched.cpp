/*
*	DKU Operating System Lab
*	    Lab1 (Scheduler Algorithm Simulator)
*	    Student id : 32190789
*	    Student name : 김승호
*	    Date : 2025/04/28
*	    Contents :
*/

#include <string>
#include <stdio.h>
#include <iostream>
#include <queue>
#include <algorithm>
#include <random>
#include <unordered_map>
#include "sched.h"

class RR : public Scheduler{
    private:
        int time_slice_;
        int left_slice_;
        std::queue<Job> waiting_queue;

    public:
        RR(std::queue<Job> jobs, double switch_overhead, int time_slice) : Scheduler(jobs, switch_overhead) {
            name = "RR_"+std::to_string(time_slice);
            /*
            * 위 생성자 선언 및 이름 초기화 코드 수정하지 말것.
            * 나머지는 자유롭게 수정 및 작성 가능 (아래 코드 수정 및 삭제 가능)
            */
            time_slice_ = time_slice; 
            left_slice_ = time_slice;
        }

        int run() override {
            if (current_job_.name == 0) {
                if (!waiting_queue.empty()) {
                    current_job_ = waiting_queue.front();
                    waiting_queue.pop();
                    left_slice_ = time_slice_;
                } else if (!job_queue_.empty()) {
                    current_job_ = job_queue_.front();
                    job_queue_.pop();
                    left_slice_ = time_slice_;
                } else {
                    return -1;
                }
            }
        
            if (current_job_.service_time == current_job_.remain_time) {
                current_job_.first_run_time = current_time_;
            }
        
            double exec_time = std::min({left_slice_, current_job_.remain_time, 1.0});
            current_time_ += exec_time;
            current_job_.remain_time -= exec_time;
            left_slice_ -= exec_time;
        
            if (current_job_.remain_time == 0) {
                current_job_.completion_time = current_time_;
                end_jobs_.push_back(current_job_);
        
                if (!waiting_queue.empty()) {
                    current_job_ = waiting_queue.front();
                    waiting_queue.pop();
                    left_slice_ = time_slice_;
                    current_time_ += switch_time_;
                } else if (!job_queue_.empty()) {
                    current_job_ = job_queue_.front();
                    job_queue_.pop();
                    left_slice_ = time_slice_;
                    current_time_ += switch_time_;
                } else {
                    return -1;
                }
            }
            else if (left_slice_ == 0) {
                waiting_queue.push(current_job_);
        
                if (!waiting_queue.empty()) {
                    current_job_ = waiting_queue.front();
                    waiting_queue.pop();
                    left_slice_ = time_slice_;
                    current_time_ += switch_time_;
                } else if (!job_queue_.empty()) {
                    current_job_ = job_queue_.front();
                    job_queue_.pop();
                    left_slice_ = time_slice_;
                    current_time_ += switch_time_;
                } else {
                    return -1;
                }
            }
        
            return current_job_.name;
        }
        
                
};

class FeedBack : public Scheduler {
private:
    std::queue<Job> queue[4]; // 각 요소가 하나의 큐인 배열 선언
    int quantum[4] = {1, 1, 1, 1};
    int left_slice_;
    int current_queue;

public:
    FeedBack(std::queue<Job> jobs, double switch_overhead, bool is_2i) : Scheduler(jobs, switch_overhead) {
        if (is_2i) {
            name = "FeedBack_2i";
        } else {
            name = "FeedBack_1";
        }
        /*
        * 위 생성자 선언 및 이름 초기화 코드 수정하지 말것.
        * 나머지는 자유롭게 수정 및 작성 가능
        */
        // Queue별 time quantum 설정
        if (name == "FeedBack_2i") {
            quantum[0] = 1;
            quantum[1] = 2;
            quantum[2] = 4;
            quantum[3] = 8;
        }
    }

    int run() override {
        // 1. 새로운 작업이 있으면, 0번 큐에 arrival time 순서로 삽입
        while (!job_queue_.empty() && job_queue_.front().arrival_time <= current_time_) {
            queue[0].push(job_queue_.front());
            job_queue_.pop();
        }

        // 2. 현재 작업이 없는 경우 (큐 탐색해서 할당)
        if (current_job_.name == 0) {
            for (int i = 0; i < 4; i++) {
                if (!queue[i].empty()) {
                    current_job_ = queue[i].front();
                    queue[i].pop();
                    current_queue = i;
                    left_slice_ = quantum[i];
                    break;
                }
            }
            if (current_job_.name == 0) {
                // 실행할 작업이 없으면 idle도 없고, 그냥 시간만 증가
                current_time_+= 1.00;
                return -1;
            }
        }

        // 3. 현재 작업이 처음 실행되는 경우
        if (current_job_.service_time == current_job_.remain_time) {
            current_job_.first_run_time = current_time_;
        }

        // 4. 1초 작업 실행
        current_time_+= 1.00;
        current_job_.remain_time--;
        left_slice_--;

        // 5. 현재 작업이 완료된 경우
        if (current_job_.remain_time == 0) {
            current_job_.completion_time = current_time_;
            end_jobs_.push_back(current_job_);
            current_job_ = Job(); // 새로운 빈 작업으로 초기화
            current_time_ += switch_time_;
        }
        // 6. time quantum이 끝났지만 작업이 아직 안 끝난 경우
        else if (left_slice_ == 0) {
            if (current_queue < 3) {
                queue[current_queue + 1].push(current_job_); // 다음 낮은 우선순위 큐로 이동
            } else {
                queue[3].push(current_job_); // 이미 제일 낮은 큐면 그대로
            }
            current_job_ = Job(); // 새로운 빈 작업으로 초기화
            current_time_ += switch_time_;
        }

        // 7. 현재 실행 중인 작업 이름 반환
        return current_job_.name;
    }
};

class Lottery : public Scheduler{
    private:
        int counter = 0;
        int total_tickets = 0;
        int winner = 0;
        std::mt19937 gen;  // 난수 생성기
        
    public:
        Lottery(std::list<Job> jobs, double switch_overhead) : Scheduler(jobs, switch_overhead) {
            name = "Lottery";
            // 난수 생성기 초기화
            uint seed = 10; // seed 값 수정 금지
            gen = std::mt19937(seed);
            total_tickets = 0;
            for (const auto& job : job_list_) {
                total_tickets += job.tickets;
            }
        }

        int getRandomNumber(int min, int max) {
            std::uniform_int_distribution<int> dist(min, max);
            return dist(gen);
        }

        int run() override {
            if (job_list_.empty()) {
                return -1; // 모든 작업 완료
            }
        
            // 1. 랜덤으로 승자 티켓 뽑기
            winner = getRandomNumber(1, total_tickets);
        
            // 2. 승자 티켓에 해당하는 작업 찾기
            int ticket_sum = 0;
            auto it = job_list_.begin();
            for (; it != job_list_.end(); ++it) {
                ticket_sum += it->tickets;
                if (ticket_sum >= winner) {
                    break;
                }
            }
        
            if (it == job_list_.end()) {
                return -1; // 이론상 여기로 오면 안 됨
            }
        
            // 3. 현재 실행할 작업 결정
            current_job_ = *it;
        
            // 4. 현재 작업이 처음 실행되는 경우
            if (current_job_.service_time == current_job_.remain_time) {
                current_job_.first_run_time = current_time_;
            }
        
            // 5. 1초 작업 실행
            current_time_+= 1.00;
            current_job_.remain_time--;
        
            // 6. 작업이 완료된 경우
            if (current_job_.remain_time == 0) {
                current_job_.completion_time = current_time_;
                end_jobs_.push_back(current_job_);
        
                // 총 티켓 수에서 해당 작업 티켓 빼주기
                total_tickets -= current_job_.tickets;
        
                // job_list_에서도 제거
                job_list_.erase(it);
        
                // Context switch 발생
                current_time_ += switch_time_;
            } else {
                // 남은 작업은 다시 job_list_에 업데이트
                *it = current_job_;
            }
        
            // 7. 현재 실행한 작업 이름 반환
            return current_job_.name;
        }
};


class Stride : public Scheduler{
    private:
        // 각 작업의 현재 pass 값과 stride 값을 관리하는 맵
        std::unordered_map<int, int> pass_map;  
        std::unordered_map<int, int> stride_map;  
        const int BIG_NUMBER = 10000; // stride 계산을 위한 상수 (보통 큰 수를 사용)

    public:
        Stride(std::list<Job> jobs, double switch_overhead) : Scheduler(jobs, switch_overhead) {
            name = "Stride";
                    // job_list_에 있는 각 작업에 대해 stride와 초기 pass 값(0)을 설정
            for (auto &job : job_list_) {
                // stride = BIG_NUMBER / tickets (tickets는 0이 아님을 가정)
                stride_map[job.name] = BIG_NUMBER / job.tickets;
                pass_map[job.name] = 0;
            }
        }

        int run() override {
            if (job_list_.empty()) {
                return -1; // 모든 작업 완료
            }
        
            // 1. 현재 실행할 작업 고르기 (pass 값이 가장 작은 작업)
            int min_pass = 1e9; // 충분히 큰 수
            auto it = job_list_.begin();
            auto selected = job_list_.end(); // 선택된 작업
        
            for (auto iter = job_list_.begin(); iter != job_list_.end(); ++iter) {
                if (pass_map[iter->name] < min_pass) {
                    min_pass = pass_map[iter->name];
                    selected = iter;
                }
            }
        
            if (selected == job_list_.end()) {
                return -1; // 이론상 여기로 오면 안 됨
            }
        
            // 2. 현재 작업 설정
            current_job_ = *selected;
        
            // 3. 처음 실행이면 first_run_time 기록
            if (current_job_.service_time == current_job_.remain_time) {
                current_job_.first_run_time = current_time_;
            }
        
            // 4. 1초 작업 실행
            current_time_+= 1.00;
            current_job_.remain_time--;
        
            // 5. 작업 완료된 경우
            if (current_job_.remain_time == 0) {
                current_job_.completion_time = current_time_;
                end_jobs_.push_back(current_job_);
        
                // pass_map, stride_map에서 제거
                pass_map.erase(current_job_.name);
                stride_map.erase(current_job_.name);
        
                // job_list_에서도 제거
                job_list_.erase(selected);
        
                // Context switch 시간 추가
                current_time_ += switch_time_;
            } else {
                // 아직 남은 작업은 업데이트
                *selected = current_job_;
        
                // pass 값 증가 (1초 실행했으니 stride만큼 추가)
                pass_map[current_job_.name] += stride_map[current_job_.name];
            }
        
            // 6. 현재 실행한 작업 이름 반환
            return current_job_.name;
        }
};
