#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <set>
#include <cppjieba/Jieba.hpp>
#include <unordered_set>
#include "utfcpp/utf8.h"
#include "DirectoryScanner.h"

class KeyWordProcessor
{
public:
    KeyWordProcessor();
    // chDir: 中文语料库                enDir: 英文语料库
    // cn_stopwordDir: 中文停止词       en_stopwordDir: 英文停止词
    // out_cnLibDir: 中文字典输出目录   out_enLibDir: 英文字典输出目录
    // out_cnIndexDir: 中文索引库       out_enIndexDir: 英文索引库
    void process(const std::string &cnDir, const std::string &enDir, const std::string &cn_stopwordDir,
                 const std::string &en_stopwordDir, const std::string &out_cnLibDir, const std::string &out_enLibDir,
                 const std::string &out_cnIndexDir, const std::string &out_enIndexDir);

private:
    // 创建中文字典
    void create_cn_dict(const std::string &corpusDir, const std::string &outfileDir);
    // 建立中文字典索引
    void build_cn_index(const std::string &dictDir, const std::string &outfileDir);
    // 创建英文字典
    void create_en_dict(const std::string &corpusDir, const std::string &outfileDir);
    // 建立英文字典
    void build_en_index(const std::string &dictDir, const std::string &outfileDir);
    // 初始化中文停止词词库
    void set_cnStopWords(const std::string &dir);
    // 初始化英文停止词词库
    void set_enStopWords(const std::string &dir);
    std::vector<std::string> tokenize_line(const std::string &line);

private:
    // jb库,只初始化一次就好(开销较大)
    cppjieba::Jieba m_tokenizer;
    // 英文停止词词库
    std::unordered_set<std::string> m_enStopWords;
    // 中文停止词词库
    std::unordered_set<std::string> m_chStopWords;
};