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
    int victim = -1;
    int max_invalid = 0;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free && blocks[i].invalid_page_cnt > max_invalid) {
            max_invalid = blocks[i].invalid_page_cnt;
            victim = i;
        }
    }
    if (victim < 0 || max_invalid == 0) return;

    // Collect valid pages
    std::vector<std::pair<int,int>> valid_pages;
    for (int p = 0; p < block_size; ++p) {
        Page &pg = blocks[victim].pages[p];
        if (pg.state == VALID) valid_pages.emplace_back(pg.logical_page_num, pg.data);
    }

    // Erase victim
    for (int p = 0; p < block_size; ++p) blocks[victim].pages[p] = Page();
    blocks[victim].valid_page_cnt = 0;
    blocks[victim].invalid_page_cnt = 0;
    blocks[victim].is_free = true;
    blocks[victim].gc_cnt++;

    // Pick new active block: lowest-index free block
    active_offset = 0;
    for (int b = 0; b < total_blocks; ++b) {
        if (blocks[b].is_free) {
            active_block = b;
            blocks[b].is_free = false;
            break;
        }
    }

    // Relocate valid pages
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

// Write a page
void GreedyFTL::writePage(int logicalPage, int data) {
    total_logical_writes++;
    // Count free blocks
    int free_cnt = 0;
    for (auto &blk : blocks) if (blk.is_free) free_cnt++;
    if (free_cnt <= 2) garbageCollect();

    // Invalidate old mapping
    int old = L2P[logicalPage];
    if (old >= 0) {
        int b = old / block_size;
        int o = old % block_size;
        Page &pg = blocks[b].pages[o];
        if (pg.state == VALID) {
            pg.state = INVALID;
            blocks[b].valid_page_cnt--;
            blocks[b].invalid_page_cnt++;
        }
    }

    // If active full, find next free
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
    Page &newp = blocks[active_block].pages[active_offset];
    newp.state = VALID;
    newp.logical_page_num = logicalPage;
    newp.data = data;
    blocks[active_block].valid_page_cnt++;
    blocks[active_block].is_free = false;
    L2P[logicalPage] = ppn;
    total_physical_writes++;
    blocks[active_block].last_write_time = static_cast<int>(total_logical_writes);
    active_offset++;
}

// Read a page
void GreedyFTL::readPage(int logicalPage) {
    int ppn = L2P[logicalPage];
    if (ppn < 0) {
        std::cout << "Read Error: Logical page " << logicalPage << " not mapped" << std::endl;
        return;
    }
    int b = ppn / block_size;
    int o = ppn % block_size;
    Page &pg = blocks[b].pages[o];
    if (pg.state != VALID) {
        std::cout << "Read Error: Page invalid or free at logical " << logicalPage << std::endl;
    } else {
        std::cout << pg.data << std::endl;
    }
}

// --------------------------
// CostBenefitFTL Implementation
// --------------------------

void CostBenefitFTL::garbageCollect() {
    // Max invalid
    int max_invalid = 0;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free)
            max_invalid = std::max(max_invalid, blocks[i].invalid_page_cnt);
    }
    if (max_invalid == 0) return;

    // Candidates with max invalid
    std::vector<int> cands;
    for (int i = 0; i < total_blocks; ++i) {
        if (!blocks[i].is_free && blocks[i].invalid_page_cnt == max_invalid)
            cands.push_back(i);
    }
    // Pick oldest
    int victim = cands[0];
    int oldest = blocks[victim].last_write_time;
    for (int c : cands) {
        if (blocks[c].last_write_time < oldest) {
            victim = c;
            oldest = blocks[c].last_write_time;
        }
    }

    // Gather valid
    std::vector<std::pair<int,int>> valid_pages;
    for (int j = 0; j < block_size; ++j) {
        Page &pg = blocks[victim].pages[j];
        if (pg.state == VALID)
            valid_pages.emplace_back(pg.logical_page_num, pg.data);
    }
    // Erase
    for (int j = 0; j < block_size; ++j)
        blocks[victim].pages[j] = Page();
    blocks[victim].valid_page_cnt = 0;
    blocks[victim].invalid_page_cnt = 0;
    blocks[victim].is_free = true;
    blocks[victim].gc_cnt++;

    // Relocate
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

void CostBenefitFTL::writePage(int logicalPage, int data) {
    total_logical_writes++;
    int free_cnt = 0;
    for (auto &blk : blocks) if (blk.is_free) free_cnt++;
    if (free_cnt <= 2) garbageCollect();

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
    int b = ppn / block_size;
    int o = ppn % block_size;
    Page &pg = blocks[b].pages[o];
    if (pg.state != VALID) {
        std::cout << "Read Error: Page invalid or free at logical " << logicalPage << std::endl;
    } else {
        std::cout << pg.data << std::endl;
    }
}
