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

void GreedyFTL::garbageCollect() {
    // 1. Find victim
    int victim = -1;
    int max_invalid = 0;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free && blocks[i].invalid_page_cnt > max_invalid) {
            max_invalid = blocks[i].invalid_page_cnt;
            victim = i;
        }
    }
    // No invalid pages to collect
    if (victim < 0 || max_invalid == 0) return;

    // 2. Gather valid pages from victim
    std::vector<std::pair<int,int>> valid_pages;
    for (int j = 0; j < block_size; ++j) {
        Page &p = blocks[victim].pages[j];
        if (p.state == VALID) {
            valid_pages.emplace_back(p.logical_page_num, p.data);
        }
    }

    // 3. Erase victim block
    for (int j = 0; j < block_size; ++j) {
        blocks[victim].pages[j] = Page();
    }
    blocks[victim].valid_page_cnt = 0;
    blocks[victim].invalid_page_cnt = 0;
    blocks[victim].is_free = true;
    blocks[victim].gc_cnt++;

    // 4. Reset active pointer to victim
    active_block = victim;
    active_offset = 0;
    blocks[active_block].is_free = false;

    // 5. Relocate valid pages into free space
    for (auto &vp : valid_pages) {
        if (active_offset >= block_size) {
            // find next free block
            for (int b = 0; b < total_blocks; ++b) {
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
        blocks[active_block].last_write_time = static_cast<int>(total_logical_writes);
        active_offset++;
    }
}

// Write operation
void GreedyFTL::writePage(int logicalPage, int data) {
    // 1. Count logical write
    total_logical_writes++;

    // 2. Trigger GC if free blocks are 2 or fewer
    int free_cnt = 0;
    for (auto &blk : blocks) if (blk.is_free) free_cnt++;
    if (free_cnt <= 2) garbageCollect();

    // 3. Invalidate old page
    int old_ppn = L2P[logicalPage];
    if (old_ppn >= 0) {
        int ob = old_ppn / block_size;
        int oo = old_ppn % block_size;
        Page &oldp = blocks[ob].pages[oo];
        if (oldp.state == VALID) {
            oldp.state = INVALID;
            blocks[ob].valid_page_cnt--;
            blocks[ob].invalid_page_cnt++;
        }
    }

    // 4. Allocate in active block
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

    // 5. Count physical write
    total_physical_writes++;
    blocks[active_block].last_write_time = static_cast<int>(total_logical_writes);
    active_offset++;
}

// Read operation
void GreedyFTL::readPage(int logicalPage) {
    int ppn = L2P[logicalPage];
    if (ppn < 0) {
        std::cout << "Read Error: Logical page " << logicalPage << " not mapped" << std::endl;
        return;
    }
    int blk = ppn / block_size;
    int off = ppn % block_size;
    Page &p = blocks[blk].pages[off];
    if (p.state != VALID) {
        std::cout << "Read Error: Page invalid or free at logical " << logicalPage << std::endl;
    } else {
        std::cout << p.data << std::endl;
    }
}

// ==============================
// CostBenefitFTL Implementation
// ==============================

void CostBenefitFTL::garbageCollect() {
    // 1. Find max invalid count
    int max_invalid = 0;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free)
            max_invalid = std::max(max_invalid, blocks[i].invalid_page_cnt);
    }
    if (max_invalid == 0) return;

    // 2. Collect candidates
    std::vector<int> candidates;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free && blocks[i].invalid_page_cnt == max_invalid)
            candidates.push_back(i);
    }
    // 3. Select oldest by last_write_time
    int victim = candidates[0];
    int min_time = blocks[victim].last_write_time;
    for (int idx : candidates) {
        if (blocks[idx].last_write_time < min_time) {
            victim = idx;
            min_time = blocks[idx].last_write_time;
        }
    }

    // 4. Gather valid pages
    std::vector<std::pair<int,int>> valid_pages;
    for (int j = 0; j < block_size; ++j) {
        Page &p = blocks[victim].pages[j];
        if (p.state == VALID)
            valid_pages.emplace_back(p.logical_page_num, p.data);
    }

    // 5. Erase victim
    for (int j = 0; j < block_size; ++j)
        blocks[victim].pages[j] = Page();
    blocks[victim].valid_page_cnt = 0;
    blocks[victim].invalid_page_cnt = 0;
    blocks[victim].is_free = true;
    blocks[victim].gc_cnt++;

    // 6. Relocate valid pages
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
        blocks[active_block].last_write_time = static_cast<int>(total_logical_writes);
        active_offset++;
    }
}

void CostBenefitFTL::writePage(int logicalPage, int data) {
    total_logical_writes++;
    // GC trigger
    int free_cnt = 0;
    for (auto &blk : blocks) if (blk.is_free) free_cnt++;
    if (free_cnt <= 2) garbageCollect();

    // Invalidate old
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

    // Allocate write
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

void CostBenefitFTL::readPage(int logicalPage) {
    int ppn = L2P[logicalPage];
    if (ppn < 0) {
        std::cout << "Read Error: Logical page " << logicalPage << " not mapped" << std::endl;
        return;
    }
    int blk = ppn / block_size;
    int off = ppn % block_size;
    Page &p = blocks[blk].pages[off];
    if (p.state != VALID) {
        std::cout << "Read Error: Page invalid or free at logical " << logicalPage << std::endl;
    } else {
        std::cout << p.data << std::endl;
    }
}
