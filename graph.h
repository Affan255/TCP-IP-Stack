#ifndef __GRAPH__
#define __GRAPH__
#include "gluethread/glthread.h"
#include "net.h"
#include "comm.h"
#include <assert.h>
#define NODE_NAME_SIZE 16
#define IF_NAME_SIZE 16
#define MAX_INTF_PER_NODE 10

typedef struct link_ link_t;
typedef struct node_ node_t;
typedef struct interface_
{
    char if_name[IF_NAME_SIZE];
    struct node_ *att_node;
    struct link_ *link;
    intf_nw_props_t intf_nw_props; 
} interface_t;

struct link_
{
    interface_t intf1;
    interface_t intf2;
    unsigned int cost;
};

struct node_
{
    char node_name[NODE_NAME_SIZE];
    interface_t *intf[MAX_INTF_PER_NODE];
    glthread_t graph_glue;
    unsigned int udp_port_number;
    int udp_socket_fd;
    node_nw_prop_t node_nw_prop;
};

typedef struct graph_
{
    char topology_name[32];
    glthread_t node_list;
} graph_t;

GLTHREAD_TO_STRUCT(graph_glue_to_node, node_t, graph_glue)

graph_t *
create_new_graph(char *topology_name);

node_t *
create_new_graph_node(graph_t *graph, char *node_name);

void insert_link_between_two_nodes(node_t *node1, node_t *node2, char *from_if_name, char *to_if_name, unsigned int cost);

static inline node_t *
get_nbr_node(interface_t *interface)
{
    assert(interface->att_node);
    assert(interface->link);
    link_t *link = interface->link;
    if (interface == &link->intf1)
        return link->intf2.att_node;
    else
        return link->intf1.att_node;
}

static inline int
get_node_intf_available_slot(node_t *node)
{
    for (int i = 0; i < MAX_INTF_PER_NODE; i++)
    {
        if (!node->intf[i])
            return i;
    }
    return -1;
}

static inline interface_t *
get_node_if_by_name(node_t *node, char *if_name)
{
    if (!node)
        return NULL;
    for (int i = 0; i < MAX_INTF_PER_NODE; i++)
    {
        if (!node->intf[i])
            return NULL;
        if (strncmp(node->intf[i]->if_name, if_name, IF_NAME_SIZE) == 0)
            return node->intf[i];
    }
    return NULL;
}

static inline node_t *
get_node_by_node_name(graph_t *topo, char *node_name)
{
    node_t *node;
    glthread_t *base_glthread = &topo->node_list;
    glthread_t *glthreadptr = NULL;
    ITERATE_GLTHREAD_BEGIN(base_glthread, glthreadptr)
    {
        node = graph_glue_to_node(glthreadptr);
        if (strncmp(node->node_name, node_name, NODE_NAME_SIZE) == 0)
            return node;
    }
    ITERATE_GLTHREAD_END(base_glthread, glthreadptr)
    return NULL;
}

void dump_graph(graph_t *graph);
void dump_node(node_t *node);
void dump_interface(interface_t *interface);
#endif