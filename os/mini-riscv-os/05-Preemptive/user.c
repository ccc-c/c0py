#include "os.h"

void user_task0(void)
{
	int count = 0;
	lib_puts("Task0: Created!\n");
	while (1) {
		lib_printf("Task0: count=%d, Running...\n", ++count);
		lib_delay(1000);
	}
}

void user_task1(void)
{
	int count = 0;
	lib_puts("Task1: Created!\n");
	while (1) {
		lib_printf("Task1: count=%d, Running...\n", ++count);
		lib_delay(1000);
	}
}

void user_init() {
	task_create(&user_task0);
	task_create(&user_task1);
}
