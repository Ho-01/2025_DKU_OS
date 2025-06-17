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
    // 1) Select victim block: block with the most invalid pages
    int victim = -1;
    int max_invalid = -1;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free && blocks[i].invalid_page_cnt > max_invalid) {
            max_invalid = blocks[i].invalid_page_cnt;
            victim = i;
        }
    }
    if (victim < 0) return;  // no candidate

    // 2) Gather valid pages from victim
    std::vector<std::pair<int,int>> valid_pages;
    for (int j = 0; j < block_size; ++j) {
        Page &p = blocks[victim].pages[j];
        if (p.state == VALID) {
            valid_pages.emplace_back(p.logical_page_num, p.data);
        }
    }

    // 3) Erase victim block
    for (int j = 0; j < block_size; ++j) {
        blocks[victim].pages[j] = Page();
    }
    blocks[victim].valid_page_cnt = 0;
    blocks[victim].invalid_page_cnt = 0;
    blocks[victim].is_free = true;
    blocks[victim].gc_cnt++;

    // 4) After erase, victim block is free, but keep current active block/offset
    //    Relocate valid pages into current active block or next free blocks

    // 5) Copy valid pages into active block(s) into the new active block (and beyond)
    for (auto &vp : valid_pages) {
        int logPage = vp.first;
        int dat     = vp.second;

        // Allocate new free block if current active is full
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
        // Physical write of valid page
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
    // 1) Count host write
    total_logical_writes++;

    // 2) Trigger GC if free blocks <= 2
    int free_count = 0;
    for (int i = 0; i < total_blocks; ++i) {
        if (blocks[i].is_free) ++free_count;
    }
    if (free_count <= 2) {
        garbageCollect();
    }

    // 3) Invalidate old mapping if exists
    int old_ppn = L2P[logicalPage];
    if (old_ppn >= 0) {
        int old_blk = old_ppn / block_size;
        int old_off = old_ppn % block_size;
        Page &op = blocks[old_blk].pages[old_off];
        if (op.state == VALID) {
            op.state = INVALID;
            blocks[old_blk].valid_page_cnt--;
            blocks[old_blk].invalid_page_cnt++;
        }
    }

    // 4) Allocate in active block, switch if full
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

    // 5) Count physical write & update WAF
    total_physical_writes++;
    blocks[active_block].last_write_time = (int)total_logical_writes;

    // 6) Advance write pointer
    active_offset++;
}

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

// CostBenefitFTL Implementation (unchanged)

void CostBenefitFTL::garbageCollect() {
    // 1) Determine max invalid count across non-free blocks
    int max_invalid = -1;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free) {
            max_invalid = std::max(max_invalid, blocks[i].invalid_page_cnt);
        }
    }
    if (max_invalid < 0) return;

    // 2) Collect candidates with that invalid count
    std::vector<int> candidates;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free && blocks[i].invalid_page_cnt == max_invalid) {
            candidates.push_back(i);
        }
    }
    if (candidates.empty()) return;

    // 3) Among candidates, select the oldest by last_write_time
    int victim = candidates[0];
    int min_time = blocks[victim].last_write_time;
    for (int idx : candidates) {
        if (blocks[idx].last_write_time < min_time) {
            victim = idx;
            min_time = blocks[idx].last_write_time;
        }
    }

    // 4) Gather valid pages from victim
    std::vector<std::pair<int,int>> valid_pages;
    for (int j = 0; j < block_size; ++j) {
        Page &p = blocks[victim].pages[j];
        if (p.state == VALID) {
            valid_pages.emplace_back(p.logical_page_num, p.data);
        }
    }

    // 5) Erase victim block
    for (int j = 0; j < block_size; ++j) {
        blocks[victim].pages[j] = Page();
    }
    blocks[victim].valid_page_cnt = 0;
    blocks[victim].invalid_page_cnt = 0;
    blocks[victim].is_free = true;
    blocks[victim].gc_cnt++;

    // 6) Reset active block to victim
    active_block = victim;
    active_offset = 0;
    blocks[active_block].is_free = false;

    // 7) Copy valid pages
    for (auto &vp : valid_pages) {
        int logPage = vp.first;
        int dat     = vp.second;
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
        blocks[active_block].pages[active_offset].state = VALID;
        blocks[active_block].pages[active_offset].logical_page_num = logPage;
        blocks[active_block].pages[active_offset].data = dat;
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
    for (int i = 0; i < total_blocks; ++i) {
        if (blocks[i].is_free) ++free_count;
    }
    if (free_count <= 2) {
        garbageCollect();
    }
    int old_ppn = L2P[logicalPage];
    if (old_ppn >= 0) {
        int old_blk = old_ppn / block_size;
        int old_off = old_ppn % block_size;
        Page &op = blocks[old_blk].pages[old_off];
        if (op.state == VALID) {
            op.state = INVALID;
            blocks[old_blk].valid_page_cnt--;
            blocks[old_blk].invalid_page_cnt++;
        }
    }
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
    blocks[active_block].last_write_time = (int)total_logical_writes;
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
