![author](https://img.shields.io/badge/Autor-Александр%20Дмитрук-brightgreen)

## Котроллер управления вытяжкой на базе Sonoff Basic R2 Switch Module (BASICR2)

<details>
<summary>Внешний вид (Фото BASICR2)</summary>

![scheme](https://templates.blakadder.com/assets/images/sonoff_BASICR2.jpg)

</details>

###  Описание выводом микроконтроллера ESP8266 : GPIO Sonoff Basic R2 Switch Module (BASICR2)
| GPIO 	| Name 		| Quantity				 |
| ----- | ------- | ---------------	 | 
| 	0 	| Button 	|	buttonState = !digitalRead(BTN_PIN)			// 0 нажата; 1 отпущена	|
| 	1 	| TX PIN 	|	SCL I2C при использовании датчика AHT10		|
| 	2		| IO2 		| не используется (свободный)								|
| 	3 	| RX PIN	|	SDA I2C при использовании датчика AHT10		|
| 	12	| RELAY		|	RELE_ON	=> digitalWrite( RELE_PIN, HIGH ) // включается 1 (3V3)	|
| 	13	| LED1		|	LED_ON => digitalWrite( LED_PIN, HIGH )		// включается 1 (3V3)	|

###  Используемые библиотеки 
* <DHT_sensor_library>	- для получения тепературы из датчика DHT11
* <GyverPortal>					- для создания вебсервера с интерефейсом управления и настроки модуля
* <Adafruit_AHTX0.h>		- для получения тепературы из датчика AHT10
* <ESP_EEPROM.h>				- для сохранения настроек в энергонезависимую память

##  Версии 
- v.0.1 - датчик только DHT11.  MODE_AP_DEFAULT - "Вентиляция",  MODE_AP_USER - собственная настройка SSID и PASS WI-FI,	MODE_STA - одключение к локальной сети
- v.0.2 - поддержка I2C датчика тепературы и влажности AHT10
- v.0.3 - добавлена время работы, полный сброс настроек, по умолчанию SSID:Вентиляция без пароля