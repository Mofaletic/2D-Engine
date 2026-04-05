#include "ResourcePathUtils.h"

#include <system_error>
#include <unordered_set>

namespace fs = std::filesystem;

namespace {

fs::path TryCanonical(const fs::path& path) {
    std::error_code ec;
    const fs::path normalized = fs::weakly_canonical(path, ec);
    if (!ec) {
        return normalized;
    }

    return path.lexically_normal();
}

bool IsRegularFile(const fs::path& path) {
    std::error_code ec;
    return fs::exists(path, ec) && fs::is_regular_file(path, ec);
}

}  // namespace

namespace ResourcePathUtils {

fs::path Utf8ToPath(const std::string& path) {
#ifdef _WIN32
    return fs::u8path(path);
#else
    return fs::path(path);
#endif
}

std::string PathToUtf8String(const fs::path& path) {
#ifdef _WIN32
    const std::u8string value = path.generic_u8string();
    return std::string(value.begin(), value.end());
#else
    return path.generic_string();
#endif
}

std::string NormalizePathString(const fs::path& path) {
    return PathToUtf8String(NormalizePath(path));
}

fs::path NormalizePath(const fs::path& path) {
    return TryCanonical(path);
}

std::vector<fs::path> BuildSearchRoots(const std::vector<std::string>& searchPaths) {
    std::vector<fs::path> roots;
    std::unordered_set<std::string> seen;

    auto append = [&roots, &seen](const fs::path& candidate) {
        const std::string normalized = NormalizePathString(candidate);
        if (!normalized.empty() && seen.insert(normalized).second) {
            roots.push_back(Utf8ToPath(normalized));
        }
    };

    append(fs::current_path());

    for (const std::string& searchPath : searchPaths) {
        if (!searchPath.empty()) {
            append(Utf8ToPath(searchPath));
        }
    }

    append(fs::current_path() / "asset");
    append(fs::current_path() / "asset" / "image");

    return roots;
}

std::string ResolvePath(const std::string& identifier, const std::vector<std::string>& searchPaths) {
    if (identifier.empty()) {
        return {};
    }

    const fs::path request = Utf8ToPath(identifier);
    const std::vector<fs::path> searchRoots = BuildSearchRoots(searchPaths);

    auto tryFile = [](const fs::path& path) -> std::string {
        if (!IsRegularFile(path)) {
            return {};
        }
        return NormalizePathString(path);
    };

    if (request.is_absolute()) {
        return tryFile(request);
    }

    if (std::string resolved = tryFile(fs::current_path() / request); !resolved.empty()) {
        return resolved;
    }

    for (const fs::path& root : searchRoots) {
        if (std::string resolved = tryFile(root / request); !resolved.empty()) {
            return resolved;
        }
    }

    const std::string requestedFilename = request.filename().generic_string();
    if (requestedFilename.empty()) {
        return {};
    }

    for (const fs::path& root : searchRoots) {
        std::error_code ec;
        if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
            continue;
        }

        fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
        fs::recursive_directory_iterator end;
        if (ec) {
            continue;
        }

        for (; it != end; it.increment(ec)) {
            if (ec) {
                ec.clear();
                continue;
            }

            if (!it->is_regular_file(ec)) {
                continue;
            }

            if (it->path().filename().generic_string() == requestedFilename) {
                return NormalizePathString(it->path());
            }
        }
    }

    return {};
}

}  // namespace ResourcePathUtils
