#pragma once

#include <functional>
#include <memory>
#include <string>

class Node;

namespace settings {
  std::unique_ptr<Node> makeTrayIconOverrideControl(
      float scale, std::shared_ptr<std::string> value, std::function<void(const std::string&)> commit
  );
}
