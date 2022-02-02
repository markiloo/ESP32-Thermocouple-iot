#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <max6675.h>
#include <Filter.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

#define DHTTYPE DHT22 //Define DHT sensor model

//number of tries connecting to network before proceeding to next step
int threshold_number_of_tries = 20;
int number_of_tries = 0;

// Set web server port number to 80
AsyncWebServer server(80);

//hotspot network credentials
const char* hotspot_ssid = "ESP32 hotspot";
const char* hotspot_password = "123456789";

//wifi network credentials
//char network_ssid[] = " ";
//char network_password[] = " ";

//Sensor Data
String device_name = "ESP32 Test Server";
float temperature[2] = {0};
float humidity = 0;

//Sensor Pins, CS and CLK are shareable between thermocouples
int thermoCS = 22;
int thermoCLK = 23;

//Thermocouple sends data to pin
int thermoDO_sensor1 = 21;
MAX6675 thermocouple1(thermoCLK, thermoCS, thermoDO_sensor1);

int thermoDO_sensor2 = 16;
MAX6675 thermocouple2(thermoCLK, thermoCS, thermoDO_sensor2);

int led_pin = GPIO_NUM_19;
int led_state = HIGH;
const int DHTPIN = GPIO_NUM_0;     // Digital pin connected to the DHT sensor

//initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

//Noise Filter
ExponentialFilter<float> TempFilter_1(10, 0);
float temp_filtered_1 = 0;

ExponentialFilter<float> TempFilter_2(10, 0);
float temp_filtered_2 = 0;

unsigned long current_millis = 0;
unsigned long start_millis = millis();
unsigned long interval = 2500;

//led interface
bool led_on = true;
int led_1 = GPIO_NUM_19;

float get_temperature1() {
  float temperature_celsius = thermocouple1.readCelsius();
 // Serial.print("Temperature: ");
 // Serial.println(temeperature_celsius);
  delay(300);
  return temperature_celsius;
}

float get_temperature2() {
  float temperature_celsius = thermocouple2.readCelsius();
 // Serial.print("Temperature: ");
 // Serial.println(temeperature_celsius);
  delay(300);
  return temperature_celsius;
}

String send_temperature1() {
  Serial.print("Temperature: ");
  Serial.println(temp_filtered_1);
  return String(temp_filtered_1);
}

String send_temperature2() {
  Serial.print("Temperature: ");
  Serial.println(temp_filtered_2);
  return String(temp_filtered_2);
}

String get_humidity() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  if ((current_millis - start_millis) > interval) 
  {
    humidity = dht.readHumidity();
    start_millis = current_millis;
    Serial.println("Reading");
  }
  
  current_millis = millis();
  // Read temperature as Celsius (the default)
  // Check if any reads failed and exit early (to try again).

  Serial.print(F("Humidity: "));
  Serial.println(humidity);
  return "Humidity: " + String(humidity);
}

void startup_server() 
{
WiFi.softAP(hotspot_ssid, hotspot_password);
//WiFi.begin(network_ssid, network_password);

//uint8_t i = 0;
//  while (WiFi.status() != WL_CONNECTED)
//  {
//   Serial.println(F("Connecting..."));
//}
 
//  Serial.print(F("Connected. IP address is: "));
//  Serial.println(WiFi.localIP());

  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP()); //Display Device IP in serial

  if(!SPIFFS.begin(true)) //Mount SPIFFS
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
}

void setup_GPIO() 
{
  //Turns on built in led when the device has finished setting up the server and initializes device pins
  pinMode(led_pin , OUTPUT);
  pinMode(GPIO_NUM_2 , OUTPUT);
  pinMode(GPIO_NUM_16, INPUT);
  pinMode(GPIO_NUM_4, INPUT);
  digitalWrite(GPIO_NUM_2, HIGH);
  digitalWrite(led_pin, HIGH);
}

String led() 
{
  if (led_on) {
    digitalWrite(led_1, LOW);
    led_on = false;
  }
  else if (!led_on) {
    digitalWrite(led_1, HIGH);
    led_on = true;
  }
  else {
    //Do Nothing
  }
  return "led";
}

void setup() 
{
  Serial.begin(9600);
  startup_server(); 

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });

  //runs the send_sensor_readings function when the clinet browser requests in the adress "/temperature", the function returns a string
  server.on("/temperature1", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send_P(200, "text/plain", send_temperature1().c_str());
  }); 

  server.on("/temperature2", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send_P(200, "text/plain", send_temperature2().c_str());
  }); 

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send_P(200, "text/plain", get_humidity().c_str());
  }); 

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/highcharts.js");
  });

  server.on("/led", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(200, "text/plain", led().c_str());
  }); 

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request){
    Serial.println("Message Received");
  });

  //Start Webserver
  server.begin();

  //Initializes device pins
  setup_GPIO();

  //Initialize Filters
  for(int i = 0; i < 10; i++) {
      temperature[0] = get_temperature1();
      TempFilter_1.Filter(temperature[0]);
      temp_filtered_1 = TempFilter_1.Current();
  }

  for(int i = 0; i < 10; i++) {
      temperature[1] = get_temperature2();
      TempFilter_2.Filter(temperature[0]);
      temp_filtered_2 = TempFilter_2.Current();
  }

  dht.begin();
}

void loop() {
  //get sensor readings and store them in array
    temperature[0] = get_temperature1();
    temperature[1] = get_temperature2();

    TempFilter_1.Filter(temperature[0]);
    temp_filtered_1 = TempFilter_1.Current();

    TempFilter_2.Filter(temperature[1]);
    temp_filtered_2 = TempFilter_2.Current();
}


