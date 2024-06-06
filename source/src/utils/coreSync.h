#ifndef CORESYNC_H_
#define CORESYNC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

// Define the socket path
#define ETH_SOCKET_PATH "/tmp/ethernetpass.socket"
#define SYSTEM_SOCKET_PATH "/tmp/mmpass.socket"

// Define service code
#define CIP_ETHERNETLINK_CLASS_CODE   0xF6U

// Define ethernetlink attributes
extern const char *ethlinkAttr[];
#define NUM_ETHLINK_ATTR (sizeof(ethlinkAttr) / sizeof(ethlinkAttr[0]))

//functions
void reverseBinaryString(char *str);
CipShortString convertToCipShortString(const char* str);
int parse_request(CipMessageRouterRequest *const message_router_request,
                    const CipInstance *instance);
void parse_sync_ethernetlink(CipInstance *instance, char *output, uint16_t interface, uint16_t attribute);
void sync_ethernetlink(CipInstance *instance, uint16_t cmd, uint16_t interface, uint16_t attribute);
void sync_ethernetlink_all(CipInstance *instance, uint16_t cmd, uint16_t interface);
char *write_to_socket(const char* message, int serviceCode);
char *generate_ethernetlink_str(uint16_t cmd, uint16_t interface, char* attribute, char* value);
char *generate_system_str(uint16_t cmd, char* attribute, char* value);

#endif /* CORESYNC_H_ */