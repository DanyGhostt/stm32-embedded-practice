# ⚡ STM32 Embedded Systems & Bare-Metal Laboratory

<p align="center">
  <img src="https://raw.githubusercontent.com/marwin1991/profile-technology-icons/main/icons/stmicroelectronics.png" alt="STMicroelectronics" width="220"/>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Target_MCU-STM32F446RE-03234B?style=for-the-badge&logo=stmicroelectronics&logoColor=white" alt="MCU STM32F446RE"/>
  <img src="https://img.shields.io/badge/Core-ARM%20Cortex--M4%20%40%20180MHz-0091BD?style=for-the-badge&logo=arm&logoColor=white" alt="ARM Cortex-M4"/>
  <img src="https://img.shields.io/badge/Paradigm-Bare--Metal%20%26%20Registers-E6007A?style=for-the-badge" alt="Bare Metal"/>
  <img src="https://img.shields.io/badge/RTOS-FreeRTOS%20%2F%20CMSIS--RTOSv2-green?style=for-the-badge&logo=c" alt="FreeRTOS"/>
  <img src="https://img.shields.io/badge/IDE-STM32CubeIDE-blue?style=for-the-badge&logo=eclipseide&logoColor=white" alt="STM32CubeIDE"/>
  <img src="https://img.shields.io/badge/Status-Active%20%2F%20Continuous%20Learning-orange?style=for-the-badge" alt="Active Status"/>
</p>

---

## 📌 Visión General / Overview

Bienvenido al repositorio de prácticas y laboratorio experimental de desarrollo embebido para la plataforma **STM32** (específicamente la familia **STM32F446RE** en tarjeta **NUCLEO-F446RE**).

El propósito central de este repositorio es dominar la arquitectura interna del microcontrolador a bajo nivel (**Bare-Metal**), profundizando en la manipulación directa de registros de hardware (*Direct Register Access*), el mapeo de memoria en C (*CMSIS Structure Mapping*), el control fino de buses y periféricos (RCC, GPIO, TIM, ADC, EXTI, 1-Wire, USART), así como la integración con sistemas operativos de tiempo real (**FreeRTOS / CMSIS-RTOS v2**).

> 💡 **Living Repository**: Este repositorio se encuentra en **aprendizaje y desarrollo constante**. Nuevos módulos, controladores periféricos bare-metal, experimentos de temporización y arquitecturas multihilo se incorporan y actualizan periódicamente.

### 🇬🇧 English
Welcome to the hands-on practice repository and experimental embedded development laboratory for the **STM32** platform (specifically targeting the **STM32F446RE** family on the **NUCLEO-F446RE** development board).

The core objective of this repository is to master low-level microcontroller architecture (**Bare-Metal**), focusing on Direct Register Access, CMSIS Structure Mapping, fine-grained bus and peripheral control (RCC, GPIO, TIM, ADC, EXTI, 1-Wire, USART), as well as integration with real-time operating systems (**FreeRTOS / CMSIS-RTOS v2**).

> 💡 **Living Repository**: This repository is under **active development and continuous learning**. New modules, bare-metal peripheral drivers, hardware timing experiments, and multithreaded architectures are periodically implemented and updated.


---

## 🛠️ Especificaciones del Hardware / Target Hardware

| Parámetro | Especificación Técnica |
| :--- | :--- |
| **Microcontrolador** | **STM32F446RET6** (Paquete LQFP64) |
| **Arquitectura de CPU** | ARM® 32-bit Cortex®-M4 con FPU (Hardware Floating-Point) y DSP |
| **Frecuencia de Reloj (SYSCLK)** | Hasta **180 MHz** (Gobernado por PLL interno con oscilador HSI / HSE) |
| **Memoria Flash** | **512 KB** de memoria de programa |
| **Memoria SRAM** | **128 KB** de memoria de datos de alta velocidad |
| **Buses de Sistema** | **AHB1** (180 MHz), **AHB2** (180 MHz), **APB1** (45/90 MHz), **APB2** (90/180 MHz) |
| **Plataforma de Desarrollo** | **STMicroelectronics NUCLEO-F446RE** con programador/depurador **ST-LINK/V2-1** |
| **Periféricos de Interés** | GPIO, TIM1/TIM2/TIM3/TIM4, ADC1 (12-bit), EXTI, SYSCFG, USART2 (VCP) |

---

## 🗺️ Mapa de Memoria y Arquitectura de Buses

El microcontrolador organiza su espacio de direccionamiento de 4 GB de acuerdo con la especificación ARM Cortex-M4 y el Reference Manual (**RM0390**):

```
+-------------------------------------------------------------------------+
|                    STM32F446RE MEMORY MAP OVERVIEW                      |
+-------------------+--------------------+--------------------------------+
| Región / Memory   | Rango de Dirección | Descripción                    |
+-------------------+--------------------+--------------------------------+
| FLASH Program Mem | 0x08000000         | Código ejecutable y constantes |
| SRAM (128 KB)     | 0x20000000         | Variables, Heap y Stacks       |
| Periféricos APB1  | 0x40000000         | TIM2, TIM3, TIM4, USART2, etc. |
| Periféricos APB2  | 0x40010000         | TIM1, ADC1, EXTI, SYSCFG       |
| Periféricos AHB1  | 0x40020000         | GPIOA..GPIOH, RCC, DMA1, DMA2  |
| Cortex-M4 Internal| 0xE000E000         | NVIC, SysTick, SCB, MPU        |
+-------------------+--------------------+--------------------------------+
```

---

## 📂 Catálogo de Prácticas y Proyectos / Project Catalog

A continuación se detalla cada uno de los proyectos implementados en el repositorio:

### 1. `01_BareMetal_GPIO_DirectRegisters`
- **Enfoque**: Programación Bare-Metal mediante punteros directos a memoria.
- **Conceptos**: Desreferenciación de punteros volátiles (`volatile uint32_t *`), activación del reloj del bus AHB1 en el registro `RCC_AHB1ENR`, configuración de dirección en `GPIOA_MODER` (Modo salida general 01), y control del pin PA5 (User LED LD2) mediante `GPIOA_ODR`.
- **Fórmula de bits**: `GPIOA_MODER &= ~(3U << (5 * 2)); GPIOA_MODER |= (1U << (5 * 2));`

---

### 2. `02_BareMetal_GPIO_StructMapping`
- **Enfoque**: Mapeo periférico mediante estructuras en C siguiendo el estándar CMSIS.
- **Conceptos**: Creación del tipo `GPIO_TypeDef` con la secuencia exacta de registros de hardware (`MODER`, `OTYPER`, `OSPEEDR`, `PUPDR`, `IDR`, `ODR`, `BSRR`, `LCKR`, `AFR[2]`). Casteo de dirección base `#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)` para un acceso a registros más limpio, mantenible y robusto.

---

### 3. `03_BareMetal_EXTI_Button_Stopwatch`
- **Enfoque**: Interrupciones externas a nivel de registros + Cronometrado de microsegundos + FreeRTOS.
- **Conceptos**:
  - Configuración del multiplexor de interrupciones externas `SYSCFG->EXTICR4` para enrutar el botón de usuario (PC13) a la línea `EXTI13`.
  - Configuración de flanco de bajada en `EXTI_FTSR` y desenmascaramiento en `EXTI_IMR`.
  - Habilitación en el NVIC de Cortex-M4 mediante `NVIC_ISER1` (IRQ 40: `EXTI15_10_IRQn`).
  - Rutina de servicio `EXTI15_10_IRQHandler` con limpieza obligatoria de bandera en `EXTI_PR`.
  - Medición de tiempos de ejecución de tareas con resolución de microsegundos usando el registro contador del Timer 4 (`TIM4_CNT`).
  - Sincronización de 3 tareas concurrentes de señalización LED mediante un Mutex de CMSIS-RTOS (`myMutex01Handle`).

---

### 4. `04_BareMetal_ADC_PWM_ServoControl`
- **Enfoque**: Generación de PWM y conversión analógica por registros + FreeRTOS.
- **Conceptos**:
  - Configuración del Convertidor Analógico-Digital **ADC1** en PA0 (Canal 0) mediante manipulación de registros de control (`REG_ADC1_CR2`, `REG_ADC1_SR`, `REG_ADC1_DR`).
  - Configuración de **Timer 3 Canal 1** (PA6) en modo PWM 1 (`TIM3_CCMR1`, `TIM3_CCER`, `TIM3_PSC = 83`, `TIM3_ARR = 19999`) para generar una señal PWM exacta de 50 Hz (periodo de 20 ms).
  - Pipeline en tiempo real: Tarea 1 muestrea el potenciómetro analógico (12 bits: 0 a 4095) y Tarea 2 mapea matemáticamente el valor al ancho de pulso del servomotor (1.0 ms a 2.0 ms / `CCR1 = 1000 a 2000`).

---

### 5. `05_BareMetal_Ultrasonic_HCSR04`
- **Enfoque**: Controlador bare-metal para sensor ultrasónico HC-SR04 sin librerías HAL.
- **Conceptos**:
  - Base de tiempo de 1 µs por tick utilizando el contador de hardware de **TIM1** (`TIM1->PSC = 83`).
  - Generación de pulso Trigger de 10 µs en PA9 utilizando el registro atómico `GPIOA->BSRR`.
  - Captura del pulso Echo en PA8 con temporización precisa y cálculo de distancia en centímetros:
    $$\text{Distancia (cm)} = \frac{\text{Tiempo Echo } (\mu\text{s}) \times 0.0343}{2} = \text{Tiempo Echo } (\mu\text{s}) \times 0.01715$$
  - Guarda de seguridad por software (*timeout guard*) para evitar bloqueos por desconexión del sensor.

---

### 6. `06_BareMetal_1Wire_DS18B20_TempSensor`
- **Enfoque**: Implementación del protocolo digital 1-Wire por registros + FreeRTOS + USART Telemetría.
- **Conceptos**:
  - Protocolo 1-Wire bit-banging sobre el pin PA0 configurado en modo **Open-Drain con Pull-Up interno**.
  - Control estricto de microsegundos con hardware timer **TIM2** (32 bits) para ranuras de tiempo críticas (Reset pulse de 480 µs, Write slot de 60 µs, Read slot de 15 µs).
  - Protección de ranuras de tiempo mediante secciones críticas de FreeRTOS (`taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()`).
  - Comunicación entre hilos mediante cola de mensajes (`QueueTemp`) transmitiendo los datos procesados a la tarea de consola (`vTaskConsole`) a través de **USART2** (115200 baudios, 8-N-1) conectado al Virtual COM Port de ST-LINK.

---

### 7. `07_FreeRTOS_Binary_Semaphore_Sync`
- **Enfoque**: Sincronización inter-tarea con Semáforos Binarios (CMSIS-RTOS v2).
- **Conceptos**:
  - Creación de semáforo binario `osSemaphoreNew(1, 1, ...)`.
  - Tarea productora/señalizadora (`SendTask`) que libera el semáforo periódicamente (`osSemaphoreRelease`).
  - Tarea consumidora/receptora (`ReciveTask`) que se bloquea en espera del semáforo con `osSemaphoreAcquire(..., osWaitForever)` y conmuta el estado del LED al recibir la señal.

---

### 8. `08_FreeRTOS_Queue_TempFanActuator`
- **Enfoque**: Comunicación entre procesos (IPC) con Colas de Mensajes en FreeRTOS.
- **Conceptos**:
  - Cola de mensajes tipada `osMessageQueueNew(...)`.
  - Tarea de telemetría térmica (`TempCheckTask`) que produce datos de temperatura y los envía con `osMessageQueuePut`.
  - Tarea de control de actuador (`FanControlTask`) que recibe los datos con `osMessageQueueGet` y evalúa umbrales térmicos para conmutar el ventilador/actuador con histéresis y reporte por consola serial.

---

### 9. `09_FreeRTOS_Multitasking_Profiling`
- **Enfoque**: Arquitectura multitarea, análisis de planificación y perfilado de CPU.
- **Conceptos**:
  - Ejecución de 5 tareas concurrentes bajo el planificador preemptivo de FreeRTOS con distintas prioridades.
  - Conmutación de pines en puerto GPIOA para medición en vivo mediante osciloscopio o analizador lógico.
  - Telemetría de tiempo de CPU y medición de jitter de planificación utilizando el cronómetro de hardware de alta resolución de **TIM4** (`TIM4_CNT_M`).

---

## 📊 Tabla Resumen de Periféricos y Registros

| Práctica | Periféricos Involucrados | Registros Clave Utilizados | Modo de Operación |
| :--- | :--- | :--- | :--- |
| **01_BareMetal_GPIO_DirectRegisters** | RCC, GPIOA | `RCC_AHB1ENR`, `GPIOA_MODER`, `GPIOA_ODR` | Bare-Metal Pointer Dereferencing |
| **02_BareMetal_GPIO_StructMapping** | RCC, GPIOA | `GPIO_TypeDef` (`MODER`, `ODR`, `BSRR`, `OTYPER`) | CMSIS Struct Memory Mapping |
| **03_BareMetal_EXTI_Button_Stopwatch** | RCC, GPIOA, GPIOC, SYSCFG, EXTI, TIM4, NVIC | `SYSCFG_EXTICR4`, `EXTI_IMR`, `EXTI_FTSR`, `EXTI_PR`, `NVIC_ISER1`, `TIM4_CNT` | External Interrupts + Microsecond Stopwatch + Mutex |
| **04_BareMetal_ADC_PWM_ServoControl** | RCC, GPIOA, TIM3, ADC1 | `TIM3_CR1`, `TIM3_CCMR1`, `TIM3_CCER`, `TIM3_CCR1`, `ADC1_CR2`, `ADC1_DR` | 50Hz PWM Output + 12-bit ADC + RTOS Pipeline |
| **05_BareMetal_Ultrasonic_HCSR04** | RCC, GPIOA, TIM1 | `TIM1_CR1`, `TIM1_PSC`, `TIM1_ARR`, `TIM1_CNT`, `GPIOA_BSRR` | 1µs Hardware Timebase + Echo Width Measurement |
| **06_BareMetal_1Wire_DS18B20_TempSensor**| RCC, GPIOA, TIM2, USART2 | `TIM2_CNT`, `GPIOA_MODER`, `GPIOA_OTYPER`, `USART2_BRR`, `USART2_CR1` | 1-Wire Bit-Banging + Critical Sections + Serial VCP |
| **07_FreeRTOS_Binary_Semaphore_Sync** | FreeRTOS Kernel, GPIOA | `osSemaphoreNew`, `osSemaphoreAcquire`, `osSemaphoreRelease` | Binary Semaphore Task Synchronization |
| **08_FreeRTOS_Queue_TempFanActuator** | FreeRTOS Kernel, USART2 | `osMessageQueueNew`, `osMessageQueuePut`, `osMessageQueueGet` | Inter-Task Message Queue IPC |
| **09_FreeRTOS_Multitasking_Profiling** | FreeRTOS Kernel, TIM4, GPIOA | `TIM4_CNT`, `GPIOA_MODER`, `GPIOA_BSRR` | Multi-Task CPU Execution Profiling |

---

## 💻 Entorno de Desarrollo y Herramientas / Tools & Toolchain

- **IDE**: [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) (v1.14.0 o superior)
- **Compilador**: GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`)
- **Estándar C**: C99 / C11
- **Depuración y Flashing**: ST-LINK/V2-1 integrado en tarjeta NUCLEO
- **Terminal Serial**: PuTTY, Tera Term, Serial Monitor de STM32CubeIDE o baudrate 115200 (8-N-1)

---

## 🚀 Guía de Uso / How to Build & Run

1. **Clonar el repositorio**:
   ```bash
   git clone https://github.com/DanyGhostt/stm32-embedded-practice.git
   ```

2. **Importar en STM32CubeIDE**:
   - Abrir **STM32CubeIDE**.
   - Ir a `File` -> `Import...` -> `General` -> `Existing Projects into Workspace`.
   - Seleccionar el directorio de la práctica que deseas compilar (por ejemplo, `06_BareMetal_1Wire_DS18B20_TempSensor`).
   - Hacer clic en `Finish`.

3. **Compilar y Flashear**:
   - Conectar la tarjeta **STM32 NUCLEO-F446RE** mediante cable USB.
   - Presionar `Ctrl + B` para compilar (Build).
   - Presionar `F11` para iniciar la sesión de depuración (Debug) o el botón `Run` para cargar el firmware directamente.

## 👤 Autor y Contacto

- **Desarrollador**: [DanyGhostt](https://github.com/DanyGhostt)
- **Repositorio**: [https://github.com/DanyGhostt/stm32-embedded-practice](https://github.com/DanyGhostt/stm32-embedded-practice)
- **Licencia**: Este repositorio se distribuye bajo la licencia MIT.

