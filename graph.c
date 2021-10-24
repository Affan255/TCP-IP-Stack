#include "graph.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

graph_t*
create_new_graph(char *topology_name) {
    graph_t *graph = (graph_t*)calloc(1,sizeof(graph_t));
    strncpy(graph->topology_name, topology_name, 32);
    graph->topology_name[31]='\0';
    init_glthread(&graph->node_list);
    return graph;
}

node_t*
create_new_graph_node(graph_t *graph, char *node_name) {
    node_t *node = (node_t*)calloc(1,sizeof(node_t));
    strncpy(node->node_name, node_name, NODE_NAME_SIZE);
    node->node_name[NODE_NAME_SIZE-1]='\0';
    init_node_nw_prop(&node->node_nw_prop);
    init_glthread(&node->graph_glue);
    glthread_add_next(&graph->node_list,&node->graph_glue);
    return node;
}

void
insert_link_between_two_nodes(node_t *node1, node_t *node2, char *from_if_name, char *to_if_name, unsigned int cost) {
    link_t *link = (link_t*)calloc(1,sizeof(link_t));
    strncpy(link->intf1.if_name, from_if_name, IF_NAME_SIZE);
    link->intf1.if_name[IF_NAME_SIZE]='\0';
    strncpy(link->intf2.if_name, to_if_name, IF_NAME_SIZE);
    link->intf2.if_name[IF_NAME_SIZE]='\0';  

    link->intf1.link = link;
    link->intf2.link = link;

    link->intf1.att_node=node1;
    link->intf2.att_node=node2;
    link->cost = cost;

    init_intf_nw_props(&link->intf1.intf_nw_props);
    interface_assign_mac_address(&link->intf1);
    init_intf_nw_props(&link->intf2.intf_nw_props);
    interface_assign_mac_address(&link->intf2);

    int empty_intf_slot;

    empty_intf_slot = get_node_intf_available_slot(node1);
    node1->intf[empty_intf_slot] = &link->intf1;
    empty_intf_slot = get_node_intf_available_slot(node2);
    node2->intf[empty_intf_slot] = &link->intf2;
}

void 
dump_graph(graph_t *graph) {
    node_t *node;
    printf("Topology Name = %s\n", graph->topology_name);
    glthread_t *base_glthread = &graph->node_list;
    glthread_t *glthreadptr = NULL;
    ITERATE_GLTHREAD_BEGIN(base_glthread, glthreadptr) {
        node = graph_glue_to_node(glthreadptr);
        dump_node(node);
    }ITERATE_GLTHREAD_END(base_glthread, glthreadptr);
}

void 
dump_node(node_t *node) {
    printf("Node name = %s :\n",node->node_name);
    for (int i=0;i<MAX_INTF_PER_NODE;i++) {
        if (!node->intf[i])
            break;
        dump_interface(node->intf[i]);    
    }
}

void
dump_interface(interface_t *interface) {
    link_t *link = interface->link;
    node_t *nbr_node = get_nbr_node(interface);
    printf("\tLocal Node = %s, Nbr Node = %s, Interface name = %s, Cost = %u\n",interface->att_node->node_name, nbr_node->node_name, interface->if_name, link->cost);
}