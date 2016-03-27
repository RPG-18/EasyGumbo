#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <gumbo.h>

#include "Gumbo.hpp"

using namespace std;

std::string readAll(const std::string &fileName)
{
    std::ifstream ifs;
    ifs.open(fileName);
    ifs.seekg(0, std::ios::end);
    size_t length = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::string buff(length, 0);
    ifs.read(&buff[0], length);
    ifs.close();

    return buff;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        return 0;
    }

    using namespace EasyGumbo;

    auto page = readAll(argv[1]);
    Gumbo parser(&page[0]);

    Gumbo::iterator iter = parser.begin();
    while (iter != parser.end()) {
        iter = std::find_if(iter, parser.end(),
                            And(Tag(GUMBO_TAG_A),
                                HasAttribute("class", "post_title")));
        if (iter == parser.end()) {
            break;
        }

        Element titleA(*iter);
        auto text = titleA.children()[0];
        std::cout << "***\n";
        std::cout << std::setw(8) << "Title" << " : " << text->v.text.text << std::endl;
        std::cout << std::setw(8) << "Url" << " : " << titleA.attribute("href")->value << std::endl;

        iter = std::find_if(iter, parser.end(),
                            And(Tag(GUMBO_TAG_DIV),
                                HasAttribute("class", "hubs")));

        std::cout << std::setw(8) << "Hubs" << " : ";
        auto hubs = findAll(iter.fromCurrent(), parser.end(), Tag(GUMBO_TAG_A));
        for (auto hub: hubs) {
            Element hubA(*hub);
            if (hub != hubs[0]) {
                std::cout << ", ";
            }
            std::cout << hubA.children()[0]->v.text.text;
        }
        std::cout << std::endl;

        iter = std::find_if(iter, parser.end(),
                            And(Tag(GUMBO_TAG_DIV),
                                HasAttribute("class", "views-count_post")));
        ++iter;
        std::cout << std::setw(8) << "Views" << " : " << iter->v.text.text << std::endl;

        iter = std::find_if(iter, parser.end(),
                            And(Tag(GUMBO_TAG_SPAN),
                                HasAttribute("class", "favorite-wjt__counter js-favs_count")));
        ++iter;
        std::cout << std::setw(8) << "Stars" << " : " << iter->v.text.text << std::endl;
        iter = std::find_if(iter, parser.end(),
                            And(Tag(GUMBO_TAG_A),
                                HasAttribute("class", "post-author__link")));
        Element authorA(*iter);
        std::cout<<std::setw(8) << "Author" << " : "<<authorA.children()[2]->v.text.text<<std::endl;

        iter = std::find_if(iter, parser.end(),
                            And(Tag(GUMBO_TAG_DIV),
                                HasAttribute("class", "post-comments")));
        auto comments = findAll(iter.fromCurrent(), parser.end(), Tag(GUMBO_TAG_A));
        if(comments.size()==1)
        {
            Element commentsA(*comments[0]);
            std::cout<<std::setw(8) << "Comments" << " : " <<commentsA.children()[0]->v.text.text<<std::endl;
        }
    }

    return 0;
}