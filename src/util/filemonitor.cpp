#include "filemonitor.hpp"
#include <sys/stat.h>
#include <thread>

FileMonitor::FileMonitor(): monitoring(false), internal_lock{},
                            files_modfied(false), modified_files{}, watched_files{}, files_modification_time{} {}

void FileMonitor::poll_files() {
    using namespace std::chrono;
    while (true) {
        internal_lock.lock();
        if (!monitoring) { internal_lock.unlock(); break; }
        for (auto &filepath : watched_files) {
            struct stat stats;
            stat(filepath.c_str(), &stats);
            if (files_modification_time.count(filepath) == 0) { // First time checked
                files_modification_time[filepath] = get_time_modified(stats);
                continue;
            }
            auto old_timestamp = files_modification_time[filepath];
            auto new_timestamp = get_time_modified(stats);
            if (old_timestamp == new_timestamp) { continue; }
            files_modfied = true;
            modified_files[filepath] = true;
            files_modification_time[filepath] = get_time_modified(stats);
        }
        internal_lock.unlock();
        std::this_thread::sleep_for(milliseconds(500));
    }
}

void FileMonitor::add_file(std::string filepath) {
    internal_lock.lock();
    watched_files.push_back(filepath);
    internal_lock.unlock();
}

void FileMonitor::clear_all_modification_flags() {
    internal_lock.lock();
    for (auto filepath : modified_files) {
        modified_files[filepath.first] = false;
    }
    files_modfied = false;
    internal_lock.unlock();
}

void FileMonitor::end_monitor() {
    if (monitoring) {
        internal_lock.lock();
        monitoring = false;
        internal_lock.unlock();
    }
}

void FileMonitor::start_monitor() {
    if (!monitoring) {
        monitoring = true;
        std::thread t1{[=]{ poll_files(); }};
        t1.detach();
    }
}

uint64_t FileMonitor::get_time_modified(struct stat &stats) {
#if defined(__APPLE__)
    return (uint64_t) stats.st_mtimespec.tv_sec;
#elif defined(__linux__)
    return (uint64_t) stats.st_mtim.tv_sec;
#elif defined(WIN32) 
	return 0; // TODO: Implement
#else
#error Unknown platform.
#endif
}
