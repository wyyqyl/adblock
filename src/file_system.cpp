#include "file_system.h"

#include <sstream>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#ifdef WIN32
#include <Windows.h>
#endif  // WIN32

namespace adblock {

class RuntimeErrorWithErrno : public std::runtime_error {
 public:
  explicit RuntimeErrorWithErrno(const std::string& message)
      : std::runtime_error(message + " (" + std::strerror(errno) + ")") {
  }
};

namespace fs = boost::filesystem;

#ifdef _MSC_VER
DefaultFileSystem::DefaultFileSystem() {
  fs::path::imbue(std::locale(""));
}
#endif  // _MSC_VER

std::string DefaultFileSystem::Read(const std::string& path) const {
  fs::ifstream fstream(path);
  if (!fstream.is_open()) {
    throw RuntimeErrorWithErrno("Failed to open \"" + path + "\"");
  }
  std::stringstream stream;
  stream << fstream.rdbuf();
  return stream.str();
}

void DefaultFileSystem::Write(const std::string& path,
                              const std::string& data) {
  fs::ofstream fstream(path);
  fstream << data;
}

bool DefaultFileSystem::Remove(const std::string& path) {
  return fs::remove(path);
}

void DefaultFileSystem::Move(const std::string& from, const std::string& to) {
  fs::rename(from, to);
}

FileSystem::StatResult DefaultFileSystem::Stat(const std::string& path) const {
  fs::path p(path);
  FileSystem::StatResult result;
  result.exists = fs::exists(p);
  result.is_directory = fs::is_directory(p);
  result.is_file = fs::is_regular_file(p);
  result.last_write_time = fs::last_write_time(p);
  return result;
}

std::string DefaultFileSystem::Resolve(const std::string& path) const {
  fs::path dir(fs::current_path());

#ifdef WIN32
  HKEY hkey = nullptr;
  char value[MAX_PATH] = { 0 };
  DWORD length = MAX_PATH;
  LONG status = RegOpenKeyExA(HKEY_CURRENT_USER, 
      "Software\\MozillaPlugins\\anvisoft.com/AdblockPlugin",
      0, KEY_READ, &hkey);
  if (status == ERROR_SUCCESS) {
    status = RegQueryValueExA(hkey, "Path", NULL, NULL, (LPBYTE)value, &length);
    RegCloseKey(hkey);
    if (status == ERROR_SUCCESS) {
      dir = std::string(value, length);
      dir = dir.parent_path();
    }
  }
#endif  // WIN32

  return fs::absolute(path, dir).string();
}

}  // namespace adblock
