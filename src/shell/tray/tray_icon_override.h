#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace tray {

  inline constexpr std::array<std::string_view, 3> iconOverrideSources{"desktop", "file", "tabler"};

  struct IconOverride {
    std::size_t source;
    std::string_view value;
  };

  inline std::optional<IconOverride> parseIconOverride(std::string_view encoded) {
    for (std::size_t i = 0; i < iconOverrideSources.size(); ++i) {
      const auto source = iconOverrideSources[i];
      if (encoded.starts_with(source) && encoded.size() > source.size() && encoded[source.size()] == ':') {
        return IconOverride{i, encoded.substr(source.size() + 1)};
      }
    }
    return std::nullopt;
  }

  inline std::string encodeIconOverride(std::size_t source, std::string_view value) {
    return std::string(iconOverrideSources.at(source)) + ":" + std::string(value);
  }

} // namespace tray
