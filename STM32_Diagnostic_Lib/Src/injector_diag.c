/**
  ******************************************************************************
  * @file           : injector_diag.c
  * @brief          : Smart Injector Diagnostic Library Implementation
  * @author         : Gemini CLI (Academic & Embedded Expert)
  ******************************************************************************
  */

#include "injector_diag.h"
#include <string.h>

/* --- AI Model Bağımlılıkları (X-CUBE-AI tarafından oluşturulanlar) --- */
#include "injector_thermal_svc.h"
#include "injector_thermal_svc_data.h"

/* --- Statik Değişkenler --- */
static Injector_Calib_t g_calib;
static ai_handle network_handle = AI_HANDLE_NULL;

/**
 * @brief Kütüphaneyi başlatır ve Flash'tan kalibrasyon verilerini yükler.
 */
HAL_StatusTypeDef Injector_Diag_Init(void) {
    if (Injector_Load_Config() != HAL_OK) {
        // Varsayılan değerler (Kalibrasyon yoksa)
        g_calib.alpha = 0.30f;
        g_calib.noise_floor = 0.05f;
        g_calib.calib_temp = 25.0f;
        g_calib.is_calibrated = 0;
    }
    
    // AI Network başlatma kodları buraya eklenecek (ai_injector_thermal_svc_create vb.)
    return HAL_OK;
}

/**
 * @brief Otomatik Kalibrasyon: Gürültü tabanını ve filtre katsayısını belirler.
 */
void Injector_Perform_AutoCalib(uint16_t *adc_raw_data, uint32_t length) {
    float32_t sum = 0, mean, var = 0;
    float32_t float_data[length];

    // ADC -> Volt dönüşümü ve ortalama
    for(uint32_t i=0; i<length; i++) {
        float_data[i] = (float32_t)adc_raw_data[i] * 3.3f / 4095.0f;
        sum += float_data[i];
    }
    mean = sum / (float32_t)length;

    // Varyans hesabı (Gürültü tabanı)
    for(uint32_t i=0; i<length; i++) {
        var += (float_data[i] - mean) * (float_data[i] - mean);
    }
    g_calib.noise_floor = sqrtf(var / (float32_t)length);

    // Adaptif Alpha: Gürültü arttıkça filtreyi sertleştir (Alpha küçülür)
    if (g_calib.noise_floor > 0.1f) g_calib.alpha = 0.15f;
    else if (g_calib.noise_floor < 0.02f) g_calib.alpha = 0.45f;
    else g_calib.alpha = 0.30f;

    g_calib.calib_temp = Get_System_Temperature();
    g_calib.is_calibrated = 1;

    Injector_Save_Config();
}

/**
 * @brief Ham sinyalden 7 parametreli SVC özellikleri çıkarır.
 */
void Injector_Process_Signal(uint16_t *adc_samples, uint32_t len, Injector_Metrics_t *metrics) {
    float32_t last_val = 0, current_val;
    float32_t max_di_dt = 0;
    float32_t peak_i = 0;
    float32_t derivative;
    uint32_t hump_index = 0;
    float32_t min_derivative = 999.0f;

    for (uint32_t i = 0; i < len; i++) {
        // 1. Filtreleme
        current_val = (float32_t)adc_samples[i] * 3.3f / 4095.0f;
        current_val = (g_calib.alpha * current_val) + (1.0f - g_calib.alpha) * last_val;

        // 2. Tepe Akımı
        if (current_val > peak_i) peak_i = current_val;

        // 3. Türev Analizi (di/dt)
        derivative = current_val - last_val;
        if (derivative > max_di_dt) max_di_dt = derivative;

        // 4. Pintle Hump Tespiti (Türevdeki yerel minimum)
        // Genellikle rampanın ortalarında (örneğin 1ms - 3ms arası) aranır
        if (i > len/10 && i < len/2) {
            if (derivative < min_derivative) {
                min_derivative = derivative;
                hump_index = i;
            }
        }
        last_val = current_val;
    }

    metrics->peak_current = peak_i;
    metrics->di_dt = max_di_dt;
    metrics->hump_time = (float32_t)hump_index; // Örnekleme periyodu ile çarpılmalı
    metrics->rms_noise = g_calib.noise_floor;
    metrics->temperature = Get_System_Temperature();
}

/**
 * @brief 7 Parametreli SVC Modelini Çalıştırır.
 */
Diag_Status_t Injector_Run_Inference(Injector_Metrics_t *metrics) {
    float32_t in_features[7];
    float32_t out_data[1];

    in_features[0] = metrics->peak_current;
    in_features[1] = metrics->di_dt;
    in_features[2] = metrics->hump_time;
    in_features[3] = metrics->rms_noise;
    in_features[4] = g_calib.alpha;
    in_features[5] = g_calib.noise_floor;
    in_features[6] = metrics->temperature;

    // X-CUBE-AI Çağrısı (Örnek Fonksiyon)
    // ai_injector_thermal_svc_run(network_handle, in_features, out_data);

    if (out_data[0] > 0.5f) return DIAG_FAULTY;
    return DIAG_HEALTHY;
}

/**
 * @brief Kalibrasyon verilerini Flash Sektör 11'e yazar.
 */
HAL_StatusTypeDef Injector_Save_Config(void) {
    Injector_Config_Storage_t storage;
    storage.magic = MAGIC_NUMBER;
    storage.alpha = g_calib.alpha;
    storage.noise_floor = g_calib.noise_floor;
    storage.calib_temp = g_calib.calib_temp;
    // storage.crc32 = HAL_CRC_Calculate(...); // Donanımsal CRC birimi kullanılabilir

    HAL_FLASH_Unlock();
    
    FLASH_EraseInitTypeDef erase;
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = FLASH_CONFIG_SECTOR;
    erase.NbSectors = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    uint32_t err;
    if (HAL_FLASHEx_Erase(&erase, &err) != HAL_OK) return HAL_ERROR;

    uint32_t *data = (uint32_t*)&storage;
    for (int i=0; i < sizeof(storage)/4; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_CONFIG_ADDR + (i*4), data[i]);
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}

HAL_StatusTypeDef Injector_Load_Config(void) {
    Injector_Config_Storage_t *storage = (Injector_Config_Storage_t*)FLASH_CONFIG_ADDR;
    if (storage->magic != MAGIC_NUMBER) return HAL_ERROR;

    g_calib.alpha = storage->alpha;
    g_calib.noise_floor = storage->noise_floor;
    g_calib.calib_temp = storage->calib_temp;
    g_calib.is_calibrated = 1;
    return HAL_OK;
}

float32_t Get_System_Temperature(void) {
    // STM32 Dahili Sıcaklık Sensörü Okuması (ADC)
    return 25.0f; // Placeholder
}
