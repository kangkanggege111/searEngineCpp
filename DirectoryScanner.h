#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
class DirectoryScanner
{
public:
    /**
     * 遍历目录 dir, 获取目录里面的所有文件名
     */
    static std::vector<std::string> scan(const std::string &dir);

private:
    DirectoryScanner() = delete;
};