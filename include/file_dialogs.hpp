#pragma once
#include <string>
namespace gv {
std::string chooseSessionFile(bool save, bool security, const std::string &initial);
void showApplicationError(const std::string &message);
} // namespace gv
