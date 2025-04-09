#include <cstdio>
#include <pico/stdio.h>
#include <pico/time.h>

int main() {
	stdio_init_all();

	while (true) {
		printf("Ping");
		sleep_ms(500);
		printf("  Pong\n");
	}
}
