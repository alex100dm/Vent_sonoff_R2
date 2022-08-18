// #define debug_print Serial    
#if defined(debug_print)
	#define my_print(...)  debug_print.print(__VA_ARGS__)
	#define my_println(...) debug_print.println(__VA_ARGS__)
#else
  #define my_print(...)
  #define my_println(...)
#endif

// GPIO0 = Button
// GPIO1 = TX PIN
// GPIO2 = IO2
// GPIO3 = RX PIN
// GPIO12 = RELAY
// GPIO13 = LED1
// in DOUT mode

#define VERS 					"0.3"		//  номер версии на странице настроек

#define TYPE_DT_AHT10  
//  либо-либо   #define TYPE_DT_DHT 

#define RELE_ON				1  //  
#define LED_ON				1  // 

#define BTN_PIN				0
#define LED_PIN				13
#define RELE_PIN			12	

#if defined(TYPE_DT_DHT)
	#define COREKCIA_T 		0.15  // ГРАД ( readTemp - COREKCIA_T )
	#define COREKCIA_H 		10	 // ПРОЦ ( readHum - COREKCIA_H )		
	#define DHTPIN				3		// RX  
	#include "DHT.h"  
	DHT dht(DHTPIN, DHT11);// DHT11, DHT22  (AM2302), AM2321, DHT21 (AM2301)// Раскомментируйте любой тип, который вы используете!
#elif defined(TYPE_DT_AHT10)
	#define SCL  1 //  TX       default: 	5-D1       SCL 5 // D1
	#define SDA  3 //  RX       				  4-D2			 SDA 4 // D2
	#include <Wire.h>
	#include <Adafruit_AHTX0.h>
	TwoWire Wire2;
	Adafruit_AHTX0 aht;	 //  I2C deviceaddress 0x38 
	
	#if defined(debug_print)
		#warning ДАТЧИК AHT10 ИСПОЛЬЗУЕ ПИНЫ ОТ Serial
	#endif
#else
	#warning НЕ ВЫБРАН ТИП ДАТИКА ТЕПЕРАТУРЫ TYPE_DT 
#endif
// #include "Ticker.h" // Ticker tc;

// #define GP_NO_MDNS      // убрать поддержку mDNS из библиотеки (вход по хосту в браузере)
// #define GP_NO_DNS       // убрать поддержку DNS из библиотеки (для режима работы как точка доступа)
// #define GP_NO_OTA       // убрать поддержку OTA обновления прошивки

const char SSID_SOFT_AP_DEFAULT[] = "Вентиляция";
#define LOGIN_WEB_STA_PORTAL "admin"      // АВТОРИЗАЦИЯ В ПОРТАЛЕ ПРИ РЕЖИМЕ WIFI_STA
#define PASS_WEB_STA_PORTAL "12345678"    //
#include <GyverPortal.h>
#include <ESP_EEPROM.h>// #include <EEPROM.h>

uint32_t timer_get_temp;
uint32_t timer_led_idic;
uint32_t timer_timeOff;
uint32_t sec;

float h; 
float t; 

volatile bool OnSwitch = true;  //bool rele_status = false;
volatile bool     g_buttonPressed = false;
volatile uint32_t g_buttonPressTime = -1;

bool eeprom_save = false;
uint8_t err_connect = 0; //   

enum enumconf_t : uint8_t {  MODE_AP_DEFAULT,  MODE_AP_USER,	MODE_STA, };  //MODE_STA_MQTT,
enumconf_t regim_connect_now;

struct MyEEPROMStruct {
	char ssid[32] = "";
  char pass[32] = "";
	char ssid_softAP[32] = "";
  char pass_softAP[32] = "";
	uint8_t regim_vent;
  uint8_t regim_connect;
  bool rele_status;
	uint8_t h_contr;
	uint8_t t_contr;
	uint8_t t_gist;
	uint8_t h_gist;
	int timeOff;
	uint8_t magic;  // 0xAD
} __attribute__((packed));

MyEEPROMStruct var;
GyverPortal portal;

void Ventilyator() {
	switch ( var.regim_vent ) { //
		case 0:   // ручное управление
			if ( OnSwitch != var.rele_status ) {
				my_print(F("Ручной - реле "));
				my_println( OnSwitch ? F("on") : F("off") );
				var.rele_status = OnSwitch;
			}
			if ( var.rele_status )	timer_timeOff = millis();
			break;
		case 1:   // выключение по таймеру
			if ( !var.rele_status && OnSwitch ) {
				timer_timeOff = millis();
				var.rele_status = 1;
				my_println(F("реж таймер. наж кнопкию реле ON"));
			}
			if ( millis() - timer_timeOff > (uint32_t)var.timeOff * 1000 ) {
				var.rele_status = 0;
				my_println(F("таймер! реле OFF"));
			}
			OnSwitch = var.rele_status;
			break;
		case 2:   // контроль влажности
			if ( h > var.h_contr + var.h_gist ) {
				var.rele_status = 1; // digitalWrite( RELE_PIN, LOW );   // on
				timer_timeOff = millis();
				my_println(F("контроль влажности. реле ON"));
			} else if ( h < var.h_contr - var.h_gist ) {   
				if ( millis() - timer_timeOff > (uint32_t)var.timeOff * 1000 ) {// задержка выключения  вышла
					var.rele_status = 0;//  digitalWrite( RELE_PIN, HIGH );   // off
					my_println(F("контроль влажности. реле OFF"));
				}
			} else {
				if ( var.rele_status ) timer_timeOff = millis();
			}
			OnSwitch = 1;
			break;
		case 3:   // контроль температуры
			if ( t > var.t_contr + var.t_gist ) {
				var.rele_status = 1; // digitalWrite( RELE_PIN, LOW );   // on
				timer_timeOff = millis();
			} else if ( t < var.t_contr - var.t_gist ) {   // задержка выключения  вышла
				if ( millis() - timer_timeOff > (uint32_t)var.timeOff * 1000 ) var.rele_status = 0;
			} else {
				if ( var.rele_status ) timer_timeOff = millis();
			}
			OnSwitch = 1;
			break;
		case 4:   // темп,влажн. 
			if ( t > var.t_contr + var.t_gist  ||  // температура выше заданной
					 h > var.h_contr + var.h_gist ) {  // влажность выше заданной
				var.rele_status = 1; 
				timer_timeOff = millis();
			} else if ( t < var.t_contr - var.t_gist &&
									h < var.h_contr - var.h_gist ) { 
				if ( millis() - timer_timeOff > (uint32_t)var.timeOff * 1000 ) var.rele_status = 0;// задержка выключения  вышла
			} else {
				if ( var.rele_status ) timer_timeOff = millis();
			}
			OnSwitch = 1;
			break;
	}
	digitalWrite( RELE_PIN, RELE_ON ? var.rele_status : !var.rele_status ); // !digitalRead(IN_230_PIN);
}

void Get_temp() {

#if defined(TYPE_DT_DHT)
	float temp = dht.readHumidity();
	if ( isnan(temp) ) {
		my_println(F("Ошибка DHT")); //  
		return;
	} else {
		h = temp - COREKCIA_H;
	}
	temp = dht.readTemperature();
	if ( isnan(temp) ) {
		my_println(F("Ошибка DHT")); // 	
		return;
	} else {
		t = temp - COREKCIA_T;
		// hic = dht.computeHeatIndex(t, h, false);// Вычислить тепловой индекс в градусах Цельсия(isFahreheit = false)
	}
	my_print(F("H: "));
  my_print(h);
  my_print(F("%  T: "));
  my_print(t);
  my_println(F("°C "));
#elif defined(TYPE_DT_AHT10)
	sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);// заполняйте объекты температуры и влажности свежими данными
	if ( aht.getStatus() == 0xFF ) {
		my_println(F("Ошибка AHT10")); // 
		return;
	}
	t = temp.temperature;
	h = humidity.relative_humidity;

	my_print(F("H: "));
  my_print(h);
  my_print(F("%  T: "));
  my_print(t);
  my_println(F("°C "));
#endif
}

void LedIdicator(){
	static uint8_t count;
	if ( millis() - timer_led_idic >= 100 ) {
		timer_led_idic = millis();
		if ( ++count > 40 ) {
			count = 0;
			digitalWrite( LED_PIN, LED_ON ); // on
		}
		if ( var.regim_vent * 2 > count ) 
			digitalWrite( LED_PIN,  !digitalRead( LED_PIN ) ); // 		
		else 
			digitalWrite( LED_PIN, LED_ON ? 1 : 0  ); // off
	}
}	

IRAM_ATTR // ICACHE_RAM_ATTR
void button_change(void) {
	bool buttonState = !digitalRead(BTN_PIN);
  if (buttonState && !g_buttonPressed) {
    g_buttonPressTime = millis();
    g_buttonPressed = true;
  } else if (!buttonState && g_buttonPressed) {
    g_buttonPressed = false;
    uint32_t buttonHoldTime = millis() - g_buttonPressTime;
    if ( millis() - g_buttonPressTime >= 10000) {
      restart_to_default(); // regim_connect_now = MODE_AP_DEFAULT; err_connect = 1;
			my_println(F("button press 10s regim = AP default"));
    } else if (buttonHoldTime >= 3000) {
      var.regim_vent = 0; // ручное управление
			my_print(F("button press 3s ")); my_println(F("ручное управление"));
    } else if (buttonHoldTime >= 30) {
      if ( !var.regim_vent ) OnSwitch = !OnSwitch;
			else OnSwitch = true;
			my_print(F("button clik sw=")); my_println(OnSwitch);
    }
    g_buttonPressTime = -1;
  }
}

void ConnectWifi() {
	WiFi.softAPdisconnect(true);  delay(100);      // отключаем AP
	WiFi.mode(WIFI_STA);	
	WiFi.begin( var.ssid , var.pass );
	my_print(F("WIFI_STA connect"));
	for (uint8_t i=0; i <= 250; i++) {
		if (WiFi.status() == WL_CONNECTED) break;
		my_print( F(".") ); delay(100); 
		digitalWrite( LED_PIN,  !digitalRead( LED_PIN ) );
		user_app();	
	}
	if (WiFi.status() == WL_CONNECTED) {
		my_print(F(" Conected! enableAuth: ")); my_print( LOGIN_WEB_STA_PORTAL );my_print(F(" "));my_println( PASS_WEB_STA_PORTAL );
		portal.enableAuth( LOGIN_WEB_STA_PORTAL , PASS_WEB_STA_PORTAL );   // char* login, char* pass включить авторизацию по логину-паролю
		portal.start(F("vent"));  //  http://vent.local/
		my_print(F("vent.local "));
		my_println( WiFi.localIP() );
		err_connect = 0;
	} else {
		if ( err_connect < 250 ) err_connect++;
		my_print( F("Faled conected. err_connect="));
		my_println( err_connect );
	}
}

void Start_Wifi_AP() {
	WiFi.disconnect(); delay(100);
	WiFi.mode(WIFI_AP);
	if ( regim_connect_now != MODE_AP_USER ) WiFi.softAP( SSID_SOFT_AP_DEFAULT );  // 192.168.4.1
	else WiFi.softAP( var.ssid_softAP , var.pass_softAP );  // 192.168.4.1
	portal.disableAuth(); // отключить авторизацию
	portal.start(WIFI_AP);
	
	my_print(F("Start WIFI_AP "));	
	if ( regim_connect_now != MODE_AP_USER ) { my_print( SSID_SOFT_AP_DEFAULT ); err_connect = 1;
	} else { my_print( var.ssid_softAP );	my_print(F(" "));	my_println( var.pass_softAP );  }
}

void ChekConnect() {
	if ( regim_connect_now != var.regim_connect ) {
		var.regim_connect = (uint8_t)regim_connect_now;
		switch ( regim_connect_now ) { //
			case MODE_AP_DEFAULT :   
			case MODE_AP_USER :   	Start_Wifi_AP();			break;
			case MODE_STA :   		  ConnectWifi();				break;
			default	:								Start_Wifi_AP();			break;
		}
		if ( !err_connect ) eeprom_save = 1;
	}
	if ( regim_connect_now == MODE_STA && err_connect >= 250 ) restart_modul(); 
}

void restart_modul() {
  my_print(F("restart"));
	ESP.restart();
  delay(10000);
  ESP.reset();
  while(1) {};
}

void 	restart_to_default() {
	memset( var.ssid, 0, 30);
	memset( var.pass, 0, 30);
	memset( var.ssid_softAP, 0, 30);
	memset( var.pass_softAP, 0, 30);
	
	var.regim_vent = 0;
	var.regim_connect	= 0;
	var.rele_status 	= 0;
	var.h_contr = 60;
	var.t_contr = 40;
	var.t_gist  = 1;
	var.h_gist  = 5;
	var.timeOff = 10;
	var.magic = 0x00;  

	EEPROM.put(0, var);	
	EEPROM.commit();	
	
	restart_modul();
}

#define SEL_AP_MODE PSTR("Точка доступа (Вентиляция),Точка доступа АР(user),домашний WIFI STA") // 0-2        ,WIFI STA MQTT
#define SELECT_REGIM PSTR("ручное управление,таймер при включении,таймер и влажность,таймер и температура,таймер температура и влажность") // 0-4

void build() {// билдер страницы
  BUILD_BEGIN();
	GP.THEME(GP_DARK); //  //GP.THEME(GP_LIGHT);
	if (portal.uri() == "/seting") {
		GP.TITLE( "Настройки" );
		GP.HR();
		GP.AJAX_UPDATE(PSTR("t,h,timer,hc,tc,gh,gt") );
		GP.LABEL("Сейчас :");		GP.LABEL("NAN", "t");		GP.LABEL("°C  ");  
		GP.LABEL("NAN", "h");   GP.LABEL("%");   GP.BREAK(); GP.BREAK(); 
		GP.LABEL("Режим работы вытяжки: "); GP.BREAK();
		GP.SELECT("sel", SELECT_REGIM, var.regim_vent);	GP.BREAK(); GP.BREAK();
		GP.LABEL("Таймер выключения:"); GP.LABEL("", "timer"); GP.LABEL("сек:"); GP.BREAK();
		GP.NUMBER("timeOff", "введите от 1 до 1800" );   GP.BREAK();  // GP.SLIDER("timeOff", var.timeOff, 10, 600, 10); // 30m=1800		
		GP.LABEL("Установить влажность:");	GP.LABEL("", "hc"); GP.LABEL("%");		GP.BREAK();
		GP.NUMBER("h_contr", "введите от 1 до 99" ); GP.BREAK(); // GP.SLIDER("h_contr", var.h_contr, 1, 99 ); GP.BREAK();
		GP.LABEL("Гистирезис влажности:");	GP.LABEL("", "gh"); GP.LABEL("%");	GP.BREAK();
		GP.NUMBER("gist_h", "введите от 1 до 20" );  GP.BREAK();
		GP.LABEL("Установить температуру:");	GP.LABEL("", "tc"); GP.LABEL("°");	GP.BREAK();
		GP.NUMBER("t_contr", "введите от 0 до 40" );  GP.BREAK();// GP.SLIDER("t_contr", var.t_contr, 0, 40 ); GP.BREAK();
		GP.LABEL("Гистирезис температуры:");	GP.LABEL("", "gt"); GP.LABEL("°");	GP.BREAK();
		GP.NUMBER("gist_t", "введите от 1 до 10" );  GP.BREAK();
		
		GP.BUTTON_MINI("btn", "Сохранить в EEPROM"); GP.BREAK();GP.BREAK();
		GP.HR();
		GP.BUTTON_LINK("/connect", "Настроки WIFI и прочее"); GP.BREAK();
		GP.BUTTON_LINK("/", "На главную страницу");// GP.BUTTON_MINI("btn", "Сохранить");
	} else 
	if (portal.uri() == "/connect") { // Подключения
		GP.TITLE( "Подключения и прочее"  );
		GP.HR();
		
		GP.FORM_BEGIN("/save_wifi");
			GP.LABEL("Выбор режима WiFi:"); GP.BREAK();
			GP.SELECT("selwifi", SEL_AP_MODE , var.regim_connect); GP.BREAK(); GP.BREAK();
			GP.LABEL("Настройка WIFI STA: "); GP.BREAK();
			GP.LABEL("пароль на вход:"); GP.LABEL( LOGIN_WEB_STA_PORTAL ); GP.LABEL( PASS_WEB_STA_PORTAL ); GP.BREAK();	
			GP.TEXT("ssid", "SSID", var.ssid);			GP.BREAK();
			GP.TEXT("pass", "PASS", var.pass);      GP.BREAK();     GP.BREAK();	
			
			GP.LABEL("Настройка WiFi АР(user):"); GP.BREAK();
			GP.TEXT("ssid_softAP", "SSID", var.ssid_softAP);      GP.BREAK();
			GP.TEXT("pass_softAP", "PASS", var.pass_softAP);      GP.BREAK();     GP.BREAK();
			
			GP.LABEL("WiFi АР(default):"); // 	GP.BLOCK_BEGIN();     GP.BLOCK_END(); 
			GP.LABEL( SSID_SOFT_AP_DEFAULT );  GP.BREAK();  GP.BREAK(); 
			GP.SUBMIT("Применить");							  GP.BREAK();
		GP.FORM_END();

		GP.HR();
		GP.BUTTON_LINK("/ota_update", "Обновить прошивку"); GP.BREAK();// GP.BUTTON_MINI("btn", "Сохранить");
		GP.BUTTON_LINK("/", "На главную страницу"); GP.BREAK();
		GP.BUTTON_LINK("/seting", "Назад");  GP.BREAK();
		
		GP.HR();
		GP.BREAK();GP.BREAK();
		GP.LABEL("Кнопка на модуле:");							  GP.BREAK();
		GP.LABEL("В ручном режиме - ON/OFF реле"); GP.BREAK();
		GP.LABEL("3 сек - переключить в ручной"); GP.BREAK();
		GP.LABEL("10 сек - запуск АР");  GP.LABEL( SSID_SOFT_AP_DEFAULT ); GP.BREAK(); GP.BREAK();
		GP.BUTTON_MINI("default", "Полный сброс"); 
		GP.BUTTON_MINI("restart", "Перезагузка"); GP.BREAK(); GP.BREAK();
	} 
	else {  // Главная страница
		GP.TITLE( "Умная вентиляция" );
		GP.HR();
		GP.AJAX_UPDATE(PSTR("led_motor,t,h,sw,timer,hc,tc,gh,gt"), 1000 );
		GP.LABEL("Сейчас :");	  GP.BREAK();
		GP.LABEL("Температура: ");		GP.LABEL("NAN", "t");		GP.LABEL("°C");  GP.BREAK();
		GP.LABEL("Влажность: ");			GP.LABEL("NAN", "h");   GP.LABEL("%");   GP.BREAK(); GP.BREAK(); 
		GP.LED_GREEN("led_motor");  GP.BREAK();  GP.BREAK();  //		GP.BREAK();GP.LED_GREEN("led_svet"); GP.LED_RED("led_motor");
		GP.LABEL("Режим работы вытяжки: "); GP.BREAK();
		switch ( var.regim_vent ) { //
			case 0:   // ручное управление,
				GP.BLOCK_BEGIN();
				GP.LABEL("РУЧНОЕ УПРАВЛЕНИЕ"); GP.BREAK();
				GP.BLOCK_END();
				GP.BREAK();		GP.SWITCH("sw", OnSwitch); 			GP.BREAK();
				break;
			case 1:   // таймер при включении,
				GP.BLOCK_BEGIN();
				GP.LABEL("ТАЙМЕР ПРИ ВКЛЮЧЕНИИ ПИТАНИЯ ИЛИ НАЖАТИЯ КНОПКИ"); GP.BREAK();
				GP.BLOCK_END();
				GP.LABEL("текущие настройки:"); GP.BREAK();
				GP.LABEL("Таймер выключения:"); 				GP.LABEL("`", "timer"); 				GP.LABEL("сек");				GP.BREAK();
				break;
			case 2:   // таймер и влажность,
				GP.BLOCK_BEGIN();
				GP.LABEL("ТАЙМЕР И КОНТРОЛЬ ВЛАЖНОСТИ"); GP.BREAK();
				GP.BLOCK_END();
				GP.LABEL("текущие настройки:"); GP.BREAK();
				GP.LABEL("Таймер выключения:"); 	GP.LABEL("`", "timer"); GP.LABEL("сек");		GP.BREAK();
				GP.LABEL("Влажность не более:");	GP.LABEL("", "hc");    GP.LABEL("±");		GP.LABEL("`", "gh");	GP.LABEL("%");		GP.BREAK();
				break;
			case 3:   // таймер и температура,
				GP.BLOCK_BEGIN();
				GP.LABEL("ТАЙМЕР И КОНТРОЛЬ ТЕМПЕРАТУРЫ"); GP.BREAK();
				GP.BLOCK_END();
				GP.LABEL("текущие настройки:"); GP.BREAK();
				GP.LABEL("Таймер выключения:"); 		GP.LABEL("`", "timer"); GP.LABEL("сек");		GP.BREAK();
				GP.LABEL("Температура не более:");	GP.LABEL("`", "tc");    GP.LABEL("±");		GP.LABEL("`", "gt");		GP.LABEL("°C");  	  GP.BREAK();
				break;
			case 4:   // таймер температура и влажность
				GP.BLOCK_BEGIN();
				GP.LABEL("ТАЙМЕР, КОНТРОЛЬ ТЕМПЕРАТУРЫ И ВЛАЖНОСТИ"); GP.BREAK();
				GP.BLOCK_END();
				GP.LABEL("текущие настройки:"); 		GP.BREAK();
				GP.LABEL("Таймер выключения:"); 		GP.LABEL("`", "timer"); 	GP.LABEL("сек");		GP.BREAK();
				GP.LABEL("Температура:");	GP.LABEL("`", "tc");  GP.LABEL("±");		GP.LABEL("`", "gt");		GP.LABEL("°"); 		GP.BREAK();
				GP.LABEL("  Влажность:");		GP.LABEL("`", "hc");  GP.LABEL("±");		GP.LABEL("`", "gh");    GP.LABEL("%");			GP.BREAK();
				break;
		}
		GP.HR();
		GP.BREAK();
		GP.BUTTON_LINK("/seting", "Настройки");  GP.BREAK();  GP.BREAK();  GP.BREAK();
		GP.LABEL("Module Sonoff Basic R2   +");	
#if defined(TYPE_DT_DHT)
		GP.LABEL("DHT11"); GP.BREAK();
#elif defined(TYPE_DT_AHT10)
		GP.LABEL("AHT10"); GP.BREAK();
#endif
		GP.LABEL("Прошивка v."); GP.LABEL(VERS); GP.LABEL("("); 
		sec = millis() / 1000ul;      // полное количество секунд
		GP.LABEL(sec/3600ul);GP.LABEL("час ");GP.LABEL((sec % 3600ul) / 60ul);GP.LABEL("мин   )"); GP.BREAK(); 		GP.BREAK(); 
	}
  BUILD_END();
}

void action_portal() {
  if (portal.click()) { // был клик по компоненту
    if (portal.click("sw")) 			OnSwitch = portal.getCheck("sw");			
		if (portal.click("sel")) 			var.regim_vent = portal.getSelected("sel", SELECT_REGIM );
		if (portal.click("h_contr")) 	var.h_contr = constrain( portal.getInt("h_contr") , 1, 99);
		if (portal.click("t_contr")) 	var.t_contr = constrain( portal.getInt("t_contr") , 0, 40);
		if (portal.click("gist_h"))  var.h_gist = constrain( portal.getInt("gist_h") , 1, 20);
		if (portal.click("gist_t"))  var.t_gist = constrain( portal.getInt("gist_t") , 1, 10);
		if (portal.click("timeOff")) 	var.timeOff = constrain( portal.getInt("timeOff"), 1, 1800);
		if (portal.click("btn")) 			eeprom_save = 1;
		if (portal.click("restart")) 	restart_modul();
		if (portal.click("default")) 	restart_to_default();
  }
	
	if (portal.update()) {      // было обновление
    if (portal.update("t")) portal.answer( t );// ищем, какой компонент запрашивает обновление 
    if (portal.update("h")) portal.answer( h );
    if (portal.update("led_motor")) portal.answer( var.rele_status );
		if (portal.update("sw")) 	portal.answer( OnSwitch );
		if (portal.update("hc")) portal.answer( var.h_contr );
		if (portal.update("tc")) portal.answer( var.t_contr );
		if (portal.update("gh")) portal.answer( var.h_gist );
		if (portal.update("gt")) portal.answer( var.t_gist );
		
		if (portal.update("h_contr")) portal.answer( var.h_contr );
		if (portal.update("t_contr")) portal.answer( var.t_contr );
		if (portal.update("timeOff")) portal.answer( var.timeOff );
		
    if (portal.update("timer")) {
			if ( var.regim_vent > 0 && var.rele_status ) {
				portal.answer( round( var.timeOff - ( (millis() - timer_timeOff )/1000) ) );
			} else {
				portal.answer( var.timeOff );
			}
		}
  }
	
	if (portal.form()) {  // формы
		if (portal.form("/save_wifi")) {      // кнопка нажата
			regim_connect_now = (enumconf_t)portal.getSelected("selwifi", SEL_AP_MODE );  // 0-3
			
			if ( regim_connect_now == MODE_AP_USER ) {
				portal.copyStr("ssid_softAP", var.ssid_softAP);  // копируем себе
				portal.copyStr("pass_softAP", var.pass_softAP);
			} else 
			if ( regim_connect_now == MODE_STA ){
				portal.copyStr("ssid", var.ssid);  // копируем себе
				portal.copyStr("pass", var.pass);
			}
			my_print(F("/save_wifi: regim_connect_now=")); my_println( (uint8_t)regim_connect_now );
		}
	}
}

void user_app() {
	if ( millis() - timer_get_temp >= 1000 ) {
		timer_get_temp = millis();
		Get_temp();
		Ventilyator();
	} 
}

void setup() {
#if defined(debug_print)	
	Serial.begin( 115200, SERIAL_8N1, SERIAL_TX_ONLY ); //	SERIAL_TX_ONLY – только отправка. SERIAL_RX_ONLY – только приём. SERIAL_FULL 
  delay(500);
	my_println();
#endif
	EEPROM.begin(sizeof(MyEEPROMStruct)); delay(100);// read eeprom
	if (EEPROM.percentUsed() >= 0) EEPROM.get(0, var);
	regim_connect_now = (enumconf_t)var.regim_connect;
	
	if ( var.magic != 0xAD && !digitalRead(BTN_PIN) ) {
		var.regim_vent = 0;
		var.regim_connect	= 0;
		var.rele_status 	= 0;
		var.h_contr = 60;
		var.t_contr = 40;
		var.t_gist  = 1;
		var.h_gist  = 5;
		var.timeOff = 10;
		
		regim_connect_now = MODE_AP_DEFAULT;
	}
	
	pinMode( RELE_PIN , OUTPUT ); digitalWrite( RELE_PIN, RELE_ON ? var.rele_status : !var.rele_status ); 
	
	pinMode(BTN_PIN, INPUT_PULLUP);
	attachInterrupt(BTN_PIN, button_change, CHANGE);//FALLING RISING CHANGE
	
	pinMode(LED_PIN, OUTPUT);	// tc.attach_ms( 100, Led_idicator );
	
	
#if defined(TYPE_DT_DHT)
	dht.begin();
#elif defined(TYPE_DT_AHT10)
	Wire2.begin(SDA, SCL);// Wire.begin(int sda, int scl)	| default:  SCL 5(D1) SDA 4(D2)
	Wire2.setClock(100000L); // 100KHz-400KHz
	aht.begin( &Wire2, 10, 0x38 );// aht.begin(TwoWire *wire, int32_t sensor_id, uint8_t i2c_address)
	// aht.begin();
#endif
	
	portal.attachBuild(build);// подключаем билдер и запускаем
	portal.attach(action_portal);
	
	// ChekConnect();
	if ( regim_connect_now == MODE_STA ) ConnectWifi(); 
	else Start_Wifi_AP();

	portal.enableOTA(); // страница обновления без пароля  portal.enableOTA("login", "pass"); // x.x.x.x/ota_update
	timer_timeOff = millis();
}

void loop() {
	
	portal.tick();
	LedIdicator();
	ChekConnect();
	
	user_app();	
	
	if ( eeprom_save ) {	eeprom_save = 0;	
		var.regim_connect = regim_connect_now;  
		var.magic = 0xAD;  
		EEPROM.put(0, var);	
#if defined(debug_print)	
		bool s = EEPROM.commit();	
		my_print(F("var save to eeprom! commit ")); my_println( s ? F("OK") : F("Fail")); 
#else
		EEPROM.commit();	
#endif
	}
}
