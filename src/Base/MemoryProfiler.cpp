#include "MemoryProfiler.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/resource.h>
#elif defined(__linux__)
#include <fstream>
#include <sstream>
#include <string>
#endif

namespace RetroRenderer {
ProcessMemorySnapshot MemoryProfiler::SampleProcessMemory() {
    ProcessMemorySnapshot snapshot{};

#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                             sizeof(counters)) != 0) {
        snapshot.residentBytes = static_cast<uint64_t>(counters.WorkingSetSize);
        snapshot.peakResidentBytes = static_cast<uint64_t>(counters.PeakWorkingSetSize);
        snapshot.supported = true;
    }
#elif defined(__APPLE__)
    task_basic_info_data_t taskInfo{};
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(),
                  TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&taskInfo),
                  &count) == KERN_SUCCESS) {
        snapshot.residentBytes = static_cast<uint64_t>(taskInfo.resident_size);
        snapshot.supported = true;
    }

    struct rusage usage {};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        snapshot.peakResidentBytes = static_cast<uint64_t>(usage.ru_maxrss);
        snapshot.supported = true;
    }
#elif defined(__linux__)
    std::ifstream status("/proc/self/status");
    if (status.is_open()) {
        std::string line;
        while (std::getline(status, line)) {
            auto parseStatusKb = [&](const char* prefix, uint64_t& outBytes) {
                const std::string key(prefix);
                if (line.rfind(key, 0) != 0) {
                    return false;
                }
                std::istringstream stream(line.substr(key.size()));
                uint64_t valueKb = 0;
                std::string unit;
                if (!(stream >> valueKb >> unit)) {
                    return false;
                }
                if (unit != "kB") {
                    return false;
                }
                outBytes = valueKb * 1024ULL;
                return true;
            };

            uint64_t parsedBytes = 0;
            if (parseStatusKb("VmRSS:", parsedBytes)) {
                snapshot.residentBytes = parsedBytes;
                snapshot.supported = true;
                continue;
            }
            if (parseStatusKb("VmHWM:", parsedBytes)) {
                snapshot.peakResidentBytes = parsedBytes;
                snapshot.supported = true;
            }
        }
    }
#endif

    return snapshot;
}
} // namespace RetroRenderer
