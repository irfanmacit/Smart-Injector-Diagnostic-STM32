import time
import framebuf
from machine import Pin, SPI, ADC
from ili9341 import Display

# ==========================================
# AYARLAR (Cheap Yellow Display - CYD için)
# ==========================================

# 1. LCD Ekran SPI ve Pin Numaraları Ayarları (Sizin belirttiğiniz standart ESP32 pinleri)
LCD_MOSI_PIN = 23
LCD_SCK_PIN  = 18
LCD_CS_PIN   = 17
LCD_DC_PIN   = 16
LCD_RST_PIN  = 5
LCD_BL_PIN   = 32

# 2. ADC (Sinyal Okuma) Ayarları
ADC_PIN = 34        # Analog sinyal okunacak pin

# 3. Zamanlama Ayarı
OKUMA_ARALIGI_SANIYE = 0.1  # Çizgi grafiğinin akıcı olması için biraz daha hızlı

# ==========================================
# RENKLER (RGB565 Formatında)
# ==========================================
RGB_BLACK  = 0x0000
RGB_WHITE  = 0xFFFF
RGB_RED    = 0xF800
RGB_GREEN  = 0x07E0
RGB_BLUE   = 0x001F
RGB_YELLOW = 0xFFE0
RGB_CYAN   = 0x07FF
RGB_MAGENTA= 0xF81F
RGB_BG     = 0x0000  # Arka plan rengi siyah
RGB_GRID   = 0x3186  # Koyu Gri (Grid çizgileri için)
RGB_LINE   = 0x07E0  # Grafik Çizgisi Rengi (Yeşil)

# Metin yazdırma fonksiyonu (Dahili framebuf kullanılarak metinler çizilir)
def draw_text(display, text, x, y, fg_color, bg_color, scale=1):
    w = len(text) * 8
    h = 8
    buf = bytearray((w * h) // 8 + 1)
    fb = framebuf.FrameBuffer(buf, w, h, framebuf.MONO_HLSB)
    fb.fill(0)
    fb.text(text, 0, 0, 1)
    
    for row in range(h):
        for col in range(w):
            pixel = fb.pixel(col, row)
            color = fg_color if pixel else bg_color
            
            x_pos = x + col*scale
            y_pos = y + row*scale
            
            if x_pos >= 320 or y_pos >= 240:
                continue

            if scale > 1:
                w_draw = min(scale, 320 - x_pos)
                h_draw = min(scale, 240 - y_pos)
                if w_draw > 0 and h_draw > 0:
                    display.fill_rectangle(x_pos, y_pos, w_draw, h_draw, color)
            else:
                display.draw_pixel(x_pos, y_pos, color)

def draw_full_grid(display, x_min, y_min, x_max, y_max, grid_size=20):
    # Arka planı temizle
    display.fill_rectangle(x_min, y_min, x_max - x_min, y_max - y_min, RGB_BG)
    
    # Dikey çizgiler
    for x in range(x_min, x_max, grid_size):
        display.draw_vline(x, y_min, y_max - y_min, RGB_GRID)
        
    # Yatay çizgiler
    for y in range(y_min, y_max, grid_size):
        display.draw_hline(x_min, y, x_max - x_min, RGB_GRID)

def main():
    spi = SPI(1, baudrate=26666666, sck=Pin(LCD_SCK_PIN), mosi=Pin(LCD_MOSI_PIN))
    display = Display(spi, cs=Pin(LCD_CS_PIN), dc=Pin(LCD_DC_PIN), rst=Pin(LCD_RST_PIN), 
                      width=320, height=240, rotation=90)
    
    backlight = Pin(LCD_BL_PIN, Pin.OUT)
    backlight.value(1)
    
    display.clear(RGB_BG)
    time.sleep(0.1)
    
    # Başlık
    draw_text(display, "ADC Line Graph", 10, 10, RGB_YELLOW, RGB_BG, scale=2)
    display.fill_rectangle(0, 35, 320, 2, RGB_WHITE)
    
    adc = ADC(Pin(ADC_PIN))
    adc.atten(ADC.ATTN_11DB)
    adc.width(ADC.WIDTH_12BIT)
    
    # Grafik Alanı Koordinatları
    grafik_x_baslangic = 10
    grafik_x_bitis = 310
    grafik_y_baslangic = 230  # Alt çizgi
    grafik_yukseklik = 140    # Orijinalde 160'tı, gridin daha iyi sığması için ayarladık
    grafik_y_tepe = grafik_y_baslangic - grafik_yukseklik
    
    step_x = 4 # Her adımda ilerlenecek piksel sayısı
    
    # İlk Grid çizimi
    draw_full_grid(display, grafik_x_baslangic, grafik_y_tepe, grafik_x_bitis, grafik_y_baslangic, grid_size=20)
    
    graph_x = grafik_x_baslangic
    prev_x = grafik_x_baslangic
    prev_y = grafik_y_baslangic
    
    while True:
        val = adc.read()
        
        # Değeri formatla ve yaz
        yazi = "Deger: {:04d}  ".format(val)
        draw_text(display, yazi, 10, 45, RGB_CYAN, RGB_BG, scale=2)
        
        # Yüksekliği sınırla ve oranla
        oranlanmis_deger = int((val / 4095.0) * grafik_yukseklik)
        if oranlanmis_deger > grafik_yukseklik:
            oranlanmis_deger = grafik_yukseklik
            
        current_y = grafik_y_baslangic - oranlanmis_deger
        
        # Önceki noktadan donanımsal çizgi çizme
        if graph_x > grafik_x_baslangic:
            # Grid çizgilerininin üstüne biner ve daha belirgin olur
            display.draw_line(prev_x, prev_y, graph_x, current_y, RGB_LINE)
            # Kalınlık katmak için bir piksel sağından da çizgi çizilebilir
            display.draw_line(prev_x+1, prev_y, graph_x+1, current_y, RGB_LINE)
        
        # Durumları güncelle
        prev_x = graph_x
        prev_y = current_y
        graph_x += step_x
        
        # Ekranın sonuna gelince baştan başla ve grid'i yenile
        if graph_x >= grafik_x_bitis:
            graph_x = grafik_x_baslangic
            draw_full_grid(display, grafik_x_baslangic, grafik_y_tepe, grafik_x_bitis, grafik_y_baslangic, grid_size=20)
            
        time.sleep(OKUMA_ARALIGI_SANIYE)

if __name__ == '__main__':
    main()
