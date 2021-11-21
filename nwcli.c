#include "CommandParser/libcli.h"
#include "CommandParser/cmdtlv.h"
#include "cmdcodes.h"
#include "graph.h"
#include <stdio.h>
#include "Layer2/layer2.h"

extern graph_t *topo;
extern void
send_arp_broadcast_request( node_t *node, interface_t *oif, char *ip_addr);


static int
show_nw_topology_handler(param_t *param, ser_buff_t *tlv_buf, op_mode enable_or_disable) {
    int CMD_CODE = -1;
    CMD_CODE = EXTRACT_CMD_CODE(tlv_buf);
    switch (CMD_CODE)
    {
    case CMDCODE_SHOW_NW_TOPOLOGY:
        dump_nw_graph(topo);
        break;
    
    default:
        break;
    }
    return 0;
}

int
validate_node_name(char *value) {
    node_t *node = get_node_by_node_name(topo, value);
    if(!node)
        return VALIDATION_FAILED;
    return VALIDATION_SUCCESS;    
}

static int
show_arp_handler (param_t *param, ser_buff_t *tlv_buf, op_mode enable_or_disable) {
    int CMD_CODE = -1;
    CMD_CODE = EXTRACT_CMD_CODE(tlv_buf);
    tlv_struct_t *tlv = NULL;
    char *node_name;
    TLV_LOOP_BEGIN(tlv_buf, tlv) {
        if (strncmp(tlv->leaf_id, "node_name", strlen("node_name"))==0)
            node_name = tlv->value;  
    }TLV_LOOP_END
    node_t *node = get_node_by_node_name(topo, node_name);

    dump_arp_table(NODE_ARP_TABLE(node));
}

static int
arp_handler(param_t *param, ser_buff_t *tlv_buf, op_mode enable_or_disable) {
    int CMD_CODE = -1;
    CMD_CODE = EXTRACT_CMD_CODE(tlv_buf);
    tlv_struct_t *tlv = NULL;
    char *node_name;
    char *ip_addr;
    TLV_LOOP_BEGIN(tlv_buf, tlv) {
        if (strncmp(tlv->leaf_id, "node_name", strlen("node_name"))==0)
            node_name = tlv->value;
        if (strncmp(tlv->leaf_id, "ip-address", strlen("ip-address"))==0)
            ip_addr = tlv->value;    
                
    }TLV_LOOP_END

    node_t *node = get_node_by_node_name(topo, node_name);
    send_arp_broadcast_request(node, NULL, ip_addr);
    
    return 0;
}

void
nw_init_cli () {
    init_libcli();

    param_t *show = libcli_get_show_hook();
    param_t *debug = libcli_get_debug_hook();
    param_t *config = libcli_get_config_hook();
    param_t *run = libcli_get_run_hook();
    param_t *debug_show = libcli_get_debug_show_hook();
    param_t *root = libcli_get_root();
    {
        /* show topology */
       static param_t topology;
       init_param(&topology, CMD, "topology", show_nw_topology_handler, 0, INVALID, 0, "Dump Complete Network Topology");
       libcli_register_param(show, &topology);
       set_param_cmd_code(&topology, CMDCODE_SHOW_NW_TOPOLOGY);
       {
           /* show node */
           static param_t node;
           init_param(&node, CMD, "node", 0, 0, INVALID, 0, "Help: node");
           libcli_register_param(show, &node);
           {
               /* show node <node_name> */
               static param_t node_name;
               init_param(&node_name, LEAF, 0, 0, validate_node_name, STRING, "node_name", "Help: Node Name");
               libcli_register_param(&node, &node_name);
               {
                   /* show node <node_name> arp */
                   static param_t arp;
                   init_param(&arp, CMD, "arp", show_arp_handler, 0, INVALID, 0, "Help: arp");
                   libcli_register_param(&node_name, &arp);
                   set_param_cmd_code(&arp, CMDCODE_SHOW_NODE_ARP_TABLE);
               }
           }
       }

    }
    {
       /* run node */ 
       static param_t node;
       init_param(&node, CMD, "node", 0, 0, INVALID, 0, "Help: node" );
       libcli_register_param(run, &node);
       {
           /* run node <node_name> */
           static param_t node_name;
           init_param(&node_name, LEAF, 0, 0, validate_node_name, STRING, "node_name", "Help: Node Name");
           libcli_register_param(&node, &node_name);
           {
               /* run node <node_name> resolv_arp */
               static param_t resolv_arp;
               init_param(&resolv_arp, CMD, "resolv-arp", 0, 0, INVALID, 0, "Help: resolv-arp");
               libcli_register_param(&node_name, &resolv_arp);
               {
                   /* run node <node_name> resolv_arp <ip_addr> */
                   static param_t ip_addr;
                   init_param(&ip_addr, LEAF, 0, arp_handler, 0, IPV4, "ip-address", "Help: ip-address");
                   libcli_register_param(&resolv_arp, &ip_addr);
                   set_param_cmd_code(&ip_addr, CMDCODE_RUN_RESOLV_ARP);
               }
           } 
       }
    }
    support_cmd_negation(config);
}