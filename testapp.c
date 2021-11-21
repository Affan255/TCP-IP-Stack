#include "graph.h"
#include "net.h"
#include "CommandParser/libcli.h"
#include <stdio.h>

extern void nw_init_cli();
extern graph_t *build_first_topo();
extern graph_t *build_linear_topo();

graph_t *topo = NULL;
int 
main(int argc, char **argv){
    nw_init_cli();
    // topo = build_linear_topo();

    topo = build_first_topo();
    // sleep(2);
    // node_t *snode = get_node_by_node_name(topo, "R0_re");
    // interface_t *oif = get_node_if_by_name(snode, "eth-0/0");
    // char msg[] = "Hello! How are you\0";
    // send_pkt_out(msg, strlen(msg), oif);
    start_shell();
    return 0;
}
