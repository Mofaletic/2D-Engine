#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ResourcePathUtils {

std::filesystem::path Utf8ToPath(const std::string& path);
std::string PathToUtf8String(const std::filesystem::path& path);
std::string NormalizePathString(const std::filesystem::path& path);
std::filesystem::path NormalizePath(const std::filesystem::path& path);
std::vector<std::filesystem::path> BuildSearchRoots(const std::vector<std::string>& searchPaths);
std::string ResolvePath(const std::string& identifier, const std::vector<std::string>& searchPaths);

}  // namespace ResourcePathUtils
