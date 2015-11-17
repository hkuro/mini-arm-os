#include <stddef.h>
#include <stdint.h>
#include "reg.h"
#include "threads.h"
#include "malloc.h"

#define MAX_Input 50

/* USART TXE Flag
 * This flag is cleared when data is written to USARTx_DR and
 * set when that data is transferred to the TDR
 */
#define USART_FLAG_TXE	((uint16_t) 0x0080)

/* USART RXNE Flag, when RXNE is set, data can be read */
#define USART_FLAG_RXNE ((uint16_t) 0x0020)

extern int fibonacci(int x);

void usart_init(void)
{
	*(RCC_APB2ENR) |= (uint32_t) (0x00000001 | 0x00000004);
	*(RCC_APB1ENR) |= (uint32_t) (0x00020000);

	/* USART2 Configuration, Rx->PA3, Tx->PA2 */
	*(GPIOA_CRL) = 0x00004B00;
	*(GPIOA_CRH) = 0x44444444;
	*(GPIOA_ODR) = 0x00000000;
	*(GPIOA_BSRR) = 0x00000000;
	*(GPIOA_BRR) = 0x00000000;

	*(USART2_CR1) = 0x0000000C;
	*(USART2_CR2) = 0x00000000;
	*(USART2_CR3) = 0x00000000;
	*(USART2_CR1) |= 0x2000;
}

void print_str(const char *str)
{
	while (*str) {
		while (!(*(USART2_SR) & USART_FLAG_TXE));
		 *(USART2_DR) = (*str & 0xFF);
		str++;
	}
}

void print_char(const char *str)
{
	if (*str) {
		while (!(*(USART2_SR) & USART_FLAG_TXE));
		*(USART2_DR) = (*str & 0xFF);
	}
}

char recv_char(void)
{
	while (1) {
		if ((*USART2_SR) & (USART_FLAG_RXNE)) {
			return (*USART2_DR) & 0xff;
		}
	}
}

void clear_buffer(char *buffer, size_t index)
{
	int i;
	for(i=index;i>=0;i--)
		buffer[i]='\0';
}


void reverse(char *str)
{
	int i, length;
	char c;
	char *s = str;
	
	length = 0;
	while (*s++) 
		length++;
	for (i = 0; i < length; i++, length--) {
		c = str[i];
		str[i] = str[length-1];
		str[length-1] = c;
	}
}

void itoa(int n, char *str)
{
	int i, sign;
	if ((sign = n) < 0)
		n = -n;
	i = 0;
	do {
		str[i++] = n % 10 + '0';
	} while((n /= 10) > 0);
	if (sign < 0)
		str[i++] = '-';
	str[i] = '\0';
	reverse(str);
}

void fibonaccishell(int number)
{
	 char *str = malloc(20);
	 int result = fibonacci(number);
	 itoa(result,str);
	 print_str(str);
	 free(str);
}

int strcmp(const char *str1, const char *str2)
{
	while(1){
		if (*str1 == '\0' || *str2 == '\0'){
			if (*str1 == *str2) return 1;
			return 0;
		}
		else if(*str1++ != *str2++) break;
	}
	return 0;
}

void command_detect(char *str, size_t index)
{
	if (strcmp("fib", str)){
		print_str("Fibonacci is excuted ...\n");
		if (thread_create((void*)(fibonaccishell), (void *)(15)) == -1)
			print_str("Fibonacci creation failed\r\n");
		else
			print_str("Fibonacci creation successfully\r\n");
	}
}

void shell(void *userdata)
{
	char buffer[MAX_Input];
	size_t index;

	while(1) {
		print_str("kuroshell:~$ ");
		index=0;
		while(1) {
			buffer[index] = recv_char();

			/*Detect "Enter" hit or a new line character */
			if (buffer[index] == 13 || buffer[index] == '\n'){
				print_char("\n");
				buffer[index]= '\0';
				/*if command exists*/
				command_detect(buffer, index);
				break;
			}
			/*Detect "Backspace"*/
			else if(buffer[index] == 8 || buffer[index] ==127){
				if(index != 0){
					print_char("\b");
					print_char(" ");
					print_char("\b");
					/*if reach end*/
					buffer[index--] = '\0';
				}
			}
			else {
				print_char(&buffer[index++]);
			}

			/*Prevent index overflow*/
			if (index == MAX_Input)
				index--;
		}
		clear_buffer(buffer, index);
	}
}

/* 72MHz */
#define CPU_CLOCK_HZ 72000000

/* 100 ms per tick. */
#define TICK_RATE_HZ 10

int main(void)
{
	const char *str1 = "Task_SHELL";
	usart_init();

	if (thread_create(shell, (void *) str1) == -1)
		print_str("SHELL creation failed\r\n");

	/* SysTick configuration */
	*SYSTICK_LOAD = (CPU_CLOCK_HZ / TICK_RATE_HZ) - 1UL;
	*SYSTICK_VAL = 0;
	*SYSTICK_CTRL = 0x07;

	thread_start();

	return 0;
}
