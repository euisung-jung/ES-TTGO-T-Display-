
#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include <Button2.h> //버튼 외부라이브러리
#include "esp_adc_cal.h"
#include "bmp.h" //이미지 코드 저장파일

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN 0x10
#endif

#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23

#define TFT_BL 4 // Display backlight control pin(디스플레이 백라이트 제어 핀)
#define ADC_EN 14
#define ADC_PIN 34
#define BUTTON_1 35  //버큰핀 번호
#define BUTTON_2 0   //버큰핀 번호

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library(커스텀 라이브러리 호출)
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

char buff[512];
int vref = 1100;
int btnCick = false;

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
//(긴 시간 지연, 전류 소비를 효과적으로 줄일 수 있는 얕은 수면을 사용하는 것이 좋습니다.)
void espDelay(int ms)            ////////////////////////////////////////// 절전모드
{
esp_sleep_enable_timer_wakeup(ms * 1000);
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,ESP_PD_OPTION_ON);
esp_light_sleep_start();
}

void showVoltage()             ////////////////////////////////////////// 전압 표시
{
static uint64_t timeStamp = 0;
if (millis() - timeStamp > 1000) {
timeStamp = millis();
uint16_t v = analogRead(ADC_PIN);
float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
String voltage = "Voltage :" + String(battery_voltage) + "V";
Serial.println(voltage);
tft.fillScreen(TFT_BLACK);
tft.setTextDatum(MC_DATUM);
tft.drawString(voltage, tft.width() / 2, tft.height() / 2 );
}
}

void button_init()            //////////////////////////////////////////
{
btn1.setLongClickHandler([](Button2 & b) {
btnCick = false;
int r = digitalRead(TFT_BL);
tft.fillScreen(TFT_BLACK);
tft.setTextColor(TFT_GREEN, TFT_BLACK);
tft.setTextDatum(MC_DATUM);
tft.drawString("Press again to wake up", tft.width() / 2, tft.height() / 2 );
espDelay(6000);
digitalWrite(TFT_BL, !r);

tft.writecommand(TFT_DISPOFF);
tft.writecommand(TFT_SLPIN);
esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);
esp_deep_sleep_start();
});
btn1.setPressedHandler([](Button2 & b) {
Serial.println("Detect Voltage..");
btnCick = true;
});

btn2.setPressedHandler([](Button2 & b) {
btnCick = false;
Serial.println("btn press wifi scan");
wifi_scan();
});
}

void button_loop()            ////////////////////////////////////////// 버튼 루프
{
btn1.loop();
btn2.loop();
}

void wifi_scan()            ////////////////////////////////////////// WIFI 스캔
{
tft.setTextColor(TFT_GREEN, TFT_BLACK);      
tft.fillScreen(TFT_BLACK);
tft.setTextDatum(MC_DATUM);
tft.setTextSize(1);

tft.drawString("Scan Network", tft.width() / 2, tft.height() / 2);

WiFi.mode(WIFI_STA);
WiFi.disconnect();
delay(100);

int16_t n = WiFi.scanNetworks();
tft.fillScreen(TFT_BLACK);
if (n == 0) {
tft.drawString("no networks found", tft.width() / 2, tft.height() / 2);
} else {
tft.setTextDatum(TL_DATUM);
tft.setCursor(0, 0);
Serial.printf("Found %d net\n", n);
for (int i = 0; i < n; ++i) {
sprintf(buff,
"[%d]:%s(%d)",
i + 1,
WiFi.SSID(i).c_str(),
WiFi.RSSI(i));
tft.println(buff);
}
}
WiFi.mode(WIFI_OFF);
}

void setup()            //////////////////////////////////////////  SETUP
{
Serial.begin(115200);
Serial.println("Start");
tft.init();
tft.setRotation(1);
tft.fillScreen(TFT_BLACK);
tft.setTextSize(2);
tft.setTextColor(TFT_WHITE);
tft.setCursor(0, 0);
tft.setTextDatum(MC_DATUM);
tft.setTextSize(1);

if (TFT_BL > 0) { // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h(TFT_BL은 사용자 설정 파일 TTGO_T_Display.h의 TFT_eSPI 라이브러리에 설정되었습니다.)
pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode(// 백라이트 핀을 출력 모드로 설정)
digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h(// 백라이트를 켭니다. TFT_BACKLIGHT_ON은 사용자 설정 파일 TTGO_T_Display.h의 TFT_eSPI 라이브러리에 설정되었습니다.)
}

tft.setSwapBytes(true);
tft.pushImage(0, 0, 240, 135, ttgo);   ///로고출력

espDelay(5000);

tft.setRotation(0);
int i = 5;
while (i--) {                       /// 색상루프
tft.fillScreen(TFT_RED);
espDelay(1000);
tft.fillScreen(TFT_BLUE);
espDelay(1000);
tft.fillScreen(TFT_GREEN);
espDelay(1000);
}

button_init();

esp_adc_cal_characteristics_t adc_chars;
esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC1_CHANNEL_6, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);
//Check type of calibration value used to characterize ADC(ADC 특성화에 사용되는 교정 값 유형 확인)
if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
vref = adc_chars.vref;
} else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
} else {
Serial.println("Default Vref: 1100mV");
}
}

void loop()            ////////////////////////////////////////// LOOP
{
if (btnCick) {
showVoltage();   /// -> 46line
}
button_loop();   // -> 90line
}
