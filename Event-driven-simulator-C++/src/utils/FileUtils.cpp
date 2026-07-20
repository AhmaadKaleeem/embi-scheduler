/**
 * @file FileUtils.cpp
 * @brief Implementation of FileUtils filesystem helpers.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#include "utils/FileUtils.hpp"

#include <stdexcept>
#include <system_error>

namespace embi {

void FileUtils::ensureDirectory(const std::string& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
        throw std::runtime_error(
            "FileUtils::ensureDirectory: failed to create '" + path +
            "': " + ec.message());
    }
}

bool FileUtils::exists(const std::string& path) noexcept {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

bool FileUtils::isFile(const std::string& path) noexcept {
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

std::string FileUtils::extension(const std::string& path) noexcept {
    return std::filesystem::path(path).extension().string();
}

std::string FileUtils::join(const std::string& base,
                             const std::string& child) noexcept {
    return (std::filesystem::path(base) / child).string();
}

std::string FileUtils::stem(const std::string& path) noexcept {
    return std::filesystem::path(path).stem().string();
}

std::uintmax_t FileUtils::fileSize(const std::string& path) noexcept {
    std::error_code ec;
    auto sz = std::filesystem::file_size(path, ec);
    return ec ? 0ULL : sz;
}

void FileUtils::remove(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (ec) {
        throw std::runtime_error(
            "FileUtils::remove: failed to remove '" + path +
            "': " + ec.message());
    }
}

} // namespace embi
