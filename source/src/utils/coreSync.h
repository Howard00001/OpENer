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

// Define service code
#define CIP_IDENTITY_CLASS_CODE       0x01U
#define CIP_ETHERNETLINK_CLASS_CODE   0xF6U

//functions
void reverseBinaryString(char *str);
CipShortString convertToCipShortString(const char* str);
int parse_request(CipMessageRouterRequest *const message_router_request);
void sync_identity(uint16_t cmd, uint16_t attribute);
void sync_identity_all(uint16_t cmd);
void parse_sync_ethernetlink(char *output, uint16_t interface, uint16_t attribute);
void sync_ethernetlink(uint16_t cmd, uint16_t interface, uint16_t attribute, const CipOctet *data);
void sync_ethernetlink_all(uint16_t cmd, uint16_t interface);
char *write_to_socket(const char* message);
char *generate_ethernetlink_str(uint16_t cmd, uint16_t interface, char* attribute, char* value);
char *generate_identity_str(uint16_t cmd, char* attribute, char* value);

#endif /* CORESYNC_H_ */