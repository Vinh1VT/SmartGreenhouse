#ifndef DRIVER_UART_H
#define DRIVER_UART_H

/* Set the pins used for UART */
#define UART2_Rx 16
#define UART2_Tx 17

/* set a default configuration that can be overiden */
#ifndef UART2_config
    #define UART2_config SERIAL_8N1
#endif

/* set a default baudrate that can be overiden */
#ifndef UART2_baudrate
    #define UART2_baudrate 9600
#endif

/* set the default size of the read buffer */
#ifndef UART2_read_buffer_size
    #define UART2_read_buffer_size 1024
#endif

/* set the default  */
#ifndef UART2_timeout_delay
    #define UART2_timeout_delay 10000 /*20 sec*/
#endif

/* set the default timeout ?*/

#if DEBUG == 1
/* Communication with PC */
    #define USE_SERIAL Serial
    #define Serial_baudrate 115200
#endif

/*
Initialise the UART pins
*/
void setup_UART(int rx_pin, int tx_pin);

/*
Write string in the UART connexion
input : 
    string : the string written to the UART connexion
*/
void write_UART(char *string);

/*
Return a character read from the UART connexion
output :
    char : the read character, 0 if their is nothing to read
*/
char read_byte_UART(void);

/*
Read a maximum of UART2_read_buffer_size character
input :
    nb_bytes : the number of bytes to read.
output :
    int : 1 if the UART buffer is not empty, 0 else.
*/
int read_bytes_UART(int nb_bytes);

/*
Read the UART connexion until either the word is found or the number of read character exceed UART2_read_buffer_size or timeout
input :
    string : the word that is to find
output :
    int : 0 if found, 1 if not found and 2 if timeout
*/
int read_until_motif_found_UART(char *string);

void end_com(void);

#endif