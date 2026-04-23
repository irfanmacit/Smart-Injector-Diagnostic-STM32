2/**
  ******************************************************************************
  * @file           : injector_diag.h
  * @brief          : Smart Injector Diagnostic Library Header
  * @author         : Gemini CLI (Academic & Embedded Expert)
  ******************************************************************************
  */

#ifndef __INJECTOR_DIAG_H
#define __INJECTOR_DIAG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "arm_math.h"

/* --- Konfigürasyon --- */
#define INJECTOR_FEATURE_COUNT    7
#define FLASH_CONFIG_SECTOR       FLASH_SECTOR_11
#define FLASH_CONFIG_ADDR         0x080E0000
#define MAGIC_NUMBER              0xDEADBEEF

/* --- Veri Yapıları --- */

/**
 * @brief Enjektör Sağlık Durumları
 */
typedef enum {
    DIAG_HEALTHY = 0,
    DIAG_FAULTY  = 1,
    DIAG_ERROR   = 2
} Diag_Status_t;

/**
 * @brief Otomatik Kalibrasyon ve Filtre Parametreleri
 */
typedef struct {
    float32_t alpha;            // Adaptif filtre katsayısı
    float32_t noise_floor;      // Gürültü varyansı
    float32_t calib_temp;       // Kalibrasyon anındaki sıcaklık
    uint32_t  is_calibrated;    // Kalibrasyon durumu
} Injector_Calib_t;

/**
 * @brief Sinyalden Çıkarılan Özellikler (Features)
 */
typedef struct {
    float32_t peak_current;     // I_max
    float32_t di_dt;            // Yükselme eğimi
    float32_t hump_time;        // İğne hareket zamanı
    float32_t rms_noise;        // Sinyal gürültüsü
    float32_t temperature;      // Çalışma sıcaklığı
} Injector_Metrics_t;

/**
 * @brief Flash Saklama Yapısı
 */
typedef struct {
    uint32_t magic;
    float32_t alpha;
    float32_t noise_floor;
    float32_t calib_temp;
    uint32_t crc32;
} Injector_Config_Storage_t;

/* --- Fonksiyon Prototipleri --- */

/* Başlatma ve Kalibrasyon */
HAL_StatusTypeDef Injector_Diag_Init(void);
void Injector_Perform_AutoCalib(uint16_t *adc_raw_data, uint32_t length);

/* Özellik Çıkarımı ve AI İşleme */
void Injector_Process_Signal(uint16_t *adc_samples, uint32_t len, Injector_Metrics_t *metrics);
Diag_Status_t Injector_Run_Inference(Injector_Metrics_t *metrics);

/* Bellek Yönetimi */
HAL_StatusTypeDef Injector_Save_Config(void);
HAL_StatusTypeDef Injector_Load_Config(void);

/* Yardımcı Fonksiyonlar */
float32_t Get_System_Temperature(void);

#ifdef __cplusplus
}
#endif

#endif /* __INJECTOR_DIAG_H */
