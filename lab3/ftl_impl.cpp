/*
*	DKU Operating System Lab
*	    Lab3 (Flash Translation Layer)
*	    Student id : 32190789
*	    Student name : 김승호
*	    Date : 2025-06-18
*/

#include "ftl_impl.h"
#include <algorithm>
#include <limits>
#include <vector>
#include <iostream>

void GreedyFTL::garbageCollect() {
    // Select victim block with maximum invalid pages
    int victim = -1;
    int max_invalid = -1;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free && blocks[i].invalid_page_cnt > max_invalid) {
            max_invalid = blocks[i].invalid_page_cnt;
            victim = i;
        }
    }
    if (victim < 0) return;
    
    // Collect valid pages
    std::vector<std::pair<int,int>> valid_pages;
    for (int j = 0; j < block_size; ++j) {
        Page &p = blocks[victim].pages[j];
        if (p.state == VALID) {
            valid_pages.emplace_back(p.logical_page_num, p.data);
        }
    }
    
    // Erase victim block
    for (int j = 0; j < block_size; ++j) blocks[victim].pages[j] = Page();
    blocks[victim].valid_page_cnt = 0;
    blocks[victim].invalid_page_cnt = 0;
    blocks[victim].is_free = true;
    blocks[victim].gc_cnt++;
    
    // Relocate valid pages starting from current active
    for (auto &vp : valid_pages) {
        if (active_offset >= block_size) {
            // find next free block circularly
            for (int k = 1; k <= total_blocks; ++k) {
                int b = (active_block + k) % total_blocks;
                if (blocks[b].is_free) {
                    active_block = b;
                    active_offset = 0;
                    blocks[b].is_free = false;
                    break;
                }
            }
        }
        int logPage = vp.first;
        int dat = vp.second;
        int ppn = active_block * block_size + active_offset;
        Page &np = blocks[active_block].pages[active_offset];
        np.state = VALID;
        np.logical_page_num = logPage;
        np.data = dat;
        blocks[active_block].valid_page_cnt++;
        L2P[logPage] = ppn;
        total_physical_writes++;
        blocks[active_block].last_write_time = (int)total_logical_writes;
        active_offset++;
    }
}

void GreedyFTL::writePage(int logicalPage, int data) {
    total_logical_writes++;
    // check free blocks
    int free_count = 0;
    for (auto &blk : blocks) if (blk.is_free) free_count++;
    if (free_count <= 2) garbageCollect();
    
    // invalidate old page
    int old_ppn = L2P[logicalPage];
    if (old_ppn >= 0) {
        int ob = old_ppn / block_size;
        int oo = old_ppn % block_size;
        Page &op = blocks[ob].pages[oo];
        if (op.state == VALID) {
            op.state = INVALID;
            blocks[ob].valid_page_cnt--;
            blocks[ob].invalid_page_cnt++;
        }
    }
    
    // allocate in active
    if (active_offset >= block_size) {
        for (int i = 0; i < total_blocks; ++i) {
            if (blocks[i].is_free) {
                active_block = i;
                active_offset = 0;
                blocks[i].is_free = false;
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
    blocks[active_block].last_write_time = (int)total_logical_writes;
    active_offset++;
}

void GreedyFTL::readPage(int logicalPage) {
    int ppn = L2P[logicalPage];
    if (ppn < 0) {
        std::cout << "Read Error: Logical page " << logicalPage << " not mapped" << std::endl;
        return;
    }
    int b = ppn / block_size;
    int o = ppn % block_size;
    Page &p = blocks[b].pages[o];
    if (p.state != VALID) {
        std::cout << "Read Error: Page invalid or free at logical " << logicalPage << std::endl;
    } else {
        std::cout << p.data << std::endl;
    }
}

// CostBenefitFTL Implementation

void CostBenefitFTL::garbageCollect() {
    int max_invalid = -1;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free)
            max_invalid = std::max(max_invalid, blocks[i].invalid_page_cnt);
    }
    if (max_invalid < 0) return;
    std::vector<int> cands;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free && blocks[i].invalid_page_cnt == max_invalid)
            cands.push_back(i);
    }
    if (cands.empty()) return;
    int victim = cands[0];
    int min_time = blocks[victim].last_write_time;
    for (int idx : cands) {
        if (blocks[idx].last_write_time < min_time) {
            victim = idx;
            min_time = blocks[idx].last_write_time;
        }
    }
    std::vector<std::pair<int,int>> valid_pages;
    for (int j = 0; j < block_size; ++j) {
        Page &p = blocks[victim].pages[j];
        if (p.state == VALID) valid_pages.emplace_back(p.logical_page_num, p.data);
    }
    for (int j = 0; j < block_size; ++j) blocks[victim].pages[j] = Page();
    blocks[victim].valid_page_cnt = 0;
    blocks[victim].invalid_page_cnt = 0;
    blocks[victim].is_free = true;
    blocks[victim].gc_cnt++;
    for (auto &vp : valid_pages) {
        if (active_offset >= block_size) {
            for (int i = 0; i < total_blocks; ++i) {
                if (blocks[i].is_free) {
                    active_block = i;
                    active_offset = 0;
                    blocks[i].is_free = false;
                    break;
                }
            }
        }
        int logPage = vp.first;
        int dat = vp.second;
        int ppn = active_block * block_size + active_offset;
        Page &np = blocks[active_block].pages[active_offset];
        np.state = VALID;
        np.logical_page_num = logPage;
        np.data = dat;
        blocks[active_block].valid_page_cnt++;
        L2P[logPage] = ppn;
        total_physical_writes++;
        blocks[active_block].last_write_time = (int)total_logical_writes;
        active_offset++;
    }
}

void CostBenefitFTL::writePage(int logicalPage, int data) {
    total_logical_writes++;
    int free_count = 0;
    for (auto &blk : blocks) if (blk.is_free) free_count++;
    if (free_count <= 2) garbageCollect();
    int old_ppn = L2P[logicalPage];
    if (old_ppn >= 0) {
        int ob = old_ppn / block_size;
        int oo = old_ppn % block_size;
        Page &op = blocks[ob].pages[oo];
        if (op.state == VALID) {
            op.state = INVALID;
            blocks[ob].valid_page_cnt--;
            blocks[ob].invalid_page_cnt++;
        }
    }
    if (active_offset >= block_size) {
        for (int i = 0; i < total_blocks; ++i) {
            if (blocks[i].is_free) {
                active_block = i;
                active_offset = 0;
                blocks[i].is_free = false;
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
    blocks[active_block].last_write_time = (int)total_logical_writes;
    active_offset++;
}

void CostBenefitFTL::readPage(int logicalPage) {
    int ppn = L2P[logicalPage];
    if (ppn < 0) {
        std::cout << "Read Error: Logical page " << logicalPage << " not mapped" << std::endl;
        return;
    }
    int b = ppn / block_size;
    int o = ppn % block_size;
    Page &p = blocks[b].pages[o];
    if (p.state != VALID) {
        std::cout << "Read Error: Page invalid or free at logical " << logicalPage << std::endl;
    } else {
        std::cout << p.data << std::endl;
    }
}
