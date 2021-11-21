#include "layer2.h"
#include <arpa/inet.h>
#include "../tcpconst.h"
#include <stdlib.h>
#include <stdio.h>

void
init_arp_table(arp_table_t **arp_table) {
    *arp_table = calloc(1, sizeof(arp_table_t));
    init_glthread(&((*arp_table)->arp_entries));
}

arp_entry_t*
arp_table_lookup(arp_table_t *arp_table, char *ip_addr) {
    arp_entry_t *arp_entry;
    glthread_t *base_glthread = &(arp_table->arp_entries);
    glthread_t *glthreadptr = NULL;
    ITERATE_GLTHREAD_BEGIN(base_glthread, glthreadptr) {
        arp_entry = arp_glue_to_arp_entry(glthreadptr);
        if (strncmp(arp_entry->ip_addr.ip_addr, ip_addr, 16)==0)
            return arp_entry;
    } ITERATE_GLTHREAD_END(base_glthread, glthreadptr)
    return NULL;
}

void
delete_arp_table_entry(arp_table_t *arp_table, char *ip_addr) {
    arp_entry_t *arp_entry = arp_table_lookup(arp_table, ip_addr);
    if (arp_entry) {
        glthread_t *arp_glue = &(arp_entry->arp_glue);
        remove_glthread(arp_glue);
    }
}

bool_t 
arp_table_entry_add(arp_table_t *arp_table, arp_entry_t *arp_entry) {
    arp_entry_t *arp_old_entry = arp_table_lookup(arp_table, arp_entry->ip_addr.ip_addr);
    if (arp_old_entry && memcmp(arp_entry, arp_old_entry, sizeof(arp_entry_t))==0)
        return FALSE;
    if (arp_old_entry) {
        delete_arp_table_entry(arp_table, arp_entry->ip_addr.ip_addr);
    }        
    glthread_t *base_glthread = &(arp_table->arp_entries);
    init_glthread(&(arp_entry->arp_glue));
    glthread_add_next(base_glthread, &(arp_entry->arp_glue));
    return TRUE;
}

void
arp_table_update_from_arp_reply(arp_table_t *arp_table, arp_hdr_t *arp_hdr, interface_t *iif) {
    unsigned int src_ip = 0;
    assert(arp_hdr->op_code == ARP_REPLY);
    arp_entry_t *arp_entry = calloc(1, sizeof(arp_entry_t));
    src_ip = htonl(arp_hdr->src_ip);
    inet_ntop(AF_INET, &src_ip, arp_entry->ip_addr.ip_addr, 16);
    arp_entry->ip_addr.ip_addr[15] = '\0';
    memcpy(arp_entry->mac_addr.mac_addr, arp_hdr->src_mac_addr.mac_addr, sizeof(mac_add_t));
    strncpy(arp_entry->oif_name, iif->if_name, IF_NAME_SIZE);
    bool_t rc = arp_table_entry_add(arp_table, arp_entry);
    if (rc ==FALSE)
        free(arp_entry);
}

void
dump_arp_table(arp_table_t *arp_table) {
    arp_entry_t *arp_entry;
    glthread_t *glthreadptr = NULL;
    ITERATE_GLTHREAD_BEGIN(&(arp_table->arp_entries), glthreadptr) {
        arp_entry = arp_glue_to_arp_entry(glthreadptr);
       printf("IP : %s, MAC : %u:%u:%u:%u:%u:%u, OIF = %s\n", 
            arp_entry->ip_addr.ip_addr, 
            arp_entry->mac_addr.mac_addr[0], 
            arp_entry->mac_addr.mac_addr[1], 
            arp_entry->mac_addr.mac_addr[2], 
            arp_entry->mac_addr.mac_addr[3], 
            arp_entry->mac_addr.mac_addr[4], 
            arp_entry->mac_addr.mac_addr[5], 
            arp_entry->oif_name);      
    } ITERATE_GLTHREAD_END(&(arp_table->arp_entries), glthreadptr)
}

void
send_arp_broadcast_request(node_t* node, interface_t *oif, char *ip_addr) {
    unsigned int payload_size = sizeof(arp_hdr_t);
    ethernet_hdr_t *ethernet_hdr = calloc(1, ETH_HDR_SIZE_EXCL_PAYLOAD + payload_size);
    if (!oif) {
        oif = node_get_matching_subnet_interface(node, ip_addr);
        if (!oif) {
            printf("Error : %s : No eligible subnet for ARP resolution for ip-address %s",node->node_name, ip_addr);
        }
    }
    layer_2_fill_with_broadcast_mac(ethernet_hdr->dest_mac_addr.mac_addr);
    memcpy(ethernet_hdr->src_mac_addr.mac_addr, IF_MAC(oif), sizeof(mac_add_t));
    ethernet_hdr->type = ARP_MSG;

    arp_hdr_t *arp_hdr = (arp_hdr_t*)(ethernet_hdr->payload);
    arp_hdr->hw_type = 1;
    arp_hdr->proto_type = 0x800;
    arp_hdr->hw_addr_len = sizeof(mac_add_t);
    arp_hdr->proto_addr_len = 4;
    arp_hdr->op_code = ARP_BROAD_REQ;
    memcpy(arp_hdr->src_mac_addr.mac_addr, IF_MAC(oif), sizeof(mac_add_t));
    memset(arp_hdr->dest_mac_addr.mac_addr, 0, sizeof(mac_add_t));
    inet_pton(AF_INET, IF_IP(oif), &(arp_hdr->src_ip));
    arp_hdr->src_ip = htonl(arp_hdr->src_ip);
    inet_pton(AF_INET, ip_addr, &(arp_hdr->dest_ip));
    arp_hdr->dest_ip = htonl(arp_hdr->dest_ip);

    ETH_FCS(ethernet_hdr, payload_size) = 0; 
    send_pkt_out((char*)ethernet_hdr, ETH_HDR_SIZE_EXCL_PAYLOAD+payload_size, oif);
    free(ethernet_hdr);
}

static void
send_arp_reply_msg(ethernet_hdr_t *ethernet_hdr, interface_t *oif) {
    arp_hdr_t *arp_hdr = (arp_hdr_t*)(GET_ETH_HDR_PAYLOAD(ethernet_hdr));
    ethernet_hdr_t *ethernet_hdr_reply = (ethernet_hdr_t*)calloc(1, MAX_PACKET_BUFFER_SIZE);
    memcpy(ethernet_hdr_reply->dest_mac_addr.mac_addr, arp_hdr->src_mac_addr.mac_addr, sizeof(mac_add_t));
    memcpy(ethernet_hdr_reply->src_mac_addr.mac_addr, IF_MAC(oif), sizeof(mac_add_t));
    ethernet_hdr_reply->type = ARP_MSG;

    arp_hdr_t *arp_hdr_reply = (arp_hdr_t*)(GET_ETH_HDR_PAYLOAD(ethernet_hdr_reply));
    arp_hdr_reply->hw_type = 1;
    arp_hdr_reply->proto_type = 0x800;
    arp_hdr_reply->hw_addr_len = sizeof(mac_add_t);
    arp_hdr_reply->proto_addr_len = 4;
    arp_hdr_reply->op_code = ARP_REPLY;
    memcpy(arp_hdr_reply->dest_mac_addr.mac_addr, arp_hdr->src_mac_addr.mac_addr, sizeof(mac_add_t));
    memcpy(arp_hdr_reply->src_mac_addr.mac_addr, IF_MAC(oif), sizeof(mac_add_t));
    arp_hdr_reply->src_ip = arp_hdr->dest_ip;
    arp_hdr_reply->dest_ip = arp_hdr->src_ip;

    ETH_FCS(ethernet_hdr_reply, sizeof(arp_hdr_t)) = 0;
    unsigned int packet_size = ETH_HDR_SIZE_EXCL_PAYLOAD + sizeof(arp_hdr_t);
    char *shifted_pkt_buffer = pkt_buffer_right_shift((char*)ethernet_hdr_reply, packet_size, MAX_PACKET_BUFFER_SIZE );
    send_pkt_out(shifted_pkt_buffer, packet_size, oif);

    free(ethernet_hdr_reply);
}

static void
process_arp_broadcast_request(node_t *node, interface_t *iif, ethernet_hdr_t *ethernet_hdr) {
    printf("%s : ARP Broadcast msg recvd on interface %s of node %s\n", 
                __FUNCTION__, iif->if_name , iif->att_node->node_name);
    char ip_addr[16];
    arp_hdr_t *arp_hdr = (arp_hdr_t*)(GET_ETH_HDR_PAYLOAD(ethernet_hdr));
    unsigned int arp_dest_ip = htonl(arp_hdr->dest_ip);
    inet_ntop(AF_INET, &arp_dest_ip, ip_addr, 16);
    ip_addr[15]='\0';
    if (strncmp(ip_addr, IF_IP(iif), 16)) {
        printf("%s: ARP broadcast msg request dropped. Dst Ip Address %s doest not match with interface Ip Address: %s\n", node->node_name, ip_addr, IF_IP(iif));
        return;
    } 
    send_arp_reply_msg(ethernet_hdr, iif);
}

static void
process_arp_reply_msg(node_t *node, interface_t *iif, ethernet_hdr_t *ethernet_hdr) {
    printf("%s : ARP reply msg recvd on interface %s of node %s\n",
             __FUNCTION__, iif->if_name , iif->att_node->node_name);
    arp_table_update_from_arp_reply(node->node_nw_prop.arp_table, (arp_hdr_t*)GET_ETH_HDR_PAYLOAD(ethernet_hdr), iif);
}

static void 
promote_pkt_to_layer3(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size) {

}

void
layer2_frame_recv(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size) {
    ethernet_hdr_t *ethernet_hdr = (ethernet_hdr_t*)pkt;
    
    if (L2_frame_recv_qualify_on_interface(interface, ethernet_hdr) == FALSE) {
        printf("L2 frame rejected!!");
        return;
    }
    printf("\n L2 Frame accepted!!\n");
    switch (ethernet_hdr->type)
    
    {
    
    case ARP_MSG:
    {
        arp_hdr_t *arp_hdr = (arp_hdr_t*)(ethernet_hdr->payload);
        switch (arp_hdr->op_code)
        {
        case ARP_BROAD_REQ:
            process_arp_broadcast_request(node, interface, ethernet_hdr);
            break;
        case ARP_REPLY:
            process_arp_reply_msg(node, interface, ethernet_hdr);
            break;    
        
        default:
            break;
        }
        break;
    }

    
    default:
        promote_pkt_to_layer3(node, interface, pkt, pkt_size);
        break;
    }
}