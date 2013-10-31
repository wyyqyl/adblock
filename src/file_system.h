#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_

#include <ctime>
#include <boost/shared_ptr.hpp>

namespace adblock {

class FileSystem {
 public:
  struct StatResult {
    StatResult() {
      exists = false;
      is_directory = false;
      is_file = false;
      last_write_time = 0;
    }

    bool exists;
    bool is_directory;
    bool is_file;
    std::time_t last_write_time;
  };

  virtual ~FileSystem() {
  }
  virtual std::string Read(const std::string& path) const = 0;
  virtual void Write(const std::string& path, const std::string& data) = 0;
  virtual bool Remove(const std::string& path) = 0;
  virtual void Move(const std::string& from, const std::string& to) = 0;
  virtual StatResult Stat(const std::string& path) const = 0;
  virtual std::string Resolve(const std::string& path) const = 0;
};

typedef boost::shared_ptr<FileSystem> FileSystemPtr;

class DefaultFileSystem : public FileSystem {
 public:
#ifdef _MSC_VER
  // fix for https://svn.boost.org/trac/boost/ticket/6320
  DefaultFileSystem();
#endif  // _MSC_VER
  std::string Read(const std::string& path) const;
  void Write(const std::string& path, const std::string& data);
  bool Remove(const std::string& path);
  void Move(const std::string& from, const std::string& to);
  FileSystem::StatResult Stat(const std::string& path) const;
  std::string Resolve(const std::string& path) const;
};

}  // namespace adblock

#endif  // FILE_SYSTEM_H_
