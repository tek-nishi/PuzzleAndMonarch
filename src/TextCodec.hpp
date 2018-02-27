#pragma once

//
// text encode/decode
//

#include <string>


namespace ngs { namespace TextCodec {

std::string encode(const std::string& input) noexcept;
std::string decode(const std::string& input) noexcept;

void write(const std::string& path, const std::string& input) noexcept;
std::string load(const std::string& path) noexcept;

} }
