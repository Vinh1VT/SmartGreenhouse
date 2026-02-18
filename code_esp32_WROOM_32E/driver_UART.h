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

#if DEBUG == 1
/* Communication with PC */
    #define USE_SERIAL Serial
    #define Serial_baudrate 115200
#endif

/*
Initialise the UART pins
*/
void setup_UART(void);

/*
Write string in the UART connexion
in : 
    char *string
*/
void write_UART(char *string);


#endif