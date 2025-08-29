#ifndef __MyTask_H__
#define __MyTask_H__

#include "TcpServer.h"
#include "utfcpp/utf8.h"
#include "cppjieba/Jieba.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <queue>
using namespace std;
struct Candidate
{
    std::string word; // 候选词
    int editDist;     // 与关键字的编辑距离
    int frequency;    // 词频
};
class MyTask
{
private:
    struct Compare
    {
        bool operator()(const Candidate &a, const Candidate &b) const
        {
            // 1. 编辑距离小的优先
            if (a.editDist != b.editDist)
                return a.editDist > b.editDist; // 小的优先

            // 2. 编辑距离相同，词频大的优先
            if (a.frequency != b.frequency)
                return a.frequency < b.frequency; // 大的优先

            // 3. 编辑距离、词频相同，字典序小的优先
            return a.word > b.word; // 字典序小的优先
        }
    };
    //cppjieba::Jieba m_tokenizer;
    Message _msg;
    TcpConnectionPtr _con;

public:
    MyTask(const Message msg, const TcpConnectionPtr &con)
        : _msg(msg), _con(con)
    {
    }

    void process();
    bool isNotChinese(std::string s);
    void Recommand();
    void send_message(int connfd, const Message &msg);
    // 选取最相近的5个候选词
    vector<Candidate> selectTopK(vector<Candidate> &candidates, int k);
    // 计算两个字符串的编辑距离
    int editDistance(const string &s1, const string &s2);
};
#endif
