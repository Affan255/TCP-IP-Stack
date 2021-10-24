#ifndef __NET__
#define __NET__
#include <stdio.h>
#include <string.h>
#include "utils.h"

typedef struct graph_ graph_t;
typedef struct interface_ interface_t;
typedef struct node_ node_t;

typedef struct ip_add_
{
    char ip_addr[16];
} ip_add_t;

typedef struct mac_add_
{
    char mac_addr[48];
} mac_add_t;

typedef struct node_nw_prop_
{
    bool_t is_lb_addr_config;
    ip_add_t lb_addr;
} node_nw_prop_t;

typedef struct intf_nw_props_ {
    mac_add_t mac_add;
    bool_t is_ip_addr_config;
    ip_add_t ip_add;
    char mask;
} intf_nw_props_t;

static inline void
init_intf_nw_props (intf_nw_props_t *intf_nw_props) {
    memset(intf_nw_props->mac_add.mac_addr, 0, 48);
    intf_nw_props->is_ip_addr_config = FALSE;
    memset(intf_nw_props->ip_add.ip_addr, 0, 16);
    intf_nw_props->mask = 0;
}

static inline void
init_node_nw_prop(node_nw_prop_t *node_nw_prop) {
    memset(node_nw_prop->lb_addr.ip_addr, 0, 16);
    node_nw_prop->is_lb_addr_config = FALSE;
}

#define IF_MAC(intf_ptr)    ((intf_ptr)->intf_nw_props.mac_add.mac_addr)
#define IF_IP(intf_ptr)    ((intf_ptr)->intf_nw_props.ip_add.ip_addr)
#define NODE_LO_ADDR(node_ptr)  ((node_ptr)->node_nw_prop.lb_addr.ip_addr)
#define IS_INTF_L3_MODE(intf_ptr)   ((intf_ptr)->intf_nw_props.is_ip_addr_config)

bool_t node_set_loopback_address(node_t *node, char *ip_addr);
bool_t node_set_intf_ip_address(node_t *node, char *local_if, char *ip_addr, char mask);

void dump_nw_graph(graph_t *graph);
void dump_nw_node(node_t *node);
void dump_nw_interface(interface_t *interface);

void interface_assign_mac_address(interface_t *interface);

#endif