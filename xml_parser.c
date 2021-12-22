#include <stdio.h>
#include <string.h>
#include "xml_parser.h"

#if 0
#define XML_DEBUG(fmt, ...) printf("%s[%d]" fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define XML_DEBUG_PRINTF_STRING(tips, str, len) do{printf("%s[%d]:[%d]%s:", __FUNCTION__, __LINE__, len, tips);for (int i=0;i<(len);i++) printf("%c", ((char *)(str))[i]);printf("\n");}while(0)
#else
#define XML_DEBUG(fmt, ...)
#define XML_DEBUG_PRINTF_STRING(tips, str, len)
#endif

static int find_tag_start(const char *context, int len, char **tag, int *tag_len)
{
 int index;
 int got_annotation;
 int tag_size;

 tag_size = 0;
 for (index = 0; index < len; )
 {
  if (context[index] == '<')
  {
   if ('!' == context[index+1]) // annotation
   {
    XML_DEBUG("find annotation start flag");
    got_annotation = 0;
    index++;
    for (;index < len; index++)
    {
     if ('>' == context[index])
     {
      index++;
      XML_DEBUG("find annotation end flag @%d", index-1);
      got_annotation = 1;
      break;
     }
    }

    if (got_annotation)
    {
     got_annotation = 0;
     continue;
    }
    
    /* no find annotation end flag */
    XML_DEBUG("annotation format err");
    return -1;
   }
   else if ('/' == context[index+1])
   {
    XML_DEBUG("find a end flag, error exit");
    return -1;
   }
   
   XML_DEBUG("find < @%d", index);
   index++;
   *tag = &context[index];
   tag_size = 0;
   break;
  }
  index++;
 }
 if (index >= len) /* no find '<' */
 {
  return -1;
 }
 
 while((index < len) && (context[index] != '>'))
 {
  tag_size++;
  index++;
 }
 index++;
 if (index >= len)
 {
  return -1;
 }
 *tag_len = tag_size;
 XML_DEBUG("find > @%d, tag len:%d", index-1, tag_size);
 XML_DEBUG_PRINTF_STRING("tag", *tag, *tag_len);

 return index;
}

//return >0: tag end position. <0:error
static int find_tag_end(const char *context, int len, const char *tag, int tag_len)
{
 int index;

 //XML_DEBUG("[%d]%s", len, context);

 for (index = 0; index < len; index++)
 {
  if ('<' == context[index])
  {
   index++;
   break;
  }
 }

 if (index >= len)
 {
  return -1;
 }
 
 if ((context[index] == '/') 
  && ((index + tag_len + 1) < len)
     && (0 == memcmp(tag, &context[index+1], tag_len))
  && (context[index+tag_len+1] == '>'))
 {
  XML_DEBUG("find tag ending");
  return (index + tag_len+1+1);
 }
 else
 { 
  XML_DEBUG("not find tag ending");
  return -1;
 }
}

static void free_node(xml_node_t *node)
{
 xml_node_t *child_node;
 
 if (NULL == node)
 {
  return;
 }

 if (node->next_node)
 {
  XML_DEBUG_PRINTF_STRING("free node tag brother node", node->next_node->tag, node->next_node->tag_len);
  free_node(node->next_node);
  XML_PARSER_FREE(node->next_node);
  node->next_node = NULL;
 }
 if (XML_END_NODE_TYPE_CHILD_NODE == node->end_node)
 {
  child_node = node->next.child_node_list;
  XML_DEBUG_PRINTF_STRING("free node tag child node", child_node->tag, child_node->tag_len);
  free_node(child_node);
  XML_PARSER_FREE(child_node);
  node->next.child_node_list = NULL;
 }
}

int find_next_node(const char *context, int len, xml_node_t *parent_node, xml_node_t *node, int *used_len)
{
 int index;
 int i;
 xml_node_t *next_node;
 xml_node_t *child_node;
 int node_used_len;
 int ret;
 
 XML_DEBUG("len:%d", len);
 
 if (len <= 0)
 {
  return XML_PARSER_ERR_PARAM;
 }

 ret = find_tag_start(context, len, &node->tag, &node->tag_len);
 if (ret < 0)
 {
  return -1;
 }
 index = ret;
 XML_DEBUG_PRINTF_STRING("tag:", node->tag, node->tag_len);

 ret = find_tag_end(&context[index], len-index, node->tag, node->tag_len);
 if (ret > (int)(node->tag_len + 3))
 {
  node->parent_node = parent_node;
  node->end_node = XML_END_NODE_TYPE_CONTEXT;
  node->next.context.context = &context[index];
  node->next.context.context_len = ret - (node->tag_len + 3);
  node->next_node = NULL;
  index += ret;
  *used_len = index;

  XML_DEBUG_PRINTF_STRING("tag", node->tag, node->tag_len);
  XML_DEBUG_PRINTF_STRING("got context", node->next.context.context, node->next.context.context_len);
 }
 else{
  XML_DEBUG("find child node...");
  
  //find child node
  child_node = XML_PARSER_MALLOC(sizeof(xml_node_t));
  if (NULL == child_node)
  {
   XML_DEBUG("malloc node err, exit");
   free_node(node);
   return -1;
  }
  memset((void *)child_node, 0, sizeof(xml_node_t));
  
  ret = find_next_node(&context[index], len-index, node, child_node, &node_used_len);
  if (ret)
  {
   XML_PARSER_FREE(child_node);
   free_node(node);
   XML_DEBUG("not find child node, err:%d", ret);
   return -1;
  }
  
  XML_DEBUG("got child node, node used len:%d", node_used_len);
  child_node->parent_node = node;
  node->parent_node = parent_node;
  node->end_node = XML_END_NODE_TYPE_CHILD_NODE;
  node->next.child_node_list = child_node;
  index += node_used_len;
  *used_len = index;
  node->next_node = NULL;

  //find the tag end to finish this tag
  ret = find_tag_end(&context[index], len-index, node->tag, node->tag_len);
  if (ret > (node->tag_len + 3))
  {
   XML_DEBUG("find tag end flag");
   index += ret;
   *used_len += index;
  }
  else
  {
   //error
   XML_DEBUG("not find tag end flag, err");
   free_node(node);
   return -1;
  }
 }
 
 /**************************************/
 XML_DEBUG("continue to search brother node");
 XML_DEBUG("[%d]%s", len-index, &context[index]);
 
 next_node = XML_PARSER_MALLOC(sizeof(xml_node_t));
 if (NULL == next_node)
 {
  XML_DEBUG("malloc node err, exit");
  free_node(node);
  return -1;
 }
 memset((void *)next_node, 0, sizeof(xml_node_t));
 
 ret = find_next_node(&context[index], len-index, parent_node, next_node, &node_used_len);
 if (ret)
 {
  XML_DEBUG("not find brother node, err:%d", ret);
  XML_PARSER_FREE(next_node);
 }
 else
 {
  node->next_node = next_node;
  *used_len += node_used_len;
  
  XML_DEBUG("find brother node, used len:%d", *used_len);
 }
 
 return 0;
}

int xml_parser_load_xml(const char *xml_doc, int xml_doc_len, xml_node_t *root_node)
{
 int i;
 int ret;
 int used_len;
 int head_ok;

 if (NULL == xml_doc)
 {
  return XML_PARSER_ERR_PARAM;
 }

 if (xml_doc_len <= 0)
 {
  return XML_PARSER_ERR_PARAM;
 }

 if (NULL == root_node)
 {
  return XML_PARSER_ERR_PARAM;
 }
 
 root_node->parent_node = NULL;
 
 //find "<?xml"
 head_ok = 0;
 for (i=0;i<xml_doc_len;i++)
 {
  if (xml_doc[i] != '<') continue;
  if ('!' == xml_doc[i+1]) 
  {
   i+=2;
   for (;i<xml_doc_len; i++)
   {
    if ('>' == xml_doc[i])
    {
     i++;
     break;
    }
   }
   
   if (i>=xml_doc_len) //error
   {
    break;
   }
   
   continue;
  }
  else {
   i++;
   if (((i + strlen("?xml")) < xml_doc_len)
    &&(0 == memcmp(&xml_doc[i], "?xml", strlen("?xml"))))
   {
    for (i += strlen("?xml");i<xml_doc_len; i++)
    {
     if ('>' == xml_doc[i])
     {
      i++;
      head_ok = 1;
      XML_DEBUG("got, @%d ", i);
      break;
     }
    }
   }
   break;
  }
 }
 if (i>=xml_doc_len) //error
 {
  return XML_PARSER_ERR_FORMAT;
 }
 
 if (!head_ok)
 {
  return XML_PARSER_ERR_FORMAT;
 }
 
 XML_DEBUG("drop xml head, body:[%d]%s", xml_doc_len-i, &xml_doc[i]);
 
 ret = find_next_node(xml_doc+i, xml_doc_len-i, NULL, root_node, &used_len);
 if (ret)
 {
  return XML_PARSER_ERR_FORMAT;
 }
 
 return XML_PARSER_OK;
}

int xml_parser_get_node(const xml_node_t *src_node, xml_node_t **des_note, const char *tag)
{
 int ret;

 if (NULL == src_node)
 {
  return XML_PARSER_ERR_PARAM;
 }

 if (NULL == des_note)
 {
  return XML_PARSER_ERR_PARAM;
 }

 if ((NULL == tag) || (strlen(tag) <= 0))
 {
  return XML_PARSER_ERR_PARAM;
 }
 
 if ((strlen(tag) == src_node->tag_len)
  && (0 == memcmp(src_node->tag, tag, src_node->tag_len)))
 {
  *des_note = src_node;
  XML_DEBUG("find node!");
  return XML_PARSER_OK;
 }

 if ((XML_END_NODE_TYPE_CHILD_NODE == src_node->end_node)
  && src_node->next.child_node_list)
 {
  ret = xml_parser_get_node(src_node->next.child_node_list, des_note, tag);
  if (XML_PARSER_OK == ret)
  {
   return XML_PARSER_OK;
  }
 }

 if (src_node->next_node)
 {
  ret = xml_parser_get_node(src_node->next_node, des_note, tag);
  if (XML_PARSER_OK == ret)
  {
   return XML_PARSER_OK;
  }
 }
 
 return XML_PARSER_ERR_NO_NODE;
}

int xml_parser_get_node_context(const xml_node_t *node, char *context, int *len)
{
 if (NULL == node)
 {
  return XML_PARSER_ERR_PARAM;
 }

 if (NULL == context)
 {
  return XML_PARSER_ERR_PARAM;
 }

 if ((NULL == len) && (*len < 0))
 {
  return XML_PARSER_ERR_PARAM;
 }

 if (XML_END_NODE_TYPE_CONTEXT == node->end_node)
 {
  if (node->next.context.context_len < *len)
  {
   *len = node->next.context.context_len;
  }

  if (*len > 0)
  {
   memcpy(context, node->next.context.context, *len);
  }

  return XML_PARSER_OK;
 }

 return XML_PARSER_ERR_NO_CONTEXT;
}

int xml_parser_get_context_by_tag(const xml_node_t *node, const char *tag, char *context, int *len)
{
 int ret;
 xml_node_t *des_node;
 
 ret = xml_parser_get_node(node, &des_node, tag);
 if (ret != XML_PARSER_OK)
 {
  XML_DEBUG("error exit:%d", ret);
  return ret;
 }

 return xml_parser_get_node_context(des_node, context, len);
}

int xml_parser_release_xml(xml_node_t *root_node)
{
 if (NULL == root_node)
 {
  return XML_PARSER_ERR_PARAM;
 }
 
 free_node(root_node);
 return XML_PARSER_OK;
}

static void display_node(const xml_node_t *root_node)
{
 xml_node_t *node;

 node = root_node;
 do 
 {
  XML_DEBUG_PRINTF_STRING("node tag", node->tag, node->tag_len);
  if (XML_END_NODE_TYPE_CONTEXT == node->end_node)
  {
   XML_DEBUG_PRINTF_STRING("context", node->next.context.context, node->next.context.context_len);
  }
  else
  {
   XML_DEBUG("find child node");
   display_node(node->next.child_node_list);
  }
  if (node->next_node != NULL)
  {
   XML_DEBUG("find brother node");
   node = node->next_node;
  }
  else
  {
   break;
  }
 }
 while(1);

}


