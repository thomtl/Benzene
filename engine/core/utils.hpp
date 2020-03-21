#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace benzene
{
    std::vector<std::byte> read_binary_file(const std::string& name);
} // namespace benzene
