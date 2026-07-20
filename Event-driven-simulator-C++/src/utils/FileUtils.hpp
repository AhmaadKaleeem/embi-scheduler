/**
 * @file FileUtils.hpp
 * @brief Filesystem utilities: directory creation, path helpers, file existence.
 *
 * Provides a thin RAII/functional wrapper around std::filesystem for the
 * simulator's output-directory and trace-file operations.
 *
 * @author  EMBI Simulator Project
 * @version 1.0.0
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace embi {

/**
 * @class FileUtils
 * @brief Static helpers for filesystem operations used by the simulator.
 *
 * All methods throw std::runtime_error on unrecoverable I/O errors.
 */
class FileUtils {
public:
    FileUtils()                              = delete;
    FileUtils(const FileUtils&)              = delete;
    FileUtils& operator=(const FileUtils&)   = delete;

    // ─── Directory operations ─────────────────────────────────────────────────

    /**
     * @brief Creates a directory and all missing parents (like mkdir -p).
     * @param path  Directory path to create.
     * @throws std::runtime_error if creation fails.
     * @complexity O(depth of path)
     */
    static void ensureDirectory(const std::string& path);

    /**
     * @brief Returns true if the given path exists (file or directory).
     * @param path  Filesystem path to check.
     * @return true if path exists.
     * @complexity O(1)
     */
    [[nodiscard]] static bool exists(const std::string& path) noexcept;

    /**
     * @brief Returns true if the given path is a regular file.
     * @param path  Filesystem path to check.
     * @return true if path is a file.
     * @complexity O(1)
     */
    [[nodiscard]] static bool isFile(const std::string& path) noexcept;

    /**
     * @brief Returns the file extension of a path (with leading dot).
     *        Returns empty string if no extension.
     * @param path  Filesystem path.
     * @return Extension string, e.g., ".csv", ".yaml", ".json".
     * @complexity O(path length)
     */
    [[nodiscard]] static std::string extension(const std::string& path) noexcept;

    /**
     * @brief Joins two path components with the platform separator.
     * @param base   Base directory path.
     * @param child  Child file or directory name.
     * @return Joined path string.
     * @complexity O(path lengths)
     */
    [[nodiscard]] static std::string join(const std::string& base,
                                          const std::string& child) noexcept;

    /**
     * @brief Returns the stem (basename without extension) of a path.
     * @param path  Filesystem path.
     * @return Stem string, e.g., "config" for "config.yaml".
     * @complexity O(path length)
     */
    [[nodiscard]] static std::string stem(const std::string& path) noexcept;

    /**
     * @brief Returns file size in bytes, or 0 if file does not exist.
     * @param path  Path to a regular file.
     * @return File size in bytes.
     * @complexity O(1)
     */
    [[nodiscard]] static std::uintmax_t fileSize(const std::string& path) noexcept;

    /**
     * @brief Removes a file or empty directory.
     * @param path  Path to remove.
     * @throws std::runtime_error if removal fails.
     * @complexity O(1)
     */
    static void remove(const std::string& path);
};

} // namespace embi
