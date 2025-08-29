#include "PageProcessor.h"
using namespace tinyxml2;
using namespace simhash;
using namespace std;
PageProcessor::PageProcessor() {}
void PageProcessor::process(const std::string &dir, const std::string &pagesDir,
                            const std::string &offsetsDir, const std::string &invertedIndexDir, const std::string &stopWordsDir)
{
    extract_documents(dir);
    std::cout << "文档提取成功" << std::endl;
    deduplicate_documents();
    std::cout << "文档去重成功" << endl;
    build_pages_and_offsets(pagesDir, offsetsDir);
    cout << "文档输出成功" << endl;
    setStopWords(stopWordsDir);
    cout << "停止词库初始化成功" << endl;
    build_inverted_index(invertedIndexDir);
    cout << "倒排索引库输出成功" << endl;
}
void PageProcessor::extract_documents(const std::string &dir)
{
    std::vector<std::string> filedirs = DirectoryScanner::scan(dir);
    // std::ofstream outfile("auto_repetend.xml");
    int _id = 1;
    for (auto &filedir : filedirs)
    {
        XMLDocument doc;

        doc.LoadFile(filedir.c_str());
        XMLElement *root = doc.RootElement();
        if (!root)
        {
            return;
        }
        XMLElement *channelNode = root->FirstChildElement("channel");
        XMLElement *itemmeNode = channelNode->FirstChildElement("item");

        /*  if (!outfile.is_open())
         {
             std::cerr << "无法打开文件: " << "auto_repetend.xml" << std::endl;
             return;
         } */
        while (itemmeNode)
        {
            XMLElement *descNode = itemmeNode->FirstChildElement("content");
            if (!descNode)
            {
                descNode = itemmeNode->FirstChildElement("description");
                if (!descNode)
                {
                    // 文件中不存在<content>标签 也不存在<description>标签
                    itemmeNode = itemmeNode->NextSiblingElement("item");
                    continue;
                }
            }
            std::string content = descNode->GetText();
            content = regex_replace(content, std::regex("<[^>]*>"), "");
            content = regex_replace(content, std::regex("&nbsp"), "");

            XMLElement *linkNode = itemmeNode->FirstChildElement("link");
            std::string link = linkNode->GetText();
            XMLElement *titleNode = itemmeNode->FirstChildElement("title");
            std::string title = titleNode->GetText();
            Document doc = {_id, link, title, content};
            m_documents.push_back(doc);
            /* outfile << "<doc>" << '\n';
            outfile << "  <id>" << _id << "</id>" << '\n';
            outfile << "  <link>" << link << "</link>" << '\n';
            outfile << "  <title>" << title << "</title>" << '\n';
            outfile << "  <content>" << desc << "</content>" << '\n';
            outfile << "</doc>" << '\n'; */

            ++_id;
            itemmeNode = itemmeNode->NextSiblingElement("item");
        }
    }
}

void PageProcessor::deduplicate_documents()
{
    unordered_set<uint64_t> hash_gather;
    vector<Document> final_document;
    Simhasher simhasher;
    int id = 1;
    for (auto &T : m_documents)
    {

        uint64_t hashcode;
        // 提取特征
        size_t topN = 5;
        simhasher.make(T.content, topN, hashcode);

        // 找不到这个hashcode
        if (hash_gather.find(hashcode) == hash_gather.end())
        {
            bool sign = true;
            // 没有重复的hashcode
            for (auto &hash : hash_gather)
            {
                // 如果类似,就不插入
                if (Simhasher::isEqual(hashcode, hash))
                {
                    sign = false;
                    break;
                }
            }

            // 没有类似的文档,就插入
            if (sign)
            {
                hash_gather.insert(hashcode);
                T.id = id++;
                final_document.push_back(move(T));
            }
        }
    }
    m_documents = move(final_document);
}

void PageProcessor::build_pages_and_offsets(const std::string &pagesDir, const std::string &offsetsDir)
{
    // 生成网页库
    std::ofstream outfile_page(pagesDir);
    if (!outfile_page.is_open())
    {
        std::cerr << "无法打开文件: " << pagesDir << std::endl;
        return;
    }
    // 生成网页偏移库
    std::ofstream outfile_offsets(offsetsDir);
    if (!outfile_offsets.is_open())
    {
        std::cerr << "无法打开文件: " << pagesDir << std::endl;
        return;
    }
    for (auto &T : m_documents)
    {
        std::streampos offset = outfile_page.tellp();
        outfile_page << "<doc>" << '\n';
        outfile_page << "  <id>" << T.id << "</id>" << '\n';
        outfile_page << "  <link>" << T.link << "</link>" << '\n';
        outfile_page << "  <title>" << T.title << "</title>" << '\n';
        outfile_page << "  <content>" << T.content << "</content>" << '\n';
        outfile_page << "</doc>" << '\n';
        outfile_offsets << T.id << " " << offset << " " << (outfile_page.tellp() - offset) << '\n';
    }
    outfile_page.close();
    outfile_offsets.close();
}

bool isNotChinese(std::string s)
{
    auto it = utf8::iterator<std::string::iterator>{s.begin(), s.begin(), s.end()};
    char32_t cp = *it;
    return (cp < 0x4E00 && cp > 0x9FFF) || (cp < 0x3400 && cp > 0x4DBF);
}

void PageProcessor::build_inverted_index(const std::string &filename)
{
    map<std::string, pair<int, double>> result;

    unordered_map<int, vector<std::string>> map_id_words; // 单个doc文档id有哪些关键词

    // 将整个xml文档里面每个doc文档的title和content切分为词语
    // 切出来的词语放在temp(vector)中
    // map_id_words记录当前doc文档的id和切分后的词语(vector)
    for (auto &item : m_documents)
    {
        vector<std::string> words;
        string s = item.title + " " + item.content;
        m_tokenizer.Cut(s, words);
        map_id_words[item.id] = move(words);
    }
    cout << "开始剔除停用词" << endl;
    map<int, map<std::string, int>> word_frqc; // 记录当前doc文档的各个单词的出现次数

    // 去掉整个xml中所有文档的停用词(遍历map_id_words)
    // 定义一个正则,去除 数字,小数的关键词
    std::regex number_pattern(R"(^\d+(\.\d+)?$)");
    std::regex weird_number_pattern(R"(^\d+(\.\d+){2,}$)");
    for (auto &T : map_id_words)
    {
        // 处理单个文档
        auto T_SecondBegin = T.second.begin();
        for (; T_SecondBegin != T.second.end();)
        {
            if (m_stopWords.find(*T_SecondBegin) != m_stopWords.end() ||
                std::regex_match(*T_SecondBegin, number_pattern) ||
                std::regex_match(*T_SecondBegin, number_pattern)/*  ||
                isNotChinese(*T_SecondBegin) */) // ← 过滤掉纯数字和小数)
            {
                // 是停用词
                // 删去
                T_SecondBegin = T.second.erase(T_SecondBegin);
            }
            else
            {
                // 不是停用词
                // 记录单词在该doc文档出现的次数
                word_frqc[T.first][*T_SecondBegin]++;
                ++T_SecondBegin;
            }
        }
    }
    cout << "停用词剔除完毕" << endl;
    unordered_map<std::string, int> word_xml_frqc;    // 记录包含词语的文档个数
    unordered_map<std::string, vector<int>> word_ids; // 记录单词都在哪个文档出现过
    // 统计整个xml文档中各个单词出现的次数
    for (auto &T : word_frqc)
    {
        for (auto &words : T.second)
        {
            word_xml_frqc[words.first]++;
            // todo 注释
            word_ids[words.first].push_back(T.first);
        }
    }
    ofstream outfile{filename};
    if (!outfile.is_open())
    {
        cerr << "文件打开失败: " << filename << endl;
        return;
    }
    // 输出
    // 计算出每个文档中的各个词语的归一化权重
    unordered_map<int, map<string, double>> ids_word_weight; // 记录各个doc文档中各个出现的词语的权重
    for (auto &T : map_id_words)
    {
        for (auto &word : T.second)
        {
            double TF = static_cast<double>(word_frqc[T.first][word]) / map_id_words[T.first].size();
            double DF = word_xml_frqc[word];
            double IDF = log2(m_documents.size() / DF);
            ids_word_weight[T.first][word] = TF * IDF;
        }
    }
    unordered_map<int, map<string, double>> ids_word_allweight; // 记录各个doc文档中各个出现的词语的归一化权重
    for (auto &T : ids_word_weight)
    {
        double weight2pp;
        // 算出权重的平方和
        for (auto &weight2 : T.second)
        {
            weight2pp += (weight2.second * weight2.second);
        }
        weight2pp = sqrt(weight2pp);
        // 写回ids_word_allweight中
        for (auto &weight2 : T.second)
        {
            ids_word_allweight[T.first][weight2.first] = weight2.second / weight2pp;
        }
    }
    // 输出文件中
    for (auto &T : word_xml_frqc)
    {
        outfile << T.first << " ";
        for (auto &ids : word_ids[T.first])
        {
            outfile << ids << " " << ids_word_allweight[ids][T.first] << " ";
        }
        outfile << '\n';
    }
}
void PageProcessor::setStopWords(const std::string &stopwordDir)
{
    std::ifstream file(stopwordDir);
    if (!file.is_open())
    {
        std::cerr << "无法打开文件: " << stopwordDir << std::endl;
        return;
    }
    std::string line;
    while (std::getline(file, line))
    {
        // 剔除该行的所有空格(不然有可能 与后续想要剔除的数据 匹配不成功)
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

        // 防止插入多余的空行数据
        if (!line.empty())
        {
            m_stopWords.insert(line);
        }
    }
    file.close();
}