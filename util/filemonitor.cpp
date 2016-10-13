#include "filemonitor.h"
#include <sys/stat.h>
#include <iostream>
#include <thread>

FileMonitor::FileMonitor(): modified_files{}, monitoring(false), watched_files{}, internal_lock{},
                            files_modification_time{} {}

void FileMonitor::poll_files() {
    using namespace std::chrono;
    while (true) {
        internal_lock.lock();
        if (!monitoring) { internal_lock.unlock(); break; }
        for (auto filepath : watched_files) {
            struct stat stats;
            stat(filepath.c_str(), &stats);
            if (files_modification_time.count(filepath) == 0 ) { // First time checked
                files_modification_time[filepath] = (uint64_t) stats.st_mtimespec.tv_sec;
                continue;
            }
            auto old_timestamp = files_modification_time[filepath];
            auto new_timestamp = stats.st_mtimespec.tv_sec;
            if (old_timestamp == new_timestamp) { continue; }
            files_modfied = true;
            modified_files[filepath] = true;
            files_modification_time[filepath] = (uint64_t) stats.st_mtimespec.tv_sec;
            std::cout << stats.st_mtimespec.tv_sec << std::endl;
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

void FileMonitor::clear_modification_flag(std::string filepath) {
    internal_lock.lock();
    modified_files[filepath] = false;
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

std::vector<std::string> FileMonitor::get_modified_files() {
    internal_lock.lock();
    std::vector<std::string> modified_files_to_return{};
    for (auto file : modified_files) {
        if (file.second) {
            modified_files_to_return.push_back(file.first);
        }
    }
    internal_lock.unlock();
    return modified_files_to_return;
}
