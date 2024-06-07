#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ciptypes.h>
#include <coreSync.h>
#include <cipethernetlink.h>
#include <cipidentity.h>

#define NUM_ETHLINK_ATTR 11
#define NUM_IDENTITY_ATTR 7

const char *ethlinkAttr[] = {
    "SPEED",
    "FLAGS",
    "PHYSICAL_ADDRESS",
    "INTERFACE_COUNTER",
    "MEDIA_COUNTER",
    "INTERFACE_CONTROL",
    "INTERFACE_TYPE",
    "INTERFACE_STATE",
    "ADMIN_STATE",
    "INTERFACE_LABEL",
    "INTERFACE_CAPABILITY"
};

const char *identityAttr[] = {
    "VENDORID",
    "DEVICE_TYPE",
    "PRODUCT_CODE",
    "REVISION",
    "STATUS",
    "SERIAL_NUMBER",
    "PRODUCT_NAME",
};

extern CipEthernetLinkObject g_ethernet_link[];
extern CipIdentityObject g_identity;

void reverseBinaryString(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

CipShortString convertToCipShortString(const char* str) {
    CipShortString cipStr;

    size_t len = strlen(str);
    if (len > UINT8_MAX) {
        fprintf(stderr, "Warning: String length exceeds UINT8_MAX. Truncating to 255.\n");
        cipStr.length = UINT8_MAX;
    } else {
        cipStr.length = (EipUint8)len;
    }

    // Allocating memory for the string and copying the content.
    // Note: The caller of this function will be responsible for freeing this memory.
    cipStr.string = (EipByte*)malloc(len + 1); // +1 for null terminator
    if (cipStr.string != NULL) {
        memcpy(cipStr.string, str, len + 1); // Include the null terminator
    } else {
        cipStr.length = 0; // Indicate failure
    }

    return cipStr;
}

int parse_request(CipMessageRouterRequest *const message_router_request) {
    /*
        return value: 0 = success, -1 = no synchronization
    */
    
    uint16_t cmd; // 0 = read, 1 = write
    uint16_t serviceType; // 0 = all, 1 = single
    uint16_t serviceCode; // 0 = ethernetlink, 1 = identity(system)
    uint16_t instance = message_router_request->request_path.instance_number;
    uint16_t attribute = message_router_request->request_path.attribute_number;
    const CipOctet *data = message_router_request->data;


    switch(message_router_request->request_path.class_id) {
        case CIP_ETHERNETLINK_CLASS_CODE:
            if(message_router_request->request_path.instance_number > 30 || 
                message_router_request->request_path.instance_number <= 0) {
                return -1;
            }
            serviceCode = 0;
            break;
        case CIP_IDENTITY_CLASS_CODE:
            serviceCode = 1;
            break;
        default:
            return -1;
    }

    switch(message_router_request->service) {
        case kGetAttributeSingle:
            cmd = 0;
            serviceType = 1;
            break;
        case kSetAttributeSingle:
            cmd = 1;
            serviceType = 1;
            break;
        case kGetAttributeAll:
            cmd = 0;
            serviceType = 0;
            break;
        default:
            return -1;
    }

    if(serviceType == 0 && serviceCode == 0) {
        sync_ethernetlink_all(cmd, instance);
    } else if (serviceType == 1 && serviceCode == 0) {
        sync_ethernetlink(cmd, instance, attribute-1, data);
    } else if (serviceType == 0 && serviceCode == 1 && instance == 1) {
        sync_identity_all(cmd);
    } else if (serviceType == 1 && serviceCode == 1 && instance == 1) {
        sync_identity(cmd, attribute);
    }

    return 0;
}

void parse_sync_identity(char *output, uint16_t attribute) {
    switch(attribute+1) {
        case 1: //VENDORID
            // no operation
            break;
        case 2: //DEVICE_TYPE
            g_identity.device_type = (EipUint16)strtoul(output, NULL, 16);
            break;
        case 3: //PRODUCT_CODE
            // no operation
            break;
        case 4: //REVISION
            // no operation
            break;
        case 5: //STATUS
            // no operation
            break;
        case 6: //SERIAL_NUMBER
            g_identity.serial_number = (CipUdint)strtoul(output, NULL, 10);
            break;
        case 7: //PRODUCT_NAME
            g_identity.product_name = convertToCipShortString(output);
            break;
    }
}

void sync_identity(uint16_t cmd, uint16_t attribute) {
    char *msg = generate_identity_str(cmd, identityAttr[attribute], NULL);
    char *output = write_to_socket(msg);
    if (strlen(output) != 0){
        parse_sync_identity(output, attribute);
    }
}

void sync_identity_all(uint16_t cmd) {
    for (size_t i = 0; i < NUM_IDENTITY_ATTR; i++) {
        sync_identity(cmd, i);
    }
}

void parse_sync_ethernetlink(char *output, uint16_t interface, uint16_t attribute) {
    switch(attribute+1) {
        case 1: //SPEED
            if (strcmp(output, "AUTO") == 0) {
                g_ethernet_link[interface-1].interface_speed = 1;
            } else {
                g_ethernet_link[interface-1].interface_speed = (EipUint32)atoi(output);
            }
            break; 
        case 2: //FLAGS
            char binaryStr[33] = {0}; // Initialize all bits to 0
            char *token;
            
            // link status: true/false -> 1/0
            token = strtok(output, ",");
            strcat(binaryStr, (strcmp(token, "true") == 0) ? "1" : "0");
            
            // duplex: HALF/FULL/AUTO/(N/A) -> 0/1/1/1
            token = strtok(NULL, ",");
            strcat(binaryStr, (strcmp(token, "HALF") == 0) ? "0" : "1");
            
            // negotiation status: 0/1/2/3/4 -> 00/01/10/11
            token = strtok(NULL, ",");
            if (strcmp(token, "0") == 0) strcat(binaryStr, "000");
            else if (strcmp(token, "1") == 0) strcat(binaryStr, "100");
            else if (strcmp(token, "2") == 0) strcat(binaryStr, "010");
            else if (strcmp(token, "3") == 0 || strcmp(token, "4") == 0) strcat(binaryStr, "110");
            
            // manual reset required: 0/1
            token = strtok(NULL, ",");
            strcat(binaryStr, (strcmp(token, "0") == 0) ? "0" : "1");
            
            // local hardware fault: 0/1
            token = strtok(NULL, ",");
            strcat(binaryStr, (strcmp(token, "0") == 0) ? "0" : "1");
            
            // Add the 26 reserve bits
            while (strlen(binaryStr) < 32) {
                strcat(binaryStr, "0");
            }
            
            // Convert to integer and then to uint32
            reverseBinaryString(binaryStr);
            g_ethernet_link[interface-1].interface_flags = (EipUint32)strtoul(binaryStr, NULL, 2);
            break;
        case 3: //PHYSICAL_ADDRESS
            int itemsRead = sscanf(output, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                            &g_ethernet_link[interface-1].physical_address[0],
                            &g_ethernet_link[interface-1].physical_address[1],
                            &g_ethernet_link[interface-1].physical_address[2],
                            &g_ethernet_link[interface-1].physical_address[3],
                            &g_ethernet_link[interface-1].physical_address[4],
                            &g_ethernet_link[interface-1].physical_address[5]);
            break;
        case 4: //INTERFACE_COUNTER
            token = strtok(output, ",");
            g_ethernet_link[interface-1].interface_cntrs.cntr32[0] = (EipUint32)atoi(token);
            for(size_t i=1; i < 11; i++) {
                token = strtok(NULL, ",");
                g_ethernet_link[interface-1].interface_cntrs.cntr32[i] = (EipUint32)atoi(token);
            }
            break;
        case 5: //MEDIA_COUNTER
            token = strtok(output, ",");
            g_ethernet_link[interface-1].media_cntrs.cntr32[0] = (EipUint32)atoi(token);
            for(size_t i=1; i < 12; i++) {
                token = strtok(NULL, ",");
                g_ethernet_link[interface-1].media_cntrs.cntr32[i] = (EipUint32)atoi(token);
            }
            break;
        case 6: //INTERFACE_CONTROL
            
            // Forced Duplex Mode: HALF/FULL -> 0/1
            token = strtok(NULL, ",");
            if(strcmp(token, "HALF") == 0){
                g_ethernet_link[interface-1].interface_control.control_bits = 0;
            } else {
                g_ethernet_link[interface-1].interface_control.control_bits = 1;
            }
            
            // Auto-negotiate: 0/1
            token = strtok(NULL, ",");
            if(strcmp(token, "0") == 0){
                g_ethernet_link[interface-1].interface_control.forced_interface_speed = 0U;
            } else {
                g_ethernet_link[interface-1].interface_control.forced_interface_speed = 1U;
            }
            
            break;
        case 7: //INTERFACE_TYPE
            g_ethernet_link[interface-1].interface_type = (CipUsint)atoi(output);
            break;
        case 8: //INTERFACE_STATE
            if (strcmp(output, "false") == 0) {
                g_ethernet_link[interface-1].interface_state = 0;
            } else if (strcmp(output, "true") == 0) {
                g_ethernet_link[interface-1].interface_state = 1;
            }
            break;
        case 9: //ADMIN_STATE
            if (strcmp(output, "false") == 0) {
                g_ethernet_link[interface-1].admin_state = 0;
            } else if (strcmp(output, "true") == 0) {
                g_ethernet_link[interface-1].admin_state = 1;
            }
            break;
        case 10: //INTERFACE_LABEL
            g_ethernet_link[interface-1].interface_label = convertToCipShortString(output);
            break;
        case 11: //INTERFACE_CAPABILITY
            // no operation
            break;
        default:
            break;
    }
}

void sync_ethernetlink(uint16_t cmd, uint16_t interface, uint16_t attribute, const CipOctet *data) {
    if (cmd == 1 && attribute == 8) { // write ADMIN_STATE (port enabled)
        char *value = (*data == 1) ? "TRUE" : "FALSE";
        char *msg = generate_ethernetlink_str(cmd, interface, ethlinkAttr[attribute], value);
        write_to_socket(msg);
    }
    // else if(cmd==1 && attribute == 5) { // write INTERFACE_CONTROL
    //     // Forced Duplex Mode: HALF/FULL -> 0/1
    //     const char *duplex;
    //     duplex = (data[0] & 1 == 0) ? "HALF" : "FULL";
    //     // // Auto-negotiate: 0/1
    //     const char *autoneg;
    //     autoneg = (data[0] & 2 == 0) ? ",0" : ",1";

    //     char *value = (char *)malloc(strlen(duplex) + strlen(autoneg) + 1);
    //     strcpy(value, duplex);
    //     strcat(value, autoneg);
    //     char *msg = generate_ethernetlink_str(cmd, interface, ethlinkAttr[attribute], value);
    //     // write_to_socket(msg);
    // } 
    else { // read
        char *msg = generate_ethernetlink_str(cmd, interface, ethlinkAttr[attribute], NULL);
        char *output = write_to_socket(msg);
        if (strlen(output) != 0){
            parse_sync_ethernetlink(output, interface, attribute);
        }
    }
}

void sync_ethernetlink_all(uint16_t cmd, uint16_t interface) {
    for(size_t i=0; i<NUM_ETHLINK_ATTR; i++) {
        sync_ethernetlink(cmd, interface, i, NULL);
    }
}

char *write_to_socket(const char* message) {
    int sockfd;
    struct sockaddr_un serv_addr;

    // create socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    // init socket server struct
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, ETH_SOCKET_PATH);

    // connect to server
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to socket");
        exit(1);
    }

    // write to socket
    write(sockfd, message, strlen(message));

    // read from socket
    char *buffer = malloc(512); 
    int n = read(sockfd, buffer, 512);
    if (n < 0) {
        perror("Error reading from socket");
        free(buffer);
        exit(1);
    }
    buffer[n] = '\0';

    // close socket connection
    close(sockfd);

    return buffer;
}

char *generate_ethernetlink_str(uint16_t cmd, uint16_t interface, char* attribute, char* value) {
    // return : {Read/Write}$ETHERNETLINK${interface}${attribute}${value}

    // cmd = 0 for read, 1 for write
    const char* cmdStr;
    switch (cmd) {
        case 0:
            cmdStr = "READ";
            break;
        case 1:
            cmdStr = "WRITE";
            break;
        default:
            return "";
    }
    char *result;
    if(value == NULL) {
        int length = snprintf(NULL, 0, "%s$ETHERNETLINK$%u$%s", cmdStr, interface, attribute);
        result = (char*)malloc(length + 1);
        snprintf(result, length + 1, "%s$ETHERNETLINK$%u$%s", cmdStr, interface, attribute);
    } else {
        int length = snprintf(NULL, 0, "%s$ETHERNETLINK$%u$%s$%s", cmdStr, interface, attribute, value);
        result = (char*)malloc(length + 1);
        snprintf(result, length + 1, "%s$ETHERNETLINK$%u$%s$%s", cmdStr, interface, attribute, value);
    }
    return result;
}

char *generate_identity_str(uint16_t cmd, char* attribute, char* value) {
    // return : {READ/WRITE}$SYSTEM${attribute}${value}

    // cmd = 0 for read, 1 for write
    const char* cmdStr;
    switch (cmd) {
        case 0:
            cmdStr = "READ";
            break;
        case 1:
            cmdStr = "WRITE";
            break;
        default:
            return "";
    }
    char *result;
    if(value == NULL) {
        int length = snprintf(NULL, 0, "%s$SYSTEM$%s", cmdStr, attribute);
        result = (char*)malloc(length + 1);
        snprintf(result, length + 1, "%s$SYSTEM$%s", cmdStr, attribute);
    } else {
        int length = snprintf(NULL, 0, "%s$SYSTEM$%s$%s", cmdStr, attribute, value);
        result = (char*)malloc(length + 1);
        snprintf(result, length + 1, "%s$SYSTEM$%s$%s", cmdStr, attribute, value);
    }
    return result;
}