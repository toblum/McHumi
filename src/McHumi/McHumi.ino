// *********************************************************************
// * McHumi - DHT22 and HTU21D Humidity / temperature logger for MQTT
// * Based on CoogleIOT lib (https://github.com/coogle/CoogleIOT)
// * and SimpleDHT (https://github.com/winlinvip/SimpleDHT)
// * by Tobias Blum (https://github.com/toblum)
// *********************************************************************

// Select which sensor type is used, uncomment the corresponding line
//#define SENSOR_DHT22
# define SENSOR_HTU21D

// *********************************************************************
// * Libraries
// *********************************************************************
#include <CoogleIOT.h>

#ifdef SENSOR_DHT22
#include <SimpleDHT.h>
#endif

#ifdef SENSOR_HTU21D
#include <SparkFunHTU21D.h>
#endif


// *********************************************************************
// * Settings
// *********************************************************************
#define COOGLEIOT_DEBUG
#define MCHUMI_STATUS_TOPIC "McHumi_02"
#define REFRESH_INTERVAL_1    15*1000   // 15 seconds (reading)
#define REFRESH_INTERVAL_2    60*1000   // 60 seconds (publishing)
#define PIN_SENSOR D2
#define MCHUMI_VERSION "1.02"
#define COOGLEIOT_TIMEZONE_OFFSET ((3600 * 2) * 1)
#define COOGLEIOT_DAYLIGHT_OFFSET 0

static unsigned long lastRefreshTime_1 = millis();
static unsigned long lastRefreshTime_2 = millis();

// Running average
const int numReadings = 20;             // Average over 20 last readings
float readings_temperature[numReadings];
float readings_humidity[numReadings];     
int readIndex = 0;
float average_temperature = 0;
float average_humidity = 0;
float total_temperature = 0;
float total_humidity = 0;
bool array_initalized = false;

// Globals - https://www.arduino.cc/en/Tutorial/Smoothing
CoogleIOT *iot;
PubSubClient *mqtt;

#ifdef SENSOR_DHT22
SimpleDHT22 dht22;
#endif

#ifdef SENSOR_HTU21D
HTU21D myHumidity;
#endif


// *********************************************************************
// * WORKER
// *********************************************************************
void interval_worker()
{
  if(millis() - lastRefreshTime_1 >= REFRESH_INTERVAL_1)
  {
    lastRefreshTime_1 += REFRESH_INTERVAL_1;
    read_sensor();
  }
  
  if(millis() - lastRefreshTime_2 >= REFRESH_INTERVAL_2)
  {
    lastRefreshTime_2 += REFRESH_INTERVAL_2;
    publish_message();
  }
}

void read_sensor()
{
  float temperature = 0;
  float humidity = 0;


  #ifdef SENSOR_DHT22
  // Read DHT22
  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(PIN_SENSOR, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    if(iot->serialEnabled()) {
      Serial.print("Read DHT22 failed, err="); 
      Serial.println(err);
      delay(2000);
    }
    return;
  }
  #endif

  #ifdef SENSOR_HTU21D
  humidity = myHumidity.readHumidity();
  temperature = myHumidity.readTemperature();

  if (humidity > 100 || temperature > 100) {
    if(iot->serialEnabled()) {
      Serial.println("Read HTU21D failed"); 
      Serial.print("Failed temperature: ");
      Serial.println(temperature, 1);
      Serial.print("Failed humidity: ");
      Serial.println(humidity, 1);
      delay(2000);
    }
    return;
  }
  #endif


  // Init array
  if (!array_initalized) {
    init_array(temperature, humidity);
  }

  // Averaging
  // subtract the last reading:
  total_temperature = total_temperature - readings_temperature[readIndex];
  total_humidity = total_humidity - readings_humidity[readIndex];
  // read from the sensor:
  readings_temperature[readIndex] = temperature;
  readings_humidity[readIndex] = humidity;
  // add the reading to the total:
  total_temperature = total_temperature + readings_temperature[readIndex];
  total_humidity = total_humidity + readings_humidity[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  if (readIndex >= numReadings) {
    readIndex = 0;
  }

  // calculate the average:
  average_temperature = total_temperature / numReadings;
  average_humidity = total_humidity / numReadings;

  if(iot->serialEnabled()) {
    Serial.print("Temperatur: ");
    Serial.println(temperature, 1);
    Serial.print("Luftfeuchtigkeit: ");
    Serial.println(humidity, 1);

    Serial.print("average_temperature: ");
    Serial.println(average_temperature, 2);
    Serial.print("average_humidity: ");
    Serial.println(average_humidity, 2);
  }
}

void publish_message()
{
  // Build JSON
  char json[80];
  sprintf(json, "{\"temperature\": %d.%02d, \"humidity\": %d.%02d, \"version\": \"%s\"}", (int)average_temperature, (int)(average_temperature*100)%100, (int)average_humidity, (int)(average_humidity*100)%100, MCHUMI_VERSION);

  if(iot->serialEnabled()) {
    Serial.print("Publish: ");
    Serial.println(json);
  }

  mqtt->publish(MCHUMI_STATUS_TOPIC, json);
}

void init_array(float t, float h)
{
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings_temperature[thisReading] = t;
    readings_humidity[thisReading] = h;

    total_temperature = total_temperature + readings_temperature[thisReading];
    total_humidity = total_humidity + readings_humidity[thisReading];
  }
  array_initalized = true;

  if(iot->serialEnabled()) {
    Serial.println("Array initialized");
  }
}


// *********************************************************************
// * SETUP
// *********************************************************************
void setup() {
  iot = new CoogleIOT(LED_BUILTIN);

  iot->enableSerial(115200);
  iot->initialize();

  if(iot->mqttActive()) {
    mqtt = iot->getMQTTClient();

    mqtt->publish(MCHUMI_STATUS_TOPIC, "{\"status\": \"ready\"}", true);

    if(iot->serialEnabled()) {
      Serial.println("McHumi Initialized");
      Serial.print("Version: ");
      Serial.println(MCHUMI_VERSION);
    }

    //mqtt->setCallback(mqttCallbackHandler);
    //mqtt->subscribe(GARAGE_DOOR_ACTION_TOPIC);
  } else {
    if(iot->serialEnabled()) {
      Serial.println("ERROR: MQTT Not Initialized!");
    }
  }
  
  #ifdef SENSOR_HTU21D
  myHumidity.begin();
  #endif

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings_temperature[thisReading] = 0;
    readings_humidity[thisReading] = 0;
  }

  // Workaround for problems with losing connection
  // https://github.com/knolleary/pubsubclient/issues/203
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
  }
}


// *********************************************************************
// * LOOP
// *********************************************************************
void loop() {
  iot->loop();

  interval_worker();

  yield();
}
