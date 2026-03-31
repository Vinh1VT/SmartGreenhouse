#ifndef DRIVER_LORA_H
#define DRIVER_LORA_H

#define MAX_COMMAND_LENGTH 528

/* !!! DON'T FORGET TO REMOVE YOUR CREDENTIAL WHEN YOU PUSH !!! */
#define AT_DR1 "AT+DR=EU868"
#define AT_DR2 "AT+DR=DR6"
#define DEVEUI "AT+ID=DevEUI,70B3D57ED0075CF1"
#define APPEUI "AT+ID=AppEUI,0000000000000000"
#define APPKEY "AT+KEY=APPKEY,63FD6858269444935182F8B5547F9549"

#define DOWNLINK_BUFFER_SIZE 256

// AppEUI
// 0000000000000000
// DevEUI
// 
// 70B3D57ED0073496
// AppKey
// 6B6C025304DA03A05C84891F66E987E1

#define LoRa_MAX_MSG_LENGTH 528

#define MAX_CONNECTION_ATEMPT 10

/*
Setup the LoRa configuration
*/
void setup_LoRa(void);

/*
Attempt the connection to the LoRa network
output :
    int : 0 if it worked, 1 else
*/
int connect_LoRa(void);

/*
Send the command to the LoRa module
input :
    command : a commande of max length MAX_COMMAND_LENGTH (see detail : https://files.seeedstudio.com/products/317990687/res/LoRa-E5%20AT%20Command%20Specification_V1.0%20.pdf)
*/
void write_LoRa(char *command);

/*
Send the message to the LoRa network
input :
    msg : the message to be sent of max length MAX_COMMAND_LENGTH - 8
output :
    int : 0 if it worked, 1 else
*/
int send_msg_LoRa(char *msg);
#endif