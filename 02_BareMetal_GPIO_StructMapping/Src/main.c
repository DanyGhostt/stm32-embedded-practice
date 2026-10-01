/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : CMSIS-style peripheral struct mapping for GPIO on STM32F446RE
 *                   Mapeo de registros perifericos mediante estructuras en C (Estilo CMSIS)
 ******************************************************************************
 */

#include <stdint.h>

/* Direcciones base de perifericos en el mapa de memoria / Memory mapped base addresses */
#define RCC_BASE      0x40023800UL  // Reset and Clock Control (AHB1)
#define GPIOA_BASE    0x40020000UL  // General Purpose I/O Port A (AHB1)

#define RCC_AHB1ENR   (*(volatile uint32_t *)(RCC_BASE + 0x30UL))

/**
 * @brief Estructura de mapeo de registros de GPIO / GPIO Register layout structure
 * Cumple con la disposicion exacta de memoria descrita en el Reference Manual (RM0390)
 */
typedef struct {
    volatile uint32_t MODER;    // 0x00: Modo de configuracion (Input, Output, AF, Analog)
    volatile uint32_t OTYPER;   // 0x04: Tipo de salida (Push-Pull, Open-Drain)
    volatile uint32_t OSPEEDR;  // 0x08: Velocidad de salida (Low, Medium, Fast, High)
    volatile uint32_t PUPDR;    // 0x0C: Resistencias de Pull-up / Pull-down
    volatile uint32_t IDR;      // 0x10: Registro de datos de entrada (Lectura de pines)
    volatile uint32_t ODR;      // 0x14: Registro de datos de salida (Escritura de pines)
    volatile uint32_t BSRR;     // 0x18: Registro atomico Set / Reset
    volatile uint32_t LCKR;     // 0x1C: Registro de bloqueo de configuracion
    volatile uint32_t AFR[2];   // 0x20 - 0x24: Registros de funcion alternativa (AFRL / AFRH)
} GPIO_TypeDef;

/* Puntero tipado a la direccion base del puerto GPIOA / Typed pointer to GPIOA base */
#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)

int main(void)
{
    /* 1. Habilitar reloj para GPIOA en el bus AHB1 / Enable GPIOA peripheral clock */
    RCC_AHB1ENR |= (1U << 0);

    /* 2. Configurar PA5 (LED de usuario LD2) como salida de proposito general (Modo 01) */
    /* Configure PA5 (User LED LD2) as general purpose output (Mode 01) */
    GPIOA->MODER &= ~(3U << (5 * 2)); // Limpiar mascara de 2 bits en posicion 10-11 / Clear mode mask
    GPIOA->MODER |=  (1U << (5 * 2)); // Establecer '01' (Salida) / Set '01' (Output mode)

    /* 3. Encender LED de usuario en PA5 / Turn on User LED on PA5 */
    GPIOA->ODR |= (1U << 5);

    /* Bucle infinito / Infinite loop */
    for(;;)
    {
    }
}
