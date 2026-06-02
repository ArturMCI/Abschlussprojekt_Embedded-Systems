#include <stm32f0xx.h>
#include <stdio.h>

int main(void) {
    printf("%lu", SystemCoreClock);
}