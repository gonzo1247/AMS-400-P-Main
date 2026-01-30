#pragma once
#include "Version.h"

#include <cstdint>
#include <cstdint>
#include <optional>

// ===== Compile-time helpers (namespace-scope, not members) =====
namespace ver_detail
{
	// Compile-time decimal parser for string literals
	template <std::size_t N>
	constexpr std::uint32_t cx_to_u32(const char(&s)[N]) noexcept
	{
		std::uint32_t v = 0;
		for (std::size_t i = 0; i < N; ++i)
		{
			const char c = s[i];
			if (c < '0' || c > '9')
				break;
			v = v * 10u + static_cast<std::uint32_t>(c - '0');
		}
		return v;
	}

	// Single integer version code
	constexpr std::uint64_t MakeVersionCode(std::uint32_t M, std::uint32_t P,
		std::uint32_t H, std::uint32_t B) noexcept
	{
		const std::uint64_t Bb = (B > 9999u) ? 9999u : B;
		return static_cast<std::uint64_t>(M) * 100000000ull
			+ static_cast<std::uint64_t>(P) * 1000000ull
			+ static_cast<std::uint64_t>(H) * 10000ull
			+ Bb;
	}

	// Map semver (APP_MAJOR/MINOR/PATCH as strings) -> DB scheme (Major.Patch.Hotfix)
	// Expect these macros as string literals in Version.h, e.g. "#define APP_MAJOR "1""
	static constexpr std::uint32_t VER_MAJOR = cx_to_u32(APP_MAJOR);
	static constexpr std::uint32_t VER_PATCH = cx_to_u32(APP_MINOR);  // Minor -> Patch
	static constexpr std::uint32_t VER_HOTFIX = cx_to_u32(APP_PATCH);  // Patch -> Hotfix

	// Build may be provided as string or number
#if defined(BUILD_VERSION_STR)
	static constexpr std::uint32_t VER_BUILD = cx_to_u32(BUILD_VERSION_STR);
#elif defined(BUILD_VERSION)
	static constexpr std::uint32_t VER_BUILD = static_cast<std::uint32_t>(BUILD_VERSION);
#else
	static constexpr std::uint32_t VER_BUILD = 0u;
#endif

	static_assert(VER_MAJOR < 10000, "Major too large");
	static_assert(VER_PATCH < 10000, "Patch too large");
	static_assert(VER_HOTFIX < 10000, "Hotfix too large");

	static constexpr std::uint64_t CLIENT_CODE =
		MakeVersionCode(VER_MAJOR, VER_PATCH, VER_HOTFIX, VER_BUILD);
}

// ===== Your types & class =====
struct ActiveVersionRow
{
	std::uint64_t version_code{};
	std::uint64_t min_client_code{};
	std::uint64_t max_client_code{};
	bool          bypass_active{};
	std::string   version_str{};
};

class VersionCheck
{
public:
	VersionCheck() = default;
	~VersionCheck() = default;

	bool CheckCompatibilityWithDB(QWidget* parent);

private:
	std::optional<ActiveVersionRow> LoadActiveVersionFor(const std::string& component);

	// Expose constants if you want to use them elsewhere
	static constexpr std::uint32_t APP_VER_MAJOR = ver_detail::VER_MAJOR;
	static constexpr std::uint32_t APP_VER_PATCH = ver_detail::VER_PATCH;
	static constexpr std::uint32_t APP_VER_HOTFIX = ver_detail::VER_HOTFIX;
	static constexpr std::uint32_t APP_VER_BUILD = ver_detail::VER_BUILD;

	static constexpr std::uint64_t kClientCode = ver_detail::CLIENT_CODE;
};

