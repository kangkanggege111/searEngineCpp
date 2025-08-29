#ifndef __MyTask_H__
#define __MyTask_H__

#include "TcpServer.h"
#include "utfcpp/utf8.h"
#include "cppjieba/Jieba.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <queue>
using namespace std;

struct Candidate
{
    std::string word;
    int editDist;
    int frequency;
};

class MyTask
{
private:
    struct Compare
    {
        bool operator()(const Candidate &a, const Candidate &b) const
        {
            if (a.editDist != b.editDist)
                return a.editDist > b.editDist;
            if (a.frequency != b.frequency)
                return a.frequency < b.frequency;
            return a.word > b.word;
        }
    };

    // 添加jieba分词器作为静态成员
    static cppjieba::Jieba m_tokenizer;
    Message _msg;
    TcpConnectionPtr _con;

public:
    MyTask(const Message msg, const TcpConnectionPtr &con)
        : _msg(msg), _con(con) {}

    void process();
    bool isNotChinese(std::string s);
    void Recommand();
    void send_message_to_client(const Message &msg);
    vector<Candidate> selectTopK(vector<Candidate> &candidates, int k);
    int editDistance(const string &s1, const string &s2);
};

#endif
