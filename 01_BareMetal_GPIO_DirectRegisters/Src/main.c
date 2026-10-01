/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Direct register bitwise manipulation for GPIO on STM32F446RE
 *                   Control directo de pines GPIO mediante registros en STM32F446RE
 ******************************************************************************
 */

#include <stdint.h>

/* Direcciones base de la memoria de perifericos / Peripheral base addresses */
#define RCC_BASE      0x40023800UL  // Reset and Clock Control (AHB1)
#define GPIOA_BASE    0x40020000UL  // General Purpose I/O Port A (AHB1)

/* Registros especificos mediante punteros volatiles / Register pointer definitions */
#define RCC_AHB1ENR   (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define GPIOA_MODER   (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_ODR     (*(volatile uint32_t *)(GPIOA_BASE + 0x14UL))

int main(void)
{
    /* 1. Habilitar reloj para el puerto GPIOA en el bus AHB1 / Enable GPIOA clock on AHB1 */
    RCC_AHB1ENR |= (1U << 0);

    /* 2. Configurar pin PA5 (LED de usuario LD2) como salida de proposito general (Modo 01) */
    /* Configure PA5 (User LED LD2) as general purpose output (Mode 01) */
    GPIOA_MODER &= ~(3U << (5 * 2)); // Limpiar bits 10 y 11 / Clear mode bits
    GPIOA_MODER |=  (1U << (5 * 2)); // Establecer '01' (Salida) / Set mode '01' (Output)

    /* 3. Encender LED de usuario en PA5 / Turn on User LED on PA5 */
    GPIOA_ODR |= (1U << 5);

    /* Bucle infinito / Infinite loop */
    for(;;)
    {
    }
}
