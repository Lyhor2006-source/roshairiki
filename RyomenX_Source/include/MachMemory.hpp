#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <mutex>
#include <map>
#include <thread>
#include <atomic>

namespace MachMemory {

enum class RegionMode : int {
    OTHER = 0,
    ANONYMOUS = 1,
    C_ALLOC = 2,
    CODE_APP = 3
};

struct MemoryRegion {
    uintptr_t address = 0;
    size_t size = 0;
    uint32_t protection = 0;
    uint32_t max_protection = 0;
    uint32_t user_tag = 0;
};

struct SearchItem {
    uint32_t type = 0; // 0 = dword, 1 = qword / int64, 2 = float / double
    uint32_t reserved = 0;
    union {
        uint32_t dword_val;
        uint64_t qword_val;
        float float_val;
        double double_val;
    };

    SearchItem() : type(0), reserved(0), qword_val(0) {}
    SearchItem(uint32_t val) : type(0), reserved(0), dword_val(val) {}
    SearchItem(uint64_t val) : type(1), reserved(0), qword_val(val) {}
    SearchItem(float val) : type(2), reserved(0), float_val(val) {}
};

struct SavedPatch {
    uintptr_t address = 0;
    std::vector<uint8_t> original_bytes;
};

class PatchState {
public:
    std::vector<SavedPatch> patches;

    void Clear();
    bool AddAndApply(uintptr_t address, const void* new_data, size_t size);
    bool RestoreAll();
};

struct FreezeEntry {
    uintptr_t address = 0;
    std::vector<uint8_t> value;
    size_t size = 0;
    bool active = true;
};

// Low-level Mach VM Memory operations
void ResetScanDiagnostics();
std::string GetScanDiagnostics();
void SetRegionMode(RegionMode mode);
RegionMode GetRegionMode();
const char* GetRegionName(RegionMode mode);

bool ReadMemory(uintptr_t address, void* buffer, size_t size);
bool WriteMemory(uintptr_t address, const void* buffer, size_t size);

std::vector<MemoryRegion> QueryRegions(RegionMode mode);
std::vector<MemoryRegion> QueryAllWritableRegions();

// Memory scanning routines
std::vector<uintptr_t> ScanDword(const std::vector<MemoryRegion>& regions, uint32_t value, uintptr_t alignment = 4);
std::vector<uintptr_t> ScanQword(const std::vector<MemoryRegion>& regions, uint64_t value, uintptr_t alignment = 8);
std::vector<uintptr_t> ScanFloat(const std::vector<MemoryRegion>& regions, float value, float epsilon = 0.001f);

// Multi-value Group Proximity Scanning (the core engine used by RyomenX for all weapon and movement cheats)
std::vector<uintptr_t> GroupScan(
    const std::vector<MemoryRegion>& regions,
    const std::vector<SearchItem>& items,
    size_t match_index,
    size_t max_distance,
    uintptr_t alignment = 4
);

std::vector<uintptr_t> GroupScanMany(
    const std::vector<MemoryRegion>& regions,
    const std::vector<SearchItem>& items,
    const std::vector<size_t>& match_indices,
    size_t max_distance,
    uintptr_t alignment = 4
);

// Freeze thread operations
void RegisterFreeze(const std::string& name, uintptr_t address, const void* value, size_t size);
void UnregisterFreeze(const std::string& name);
void StartFreezeThread();
void StopFreezeThread();

} // namespace MachMemory
