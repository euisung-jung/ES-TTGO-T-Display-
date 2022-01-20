#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include <Button2.h>
#include "esp_adc_cal.h"
#include "bmp.h"

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

#define TFT_BL 4  // 디스플레이 백라이트 제어 핀
#define ADC_EN 14 // ADC_EN은 ADC 감지 활성화 포트입니다.
#define ADC_PIN 34
#define BUTTON_1 35
#define BUTTON_2 0

TFT_eSPI tft = TFT_eSPI(135, 240); // 커스텀 라이브러리 호출
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

char buff[512];
int vref = 1100;
int btnCick = false;

//! 오랜 시간 지연, 얕은 소비를 사용하여 전류 소비를 효과적으로 줄일 수 있습니다.
void espDelay(int ms) ///////////슬립모드
{
    esp_sleep_enable_timer_wakeup(ms * 1000); //슬립모드 깨우기 시간지정   초단위
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void showVoltage() ////////////전압 출력
{
    static uint64_t timeStamp = 0;
    if (millis() - timeStamp > 1000)
    {
        timeStamp = millis();
        uint16_t v = analogRead(ADC_PIN);
        float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
        String voltage = "Voltage :" + String(battery_voltage) + "V"; // 출력형식 지정
        Serial.println(voltage);
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString(voltage, tft.width() / 2, tft.height() / 2);
    }
}

void button_init() /////////////버튼 핸드러
{
    btn1.setLongClickHandler([](Button2 &b) {
        btnCick = false;
        int r = digitalRead(TFT_BL);
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.drawString("Press again to wake up", tft.width() / 2, tft.height() / 2);
        espDelay(6000);
        digitalWrite(TFT_BL, !r);

        tft.writecommand(TFT_DISPOFF); ///////////???
        tft.writecommand(TFT_SLPIN);   ///////////???
        // 라이트 절전을 사용한 후에는 외부 IO 포트를 사용하여 깨우기 때문에 타이머 깨우기를 비활성화해야합니다.
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER); ///////////???
        // esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW);    ///////////???
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, 0);
        delay(200);
        esp_deep_sleep_start();
    });
    btn1.setPressedHandler([](Button2 &b) {
        Serial.println("Detect Voltage..");
        btnCick = true;
    });

    btn2.setPressedHandler([](Button2 &b) {
        btnCick = false;
        Serial.println("btn press wifi scan");
        wifi_scan();
    });
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(3);
    btn1.setClickHandler([](Button2 &b) {
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Click", tft.width() / 2, tft.height() / 2);
        Serial.println("Click");

        btnCick = true;
    });
    btn1.setDoubleClickHandler([](Button2 &b) {
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Double", tft.width() / 2, tft.height() / 2);
        Serial.println("Double");
        btnCick = true;
    });
    btn1.setTripleClickHandler([](Button2 &b) {
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Triple", tft.width() / 2, tft.height() / 2);
        Serial.println("Triple");
        btnCick = true;
    });
    btn1.setLongClickHandler([](Button2 &b) {
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Long~~", tft.width() / 2, tft.height() / 2);
        Serial.println("Long~~");
        btnCick = true;
        espDelay(5);
    });
}

void button_loop()
{
    btn1.loop();
    btn2.loop();
}

void wifi_scan() ////////////wifi스캔
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
    if (n == 0)
    {
        tft.drawString("no networks found", tft.width() / 2, tft.height() / 2);
    }
    else
    {
        tft.setTextDatum(TL_DATUM);
        tft.setCursor(0, 0);
        Serial.printf("Found %d net\n", n);
        for (int i = 0; i < n; ++i)
        {
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

void setup()
{
    Serial.begin(115200);
    Serial.println("Start");

    /*
    ADC_EN은 ADC 감지 활성화 포트입니다.
    USB 포트가 전원 공급 장치에 사용되는 경우 기본적으로 켜져 있습니다.
    배터리로 전원을 공급받는 경우 높은 수준으로 설정해야합니다
    */
    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);

    tft.init(); //화면초기세팅
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    if (TFT_BL > 0)
    {                                           // TFT_BL은 사용자 설정 파일 TTGO_T_Display.h의 TFT_eSPI 라이브러리에서 설정되었습니다.
        pinMode(TFT_BL, OUTPUT);                // 백라이트 핀을 출력 모드로 설정
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // 백라이트를 켭니다. 사용자 설정 파일 TTGO_T_Display.h의 TFT_eSPI 라이브러리에서 TFT_BACKLIGHT_ON이 설정되었습니다.
    }

    // tft.setSwapBytes(true);      // 부팅이미지
    // tft.pushImage(0, 0,  240, 135, ttgo);
    // espDelay(5000);

    // tft.setRotation(0);           //화면테스트
    // tft.fillScreen(TFT_RED);
    // espDelay(1000);
    // tft.fillScreen(TFT_BLUE);
    // espDelay(1000);
    // tft.fillScreen(TFT_GREEN);
    // espDelay(1000);

    button_init();

    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC1_CHANNEL_6, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);
    // ADC 특성 분석에 사용되는 캘리브레이션 값 유형 확인
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
        vref = adc_chars.vref;
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
    }
    else
    {
        Serial.println("Default Vref: 1100mV");
    }

    // tft.fillScreen(TFT_BLACK);    //초기화면 출력
    // tft.setTextDatum(MC_DATUM);
    // tft.drawString("LeftButton:", tft.width() / 2, tft.height() / 2 - 16);
    // tft.drawString("[WiFi Scan]", tft.width() / 2, tft.height() / 2 );
    // tft.drawString("RightButton:", tft.width() / 2, tft.height() / 2 + 16);
    // tft.drawString("[Voltage Monitor]", tft.width() / 2, tft.height() / 2 + 32 );
    // tft.drawString("RightButtonLongPress:", tft.width() / 2, tft.height() / 2 + 48);
    // tft.drawString("[Deep Sleep]", tft.width() / 2, tft.height() / 2 + 64 );
    // tft.setTextDatum(TL_DATUM);
    tft.fillRect(0, 0, 240, 27, TFT_RED);
    tft.fillRect(2, 2, 40, 20, TFT_WHITE);
}

void loop()
{
    //     if (btnCick) {
    //          espDelay(1000);
    //      }
    button_loop();
}

/* ////////////////////////커스텀/////////////////////////////
1. 부팅이미지 커스텀   (L159)
    기존의 파엘에 이미지 코드를 추가 하거나 별도의 파일을 만들어 기존과 같이 코드에 추가 시킨다. 

2. 배터리 커스텀   (L45)
    배터리 배경이미지
    베터리 범위설정  3.4~4.2V 
    void battery_gauge() {
        int spw = 2;  //시작위치x좌표 변경
        int sph = 2;  //시작위치y좌표 변경
        int gw = 8;   //게이지 넓이 변경
        int gh = 12;  //게이지 높이 변경
        int p = 4;    //패딩 선두깨  변경
        int m = p*2; //변경x
        tft.fillRect( spw,                    sph,       (p+gw)*4+p, gh+m, TFT_WHITE);   //틀
        tft.fillRect( spw,                    sph+p,    (p+gw)*4+m,  gh, TFT_WHITE);   //틀 
        tft.fillRect( spw+p,                sph+p,    gw, gh, TFT_BLACK);      //게이지 1
        tft.fillRect( spw+(p+gw)+p,    sph+p,    gw, gh, TFT_BLACK);    //게이지 2
        tft.fillRect( spw+(p+gw)*2+p, sph+p,   gw, gh, TFT_BLACK);    //게이지 3
        tft.fillRect( spw+(p+gw)*3+p, sph+p,   gw, gh, TFT_BLACK);    //게이지 4
    }

3. wifi 이미지
    wifi 접속이미지 / 미접속이미지
    가로보기 기준으로  1/5사이즈(27px) 권장

4. 시계(인터넷에서 시간가져오기)
    getLocalTime(&timeInfo);   //tm개체 timeInfo에 현재 시각을 주입하는
    sprintf(h, "%02d:%02d",
            timeInfo.tm_hour, timeInfo.tm_min ); //출력 형식설정
    tft.fillRect(0, 0, 80, 16, 0x4312);     //이전시간 지우기 
    tft.drawString(h, 2 , 2 );              //시간출력
    tft.fillRect( 0,67, 240, 16, 0x4312);   //이전시간 지우기 
       
    sprintf(s, "%02d/%02d/%02d %02d:%02d:%02d",
           timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
           timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec); //출력 형식설정
    tft.drawString(s, tft.width() / 2 , tft.height() / 2  );
    Serial.println(s);

5. 백라이트  (L64) 

int r = digitalRead(TFT_BL); //현재백라이트 상태
digitalWrite(TFT_BL, !r); //백라이트 값 0 또는 1

6. 버튼컨트롤 
 btn1.setPressedHandler([](Button2 & b) {   //버튼변경(1/2)와 핸들러 변경
        btnCick = true;                     //btnCick 변수 참거짓 확인후 명령실행
    });
핸들러
setChangedHandler	
setPressedHandler	
setReleasedHandler
setClickHandler
setTapHandler
setLongClickHandler	
setDoubleClickHandler	
setTripleClickHandler	

if (btnCick) {                   //void loop()에서 체크
        showVoltage();
    }

7. wifi연결
char ssid[] = "wifi101-network";  // created AP name
char pass[] = "1234567890";
WiFiServer server(80);
//////////setup////////////
 if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("WiFi shield not present");
        while (true);
  }


움직이는 부팅이미지
for(int i=-240; i <= 0 ; ++i){  //왼쪽에서 오른쪽으로 등장 
  tft.pushImage(i, 0,  240, 135, ttgo); //이미지출력
  espDelay(41);             //속도24p 41 30p 33
}

for(int i=240; i >=0; --i){  //오른쪽에서 왼쪽으로 등장
 tft.pushImage(i, 0,  240, 135, ttgo);
 espDelay(41);
}

for(int i=-135; i <= 0 ; ++i){  //위에서 아래로 등장 
 tft.pushImage(0, i,  240, 135, ttgo);
 espDelay(41);
}

for(int i=135; i >=0; --i){  //아래에서 위로등장 대각선
 tft.pushImage(0, i,  240, 135, ttgo);
 espDelay(41);
}

한글올리기
https://blog.naver.com/PostView.nhn?blogId=dreve&logNo=221818135061


폰트사이즈
tft.setTextSize(1);
폰트컬러
tft.setTextColor(TFT_GREEN);
tft.setTextColor(TFT_WHITE, TFT_BLACK);
글자출력
tft.drawString("글자", tft.width()/2, tft.height()/2); 
배경화면색
tft.fillScreen(TFT_BLACK); 
커서위치
tft.setCursor(0, 0);
이미지출력
tft.pushImage(0, 0,  240, 135, ttgo);
폰트
tft.setFreeFont(MYFONT32);
tft.drawString("MyFont 32", 160, 60, GFXFF);
내모박스그리기(선)
tft.drawRect(0,0,319,239, TFT_BLUE);
네모그리기(면)
tft.fillRect( 0, 0, 16, 16, TFT_RED); 

기준점설정
tft.setTextDatum(TR_DATUM);

tft.setTextPadding(0);   줄바꿈 있고 없고?
tft.writecommand(TFT_DISPOFF); 
tft.writecommand(TFT_SLPIN);

딥슬립 
함수를 통해 팁슬립을 실행하고 어떻케 깨울것인지도 지정한다.
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);  //슬립모드 깨우기 시간지정   초단위
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON); //슬립시 설정
    esp_light_sleep_start(); //딥슬립스타트
}
esp_sleep_enable_timer_wakeup(ms * 1000); //시간으로 깨우기
esp_sleep_enabled_touchpad_Wakeup() //터치패드를 이용해 깨우기
esp_sleep_enable_ext0_wakeup()
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html


   battery_gauge();
   
   tft.setSwapBytes(true);      // 부팅이미지
   tft.pushImage(120, 0,  29, 20, myimg1);
   tft.pushImage(60, 0,  46, 20, myimg2);
   tft.pushImage(165, 0,  27, 20, myimg3);
   tft.pushImage(180, 0,  28, 20, myimg4);
   tft.pushImage(210,0,  60, 20, myimg5);



*/
