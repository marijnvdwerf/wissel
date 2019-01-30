#pragma once

#include <cstdint>
#ifdef _OPENLOCO_USE_BOOST_FS_
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace wissel::platform
{
    uint32_t get_time();
    fs::path get_user_directory();
    std::string prompt_directory(const std::string& title);
    fs::path GetCurrentExecutablePath();
#if defined(__APPLE__) && defined(__MACH__)
    fs::path GetBundlePath();
#endif
}
