#include "parsing/HTML_parser.h"
#include <iostream>

HtmlParser::HtmlParser() : gumbo_output(nullptr) {}

HtmlParser::~HtmlParser() {
    if (gumbo_output) {
        gumbo_destroy_output(&kGumboDefaultOptions, gumbo_output);
    }
}

bool HtmlParser::parse(const std::string& html) {
    if (gumbo_output) {
        gumbo_destroy_output(&kGumboDefaultOptions, gumbo_output);
    }
    links.clear();
    plain_text_content_.clear();

    gumbo_output = gumbo_parse(html.c_str());
    return gumbo_output != nullptr;
}

std::vector<std::string> HtmlParser::getLinks() {
    if (!gumbo_output) return {};
    searchForLinks(gumbo_output->root);
    return links;
}

void HtmlParser::searchForLinks(GumboNode* node) {
    if (node->type != GUMBO_NODE_ELEMENT) return;

    if (node->v.element.tag == GUMBO_TAG_A) {
        GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
        if (href) {
            links.push_back(href->value);
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        searchForLinks(static_cast<GumboNode*>(children->data[i]));
    }
}

std::string HtmlParser::getText() {
    if (!gumbo_output) return "";
    extractText(gumbo_output->root);
    return plain_text_content_;
}

void HtmlParser::extractText(GumboNode* node) {
    if (node->type == GUMBO_NODE_TEXT) {
        plain_text_content_.append(node->v.text.text);
        plain_text_content_.append(" ");
    } else if (node->type == GUMBO_NODE_ELEMENT &&
               node->v.element.tag != GUMBO_TAG_SCRIPT &&
               node->v.element.tag != GUMBO_TAG_STYLE) {
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            extractText(static_cast<GumboNode*>(children->data[i]));
        }
    }
}