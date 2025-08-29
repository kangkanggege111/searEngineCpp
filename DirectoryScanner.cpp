#include "DirectoryScanner.h"

std::vector<std::string> DirectoryScanner::scan(const std::string &dir)
{
    std::vector<std::string> result;
    for (const auto &entry : std::filesystem::directory_iterator(dir))
    {
        result.push_back(entry.path());
    }
    return result;
}