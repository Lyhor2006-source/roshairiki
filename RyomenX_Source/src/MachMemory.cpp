#include "MachMemory.hpp"
#include <cstring>
#include <chrono>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach-o/dyld.h>
#endif

namespace MachMemory {

static RegionMode s_current_mode = RegionMode::OTHER;
static std::mutex s_scan_mutex;
static std::string s_scan_diag = "OK";

// Freeze lifecycle
static std::mutex s_freeze_mutex;
static std::map<std::string, FreezeEntry> s_freeze_entries;
static std::atomic<bool> s_freeze_running{false};
static std::thread s_freeze_thread;

void ResetScanDiagnostics() {
    std::lock_guard<std::mutex> lock(s_scan_mutex);
    s_scan_diag = "OK";
}

std::string GetScanDiagnostics() {
    std::lock_guard<std::mutex> lock(s_scan_mutex);
    return s_scan_diag;
}

void SetRegionMode(RegionMode mode) {
    s_current_mode = mode;
}

RegionMode GetRegionMode() {
    return s_current_mode;
}

const char* GetRegionName(RegionMode mode) {
    switch (mode) {
        case RegionMode::OTHER: return "OTHER";
        case RegionMode::ANONYMOUS: return "ANONYMOUS";
        case RegionMode::C_ALLOC: return "C_ALLOC";
        case RegionMode::CODE_APP: return "CODE_APP";
        default: return "UNKNOWN";
    }
}

bool ReadMemory(uintptr_t address, void* buffer, size_t size) {
    if (!address || !buffer || size == 0) return false;
#if defined(__APPLE__)
    vm_size_t bytes_read = 0;
    kern_return_t kr = vm_read_overwrite(mach_task_self(),
                                         static_cast<vm_address_t>(address),
                                         static_cast<vm_size_t>(size),
                                         reinterpret_cast<vm_address_t>(buffer),
                                         &bytes_read);
    return (kr == KERN_SUCCESS && bytes_read == size);
#else
    // Windows/Sim fallback
    std::memcpy(buffer, reinterpret_cast<const void*>(address), size);
    return true;
#endif
}

bool WriteMemory(uintptr_t address, const void* buffer, size_t size) {
    if (!address || !buffer || size == 0) return false;
#if defined(__APPLE__)
    mach_port_t task = mach_task_self();
    vm_address_t page_addr = address & ~static_cast<uintptr_t>(PAGE_SIZE - 1);
    vm_size_t page_size = ((address + size) - page_addr + PAGE_SIZE - 1) & ~static_cast<uintptr_t>(PAGE_SIZE - 1);

    // Ensure page is writable
    vm_protect(task, page_addr, page_size, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);

    kern_return_t kr = vm_write(task, static_cast<vm_address_t>(address),
                                reinterpret_cast<vm_offset_t>(buffer),
                                static_cast<mach_msg_type_number_t>(size));

    return (kr == KERN_SUCCESS);
#else
    std::memcpy(reinterpret_cast<void*>(address), buffer, size);
    return true;
#endif
}

std::vector<MemoryRegion> QueryRegions(RegionMode mode) {
    std::vector<MemoryRegion> regions;
#if defined(__APPLE__)
    mach_port_t task = mach_task_self();
    vm_address_t address = 0;
    vm_size_t size = 0;
    natural_t depth = 0;

    while (true) {
        vm_region_submap_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
        kern_return_t kr = vm_region_recurse_64(task, &address, &size, &depth,
                                                reinterpret_cast<vm_region_recurse_info_t>(&info), &count);
        if (kr != KERN_SUCCESS) break;

        if (info.is_submap) {
            depth++;
            continue;
        }

        bool writable = (info.protection & VM_PROT_WRITE) != 0;
        if (writable) {
            bool matches_mode = false;
            switch (mode) {
                case RegionMode::OTHER:
                    matches_mode = true;
                    break;
                case RegionMode::ANONYMOUS:
                    matches_mode = (info.user_tag == 0 || info.user_tag == VM_MEMORY_MALLOC_TINY || info.user_tag == VM_MEMORY_MALLOC_SMALL);
                    break;
                case RegionMode::C_ALLOC:
                    matches_mode = (info.user_tag >= VM_MEMORY_MALLOC && info.user_tag <= VM_MEMORY_MALLOC_NANO);
                    break;
                case RegionMode::CODE_APP:
                    matches_mode = (info.user_tag == VM_MEMORY_APPLICATION_SPECIFIC_1 || info.user_tag == 0);
                    break;
            }

            if (matches_mode) {
                MemoryRegion r;
                r.address = address;
                r.size = size;
                r.protection = info.protection;
                r.max_protection = info.max_protection;
                r.user_tag = info.user_tag;
                regions.push_back(r);
            }
        }

        address += size;
    }
#endif
    return regions;
}

std::vector<MemoryRegion> QueryAllWritableRegions() {
    return QueryRegions(RegionMode::OTHER);
}

std::vector<uintptr_t> ScanDword(const std::vector<MemoryRegion>& regions, uint32_t value, uintptr_t alignment) {
    std::vector<uintptr_t> results;
    std::vector<uint8_t> buffer;

    for (const auto& reg : regions) {
        if (reg.size < sizeof(uint32_t)) continue;
        buffer.resize(reg.size);

        if (!ReadMemory(reg.address, buffer.data(), reg.size)) continue;

        const uint8_t* ptr = buffer.data();
        size_t limit = reg.size - sizeof(uint32_t);

        for (size_t offset = 0; offset <= limit; offset += alignment) {
            uint32_t cur = *reinterpret_cast<const uint32_t*>(ptr + offset);
            if (cur == value) {
                results.push_back(reg.address + offset);
            }
        }
    }
    return results;
}

std::vector<uintptr_t> ScanQword(const std::vector<MemoryRegion>& regions, uint64_t value, uintptr_t alignment) {
    std::vector<uintptr_t> results;
    std::vector<uint8_t> buffer;

    for (const auto& reg : regions) {
        if (reg.size < sizeof(uint64_t)) continue;
        buffer.resize(reg.size);

        if (!ReadMemory(reg.address, buffer.data(), reg.size)) continue;

        const uint8_t* ptr = buffer.data();
        size_t limit = reg.size - sizeof(uint64_t);

        for (size_t offset = 0; offset <= limit; offset += alignment) {
            uint64_t cur = *reinterpret_cast<const uint64_t*>(ptr + offset);
            if (cur == value) {
                results.push_back(reg.address + offset);
            }
        }
    }
    return results;
}

std::vector<uintptr_t> ScanFloat(const std::vector<MemoryRegion>& regions, float value, float epsilon) {
    std::vector<uintptr_t> results;
    std::vector<uint8_t> buffer;

    for (const auto& reg : regions) {
        if (reg.size < sizeof(float)) continue;
        buffer.resize(reg.size);

        if (!ReadMemory(reg.address, buffer.data(), reg.size)) continue;

        const uint8_t* ptr = buffer.data();
        size_t limit = reg.size - sizeof(float);

        for (size_t offset = 0; offset <= limit; offset += 4) {
            float cur = *reinterpret_cast<const float*>(ptr + offset);
            if (std::abs(cur - value) <= epsilon) {
                results.push_back(reg.address + offset);
            }
        }
    }
    return results;
}

// Group Proximity Scan: Searches for a cluster of SearchItems situated within max_distance bytes
std::vector<uintptr_t> GroupScan(
    const std::vector<MemoryRegion>& regions,
    const std::vector<SearchItem>& items,
    size_t match_index,
    size_t max_distance,
    uintptr_t alignment
) {
    std::vector<uintptr_t> results;
    if (items.empty() || match_index >= items.size()) return results;

    std::vector<uint8_t> buffer;

    for (const auto& reg : regions) {
        if (reg.size < max_distance) continue;
        buffer.resize(reg.size);

        if (!ReadMemory(reg.address, buffer.data(), reg.size)) continue;

        const uint8_t* ptr = buffer.data();
        size_t limit = reg.size - max_distance;

        for (size_t offset = 0; offset <= limit; offset += alignment) {
            // Check if primary anchor item matches at current offset
            const SearchItem& anchor = items[match_index];
            bool anchor_match = false;

            if (anchor.type == 0) {
                anchor_match = (*reinterpret_cast<const uint32_t*>(ptr + offset) == anchor.dword_val);
            } else if (anchor.type == 1) {
                anchor_match = (*reinterpret_cast<const uint64_t*>(ptr + offset) == anchor.qword_val);
            } else if (anchor.type == 2) {
                anchor_match = (std::abs(*reinterpret_cast<const float*>(ptr + offset) - anchor.float_val) < 0.001f);
            }

            if (!anchor_match) continue;

            // Anchor matched! Check if all other items exist in proximity window [offset - max_distance, offset + max_distance]
            size_t window_start = (offset > max_distance) ? (offset - max_distance) : 0;
            size_t window_end = std::min(offset + max_distance, reg.size - sizeof(uint32_t));

            bool all_found = true;
            for (size_t i = 0; i < items.size(); ++i) {
                if (i == match_index) continue;

                const SearchItem& target = items[i];
                bool found_target = false;

                for (size_t w = window_start; w <= window_end; w += alignment) {
                    if (target.type == 0) {
                        if (*reinterpret_cast<const uint32_t*>(ptr + w) == target.dword_val) {
                            found_target = true;
                            break;
                        }
                    } else if (target.type == 1) {
                        if (w + sizeof(uint64_t) <= reg.size && *reinterpret_cast<const uint64_t*>(ptr + w) == target.qword_val) {
                            found_target = true;
                            break;
                        }
                    } else if (target.type == 2) {
                        if (std::abs(*reinterpret_cast<const float*>(ptr + w) - target.float_val) < 0.001f) {
                            found_target = true;
                            break;
                        }
                    }
                }

                if (!found_target) {
                    all_found = false;
                    break;
                }
            }

            if (all_found) {
                results.push_back(reg.address + offset);
            }
        }
    }
    return results;
}

std::vector<uintptr_t> GroupScanMany(
    const std::vector<MemoryRegion>& regions,
    const std::vector<SearchItem>& items,
    const std::vector<size_t>& match_indices,
    size_t max_distance,
    uintptr_t alignment
) {
    if (match_indices.empty()) return {};
    return GroupScan(regions, items, match_indices[0], max_distance, alignment);
}

void PatchState::Clear() {
    patches.clear();
}

bool PatchState::AddAndApply(uintptr_t address, const void* new_data, size_t size) {
    SavedPatch p;
    p.address = address;
    p.original_bytes.resize(size);

    if (!ReadMemory(address, p.original_bytes.data(), size)) {
        return false;
    }

    if (!WriteMemory(address, new_data, size)) {
        return false;
    }

    patches.push_back(p);
    return true;
}

bool PatchState::RestoreAll() {
    bool all_ok = true;
    for (const auto& p : patches) {
        if (!WriteMemory(p.address, p.original_bytes.data(), p.original_bytes.size())) {
            all_ok = false;
        }
    }
    patches.clear();
    return all_ok;
}

void RegisterFreeze(const std::string& name, uintptr_t address, const void* value, size_t size) {
    std::lock_guard<std::mutex> lock(s_freeze_mutex);
    FreezeEntry e;
    e.address = address;
    e.size = size;
    e.value.assign(reinterpret_cast<const uint8_t*>(value), reinterpret_cast<const uint8_t*>(value) + size);
    e.active = true;
    s_freeze_entries[name] = e;
    StartFreezeThread();
}

void UnregisterFreeze(const std::string& name) {
    std::lock_guard<std::mutex> lock(s_freeze_mutex);
    s_freeze_entries.erase(name);
    if (s_freeze_entries.empty()) {
        StopFreezeThread();
    }
}

void StartFreezeThread() {
    if (s_freeze_running) return;
    s_freeze_running = true;
    s_freeze_thread = std::thread([]() {
        while (s_freeze_running) {
            {
                std::lock_guard<std::mutex> lock(s_freeze_mutex);
                for (auto& kv : s_freeze_entries) {
                    if (kv.second.active && kv.second.address != 0) {
                        WriteMemory(kv.second.address, kv.second.value.data(), kv.second.size);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
}

void StopFreezeThread() {
    if (!s_freeze_running) return;
    s_freeze_running = false;
    if (s_freeze_thread.joinable()) {
        s_freeze_thread.join();
    }
}

} // namespace MachMemory
