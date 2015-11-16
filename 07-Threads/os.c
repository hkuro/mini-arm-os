#include <stddef.h>
#include <stdint.h>
#include "reg.h"
#include "threads.h"

#define MAX_Input 50

/* USART TXE Flag
 * This flag is cleared when data is written to USARTx_DR and
 * set when that data is transferred to the TDR
 */
#define USART_FLAG_TXE	((uint16_t) 0x0080)

/* USART RXNE Flag, when RXNE is set, data can be read */
#define USART_FLAG_RXNE ((uint16_t) 0x0020)

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

static void delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

static void busy_loop(void *str)
{
	while (1) {
		print_str(str);
		print_str(": Running...\n");
		delay(1000);
	}
}

void test1(void *userdata)
{
	busy_loop(userdata);
}

void test2(void *userdata)
{
	busy_loop(userdata);
}

void test3(void *userdata)
{
	busy_loop(userdata);
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
				break;
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
	//const char *str1 = "Task1", *str2 = "Task2", *str3 = "Task3";

	const char *str1 = "Task_SHELL";
	usart_init();

	/*
	if (thread_create(test1, (void *) str1) == -1)
		print_str("Thread 1 creation failed\r\n");

	if (thread_create(test2, (void *) str2) == -1)
		print_str("Thread 2 creation failed\r\n");

	if (thread_create(test3, (void *) str3) == -1)
		print_str("Thread 3 creation failed\r\n");
		*/

	if (thread_create(shell, (void *) str1) == -1)
		print_str("SHELL creation failed\r\n");

	/* SysTick configuration */
	*SYSTICK_LOAD = (CPU_CLOCK_HZ / TICK_RATE_HZ) - 1UL;
	*SYSTICK_VAL = 0;
	*SYSTICK_CTRL = 0x07;

	thread_start();

	return 0;
}
