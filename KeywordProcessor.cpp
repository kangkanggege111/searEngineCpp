#include "KeywordProcessor.h"

// KeyWordProcessor类默认构造
KeyWordProcessor::KeyWordProcessor() {}
void KeyWordProcessor::process(const std::string &cnDir, const std::string &enDir, const std::string &cn_stopwordDir,
                               const std::string &en_stopwordDir, const std::string &out_cnLibDir,
                               const std::string &out_enLibDir, const std::string &out_cnIndexDir, const std::string &out_enIndexDir)
{
    set_cnStopWords(cn_stopwordDir);
    set_enStopWords(en_stopwordDir);

    create_cn_dict(cnDir, out_cnLibDir);
    create_en_dict(enDir, out_enLibDir);

    build_cn_index(out_cnLibDir, out_cnIndexDir);
    build_en_index(out_enLibDir, out_enIndexDir);
}
void KeyWordProcessor::create_cn_dict(const std::string &corpusDir, const std::string &outfileDir)
{
    std::vector<std::string> CH_filenames;
    CH_filenames = DirectoryScanner::scan(corpusDir);
    std::unordered_map<std::string, int> map_result;

    for (auto &filename : CH_filenames)
    {
        std::vector<std::string> vec_result;
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "无法打开文件: " << filename << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) // 一次读一行
        {

            m_tokenizer.Cut(line, vec_result);
            // 将vector中的内容写入map中去(自动去重,带次数)
            for (auto &T : vec_result)
            {
                map_result[T]++;
            }
            line.clear();
        }
        file.close();
    }
    // 清洗,过滤非中文字符
    auto it = map_result.begin();
    while (it != map_result.end())
    {
        const std::string &s = it->first;
        bool allChinese = true;

        // 用 UTF-8 迭代器逐个检查字符
        for (auto ch_it = utf8::iterator{s.begin(), s.begin(), s.end()};
             ch_it != utf8::iterator{s.end(), s.begin(), s.end()};
             ++ch_it)
        {
            char32_t cp = *ch_it;
            if (!((cp >= 0x4E00 && cp <= 0x9FFF) || // CJK Unified Ideographs
                  (cp >= 0x3400 && cp <= 0x4DBF)))  // CJK Extension A
            {
                allChinese = false;
                break;
            }
        }

        if (!allChinese)
        {
            it = map_result.erase(it); // 删除非汉字词条
        }
        else
        {
            ++it; // 继续下一个
        }
    }
    // 过滤掉停用词
    for (const auto &stop : m_chStopWords)
    {
        auto it = map_result.find(stop);
        if (it != map_result.end())
        {
            map_result.erase(it);
        }
    }

    // 将map的内容写入到文件中去
    std::ofstream outfile(outfileDir);
    if (!outfile.is_open())
    {
        std::cerr << "无法打开文件: " << outfileDir << std::endl;
        return;
    }
    for (const auto &[key, value] : map_result)
    {
        outfile << key << " " << value << '\n';
    }
    outfile.close();
}
// 构建中文索引库
void KeyWordProcessor::build_cn_index(const std::string &dictDir, const std::string &outfileDir)
{
    std::string line;
    int lineNum = 1;
    std::string num;
    std::map<std::string, std::set<int>> map_result;
    std::ifstream ifs(dictDir);
    while (ifs >> line >> num) // 一次读一行
    {
        const char *it = line.c_str();
        const char *end = line.c_str() + line.size();
        while (it != end)
        {
            auto start = it;
            utf8::next(it, end); // 将it移动到下一个utf8字符所在的位置
            // 因为一个汉字需要占用多个字节,我们可以用std::string来表示一个汉字
            std::string alpha = std::string{start, it};
            // std::cout << alpha << std::endl;
            // 目前还不存在这个key
            map_result[alpha].insert(lineNum);
        }
        ++lineNum;
        line.clear();
    }
    ifs.close();
    // 将map_result中的内容塞到文件中去
    std::ofstream outfile(outfileDir);
    if (!outfile.is_open())
    {
        std::cerr << "无法打开文件: " << outfileDir << std::endl;
        return;
    }
    for (const auto &[key, value_set] : map_result)
    {
        outfile << key << " ";
        for (auto &T : value_set)
        {
            outfile << T << " ";
        }
        outfile << '\n';
    }
    outfile.close();
}

void KeyWordProcessor::create_en_dict(const std::string &corpusDir, const std::string &outfileDir)
{
    // 将目录中的全部文件的路径存放到vector en_filenames中
    std::vector<std::string> en_filenames;
    en_filenames = DirectoryScanner::scan(corpusDir);
    std::unordered_map<std::string, int> map_result;
    for (auto &filename : en_filenames)
    {
        std::vector<std::string> vec_result;
        std::ifstream file(filename);
        if (!file.is_open())
        {
            std::cerr << "无法打开文件: " << filename << std::endl;
            return;
        }
        std::string line;

        while (std::getline(file, line)) // 一次读一行
        {
            vec_result = tokenize_line(line);
            // 将vector中的内容写入map中去(自动去重,带次数)
            for (auto &T : vec_result)
            {
                map_result[T]++;
            }
            line.clear();
        }
        file.close();
    }
    // 过滤掉停用词
    for (const auto &stop : m_enStopWords)
    {
        auto it = map_result.find(stop);
        if (it != map_result.end())
        {
            map_result.erase(it);
        }
    }

    // 将map的内容写入到文件中去
    std::ofstream outfile(outfileDir);
    if (!outfile.is_open())
    {
        std::cerr << "无法打开文件: " << outfileDir << std::endl;
        return;
    }
    for (const auto &[key, value] : map_result)
    {
        outfile << key << " " << value << '\n';
    }
    outfile.close();
}
void KeyWordProcessor::build_en_index(const std::string &dictDir, const std::string &outfileDir)
{
    std::string line;
    int lineNum = 1;
    std::string num; // 无用
    std::map<char, std::set<int>> map_result;
    std::ifstream ifs(dictDir);
    while (ifs >> line >> num) // 一次读一行
    {
        const char *it = line.c_str();
        const char *end = line.c_str() + line.size();
        while (it != end)
        {
            map_result[(*it++)].insert(lineNum);
        }
        ++lineNum;
        line.clear();
    }
    ifs.close();
    // 将map_result中的内容塞到文件中去
    std::ofstream outfile(outfileDir);
    if (!outfile.is_open())
    {
        std::cerr << "无法打开文件: " << outfileDir << std::endl;
        return;
    }
    for (const auto &[key, value_set] : map_result)
    {
        outfile << key << " ";
        for (auto &T : value_set)
        {
            outfile << T << " ";
        }
        outfile << '\n';
    }
    outfile.close();
}

void KeyWordProcessor::set_cnStopWords(const std::string &dir)
{
    std::ifstream file(dir);
    if (!file.is_open())
    {
        std::cerr << "无法打开文件: " << dir << std::endl;
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
            m_chStopWords.insert(line);
        }
    }
}
void KeyWordProcessor::set_enStopWords(const std::string &dir)
{
    std::ifstream file(dir);
    if (!file.is_open())
    {
        std::cerr << "无法打开文件: " << dir << std::endl;
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
            m_enStopWords.insert(line);
        }
    }
}
std::vector<std::string> KeyWordProcessor::tokenize_line(const std::string &line)
{
    std::string cleaned;
    cleaned.reserve(line.size());

    for (unsigned char ch : line)
    {
        if (std::isalpha(ch))
        { // 只保留字母,顺带转小写
            cleaned.push_back(static_cast<char>(std::tolower(ch)));
        }
        else
        {
            cleaned.push_back(' '); // 非字母变空格，便于分割
        }
    }

    std::vector<std::string> tokens;
    std::istringstream iss(cleaned);
    // 按空白分割（连续空白自动合并）,依次塞到vector中去
    for (std::string w; iss >> w;)
    {
        tokens.push_back(std::move(w));
    }
    return tokens;
}