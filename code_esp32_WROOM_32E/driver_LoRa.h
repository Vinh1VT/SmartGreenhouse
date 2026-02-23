#ifndef DRIVER_LORA_H
#define DRIVER_LORA_H

#define MAX_COMMAND_LENGTH 528

/* !!! DON'T FORGET TO REMOVE YOUR CREDENTIAL WHEN YOU PUSH !!! */
#define AT_DR1 "AT+DR=" /* Use Yours*/
#define AT_DR2 "AT+DR=" /* Use Yours*/
#define DEVEUI "AT+ID=DevEUI," /* Use Yours*/
#define APPEUI "AT+ID=AppEUI," /* Use Yours*/
#define APPKEY "AT+KEY=APPKEY," /* Use Yours*/

#define LoRa_MAX_MSG_LENGTH 528

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