#ifndef XML_PARSER_H
#define XML_PARSER_H

typedef struct XmlNode {
    char *tag;
    char *content;
    struct XmlNode *children;
    struct XmlNode *next;
} XmlNode;

XmlNode *xml_parse(const char *xml);
void xml_free(XmlNode *root);
void xml_print(XmlNode *node, int indent);

#endif