#include "ftl.h"

#ifndef FTL_IMPL_H
#define FTL_IMPL_H

class GreedyFTL : public FlashTranslationLayer {
    private:
        // 멤버 변수 추가 선언 가능
    public:
        // 생성자
        GreedyFTL(int total_blocks, int block_size) : FlashTranslationLayer(total_blocks, block_size) {
            name = "GreedyFTL";
        }
        // 소멸자
        ~GreedyFTL() {}
        // 멤버 함수 추가 선언 가능
        void garbageCollect() override;
        void writePage(int logicalPage, int data) override;
        void readPage(int logicalPage) override;
};

class CostBenefitFTL : public FlashTranslationLayer {
    private:
        // 멤버 변수 추가 선언 가능
    public:
        // 생성자
        CostBenefitFTL(int total_blocks, int block_size) : FlashTranslationLayer(total_blocks, block_size) {
            name = "CostBenefitFTL";
        }
        // 소멸자
        ~CostBenefitFTL() {}
        // 멤버 함수 추가 선언 가능
        void garbageCollect() override;
        void writePage(int logicalPage, int data) override;
        void readPage(int logicalPage) override;
};

#endif // FTL_IMPL_H