
/**
 * xml parser head file
 */
 
#ifndef _XML_PARSER_H_
#define _XML_PARSER_H_

#include <stdlib.h>

#define XML_PARSER_OK           (0)
#define XML_PARSER_ERR_PARAM    (-1)
#define XML_PARSER_ERR_NO_NODE  (-2)
#define XML_PARSER_ERR_FORMAT   (-3)
#define XML_PARSER_ERR_NO_CONTEXT (-4)

typedef struct xml_context_s
{
 const char *context; 
 unsigned int context_len;
}xml_context_t;

#define XML_END_NODE_TYPE_CONTEXT (1)
#define XML_END_NODE_TYPE_CHILD_NODE (2)

typedef struct xml_node_s xml_node_t;

struct xml_node_s
{
 xml_node_t *parent_node;
 xml_node_t *next_node;
 
 unsigned int node_len;
 unsigned int tag_len;
 const char *tag;
 int end_node; /* if end_node = 1, union is context, or child_node_list */
 union 
 {
  xml_context_t context;
  xml_node_t *child_node_list;
 }next;
};

#if 0
static int malloc_cnt=0;
#define XML_PARSER_MALLOC(size) (XML_DEBUG("malloc cnt:%d", ++malloc_cnt),(xml_node_t *)malloc(size))
#define XML_PARSER_FREE(ptr) (XML_DEBUG("malloc cnt:%d", --malloc_cnt),free((void *)(ptr)))
#endif
#define XML_PARSER_MALLOC(size) (xml_node_t *)malloc(size)
#define XML_PARSER_FREE(ptr) free((void *)(ptr))



int xml_parser_load_xml(const char *xml_doc, int xml_doc_len, xml_node_t *root_node);

int xml_parser_get_node_context(const xml_node_t *node, char *context, int *len);
int xml_parser_get_node_context(const xml_node_t *node, char *context, int *len);
int xml_parser_get_context_by_tag(const xml_node_t *node, const char *tag, char *context, int *len);


int xml_parser_release_xml(xml_node_t *root_node);












#endif
