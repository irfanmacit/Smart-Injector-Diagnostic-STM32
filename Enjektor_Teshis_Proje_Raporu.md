# Teknik Tasarım Raporu: Akıllı Enjektör Bobin Teşhis Sistemi

**Hazırlayan:** Gemini CLI (Gömülü Sistemler ve AI Uzmanı)  
**Platform:** STM32F4 + X-CUBE-AI  
**Metodoloji:** 7-Parametreli Termal Kompanzasyonlu SVC Analizi

---

## 1. Giriş ve Proje Amacı
Bu proje, enjektör solenoid bobinlerinin elektriksel ve mekanik sağlığını, STM32F4 mikrodenetleyicisi üzerinde koşan bir Yapay Zeka (Edge AI) motoru ile gerçek zamanlı olarak teşhis etmeyi amaçlar. Klasik eşik değeri yöntemlerinin aksine, sistem çevre koşullarına (sıcaklık, gürültü) adaptif olarak uyum sağlar.

---

## 2. Teorik Altyapı

### 2.1. Bobin Dinamiği ve R-L Devresi
Enjektör bobini bir indüktif yüktür. Akım yükselişi $I(t) = \frac{V}{R}(1 - e^{-\frac{Rt}{L}})$ formülüyle tanımlanır. Zaman sabiti ($\tau = L/R$), bobinin manyetik doyuma ulaşma hızını ve sargı sağlığını belirler.

### 2.2. "Pintle Hump" Fenomeni
Enjektör iğnesi (pintle) hareket ettiğinde, manyetik devredeki hava boşluğu değişir. Bu değişim, akım grafiğinde bir "çentik" (hump) oluşturur. Bu çentiğin zamanlaması ve derinliği, iğnenin mekanik durumunu (sıkışma, yay yorgunluğu vb.) gösterir.

### 2.3. Termal Drift (Isıl Kayma)
Bakırın direnç-sıcaklık katsayısı ($\alpha \approx 0.00393 / ^\circ\text{C}$) nedeniyle, sıcaklık arttıkça bobin direnci artar ve akım tepe değeri düşer. Sistem, bu doğal değişimi arızadan ayırt etmek için termal kompanzasyon kullanır.

---

## 3. Algoritma Tasarımı: 7-Parametreli SVC Modeli

Sistem, karar vermek için Support Vector Classification (SVC) algoritmasını kullanır. Giriş vektörü aşağıdaki 7 parametreden oluşur:

1.  **I_max:** Akım tepe değeri (Direnç analizi).
2.  **di/dt:** Akım yükselme eğimi (Endüktans analizi).
3.  **T_hump:** İğne hareket zamanı (Mekanik analiz).
4.  **RMS_noise:** Sinyal gürültü seviyesi (EMI analizi).
5.  **Alpha (α):** Adaptif filtre sertlik katsayısı (Kalibrasyon verisi 1).
6.  **Noise Floor:** Donanımsal gürültü tabanı (Kalibrasyon verisi 2).
7.  **Temperature:** Çalışma sıcaklığı (Termal kompanzasyon).

---

## 4. Yazılım Mimarisi ve Uygulama

### 4.1. Ön İşleme (Preprocessing)
Sinyal, türev tabanlı bir algoritma ile işlenir. Gürültü amfifikasyonunu önlemek için **Savitzky-Golay** benzeri bir dijital filtreleme ve adaptif eşikleme uygulanır.

### 4.2. Otomatik Kalibrasyon (Self-Calibration)
Sistem, her ölçüm öncesi gürültü tabanını analiz ederek filtre katsayılarını otomatik günceller. Bu veriler, STM32F4'ün Flash belleği (EEPROM emülasyonu) üzerinde saklanarak cihazın yaşlanma etkileri takip edilir.

### 4.3. X-CUBE-AI Entegrasyonu
Python (Scikit-learn) ortamında eğitilen SVC modeli, ONNX formatına dönüştürülür ve X-CUBE-AI aracılığıyla STM32F4 için optimize edilmiş C koduna (MACC optimizasyonu ve SIMD desteği) çevrilir.

---

## 5. Donanım ve Bellek Yönetimi
- **Flash Saklama:** Kalibrasyon verileri CRC32 kontrolü ile Flash Sektör 11'de saklanır.
- **Gerçek Zamanlılık:** STM32F4 FPU desteği ile AI çıkarım süresi <200µs olarak hedeflenmiştir.
- **Sensör Füzyonu:** ADC verileri, dahili sıcaklık sensörü ve besleme voltajı bilgileri tek bir analiz motorunda birleştirilir.

---

## 6. Sonuç
Bu ileri düzey mimari, enjektör test cihazının sadece bir ölçüm aleti değil, aynı zamanda fiziksel süreçleri anlayan ve çevre koşullarına uyum sağlayan akıllı bir teşhis cihazı olmasını sağlar.
