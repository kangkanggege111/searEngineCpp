#include "MyTask.h"

// 静态成员初始化
cppjieba::Jieba MyTask::m_tokenizer("./dict/jieba.dict.utf8",
                                    "./dict/hmm_model.utf8",
                                    "./dict/user.dict.utf8",
                                    "./dict/idf.utf8",
                                    "./dict/stop_words.utf8");

void MyTask::process()
{
    cout << "开始执行MyTask::process()" << endl;
    try
    {
        if (_msg.tag == 1)
        {
            cout << "处理关键词推荐请求: " << _msg.value << endl;
            Recommand();
        }
        else if (_msg.tag == 2)
        {
            cout << "处理网页搜索请求: " << _msg.value << endl;
            // TODO: 实现网页搜索
        }
        else
        {
            cerr << "未知的请求类型: " << _msg.tag << endl;
        }
    }
    catch (const std::exception &e)
    {
        cerr << "处理请求时发生错误: " << e.what() << endl;
        Message errorMsg;
        errorMsg.tag = _msg.tag;
        errorMsg.value = "Error: " + string(e.what());
        errorMsg.length = errorMsg.value.size();
        send_message_to_client(errorMsg);
    }
}

bool MyTask::isNotChinese(std::string s)
{
    auto it = utf8::iterator<std::string::iterator>{s.begin(), s.begin(), s.end()};
    char32_t cp = *it;
    return !((cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0x3400 && cp <= 0x4DBF));
}

void MyTask::Recommand()
{
    map<string, int> words_frqc;
    vector<string> words;

    // 使用jieba分词
    m_tokenizer.Cut(_msg.value, words);

    for (auto &word : words)
    {
        cout << "解析一个单词: " << word << endl;

        if (isNotChinese(word))
        {
            // 英文处理
            string enIndexLib = "./IndexDatabase/enIndexLib.txt";
            ifstream ifs_IndexLib(enIndexLib);
            if (!ifs_IndexLib.is_open())
            {
                cerr << "文件打开失败: " << enIndexLib << endl;
                continue;
            }

            std::transform(word.begin(), word.end(), word.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });

            cout << "处理英文单词(小写): " << word << endl;

            // 修正：使用字符减'a'而不是减48
            char firstChar = word[0];
            if (firstChar < 'a' || firstChar > 'z')
            {
                ifs_IndexLib.close();
                continue;
            }

            string line;
            // 跳到对应行
            for (int i = 0; i < (firstChar - 'a'); i++)
            {
                if (!getline(ifs_IndexLib, line))
                {
                    break;
                }
            }

            if (!getline(ifs_IndexLib, line))
            {
                ifs_IndexLib.close();
                continue;
            }

            string begin_alphabet;
            istringstream iss(line);
            iss >> begin_alphabet;

            set<int> lineNums;
            int num = 0;
            while (iss >> num)
            {
                lineNums.insert(num);
            }
            ifs_IndexLib.close();

            // 去英文字典库找对应单词
            string enDicLib = "./DictionaryLibrary/enLib.txt";
            ifstream ifs_DicLib(enDicLib);
            if (!ifs_DicLib.is_open())
            {
                cout << "文件打开失败: " << enDicLib << endl;
                continue;
            }

            string dicLine;
            int currentLine = 1;

            for (auto &lineNum : lineNums)
            {
                // 重置文件指针到开头
                ifs_DicLib.clear();
                ifs_DicLib.seekg(0);
                currentLine = 1;

                // 跳到指定行
                while (currentLine < lineNum && getline(ifs_DicLib, dicLine))
                {
                    currentLine++;
                }

                if (currentLine == lineNum && getline(ifs_DicLib, dicLine))
                {
                    size_t pos = dicLine.find_first_of(' ');
                    string wordFromDict;
                    if (pos != std::string::npos)
                    {
                        wordFromDict = dicLine.substr(0, pos);
                    }
                    else
                    {
                        wordFromDict = dicLine;
                    }

                    if (!wordFromDict.empty())
                    {
                        words_frqc[wordFromDict]++;
                    }
                }
            }
            ifs_DicLib.close();
        }
        else
        {
            // 中文处理
            string cnIndexLib = "./IndexDatabase/cnIndexLib.txt";
            ifstream ifs_IndexLib(cnIndexLib);
            if (!ifs_IndexLib.is_open())
            {
                cerr << "文件打开失败: " << cnIndexLib << endl;
                continue;
            }

            set<int> lineNums;
            string line;
            bool found = false;

            while (getline(ifs_IndexLib, line))
            {
                istringstream iss(line);
                string begin_word;
                iss >> begin_word;

                if (word == begin_word) // 修正：正确比较
                {
                    int num;
                    while (iss >> num)
                    {
                        lineNums.insert(num);
                    }
                    found = true;
                    break;
                }
            }
            ifs_IndexLib.close();

            if (!found || lineNums.empty())
            {
                continue;
            }

            // 去中文字典库找对应单词
            string cnDicLib = "./DictionaryLibrary/cnLib.txt";
            ifstream ifs_DicLib(cnDicLib);
            if (!ifs_DicLib.is_open())
            {
                cerr << "文件打开失败: " << cnDicLib << endl;
                continue;
            }

            string dicLine;
            int currentLine = 1;

            for (auto &lineNum : lineNums)
            {
                // 重置文件指针到开头
                ifs_DicLib.clear();
                ifs_DicLib.seekg(0);
                currentLine = 1;

                // 跳到指定行
                while (currentLine < lineNum && getline(ifs_DicLib, dicLine))
                {
                    currentLine++;
                }

                if (currentLine == lineNum && getline(ifs_DicLib, dicLine))
                {
                    size_t pos = dicLine.find_first_of(' ');
                    string wordFromDict;
                    if (pos != std::string::npos)
                    {
                        wordFromDict = dicLine.substr(0, pos);
                    }
                    else
                    {
                        wordFromDict = dicLine;
                    }

                    if (!wordFromDict.empty())
                    {
                        words_frqc[wordFromDict]++;
                    }
                }
            }
            ifs_DicLib.close();
        }
    }

    // 构建候选词
    vector<Candidate> candidates;
    for (auto &wordPair : words_frqc)
    {
        int edit_dis = editDistance(_msg.value, wordPair.first);
        int frqc = wordPair.second;
        candidates.push_back({wordPair.first, edit_dis, frqc});
    }

    vector<Candidate> result = selectTopK(candidates, 5);

    // 构造返回消息（修正：使用TLV格式）
    Message response;
    response.tag = 1;

    string value;
    for (size_t i = 0; i < result.size(); ++i)
    {
        value += result[i].word;
        if (i < result.size() - 1)
        {
            value += " ";
        }
    }

    response.value = value;
    response.length = value.size();

    cout << "推荐结果: " << value << endl;

    // 发送给客户端
    send_message_to_client(response);
}

void MyTask::send_message_to_client(const Message &msg)
{
    // 构造TLV格式的消息
    vector<char> buffer;
    buffer.reserve(sizeof(msg.tag) + sizeof(msg.length) + msg.length);

    // 头部
    const char *p = reinterpret_cast<const char *>(&msg.tag);
    buffer.insert(buffer.end(), p, p + sizeof(msg.tag));

    p = reinterpret_cast<const char *>(&msg.length);
    buffer.insert(buffer.end(), p, p + sizeof(msg.length));

    // 主体
    buffer.insert(buffer.end(), msg.value.begin(), msg.value.end());

    // 通过连接发送
    string dataToSend(buffer.begin(), buffer.end());
    _con->sendInLoop(dataToSend);
}

vector<Candidate> MyTask::selectTopK(vector<Candidate> &candidates, int k)
{
    priority_queue<Candidate, vector<Candidate>, Compare> pq;

    for (auto &c : candidates)
    {
        pq.push(c);
    }

    vector<Candidate> result;
    for (int i = 0; i < k && !pq.empty(); ++i)
    {
        result.push_back(pq.top());
        pq.pop();
    }
    return result;
}

int MyTask::editDistance(const string &s1, const string &s2)
{
    int m = s1.size(), n = s2.size();
    vector<vector<int>> dp(m + 1, vector<int>(n + 1, 0));

    for (int i = 0; i <= m; i++)
        dp[i][0] = i;
    for (int j = 0; j <= n; j++)
        dp[0][j] = j;

    for (int i = 1; i <= m; i++)
    {
        for (int j = 1; j <= n; j++)
        {
            if (s1[i - 1] == s2[j - 1])
            {
                dp[i][j] = dp[i - 1][j - 1];
            }
            else
            {
                dp[i][j] = 1 + min({dp[i - 1][j],
                                    dp[i][j - 1],
                                    dp[i - 1][j - 1]});
            }
        }
    }

    return dp[m][n];
}
