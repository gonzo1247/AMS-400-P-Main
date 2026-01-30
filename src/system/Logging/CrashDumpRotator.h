#pragma once

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <mutex>
#include <string>
#include <system_error>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

class CrashDumpRotator
{
   public:
    explicit CrashDumpRotator(fs::path directory) : _dir(std::move(directory)) {}

    // --- config ---
    CrashDumpRotator& setRetention(std::chrono::days days)
    {
        _keepDays = days;
        return *this;
    }

    CrashDumpRotator& setMaxFiles(std::size_t maxFiles)
    {
        _maxFiles = maxFiles;
        return *this;
    }

    CrashDumpRotator& setPrefix(std::wstring prefix)
    {
        _prefix = std::move(prefix);
        return *this;
    }

    CrashDumpRotator& setExtension(std::wstring ext)
    {
        _extension = std::move(ext);
        return *this;
    }

    // --- sync rotation ---
    std::size_t rotate() const
    {
        std::error_code ec;

        if (!fs::exists(_dir, ec) || !fs::is_directory(_dir, ec))
        {
            return 0;
        }

        const auto now = fs::file_time_type::clock::now();

        struct FileInfo
        {
            fs::path path;
            fs::file_time_type mtime;
        };

        std::vector<FileInfo> candidates;

        // Collect candidates
        for (fs::directory_iterator it(_dir, fs::directory_options::skip_permission_denied, ec);
             !ec && it != fs::end(it); it.increment(ec))
        {
            const fs::directory_entry& e = *it;

            if (!e.is_regular_file(ec))
            {
                ec.clear();
                continue;
            }

            const fs::path& p = e.path();
            if (!isCandidate(p))
            {
                continue;
            }

            const auto ft = e.last_write_time(ec);
            if (ec)
            {
                ec.clear();
                continue;
            }

            candidates.push_back({p, ft});
        }

        std::size_t deleted = 0;

        // 1) age-based deletion
        for (auto it = candidates.begin(); it != candidates.end();)
        {
            if ((now - it->mtime) > _keepDays)
            {
                if (fs::remove(it->path, ec) && !ec)
                    ++deleted;
                ec.clear();
                it = candidates.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // 2) count-based deletion (keep newest _maxFiles)
        if (_maxFiles > 0 && candidates.size() > _maxFiles)
        {
            std::sort(candidates.begin(), candidates.end(),
                      [](FileInfo const& a, FileInfo const& b)
                      {
                          return a.mtime > b.mtime;  // newest first
                      });

            for (std::size_t i = _maxFiles; i < candidates.size(); ++i)
            {
                fs::remove(candidates[i].path, ec);
                if (!ec)
                    ++deleted;
                ec.clear();
            }
        }

        return deleted;
    }

    // --- static sync helper ---
    static std::size_t Rotate(const fs::path& directory, std::chrono::days keepDays = std::chrono::days{7},
                              std::size_t maxFiles = 10, std::wstring prefix = L"crash_dump_",
                              std::wstring ext = L".dmp")
    {
        CrashDumpRotator r(directory);
        r.setRetention(keepDays).setMaxFiles(maxFiles).setPrefix(std::move(prefix)).setExtension(std::move(ext));
        return r.rotate();
    }

    // --- async (non-overlapping per directory) ---
    static std::future<std::size_t> RotateAsyncNonOverlapping(const fs::path& directory,
                                                              std::chrono::days keepDays = std::chrono::days{7},
                                                              std::size_t maxFiles = 10,
                                                              std::wstring prefix = L"crash_dump_",
                                                              std::wstring ext = L".dmp")
    {
        const auto key = normalizedKey(directory);

        // Try to acquire run-lock for this directory
        {
            std::scoped_lock lk(_mx());
            if (!_running().insert(key).second)
            {
                // Already running for this directory -> return a ready future with 0
                return std::async(std::launch::deferred, [] { return std::size_t{0}; });
            }
        }

        // Capture all by value to avoid dangling references
        return std::async(
            std::launch::async,
            [key, d = fs::path(directory), keepDays, maxFiles, p = std::move(prefix), e = std::move(ext)]() mutable
            {
                // Run rotation
                CrashDumpRotator r(d);
                r.setRetention(keepDays).setMaxFiles(maxFiles).setPrefix(std::move(p)).setExtension(std::move(e));
                std::size_t result = r.rotate();

                // Release run-lock
                std::scoped_lock lk(_mx());
                _running().erase(key);
                return result;
            });
    }

   private:
    bool isCandidate(const fs::path& p) const
    {
        const std::wstring name = p.filename().native();
        const std::wstring ext = p.extension().native();

        if (ext != _extension)
            return false;

        if (name.size() < _prefix.size())
            return false;

        return std::wstring_view(name).substr(0, _prefix.size()) == _prefix;
    }

    // Normalize path key (wstring to match L"" usage on Windows)
    static std::wstring normalizedKey(const fs::path& d)
    {
        std::error_code ec;
        fs::path abs = fs::weakly_canonical(d, ec);
        if (ec)
            abs = d;
#ifdef _WIN32
        // Case-insensitive key on Windows
        std::wstring k = abs.native();
        std::transform(k.begin(), k.end(), k.begin(), ::towlower);
        return k;
#else
        return abs.native();
#endif
    }

    // Global registry for running directories
    static std::unordered_set<std::wstring>& _running()
    {
        static std::unordered_set<std::wstring> s;
        return s;
    }

    static std::mutex& _mx()
    {
        static std::mutex m;
        return m;
    }

   private:
    fs::path _dir;
    std::chrono::days _keepDays{std::chrono::days{7}};
    std::size_t _maxFiles{10};
    std::wstring _prefix{L"crash_dump_"};
    std::wstring _extension{L".dmp"};
};
