#include "stdio-task.h"
#include "stdio.h"
#include "pico/stdlib.h"

#define COMMAND_BUF_LEN 128

//Буфер приема символов
char command[COMMAND_BUF_LEN] = {0};

//Счетчик длины буфера
int command_buf_idx;

//Функция инициализации
void stdio_task_init()
{
	command_buf_idx = 0;
}

//Функция обработчика
char* stdio_task_handle()
{
	int symbol = getchar_timeout_us(0);
	if (symbol == PICO_ERROR_TIMEOUT)
	{
		return NULL;
	}
    
    putchar(symbol);

	if (symbol == '\r' || symbol == '\n')
	{
		command[command_buf_idx] = '\0';
		command_buf_idx = 0;
		
		printf("received string: '%s'\n", command);

	return command;
	}

	if (command_buf_idx >= COMMAND_BUF_LEN - 1)
	{
		command_buf_idx = 0;
		return NULL;
	}

	command[command_buf_idx] = symbol;
	command_buf_idx++;
	return NULL;
}
