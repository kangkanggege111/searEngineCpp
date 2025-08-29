#include "MyTask.h"
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
        _con->sendInLoop("Error: " + string(e.what()));
    }
}

bool MyTask::isNotChinese(std::string s)
{
    auto it = utf8::iterator<std::string::iterator>{s.begin(), s.begin(), s.end()};
    char32_t cp = *it;
    return (cp < 0x4E00 && cp > 0x9FFF) || (cp < 0x3400 && cp > 0x4DBF);
}

void MyTask::Recommand()
{
    map<string, int> words_frqc;
    vector<string> words;
    //m_tokenizer.Cut(_msg.value, words);
    for (auto &word : words)
    {
        cout << "解析一个单词: " << word << endl;
        // 英文
        if (isNotChinese(word))
        {
            // 不是汉字 -> 在英文索引库和词库中查找
            // 1.先打开 索引库 找到英文词库中都有哪些行有该关键字
            string enIndexLib = "./IndexDatabase/enIndexLib.txt";
            ifstream ifs_IndexLib(enIndexLib);
            if (!ifs_IndexLib.is_open())
            {
                cerr << "文件打开失败: " << enIndexLib << endl;
                return;
            }
            std::transform(word.begin(), word.end(), word.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });
            cout << "接到了输入的字母(已经转换为小写): " << word << endl;
            // 英文好找,正好对应1-26行,这里无需一行一行地遍历之后再读了
            string line;
            for (int i = 0; i < (*word.c_str()) - 48; i++)
            {
                getline(ifs_IndexLib, line);
            }
            // 将所有的行号塞到一个vector里面
            string begin_alphabet;
            istringstream iss(line);
            iss >> begin_alphabet;
            set<int> lineNums; // 所有的行号集合
            int num = 0;
            while (iss >> num)
            {
                lineNums.insert(num);
            }
            ifs_IndexLib.close();

            // 去英文字段库找到对应的单词
            string enDicLib = "./DictionaryLibrary/enLib.txt";
            ifstream ifs_DicLib(enDicLib);
            if (!ifs_DicLib.is_open())
            {
                cout << "文件打开失败: " << enDicLib << endl;
                return;
            }
            vector<string> words_en;
            int index = 0;
            int i = 0;
            for (auto &lineNum : lineNums)
            {

                index = lineNum;
                string s;

                while (i++ < index)
                {
                    getline(ifs_DicLib, s);
                }

                // 找到第一个空格的位置
                size_t pos = s.find_first_of(' ');

                // 截取空格之前的部分
                std::string before;
                if (pos != std::string::npos)
                {
                    before = s.substr(0, pos);
                }
                else
                {
                    // 如果没有空格，就整个字符串
                    before = s;
                }

                words_en.push_back(before);
            }

            // 将所有关键词添加到结果中
            for (auto &word : words_en)
            {
                words_frqc[word]++;
            }
        }
        else
        {
            // 中文
            string cnIndexLib = "./IndexDatabase/cnIndexLib.txt";
            ifstream ifs_IndexLib(cnIndexLib);
            if (!ifs_IndexLib.is_open())
            {
                cerr << "文件打开失败: " << cnIndexLib << endl;
                return;
            }
            set<int> lineNums;
            string begin_word;
            string line;
            while (getline(ifs_IndexLib, line))
            {
                if (word == begin_word)
                {
                    istringstream iss(line);
                    string temp;
                    iss >> temp;
                    temp.clear();
                    while (iss >> temp)
                    {
                        lineNums.insert(stoi(temp));
                    }
                }
                break;
            }
            // 带着行号集合set<int> lines 去中文字典库中找
            string cnDicLib = "./DictionaryLibrary/cnLib.txt";
            ifstream ifs_DicLib(cnDicLib);
            if (!ifs_DicLib.is_open())
            {
                cerr << "文件打开失败: " << cnDicLib << endl;
                return;
            }
            vector<string> words_cn;
            int num = 0;
            int i = 0;
            for (auto &lineNum : lineNums)
            {

                num = lineNum;
                string s;

                while (i++ < num)
                {
                    getline(ifs_DicLib, s);
                }

                // 找到第一个空格的位置
                size_t pos = s.find_first_of(' ');

                // 截取空格之前的部分
                std::string before;
                if (pos != std::string::npos)
                {
                    before = s.substr(0, pos);
                }
                else
                {
                    // 如果没有空格，就整个字符串
                    before = s;
                }

                words_cn.push_back(before);
            }

            // 将所有关键词添加到结果中
            for (auto &word : words_cn)
            {
                words_frqc[word]++;
            }
        }
    }
    vector<Candidate> candidates;
    for (auto &word : words_frqc)
    {
        int edit_dis = editDistance(_msg.value, word.first);
        int frqc = word.second;
        candidates.push_back({word.first, edit_dis, frqc});
    }
    vector<Candidate> result = selectTopK(candidates, 5);

    //  返回给客户端
    // 构造返回消息
    Message response;
    response.tag = 1;

    string value;
    for (const auto &candidate : result)
    {
        value += candidate.word + " ";
    }

    response.value = value;
    response.length = value.size();

    // 发送给客户端
    stringstream ss;
    ss << response.tag << " " << response.length << " " << response.value;
    _con->sendInLoop(ss.str());
}

void MyTask::send_message(int connfd, const Message &msg)
{
    send(connfd, &msg.tag, sizeof(msg.tag), 0);
    send(connfd, &msg.length, sizeof(msg.length), 0);
    send(connfd, msg.value.c_str(), msg.length, 0);
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

    // 初始化边界
    for (int i = 0; i <= m; i++)
        dp[i][0] = i;
    for (int j = 0; j <= n; j++)
        dp[0][j] = j;

    // 动态规划填表
    for (int i = 1; i <= m; i++)
    {
        for (int j = 1; j <= n; j++)
        {
            if (s1[i - 1] == s2[j - 1])
            {
                dp[i][j] = dp[i - 1][j - 1]; // 相等，无需操作
            }
            else
            {
                dp[i][j] = 1 + min({
                                   dp[i - 1][j],    // 删除
                                   dp[i][j - 1],    // 插入
                                   dp[i - 1][j - 1] // 替换
                               });
            }
        }
    }

    return dp[m][n];
}