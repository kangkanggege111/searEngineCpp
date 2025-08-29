#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <regex>
#include "DirectoryScanner.h"
#include "cppjieba/Jieba.hpp"
#include "simhash/Simhasher.hpp"
#include "utfcpp/utf8.h"
#include "tinyxml2.h"
class PageProcessor
{
public:
    PageProcessor();
    void process(const std::string &dir, const std::string &pagesDir,
                 const std::string &offsetsDir, const std::string &invertedIndexDir, const std::string &stopWordsDir);

private:
    // 提取文档
    void extract_documents(const std::string &dir);

    // 文档去重
    void deduplicate_documents();
    // 创建网页库和网页偏移库
    void build_pages_and_offsets(const std::string &pagesDir, const std::string &offsetsDir);
    // 创建倒排索引库
    void build_inverted_index(const std::string &filename);

private:
    void setStopWords(const std::string &stopwordDir);
    struct Document
    {
        int id;
        std::string link;
        std::string title;
        std::string content;
    };

private:
    cppjieba::Jieba m_tokenizer;
    simhash::Simhasher m_hasher;
    std::unordered_set<std::string> m_stopWords; // 使用set, 而非vector, 是为了方便查找
    std::vector<Document> m_documents;
    std::map<std::string, std::map<int, double>> m_invertedIndex;
};