/*
*	DKU Operating System Lab
*	    Lab3 (Flash Translation Layer)
*	    Student id : 
*	    Student name : 
*	    Date : 
*/

#include "ftl_impl.h"

void GreedyFTL::garbageCollect() {
   /*
   GreedyFTL's garbage collection policy
   - 가장 invalid한 페이지가 많은 블록을 선택
   - WAF 측정을 위해 total_logical_writes와 total_physical_writes를 업데이트한다.
   */
}

void GreedyFTL::writePage(int logicalPage, int data) {
   /*
   write operation in GreedyFTL
   - 만약 남아있는 free block이 2개 이하라면, GC를 수행한다. 
   - 현재 활성 블록에 페이지를 쓰고, L2P 테이블을 업데이트한다.
   - 만약 활성 블록이 꽉 찼다면, 다음 새로운 블록을 활성 블록으로 설정한다.
   - WAF 측정을 위해 total_logical_writes와 total_physical_writes를 업데이트한다.
   */
}

void GreedyFTL::readPage(int logicalPage) {
   /*
   read operation in GreedyFTL
   - L2P 테이블을 통해 논리 페이지 번호에 해당하는 물리 페이지 번호를 찾는다.
   - 해당 페이지의 데이터를 출력한다.
   - 만일 invalid or Free 페이지라면, 에러문구를 출력한다.
   */
}


void CostBenefitFTL::garbageCollect() {
   /*
   CostBenefitFTL's garbage collection policy
   - 가장 invalid한 페이지가 많은 블록들중, 쓰여진지 가장 오래된 블록을 선택한다. 
   - WAF 측정을 위해 total_logical_writes와 total_physical_writes를 업데이트한다.
   */
}

void CostBenefitFTL::writePage(int logicalPage, int data) {
   /*
   write operation in CostBenefitFT
   - 만약 남아있는 free block이 2개 이하라면, GC를 수행한다. 
   - 현재 활성 블록에 페이지를 쓰고, L2P 테이블을 업데이트한다.
   - 만약 활성 블록이 꽉 찼다면, 다음 새로운 블록을 활성 블록으로 설정한다.
   - WAF 측정을 위해 total_logical_writes와 total_physical_writes를 업데이트한다.
   */
}

void CostBenefitFTL::readPage(int logicalPage) {
   /*
   read operation in CostBenefitFT
   - L2P 테이블을 통해 논리 페이지 번호에 해당하는 물리 페이지 번호를 찾는다.
   - 해당 페이지의 데이터를 출력한다.
   - 만일 invalid or Free 페이지라면, 에러문구를 출력한다.
   */
}
