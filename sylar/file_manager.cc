#include "file_manager.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sylar/log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

FileInfo::FileInfo()
    :m_mtime(0)
    ,m_size(0)
    ,m_loadTime(0) {
}

void FileInfo::clear() {
    std::string().swap(m_data);
}

std::string FileInfo::toString() const {
    std::stringstream ss;
    ss << "[FileInfo path=" << m_path
       << " md5=" << m_md5
       << " data_size=" << m_size
       << " mtime=" << sylar::Time2Str(m_mtime)
       << " load_time=" << sylar::Time2Str(m_loadTime)
       << "]";
    return ss.str();
}

FileInfo::ptr FileInfoManager::load(const std::string& path, uint64_t limit) {
    uint64_t ts = sylar::GetCurrentUS();
    do {
        struct stat st;
        if(stat(path.c_str(), &st) != 0) {
            SYLAR_LOG_ERROR(g_logger) << "stat(" << path << ") error:" << strerror(errno);
            break;
        }
        FileInfo::ptr info = get(path);
        if(info && info->m_mtime == (uint64_t)st.st_mtime) {
            break;
        }
        std::ifstream ifs_md5(path + ".md5");
        std::string md5_value;
        std::getline(ifs_md5, md5_value);
        if(md5_value.empty()) {
            SYLAR_LOG_ERROR(g_logger) << path << ".md5 empty";
            break;
        }

        if(info && info->m_md5 == md5_value) {
            break;
        }

        std::ifstream ifs(path, std::ios::binary);
        FileInfo::ptr new_info(new FileInfo);
        new_info->m_path = path;
        new_info->m_data.resize(st.st_size);
        if(!sylar::ReadFixFromStreamWithSpeed(ifs, &new_info->m_data[0]
                    ,new_info->m_data.size(), limit)) {
            SYLAR_LOG_ERROR(g_logger) << path << " ReadFixFromStreamWithSpeed error: " << strerror(errno);
            break;
        }
        std::string md5sum = sylar::md5(new_info->m_data);
        if(md5sum != md5_value) {
            SYLAR_LOG_ERROR(g_logger) << "filename=" << path << " md5=" << md5sum
                        << " not (" << md5_value << ")";
            break;
        }
        new_info->m_md5 = md5sum;
        new_info->m_mtime = (uint64_t)st.st_mtime;
        new_info->m_size = (uint64_t)st.st_size;
        SYLAR_LOG_INFO(g_logger) << "load " << path << " speed=" << (int64_t)limit
                   << " size=" << new_info->m_size
                   << " ok. used: " << (sylar::GetCurrentUS() - ts) / 1000.0 << "ms";
        return new_info;
    } while(0);
    SYLAR_LOG_INFO(g_logger) << "load " << path << " speed=" << (int64_t)limit
               << " fail. used: " << (sylar::GetCurrentUS() - ts) / 1000.0 << "ms"
               << " " << strerror(errno);
    return nullptr;
}

bool FileInfoManager::load(const std::string& path, callback cb, uint64_t limit, bool auto_clear) {
    FileInfo::ptr info = load(path, limit);
    if(info && !info->m_data.empty()) {
        cb(info);
        add(info, auto_clear);
        return true;
    }
    return false;
}

FileInfo::ptr FileInfoManager::get(const std::string& path) {
    sylar::RWMutex::ReadLock lock(m_mutex);
    auto it = m_datas.find(path);
    return it == m_datas.end() ? nullptr : it->second;
}

void FileInfoManager::add(FileInfo::ptr info, bool auto_clear) {
    if(auto_clear) {
        info->clear();
    }
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_datas[info->m_path] = info;
}

void FileInfoManager::del(const std::string& path) {
    sylar::RWMutex::WriteLock lock(m_mutex);
    m_datas.erase(path);
}

std::string FileInfoManager::toString() {
    sylar::RWMutex::ReadLock lock(m_mutex);
    std::stringstream ss;
    ss << "[FileInfoManager size=" << m_datas.size()  << "]" << std::endl;
    for(auto& i : m_datas) {
        ss << "\t" << i.first << "\t: " << i.second->toString() << std::endl;
    }
    return ss.str();
}

}
