/*
*	DKU Operating System Lab
*	    Lab3 (Flash Translation Layer)
*	    Student id : 32190789
*	    Student name : 김승호
*	    Date : 2025-06-18
*/

#include "ftl_impl.h"
#include <algorithm>
#include <vector>
#include <iostream>

// 가비지 컬렉션(GC): invalid 페이지가 가장 많은 사용중 블록을 찾아서 정리
void GreedyFTL::garbageCollect() {
    // victim: GC 대상 블록, max_invalid: 가장 많은 invalid 개수
    int victim = -1, max_invalid = -1;
    for (int i = 0; i < total_blocks; ++i) {
        // 사용 중이면서 invalid가 더 많은 블록을 찾음
        if (!blocks[i].is_free && blocks[i].invalid_page_cnt > max_invalid) {
            max_invalid = blocks[i].invalid_page_cnt;
            victim = i;
        }
    }
    // GC할 블록이 없거나 모두 유효하면 종료
    if (victim == -1 || max_invalid == 0) return;

    // valid 페이지들 임시로 저장
    std::vector<std::pair<int,int>> valid_pages;
    for (int j = 0; j < block_size; ++j) {
        Page &pg = blocks[victim].pages[j];
        if (pg.state == VALID)
            valid_pages.emplace_back(pg.logical_page_num, pg.data);
    }

    // victim 블록 페이지 모두 초기화(erase)
    for (int j = 0; j < block_size; ++j) blocks[victim].pages[j] = Page();
    blocks[victim].valid_page_cnt = 0;
    blocks[victim].invalid_page_cnt = 0;
    blocks[victim].is_free = true;
    blocks[victim].gc_cnt++;

    // active_block이 victim이거나 가득 찼으면 새 free block 할당
    if (active_block == victim || active_offset >= block_size) {
        for (int b = 0; b < total_blocks; ++b) {
            if (blocks[b].is_free) {
                active_block = b;
                active_offset = 0;
                blocks[b].is_free = false;
                break;
            }
        }
    }

    // 임시 저장된 valid 페이지들을 다시 기록(복구)
    for (auto &vp : valid_pages) {
        // active_block이 가득 차면 다시 할당
        if (active_offset >= block_size) {
            for (int b = 0; b < total_blocks; ++b) {
                if (blocks[b].is_free) {
                    active_block = b;
                    active_offset = 0;
                    blocks[b].is_free = false;
                    break;
                }
            }
        }
        int lpn = vp.first;
        int dat = vp.second;
        int ppn = active_block * block_size + active_offset;
        Page &dest = blocks[active_block].pages[active_offset];
        dest.state = VALID;
        dest.logical_page_num = lpn;
        dest.data = dat;
        blocks[active_block].valid_page_cnt++;
        L2P[lpn] = ppn;
        total_physical_writes++;
        blocks[active_block].last_write_time = static_cast<int>(total_logical_writes);
        active_offset++;
    }
}

// 논리 페이지 쓰기(write)
void GreedyFTL::writePage(int logicalPage, int data) {
    total_logical_writes++;
    // free 블록 개수 체크
    int free_cnt = 0;
    for (auto &blk : blocks) if (blk.is_free) free_cnt++;
    if (free_cnt <= 2) garbageCollect();

    // 기존 old page가 있으면 invalid로 처리
    int old_ppn = L2P[logicalPage];
    if (old_ppn >= 0) {
        int b = old_ppn / block_size;
        int o = old_ppn % block_size;
        Page &op = blocks[b].pages[o];
        if (op.state == VALID) {
            op.state = INVALID;
            blocks[b].valid_page_cnt--;
            blocks[b].invalid_page_cnt++;
        }
    }
    // active block이 가득 찼거나 free였으면 새로 할당
    if (active_offset >= block_size || blocks[active_block].is_free) {
        for (int b = 0; b < total_blocks; ++b) {
            if (blocks[b].is_free) {
                active_block = b;
                active_offset = 0;
                blocks[b].is_free = false;
                break;
            }
        }
    }
    int ppn = active_block * block_size + active_offset;
    Page &np = blocks[active_block].pages[active_offset];
    np.state = VALID;
    np.logical_page_num = logicalPage;
    np.data = data;
    blocks[active_block].valid_page_cnt++;
    blocks[active_block].is_free = false;
    L2P[logicalPage] = ppn;
    total_physical_writes++;
    blocks[active_block].last_write_time = static_cast<int>(total_logical_writes);
    active_offset++;
}

// 논리 페이지 읽기(read)
void GreedyFTL::readPage(int logicalPage) {
    int ppn = L2P[logicalPage];
    if (ppn < 0) {
        // 맵핑된 페이지가 없음
        std::cout << "Read Error: Logical page " << logicalPage << " not mapped" << std::endl;
        return;
    }
    int b = ppn / block_size;
    int o = ppn % block_size;
    Page &pg = blocks[b].pages[o];
    if (pg.state != VALID) {
        // 매핑은 있지만 invalid 또는 free 상태
        std::cout << "Read Error: Page invalid or free at logical " << logicalPage << std::endl;
    } else {
        std::cout << pg.data << std::endl;
    }
}

// --------------------------
// CostBenefitFTL Implementation
// --------------------------

// Cost-Benefit FTL의 GC: invalid가 가장 많은 블록 중 가장 오래된(last_write_time) 블록을 victim으로
void CostBenefitFTL::garbageCollect() {
    // invalid 페이지가 가장 많은 값 찾기
    int max_invalid = 0;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free)
            max_invalid = std::max(max_invalid, blocks[i].invalid_page_cnt);
    }
    if (max_invalid == 0) return; // 모두 유효하면 GC 필요 없음

    // max_invalid 개수를 가진 블록들 모으기
    std::vector<int> cands;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free && blocks[i].invalid_page_cnt == max_invalid)
            cands.push_back(i);
    }
    // 이들 중 가장 last_write_time이 오래된 것을 선택
    int victim = cands[0];
    int oldest = blocks[victim].last_write_time;
    for (int c : cands) {
        if (blocks[c].last_write_time < oldest) {
            victim = c;
            oldest = blocks[c].last_write_time;
        }
    }

    // valid 페이지 임시로 모으기
    std::vector<std::pair<int,int>> valid_pages;
    for (int j = 0; j < block_size; ++j) {
        Page &pg = blocks[victim].pages[j];
        if (pg.state == VALID)
            valid_pages.emplace_back(pg.logical_page_num, pg.data);
    }
    // victim 블록 erase
    for (int j = 0; j < block_size; ++j)
        blocks[victim].pages[j] = Page();
    blocks[victim].valid_page_cnt = 0;
    blocks[victim].invalid_page_cnt = 0;
    blocks[victim].is_free = true;
    blocks[victim].gc_cnt++;

    // valid 페이지 다시 기록(복구)
    for (auto &vp : valid_pages) {
        if (active_offset >= block_size) {
            for (int b = 0; b < total_blocks; ++b) {
                if (blocks[b].is_free) {
                    active_block = b;
                    active_offset = 0;
                    blocks[b].is_free = false;
                    break;
                }
            }
        }
        int lpn = vp.first;
        int dat = vp.second;
        int ppn = active_block * block_size + active_offset;
        Page &dest = blocks[active_block].pages[active_offset];
        dest.state = VALID;
        dest.logical_page_num = lpn;
        dest.data = dat;
        blocks[active_block].valid_page_cnt++;
        L2P[lpn] = ppn;
        total_physical_writes++;
        blocks[active_block].last_write_time = static_cast<int>(total_logical_writes);
        active_offset++;
    }
}

// 논리 페이지 쓰기 (CostBenefit)
void CostBenefitFTL::writePage(int logicalPage, int data) {
    total_logical_writes++;
    // free 블록 개수 체크
    int free_cnt = 0;
    for (auto &blk : blocks) if (blk.is_free) free_cnt++;
    if (free_cnt <= 2) garbageCollect();

    // old page 무효화
    int old_ppn = L2P[logicalPage];
    if (old_ppn >= 0) {
        int b = old_ppn / block_size;
        int o = old_ppn % block_size;
        Page &op = blocks[b].pages[o];
        if (op.state == VALID) {
            op.state = INVALID;
            blocks[b].valid_page_cnt--;
            blocks[b].invalid_page_cnt++;
        }
    }

    // active block이 가득 찼으면 새 할당
    if (active_offset >= block_size) {
        for (int b = 0; b < total_blocks; ++b) {
            if (blocks[b].is_free) {
                active_block = b;
                active_offset = 0;
                blocks[b].is_free = false;
                break;
            }
        }
    }
    int ppn = active_block * block_size + active_offset;
    Page &np = blocks[active_block].pages[active_offset];
    np.state = VALID;
    np.logical_page_num = logicalPage;
    np.data = data;
    blocks[active_block].valid_page_cnt++;
    blocks[active_block].is_free = false;
    L2P[logicalPage] = ppn;
    total_physical_writes++;
    blocks[active_block].last_write_time = static_cast<int>(total_logical_writes);
    active_offset++;
}

// 논리 페이지 읽기 (CostBenefit)
void CostBenefitFTL::readPage(int logicalPage) {
    int ppn = L2P[logicalPage];
    if (ppn < 0) {
        // 매핑 안됨
        std::cout << "Read Error: Logical page " << logicalPage << " not mapped" << std::endl;
        return;
    }
    int b = ppn / block_size;
    int o = ppn % block_size;
    Page &pg = blocks[b].pages[o];
    if (pg.state != VALID) {
        // invalid 혹은 free
        std::cout << "Read Error: Page invalid or free at logical " << logicalPage << std::endl;
    } else {
        std::cout << pg.data << std::endl;
    }
}
