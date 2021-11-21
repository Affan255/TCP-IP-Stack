#ifndef __UTILS__
#define __UTILS__
typedef enum{
    FALSE,
    TRUE
}bool_t;

#define IS_BIT_SET(n,pos)   ((n & (1<<pos))!=0)
#define TOGGLE_BIT(n,pos)   (n = n ^ (1<<pos))
#define COMPLEMENT(num)     (num = num ^ 0xFFFFFFFF)
#define UNSET_BIT(n)        (n = n & ((1<<pos) ^ 0xFFFFFFFF))
#define SET_BIT(n)          (n = n | (1<<pos))
#define IS_MAC_BROADCAST_ADDR(mac)    (mac[0]==0xFF && mac[1]==0xFF && mac[2]==0xFF && mac[3]==0xFF && mac[4]==0xFF && mac[5]==0xFF)

void apply_mask(char *prefix, char mask, char *str_prefix);
void layer_2_fill_with_broadcast_mac(char *mac_array);

#endif