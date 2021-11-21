#include "net.h"
#include "graph.h"
#include <memory.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <arpa/inet.h>

bool_t
node_set_loopback_address(node_t *node, char *ip_addr)
{
    assert(ip_addr);

    node->node_nw_prop.is_lb_addr_config = TRUE;
    strncpy(NODE_LO_ADDR(node), ip_addr, 16);
    NODE_LO_ADDR(node)
    [16] = '\0';

    return TRUE;
}

bool_t
node_set_intf_ip_address(node_t *node, char *local_if, char *ip_addr, char mask)
{
    interface_t *interface = get_node_if_by_name(node, local_if);
    if (!interface)
        assert(0);
    interface->intf_nw_props.is_ip_addr_config = TRUE;
    strncpy(IF_IP(interface), ip_addr, 16);
    IF_IP(interface)
    [16] = '\0';
    interface->intf_nw_props.mask = mask;
    return TRUE;
}

static unsigned int
hash_code(void *ptr, unsigned int size){
    unsigned int value=0, i =0;
    char *str = (char*)ptr;
    while(i < size)
    {
        value += *str;
        value*=97;
        str++;
        i++;
    }
    return value;
}


/*Heuristics, Assign a unique mac address to interface*/
void
interface_assign_mac_address(interface_t *interface){

    unsigned int hash_code_val = hash_code(interface, sizeof(interface_t));
    memset(IF_MAC(interface), 0, sizeof(IF_MAC(interface)));
    memcpy(IF_MAC(interface), (char *)&hash_code_val, sizeof(unsigned int));
}


interface_t *
node_get_matching_subnet_interface(node_t *node, char *ip_addr)
{
    char *intf_addr = NULL;
    char mask;
    char intf_subnet[16];
    char subnet[16];
    for (int i = 0; i < MAX_INTF_PER_NODE; i++)
    {
        interface_t *intf = node->intf[i];
        if (!intf)
            return NULL;
        if (intf->intf_nw_props.is_ip_addr_config)
        {
            intf_addr = IF_IP(intf);
            mask = intf->intf_nw_props.mask;
            memset(intf_subnet, 0, 16);
            memset(subnet, 0, 16);
            apply_mask(intf_addr, mask, intf_subnet);
            apply_mask(ip_addr, mask, subnet);
            if (strcmp(intf_subnet, subnet) == 0)
                return intf;
        }
    }
}

unsigned int
ip_addr_p_to_n(char *ip_addr)
{
    unsigned int decimal_prefix;
    inet_pton(AF_INET, ip_addr, &decimal_prefix);
    decimal_prefix = htonl(decimal_prefix);
    return decimal_prefix;
}

void 
ip_addr_n_to_p(unsigned int ip_addr, char *ip_add_str)
{
    ip_addr = htonl(ip_addr);
    inet_ntop(AF_INET, &ip_addr, ip_add_str, 16);
}

void dump_nw_graph(graph_t *graph)
{
    node_t *node;
    printf("Topology Name: %s\n", graph->topology_name);
    glthread_t *glthreadptr = NULL;
    glthread_t *base_glthread = &graph->node_list;
    ITERATE_GLTHREAD_BEGIN(base_glthread, glthreadptr)
    {
        node = graph_glue_to_node(glthreadptr);
        dump_nw_node(node);
    }
    ITERATE_GLTHREAD_END(base_glthread, glthreadptr)
}

void dump_nw_node(node_t *node)
{
    printf("Node name: %s\n", node->node_name);
    if (node->node_nw_prop.is_lb_addr_config)
        printf("\tlo addr: %s/32\n", NODE_LO_ADDR(node));
    for (int i = 0; i < MAX_INTF_PER_NODE; i++)
    {
        if (!node->intf[i])
            return;
        dump_nw_interface(node->intf[i]);
    }
}

void dump_nw_interface(interface_t *interface)
{
    dump_interface(interface);
    if (IS_INTF_L3_MODE(interface))
        printf("\tIp Addr: %s/%u", IF_IP(interface), interface->intf_nw_props.mask);
    else
        printf("\tIp Addr: %s/%u", "Nil", 0);
        printf("\t MAC : %u:%u:%u:%u:%u:%u\n", 
            IF_MAC(interface)[0], IF_MAC(interface)[1],
            IF_MAC(interface)[2], IF_MAC(interface)[3],
            IF_MAC(interface)[4], IF_MAC(interface)[5]);
}

char* pkt_buffer_right_shift(char *pkt, unsigned int pkt_size, unsigned int total_buffer_size) {
    char *temp = NULL;
    bool_t need_temp_memory = FALSE;

    if(pkt_size * 2 > total_buffer_size){
        need_temp_memory = TRUE;
    }
    
    if(need_temp_memory){
        temp = calloc(1, pkt_size);
        memcpy(temp, pkt, pkt_size);
        memset(pkt, 0, total_buffer_size);
        memcpy(pkt + (total_buffer_size - pkt_size), temp, pkt_size);
        free(temp);
        return pkt + (total_buffer_size - pkt_size);
    }
    
    memcpy(pkt + (total_buffer_size - pkt_size), pkt, pkt_size);
    memset(pkt, 0, pkt_size);
    return pkt + (total_buffer_size - pkt_size);
}

