#include "DirectoryScanner.h"
#include "KeywordProcessor.h"
#include "PageProcessor.h"
#include "EchoServer.h"
int main()
{
 /*    KeyWordProcessor localProcessor;
    std::string chDir = "./corpus/CN";
    std::string enDir = "./corpus/EN";
    // 停止词存放路径
    std::string cn_stopwordDir = "./corpus/stopwords/cn_stopwords.txt";
    std::string en_stopwordDir = "./corpus/stopwords/en_stopwords.txt";
    // 字典库输出路径
    std::string out_cnLibDir = "./DictionaryLibrary/cnLib.txt";
    std::string out_enLibDir = "./DictionaryLibrary/enLib.txt";

    // 索引库输出路径
    std::string out_cnIndexDir = "./IndexDatabase/cnIndexLib.txt";
    std::string out_enIndexDie = "./IndexDatabase/enIndexLib.txt";

    localProcessor.process(chDir, enDir,
                           cn_stopwordDir, en_stopwordDir,
                           out_cnLibDir, out_enLibDir,
                           out_cnIndexDir, out_enIndexDie);

    PageProcessor webProvessor;
    std::string webpageSource = "./webpages";
    std::string webpagesDir = "./webpages.xml";
    std::string webOffsetsDir = "./webOffsetsPages.txt";
    std::string invertedIndexDir = "./invertedIndexLib.txt";
    std::string stopWordsDir = "./corpus/stopwords/cn_stopwords.txt";
    webProvessor.process(webpageSource, webpagesDir, webOffsetsDir, invertedIndexDir, stopWordsDir);
 */
    EchoServer server(4, 10, "127.0.0.1", 8888);
    server.start();
}