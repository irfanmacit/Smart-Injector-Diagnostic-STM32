/**
  ******************************************************************************
  * @file           : main_example.c
  * @brief          : Example usage of the Smart Injector Diagnostic Library
  * @author         : Gemini CLI (Academic & Embedded Expert)
  ******************************************************************************
  */

#include "main.h"
#include "injector_diag.h"

/* --- Global Tanımlamalar --- */
#define ADC_BUFFER_SIZE  2048
uint16_t g_adc_raw_buffer[ADC_BUFFER_SIZE];
Injector_Metrics_t g_current_metrics;

/* --- Fonksiyon Prototipleri --- */
void System_Error_Handler(void);
void Run_Single_Injector_Test(void);

/**
  * @brief  Main program entry point
  */
int main(void) {
    /* 1. Donanım Başlatma */
    HAL_Init();
    // SystemClock_Config(); // CubeIDE tarafından oluşturulan clock ayarı
    // MX_GPIO_Init();
    // MX_DMA_Init();
    // MX_ADC1_Init();
    // MX_CRC_Init(); // Flash güvenliği için önerilir

    /* 2. Teşhis Kütüphanesini Başlatma */
    if (Injector_Diag_Init() != HAL_OK) {
        System_Error_Handler();
    }

    /* 3. İlk Kalibrasyon (Opsiyonel: Kullanıcı butonu veya açılış testi) */
    // Not: ADC boştayken (enjektör kapalı) gürültü tabanını ölçer
    // HAL_ADC_Start_DMA(&hadc1, (uint32_t*)g_adc_raw_buffer, ADC_BUFFER_SIZE);
    // Injector_Perform_AutoCalib(g_adc_raw_buffer, ADC_BUFFER_SIZE);

    while (1) {
        /* Kullanıcı testi tetiklediğinde (Örn: Buton veya UART komutu) */
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
            Run_Single_Injector_Test();
            HAL_Delay(500); // Debounce
        }
    }
}

/**
  * @brief Tek bir enjektör atımı yapar ve sonucunu AI ile analiz eder.
  */
void Run_Single_Injector_Test(void) {
    /* 1. Enjektörü Sür (PWM veya GPIO High) */
    // HAL_GPIO_WritePin(INJECTOR_DRV_GPIO_Port, INJECTOR_DRV_Pin, GPIO_PIN_SET);

    /* 2. ADC Verisini Topla (DMA ile eşzamanlı) */
    // HAL_ADC_Start_DMA(&hadc1, (uint32_t*)g_adc_raw_buffer, ADC_BUFFER_SIZE);
    
    // Veri toplamanın bitmesini bekle (Timeout veya Callback ile)
    HAL_Delay(10); // Enjektörün açık kalma süresi (Örn: 10ms)

    /* 3. Enjektörü Kapat */
    // HAL_GPIO_WritePin(INJECTOR_DRV_GPIO_Port, INJECTOR_DRV_Pin, GPIO_PIN_RESET);

    /* 4. Sinyal İşleme (Feature Extraction) */
    // Ham veriden 7 parametre çıkarılır
    Injector_Process_Signal(g_adc_raw_buffer, ADC_BUFFER_SIZE, &g_current_metrics);

    /* 5. AI Çıkarımı (Inference) */
    // 7 parametreli SVC modeline göre karar verilir
    Diag_Status_t result = Injector_Run_Inference(&g_current_metrics);

    /* 6. Sonucu Bildir */
    if (result == DIAG_HEALTHY) {
        // HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
        // HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
    } else {
        // HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
        // HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
    }
}

void System_Error_Handler(void) {
    while(1) {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14); // Kırmızı LED yanıp söner
        HAL_Delay(100);
    }
}
