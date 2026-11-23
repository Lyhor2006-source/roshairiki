#pragma once

#include <string>

namespace RyomenX {

namespace StealthBypass {

void Initialize();
std::string GetDeviceIDFV();
std::string RandomizeDeviceIDFV();
bool IsStealthActive();

} // namespace StealthBypass

} // namespace RyomenX
