![author](https://img.shields.io/badge/Autor-Александр%20Дмитрук-brightgreen)

## Котроллер управления вытяжкой на базе Sonoff Basic R2 Switch Module (BASICR2)
###### ESP8266 GPIO - Sonoff Basic R2 Switch Module (BASICR2)
| GPIO 	| Name 		| Quantity				 |
| ----- | ------- | ---------------	 | 
| 	0 	| Button 	|	buttonState = !digitalRead(BTN_PIN)			// 0 нажата; 1 отпущена	|
| 	1 	| TX PIN 	|	SCL I2C при использовании датчика AHT10		|
| 	2		| IO2 		| не используется (свободный)								|
| 	3 	| RX PIN	|	SDA I2C при использовании датчика AHT10		|
| 	12	| RELAY		|	RELE_ON	=> digitalWrite( RELE_PIN, HIGH ) // включается 1 (3V3)	|
| 	13	| LED1		|	LED_ON => digitalWrite( LED_PIN, HIGH )		// включается 1 (3V3)	|

## Используемые библиотеки 
1. _DHT_sensor_library_ - для получения тепературы из датчика DHT11
2. _GyverPortal-main_ - для создания вебсервера с интерефейсом управления и настроки модуля
3. _Adafruit_AHTX0.h_ - для получения тепературы из датчика AHT10
4. _ESP_EEPROM.h_ - для сохранения настроек в энергонезависимую память

<details>
<summary>Внешний вид модуля</summary>
  
![](https://community-assets.home-assistant.io/original/3X/a/7/a7e73ad500b293a8f1e94b5dc9e0ede56cddf1a8.jpeg)
  
</details>

##  WEB интерфейс управлния 

<details>
<summary>Авторизация</summary>
  
<p align="center">
  <img src="/jpg/Screenshot_Chrome_0.png" width="300px"/>
</p>
  
</details>

<details>
<summary>Главная страница (режим ручное управление, вентиляция выключена/включена)</summary>
  
<p align="center">
  <img src="/jpg/Screenshot_Chrome_1.png" width="300px"/>
  <img src="/jpg/Screenshot_Chrome_2.png" width="300px"/>
</p>
  
</details>

<details>
<summary>Страница Настройки - выбор режима работы вытяжки</summary>
  
<p align="center">
  <img src="/jpg/Screenshot_Chrome_3.png" width="300px"/>
  <img src="/jpg/Screenshot_Chrome_4.png" width="300px"/>
</p>

</details>

<details>
<summary>Страница настроек WI-FI и прочего - выбор режима работы WI-FI</summary>
  
<p align="center">
  <img src="/jpg/Screenshot_Chrome_5.png" width="300px"/>
  <img src="/jpg/Screenshot_Chrome_6.png" width="300px"/>
</p>
  
</details>

##  Версии 
- v.0.1 - датчик только DHT11.  MODE_AP_DEFAULT - "Вентиляция",  MODE_AP_USER - собственная настройка SSID и PASS WI-FI,	MODE_STA - одключение к локальной сети
- v.0.2 - поддержка I2C датчика тепературы и влажности AHT10
- v.0.3 - добавлена время работы, полный сброс настроек, по умолчанию SSID:Вентиляция без пароля
