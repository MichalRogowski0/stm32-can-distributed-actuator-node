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
void GPIO_Configuration(void);
void TIM2_Configuration(void);
void CAN1_Configuration(void);
uint8_t CAN1_Receive_Message(volatile uint32_t *id, volatile uint8_t *data, volatile uint8_t *len);
void CAN1_SendMessage(uint32_t id, uint8_t *data, uint8_t len);
void CAN1_RX0_IRQHandler(void);

volatile uint32_t received_id;
volatile uint8_t received_data;
volatile uint8_t received_length;

int main(void)
{
    SystemClock_Config();
    GPIO_Configuration();
    TIM2_Configuration();
    CAN1_Configuration();

    while (1)
    {
        
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

void GPIO_Configuration(void)
{
    RCC -> AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // Enable GPIOB clock

    GPIOB -> MODER &= ~GPIO_MODER_MODER8; // Clear mode bits for PB8
    GPIOB -> MODER |= GPIO_MODER_MODER8_1; // Set PB8 as alternate function mode

    GPIOB -> AFR[1] &= ~GPIO_AFRH_AFRH0; // Clear alternate function bits for PB8
    GPIOB -> AFR[1] |= GPIO_AFRH_AFRH0_0 | GPIO_AFRH_AFRH0_3; // Set alternate function 9 (CAN1) for PB8

    GPIOB -> PUPDR &= ~GPIO_PUPDR_PUPDR8; // Clear pull-up/pull-down bits for PB8
    GPIOB -> PUPDR |= GPIO_PUPDR_PUPDR8_0; // Set PB8 as pull-up

    //PB9 - CAN1_TX
    GPIOB -> MODER &= ~GPIO_MODER_MODER9; // Clear mode bits for PB9
    GPIOB -> MODER |= GPIO_MODER_MODER9_1; // Set PB9 as alternate function mode

    GPIOB -> AFR[1] &= ~GPIO_AFRH_AFRH1; // Clear alternate function bits for PB9
    GPIOB -> AFR[1] |= GPIO_AFRH_AFRH1_0 | GPIO_AFRH_AFRH1_3; // Set alternate function 9 (CAN1) for PB9
    
    GPIOB -> OSPEEDR |= GPIO_OSPEEDR_OSPEED8 | GPIO_OSPEEDR_OSPEED9; // Set PB8 and PB9 to high speed
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

void CAN1_Configuration(void){
    RCC -> APB1ENR |= RCC_APB1ENR_CAN1EN; // Enable CAN1 clock

    CAN1 -> MCR &= ~CAN_MCR_SLEEP; // Exit sleep mode
    while(CAN1 -> MSR & CAN_MSR_SLAK); // Wait until sleep mode is exited
    CAN1 -> MCR |= CAN_MCR_INRQ; // Request initialization mode
    while(!(CAN1 -> MSR & CAN_MSR_INAK)); // Wait until initialization

    CAN1 -> BTR = (2 - 1) << CAN_BTR_BRP_Pos; // Set prescaler to 2 (16 MHz / 2 = 8 MHz)
    CAN1 -> BTR |= (11 -1) << CAN_BTR_TS1_Pos; // Set time segment 1 to 11 (8 MHz / (1 + 11 + 2) = 500 kHz)
    CAN1 -> BTR |= (4 - 1) << CAN_BTR_TS2_Pos; // Set time segment 2 to 4

    CAN1 -> MCR &= ~CAN_MCR_INRQ; // Exit initialization mode
    while(CAN1 -> MSR & CAN_MSR_INAK); // Wait until normal mode is entered

    // Filter configuration for CAN1
    CAN1->FMR |= CAN_FMR_FINIT;                  // Enter filter initialization mode
    CAN1->FA1R &= ~CAN_FA1R_FACT0;               // Deactivate filter 0 prior to configuration

    CAN1->FMR &= ~CAN_FMR_CAN2SB;                // Clear CAN2 start bank bits
    CAN1->FMR |= (14 << CAN_FMR_CAN2SB_Pos);     // Assign 14 filter banks (0-13) to CAN1

    CAN1->FM1R &= ~CAN_FM1R_FBM0;                // Set filter 0 to identifier mask mode
    CAN1->FS1R |= CAN_FS1R_FSC0;                 // Set single 32-bit scale configuration for filter 0
    CAN1->FFA1R &= ~CAN_FFA1R_FFA0;              // Assign filter 0 to FIFO 0

    CAN1->sFilterRegister[0].FR1 = 0x00000000;   // Filter ID = 0 (Accept all IDs)
    CAN1->sFilterRegister[0].FR2 = 0x00000000;   // Filter Mask = 0 (Ignore all bits)

    CAN1->FA1R |= CAN_FA1R_FACT0;                // Activate filter 0
    CAN1->FMR &= ~CAN_FMR_FINIT;                 // Exit filter initialization mode

    CAN1 -> IER |= CAN_IER_FMPIE0; // Enable FIFO 0 message pending interrupt
    NVIC_SetPriority(CAN1_RX0_IRQn, 2); // Set priority for CAN1 RX0 interrupt
    NVIC_EnableIRQ(CAN1_RX0_IRQn); // Enable CAN1 RX0 interrupt in NVIC
}

uint8_t CAN1_Receive_Message(volatile uint32_t *id, volatile uint8_t *data, volatile uint8_t *len) {
    if(CAN1 -> RF0R & CAN_RF0R_FMP0) { // Check if there is a message pending in FIFO 0
        *id = (CAN1 -> sFIFOMailBox[0].RIR >> 21) & 0x7FF; // Get the ID of the received message
        *len = (uint8_t)(CAN1 -> sFIFOMailBox[0].RDTR & 0x0F); // Get the length of the received message
        *data = (uint8_t)(CAN1 -> sFIFOMailBox[0].RDLR & 0xFF); // Get the data of the received message
        CAN1 -> RF0R |= CAN_RF0R_RFOM0; // Release FIFO 0 output mailbox
        return 1; // Message received successfully
    }
    return 0; // No message received
}

void CAN1_SendMessage(uint32_t id, uint8_t *data, uint8_t len)
{
    if(len > 8) 
    {
        len = 8; // Limit data length to 8 bytes
    }

    uint32_t tsr = CAN1->TSR;
    uint8_t mailbox;

    if (tsr & CAN_TSR_TME0) mailbox = 0;
    else if (tsr & CAN_TSR_TME1) mailbox = 1;
    else if (tsr & CAN_TSR_TME2) mailbox = 2;
    else return;

    CAN1 -> sTxMailBox[mailbox].TDTR = len << CAN_TDT0R_DLC_Pos; // Set data length
    CAN1 -> sTxMailBox[mailbox].TDLR = *data; // Set data to send
    CAN1 -> sTxMailBox[mailbox].TIR = (id << CAN_TI0R_STID_Pos) | CAN_TI0R_TXRQ; // Set standard identifier and request transmission
}

void CAN1_RX0_IRQHandler(void)
{
    if(CAN1 -> RF0R & CAN_RF0R_FMP0)
    { //check if there is a message pending in FIFO 0
        uint32_t received_id = (CAN1 -> sFIFOMailBox[0].RIR >> 21) & 0x7FF;
        uint8_t received_len = (uint8_t)(CAN1 -> sFIFOMailBox[0].RDTR & 0x0F);
        uint8_t received_data = (uint8_t)(CAN1 -> sFIFOMailBox[0].RDLR & 0xFF);

        CAN1 -> RF0R |= CAN_RF0R_RFOM0; // Release FIFO 0 output mailbox
        if(received_id == 0x103)
        {
            TIM2 -> CCR1 = received_data * 4095 / 100; // Update PWM duty cycle based on received data
        }
    }
}
