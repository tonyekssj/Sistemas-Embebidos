/*Transmisor*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "myUart.h"
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

#define UART_CLK _DEF_REG_32b(0X3FF40014)

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

void myItoa(uint16_t number, char* str, uint8_t base){
    char num_temp[15]={0};
	uint16_t count = 0;
	
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


void app_main(void)
{

    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    gpio_set_level(2,1);

    uint8_t state=gpio_get_level(2);

    uartInit(0,UARTS_BAUD_RATE,8,1,1,UART_TX_PIN,UART_RX_PIN);
    uartInit(2,UARTS_BAUD_RATE,8,0,1,UART2_TX_PIN,UART2_RX_PIN);
    delayMs(1000);

    while(1) 
    {
        char cad[5]={0};
        

        uartClrScr(0);
        uartPuts(0,"Comando 0x");
        uartGets(0,cad);

        if(cad[0] == 49 && cad[1] == 49){
            if(state == 0)
                uartPuts(2,"11 0");
            else  
                uartPuts(2,"11 1");   
        }

        uartPuts(2,cad);
        delayMs(1000);
    }
}