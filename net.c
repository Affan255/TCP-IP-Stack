#include "net.h"
#include "graph.h"
#include "utils.h"
#include<assert.h>

bool_t 
node_set_loopback_address(node_t *node, char *ip_addr) {
    assert(ip_addr);
    
    node->node_nw_prop.is_lb_addr_config = TRUE;
    strncpy(NODE_LO_ADDR(node), ip_addr, 16);
    NODE_LO_ADDR(node)[16]='\0';

    return TRUE;
}

bool_t 
node_set_intf_ip_address(node_t *node, char *local_if, char *ip_addr, char mask) {
    interface_t *interface = get_node_if_by_name(node, local_if);
    if (!interface)
        assert(0);
    interface->intf_nw_props.is_ip_addr_config = TRUE;
    strncpy(IF_IP(interface), ip_addr, 16);
    IF_IP(interface)[16]='\0';
    interface->intf_nw_props.mask = mask;
    return TRUE;
}

void
interface_assign_mac_address(interface_t *interface) {
    memset(IF_MAC(interface), 0, 48);
    strcpy(IF_MAC(interface), interface->att_node->node_name);
    strcat(IF_MAC(interface), interface->if_name);
}

void
dump_nw_graph(graph_t *graph) {
    node_t* node;
    printf("Topology Name: %s\n",graph->topology_name);
    glthread_t *glthreadptr = NULL;
    glthread_t *base_glthread = &graph->node_list;
    ITERATE_GLTHREAD_BEGIN(base_glthread, glthreadptr) {
        node = graph_glue_to_node(glthreadptr);
        dump_nw_node(node);
    }ITERATE_GLTHREAD_END(base_glthread, glthreadptr)
}

void
dump_nw_node(node_t* node) {
    printf("Node name: %s\n",node->node_name);
    if (node->node_nw_prop.is_lb_addr_config)
        printf("\tlo addr: %s/32\n", NODE_LO_ADDR(node));
    for (int i=0;i<MAX_INTF_PER_NODE;i++) {
        if (!node->intf[i])
            return;
        dump_nw_interface(node->intf[i]);
    }
}

void
dump_nw_interface(interface_t *interface) {
    dump_interface(interface);
    if(IS_INTF_L3_MODE(interface))
        printf("\tIp Addr: %s/%u",IF_IP(interface), interface->intf_nw_props.mask);
    else
        printf("\tIp Addr: %s/%u","Nil",0);
    printf(" MAC: %s\n",IF_MAC(interface));
}
