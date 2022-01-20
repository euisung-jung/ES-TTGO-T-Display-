# ES-TTGO-T-Display-
TTGO-T-Display  관련 코드 및 테스트

1. 보드매니저 URL 입력
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
2. 보드매니저에서 보드 설치
 esp32보드
3. 보드 드라이버 설치
4. 라이브러리 설치
 TFT_eSPI
5.TFT_eSPI 라이브러리 수정
 Documents\Arduino\libraries\TFT_eSPI폴더 User_Setup_Select.h파일
 22번 라인의 #include <User_Setup.h> 주석처리
 52번 라인의 #include <User_Setup/Setup25_TTGO_T_Display.h> 주석처리 제거

6. T-Display 기본 코드

7. 코드 저장후 bmp.h 파일 확인 없으면 생성
8. bmp.h 이미지 코드 입력
 
1310720바이트 1.25MB
327680바이트 0.3MB
