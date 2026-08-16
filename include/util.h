#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace wgfx::util {

[[nodiscard]] std::string readTextFile(const std::filesystem::path& path);
[[nodiscard]] std::vector<char> readBinaryFile(const std::filesystem::path& path);
void writeBinaryFile(const std::filesystem::path& path, std::span<const std::byte> data);

[[nodiscard]] bool fileExists(const std::filesystem::path& path) noexcept;
[[nodiscard]] std::filesystem::path directoryOf(const std::filesystem::path& path);
[[nodiscard]] std::int64_t currentTimeMilliseconds() noexcept;

void enableOpenGLDebugOutput();

} // namespace wgfx::util
