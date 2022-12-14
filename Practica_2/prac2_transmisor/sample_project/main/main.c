/*Transmisor*/

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "myUart.h"
#include "crc.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"




#define UART_RX_PIN     (3)
#define UART_TX_PIN     (1)

#define UART1_RX_PIN     (9)
#define UART1_TX_PIN     (10)

#define UART2_RX_PIN     (16)
#define UART2_TX_PIN     (17)

#define UARTS_BAUD_RATE         (115200)
#define TASK_STACK_SIZE         (1048)
#define READ_BUF_SIZE           (1024)

#define LEN 20

struct uartSendPacket{

    uint8_t init;
    uint8_t command;
    uint8_t longitudDato;
    uint32_t dato;
    uint8_t end;
    uint32_t checkCRC32;

}packageUart;

void uartInit(uart_port_t uart_num, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop, uint8_t txPin, uint8_t rxPin)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = (int) baudrate,
        .data_bits = (uart_word_length_t)(size-5),
        .parity    = (uart_parity_t)parity,
        .stop_bits = (uart_stop_bits_t)stop,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_driver_install(uart_num, READ_BUF_SIZE, READ_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, txPin, rxPin,UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

}

void delayMs(uint16_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void uartClrScr(uart_port_t uart_num)
{
    // Uso "const" para sugerir que el contenido del arreglo lo coloque en Flash y no en RAM
    const char caClearScr[] = "\e[2J";
    uart_write_bytes(uart_num, caClearScr, sizeof(caClearScr));
}

void uartGoto11(uart_port_t uart_num)
{
    // Limpie un poco el arreglo de caracteres, los siguientes tres son equivalentes:
     // "\e[1;1H" == "\x1B[1;1H" == {27,'[','1',';','1','H'}
    const char caGoto11[] = "\e[1;1H";
    uart_write_bytes(uart_num, caGoto11, sizeof(caGoto11));
}

bool uartKbhit(uart_port_t uart_num)
{
    uint8_t length;
    uart_get_buffered_data_len(uart_num, (size_t*)&length);
    return (length > 0);
}

void uartPutchar(uart_port_t uart_num, char c)
{  
    uart_write_bytes(uart_num, &c, sizeof(c));
}

char uartGetchar(uart_port_t uart_num)
{ 
    char c;
    // Wait for a received byte
    while(!uartKbhit(uart_num))
    {
        delayMs(10);
    }
    // read byte, no wait
    uart_read_bytes(uart_num, &c, sizeof(c), 0);

    return c;
}

void uartGets(uart_port_t com,char* str){
    
    char c=0;
    uint8_t contador=0;

    while(c!=13){

        c=uartGetchar(com);

        if((c==8) && (contador>0)){
            contador--;
            uartPutchar(com,8);
            uartPutchar(com,' ');
            uartPutchar(com,8);
            *str-- =0;
        }

        else if (c!=8)
        {
            contador++;
            uartPutchar(com,c); 
            *str++=c;   
        }
                
    }
}

void uartPuts(uart_port_t com, char *str){
    while(*str){
        uartPutchar(com,*str++);
    }
}

uint16_t myAtoi(char *str){
    uint16_t temp=0;
    uint16_t num=0;
    char c;

    while(*str){

		c=*str++;
        temp=c-48;

        if(c== '.')
            break;
            
        if (c>='0' && c<='9'){
            if(c==0){
                num=num+temp;
    	    }  

            else{
            num=num*10;
            num=num+temp;
    	    } 
        }
    }
    return num;  
}

void myItoa(uint32_t number, char* str, uint8_t base){
    char num_temp[15]={0};
	uint32_t count = 0;
	
	if(number < 1 || base < 2) {
		*str++ = '0';
		*str = 0;
		return;
	}

	while(number) {
		if((number % base) > 9) num_temp[count++] = (number % base) + '0' + 7;
		else num_temp[count++] = (number % base) + '0';
		number -= (number % base);
		number /= base;
	}

	while(count > 0) {
		*str++ = num_temp[--count];
	}

	*str = 0;
}

void uartGotoxy(uart_port_t com, uint8_t x, uint8_t y){
    char caGotoxy[] = "\e[0;0H";/*y;x*/
    char caGotoxy2[] = "\e[00;00H";/*y;x*/
    char c_y[] = "00";
    char c_x[] ="00";
    char c;

    myItoa(x,c_x,10);
    myItoa(y,c_y,10);
    
    if (x<=9 && y<=9){
        c = y + '0';
        caGotoxy[2]=c;
        c = x + '0';
        caGotoxy[4]=c;
        uart_write_bytes(com, caGotoxy, sizeof(caGotoxy));
    }
    else{
        caGotoxy2[2]=c_y[0];
        caGotoxy2[3]=c_y[1];

        caGotoxy2[5]=c_x[0];
        caGotoxy2[6]=c_x[1];
        uart_write_bytes(com, caGotoxy2, sizeof(caGotoxy2));      
    }
}

bool checkPackage(){

    uint8_t data[]={0x5a,packageUart.command,packageUart.longitudDato,packageUart.dato,0xb2}; 
    uint32_t calculateCRC32;

    calculateCRC32=crc32_le(0,data,sizeof(data));

    if(calculateCRC32 == packageUart.checkCRC32)
        return true;
    else 
        return false;
}

void sendPacket(uart_port_t uart_num, char *str,uint8_t longitudDato,uint32_t dato){

    packageUart.init=0x5a;
    packageUart.command=myAtoi(str);
    packageUart.longitudDato=longitudDato;
    packageUart.dato=dato;
    packageUart.end=0xb2;

    uint8_t dataSend[]={packageUart.init,packageUart.command,packageUart.longitudDato,packageUart.dato,packageUart.end};  
    packageUart.checkCRC32=crc32_le(0,dataSend,sizeof(dataSend));

    uartPutchar(uart_num,packageUart.init);
    uartPutchar(uart_num,packageUart.command);
    uartPutchar(uart_num,packageUart.longitudDato);
    uartPutchar(uart_num,packageUart.dato);
    uartPutchar(uart_num,packageUart.end);
    //Send CRC32
    uartPutchar(uart_num,packageUart.checkCRC32);
    uartPutchar(uart_num,packageUart.checkCRC32>>8);
    uartPutchar(uart_num,packageUart.checkCRC32>>16);
    uartPutchar(uart_num,packageUart.checkCRC32>>24);

}

void receivePacket(uart_port_t uart_num){

    packageUart.init=uartGetchar(uart_num);
    packageUart.command=uartGetchar(uart_num);
    packageUart.longitudDato=uartGetchar(uart_num);
    packageUart.dato=uartGetchar(uart_num);
    packageUart.end=uartGetchar(uart_num);

    packageUart.checkCRC32=uartGetchar(uart_num);
    packageUart.checkCRC32|=uartGetchar(uart_num) << 8;
    packageUart.checkCRC32|=uartGetchar(uart_num) << 16;
    packageUart.checkCRC32|=uartGetchar(uart_num) << 24;
}

void app_main(void)
{
    uartInit(0,UARTS_BAUD_RATE,8,0,1,UART_TX_PIN,UART_RX_PIN);
    uartInit(2,UARTS_BAUD_RATE,8,0,1,UART2_TX_PIN,UART2_RX_PIN);
    delayMs(1000);

    while(1) 
    {
        char cad[LEN]={};
        char datoString[LEN]={};

        uartClrScr(0);
        uartPuts(0,"Comando 0x");
        uartGets(0,cad);

        sendPacket(2,cad,0,0);
        receivePacket(2);

        if (checkPackage()){
            switch (packageUart.command)
            {
            case 10:
                myItoa(packageUart.dato, datoString,10);
                uartPuts(0,datoString);
                uartPuts(0,"ms");
                delayMs(1000);
                break;
            
            case 11:
                myItoa(packageUart.dato, datoString,10);
                uartPuts(0,"Estado ");
                uartPuts(0,datoString);
                delayMs(1000);
                break;

            case 12:
                myItoa(packageUart.dato, datoString,10);
                uartPuts(0,datoString);
                uartPuts(0," C");
                delayMs(1000);
                break;
            
            case 13:
                uartPuts(0,"Led Invertido");
                delayMs(1000);
                break;

            default:
                break;
            }
        }
        else  
            delayMs(2000);
  

    }
}