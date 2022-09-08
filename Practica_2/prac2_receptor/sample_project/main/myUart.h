#include "driver/uart.h"

void uartInit(uart_port_t uart_num, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop, uint8_t txPin, uint8_t rxPin);
void uartGoto11(uart_port_t uart_num);

// Send
void uartPuts(uart_port_t uart_num, char *str);
void uartPutchar(uart_port_t uart_num, char data);

bool uartKbhit(uart_port_t uart_num);
char uartGetchar(uart_port_t uart_num );
void uartGets(uart_port_t uart_num, char *str);

void uartClrScr( uart_port_t uart_num );

void myItoa(uint16_t number, char* str, uint8_t base) ;
uint16_t myAtoi(char *str);

void comandoTimestamp(uint32_t start);
void comandoEstado();
void invertirEstado();
void mostrarTemperatura();

void sendPacket(uart_port_t uart_num, char *str,uint8_t longitudDato,uint32_t dato);
void sendWrongpacket(uart_port_t uart_num);
void receivePacket(uart_port_t uart_num);

bool checkPackage();
