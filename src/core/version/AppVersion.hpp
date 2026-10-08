#pragma once

#include "SemanticVersion.hpp"

#include <string_view>

namespace app_version {

inline constexpr SemanticVersion current{ 2, 5, 0 };
inline constexpr std::string_view current_text{ "2.5.0" };

} // namespace app_version
