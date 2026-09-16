
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "main.h"
#include "stm32f446xx.h"


void SystemClock_Config(void);
void Error_Handler(void);
void UART2_Configuration(void);
void USART2_SendChar(char c);
void USART2_SendString(char *str);
uint8_t USART2_ReceiveChar(char *c);
void ADC_configuration(void);
void DMA_configuration(void);
void TIM2_Configuration(void);

char receivedChar;
volatile uint16_t adcBuffer[1]; // Buffer to store ADC value
uint8_t PWM_value = 0; // Variable to store scaled ADC value for PWM

int main(void)
{

    SystemClock_Config();
    UART2_Configuration();
    DMA_configuration();
    ADC_configuration();
    TIM2_Configuration();

    char msg[64];

    while(1)
    {
        snprintf(msg, sizeof(msg), "ADC Value: %u | Duty: %u\r\n", adcBuffer[0], PWM_value);
        USART2_SendString(msg);
        TIM2 -> CCR1 = (adcBuffer[0] * 999) / 4095; // Update PWM duty cycle based on ADC value
        PWM_value = (adcBuffer[0] * 100) / 4095; // Scale ADC value to 0-100 for PWM percentage
        for (volatile int i = 0; i < 500000; i++);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}

void UART2_Configuration(void)
{
    RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    //PA2 - TX
    GPIOA -> MODER |= GPIO_MODER_MODE2_1; // Alternate function mode
    GPIOA -> AFR[0] &= ~GPIO_AFRL_AFRL2;
    GPIOA -> AFR[0] |= GPIO_AFRL_AFRL2_0 | GPIO_AFRL_AFRL2_1 | GPIO_AFRL_AFRL2_2; // Alternate function 7 (USART2)

    //PA3 - RX
    GPIOA -> MODER |= GPIO_MODER_MODE3_1; // Alternate function mode
    GPIOA -> AFR[0] &= ~GPIO_AFRL_AFRL3;
    GPIOA -> AFR[0] |= GPIO_AFRL_AFRL3_0 | GPIO_AFRL_AFRL3_1 | GPIO_AFRL_AFRL3_2; // Alternate function 7 (USART2)

    RCC -> APB1ENR |= RCC_APB1ENR_USART2EN; // Enable USART2 clock

    USART2 -> CR1 |= USART_CR1_UE | USART_CR1_RE | USART_CR1_TE; // Enable USART2
    USART2 -> BRR = (16000000 + 115200 / 2) / 115200; // Set baud rate to 115200 (16 MHz clock)
}

void USART2_SendChar(char c)
{
    while(!(USART2 -> SR & USART_SR_TXE));
    USART2 -> DR = c;
}

void USART2_SendString(char *str)
{
    while(*str != '\0')
    {
        USART2_SendChar(*str);
        str++;
    }
}

uint8_t USART2_ReceiveChar(char *c)
{
    while(USART2 -> SR & USART_SR_RXNE)
    {
        *c = USART2 -> DR;
        return 1;
    }
    return 0;
}

void ADC_configuration(void)
{
    RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // Enable GPIOA clock
    RCC -> APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable ADC1 clock

    GPIOA -> MODER |= GPIO_MODER_MODER1; // Set PA1 as analog mode

    ADC1 -> SQR1 &= ~ADC_SQR1_L; // Set regular sequence length to 1
    ADC1 -> SQR3 |= ADC_SQR3_SQ1_0; // Set first conversion in regular sequence to channel 1 (PA1)
    ADC1 -> SMPR2 |= ADC_SMPR2_SMP1; // Set sample time for channel 480 cycles
    ADC1 -> CR2 |= ADC_CR2_ADON; // Enable ADC1
    ADC1 -> CR2 |= ADC_CR2_CONT; // Enable continuous conversion mode
    ADC1 -> CR2 |= ADC_CR2_DMA; // Enable DMA mode
    ADC1 -> CR2 |= ADC_CR2_DDS; // Enable DMA continuous requests
    ADC1 -> CR2 |= ADC_CR2_SWSTART; // Start conversion
}

void DMA_configuration(void)
{
    RCC -> AHB1ENR |= RCC_AHB1ENR_DMA2EN; // Enable DMA2 clock

    DMA2_Stream0 -> CR &= ~DMA_SxCR_EN; // Disable DMA stream 0
    while(DMA2_Stream0 -> CR & DMA_SxCR_EN); // Wait until stream is disabled

    DMA2_Stream0 -> PAR = (uint32_t)&ADC1 -> DR; // Set peripheral address to ADC1 data register
    DMA2_Stream0 -> M0AR = (uint32_t)adcBuffer; // Set memory address to adcBuffer
    DMA2_Stream0 -> NDTR = 1; // Set number of data items to transfer

    DMA2_Stream0 -> CR &= ~DMA_SxCR_CHSEL; // Select channel 0 for stream 0
    DMA2_Stream0 -> CR &= ~DMA_SxCR_DIR; // Set data transfer direction to peripheral-to-memory
    DMA2_Stream0 -> CR |= DMA_SxCR_PSIZE_0; // Set peripheral data size to 16 bits
    DMA2_Stream0 -> CR |= DMA_SxCR_MSIZE_0; // Set memory data size to 16 bits
    DMA2_Stream0 -> CR |= DMA_SxCR_CIRC; // Enable circular mode

    DMA2_Stream0 -> CR |= DMA_SxCR_EN; // Enable DMA stream 0
}

void TIM2_Configuration(void)
{
    RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // Enable GPIOA clock
    RCC -> APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 clock

    GPIOA -> MODER &= ~GPIO_MODER_MODER5; // Clear mode bits for PA5
    GPIOA -> MODER |= GPIO_MODER_MODER5_1; // Set PA5 as alternate function mode

    GPIOA -> AFR[0] &= ~GPIO_AFRL_AFRL5; // Clear alternate function bits for PA5
    GPIOA -> AFR[0] |= GPIO_AFRL_AFRL5_0; // Set alternate function 1 (TIM2) for PA5

    TIM2 -> PSC = (16 - 1); // Set prescaler to 16 (16 MHz / 16 = 1 MHz)
    TIM2 -> ARR = (1000 - 1); // Set auto-reload value to 1000 (1 MHz / 1000 = 1 kHz)
    TIM2 -> CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1; // Set output compare mode to PWM mode 1
    TIM2 -> CCMR1 |= TIM_CCMR1_OC1PE; // Enable output compare preload
    TIM2 -> CCER |= TIM_CCER_CC1E; // Enable output for channel 1
    TIM2 -> CR1 |= TIM_CR1_ARPE; // Enable auto-reload preload
    TIM2 -> CR1 |= TIM_CR1_CEN; // Enable TIM2
}