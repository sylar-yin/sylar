#ifndef __SYLAR_FILE_MANAGER_H__
#define __SYLAR_FILE_MANAGER_H__

#include <memory>
#include <map>
#include <string>
#include <functional>
#include "sylar/mutex.h"
#include "sylar/singleton.h"

namespace sylar {

class FileInfoManager;
class FileInfo {
friend class FileInfoManager;
public:
    typedef std::shared_ptr<FileInfo> ptr;
    FileInfo();

    const std::string& getPath() const { return m_path;}
    const std::string& getMd5() const { return m_md5;}
    const std::string& getData() const { return m_data;}

    void clear();
    uint64_t getSize() const { return m_size;}

    std::string toString() const;
private:
    std::string m_path;
    std::string m_md5;
    std::string m_data;
    uint64_t m_mtime;
    uint64_t m_size;
    uint64_t m_loadTime;
};

class FileInfoManager {
public:
    typedef std::function<void(FileInfo::ptr info)> callback;

    FileInfo::ptr load(const std::string& path, uint64_t limit = -1);
    bool load(const std::string& path, callback cb, uint64_t limit = -1, bool auto_clear = true);
    FileInfo::ptr get(const std::string& path);

    void add(FileInfo::ptr info, bool auto_clear = true);
    void del(const std::string& path);

    std::string toString();
private:
    sylar::RWMutex m_mutex;
    std::map<std::string, FileInfo::ptr> m_datas;
};

typedef sylar::Singleton<FileInfoManager> FileInfoMgr;

}

#endif
