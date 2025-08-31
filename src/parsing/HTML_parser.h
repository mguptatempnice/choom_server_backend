#ifndef HTML_PARSER_H
#define HTML_PARSER_H

#include <string>
#include <vector>
#include <gumbo.h>

class HtmlParser {
public:
    HtmlParser();
    ~HtmlParser();

    bool parse(const std::string& html);
    std::vector<std::string> getLinks();
    std::string getText(); 

private:
    void searchForLinks(GumboNode* node);
    void extractText(GumboNode* node); 

    GumboOutput* gumbo_output;
    std::vector<std::string> links;
    std::string plain_text_content_; 
};

#endif 