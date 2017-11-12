#include <OneWire.h>
#include <DallasTemperature.h>

#include <SPI.h>
#include <Ethernet.h>



// *************************************************
// Temperature sensors
// *************************************************
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
// arrays to hold device addresses
DeviceAddress sensor_0, sensor_1, sensor_2, sensor_3;


// *************************************************
// MQ gas sensors
// *************************************************
#define ANALOG_0_PIN A0
//pinMode(A0, INPUT);
int analog_0;


// *************************************************
// Ethernet Server
// *************************************************
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xD7, 0x8E, 0xBD
};
EthernetServer server(80);

const int MAX_PAGE_NAME_LENGTH = 40;
char buffer[MAX_PAGE_NAME_LENGTH+6];


// *************************************************
// Globals
// *************************************************
float temp_0, temp_1, temp_2, temp_3;


// *************************************************
// Helper functions
// *************************************************

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}


// *************************************************
// Temperature functions
// *************************************************
void getTemperatures() {
  sensors.requestTemperatures();
  float tempC;
  
  tempC = sensors.getTempC(sensor_0);
  if (temp_0 != tempC) {
    temp_0 = tempC;
  }
  
  tempC = sensors.getTempC(sensor_1);
  if (temp_1 != tempC) {
    temp_1 = tempC;
  }
  
  tempC = sensors.getTempC(sensor_2);
  if (temp_2 != tempC) {
    temp_2 = tempC;
  }
  
  tempC = sensors.getTempC(sensor_3);
  if (temp_3 != tempC) {
    temp_3 = tempC;
  }
}


// *************************************************
// Ethernet functions
// *************************************************
void listenForEthernetClients() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Got a client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        
        if(client.find("GET ")) {
          memset(buffer, 0, sizeof(buffer)); // Puffer löschen
          
          if(client.find("/")) {
            if (client.readBytesUntil(' HTTP', buffer, MAX_PAGE_NAME_LENGTH)) {
              String req_page = String(buffer);

              getTemperatures();
              analog_0 = analogRead(ANALOG_0_PIN);

              // send a standard http response header
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: application/json");
              client.println("Access-Control-Allow-Origin: http://raspberrypi");
              client.println();
              
              client.print("{\"analog_0\":");
              client.print(analog_0); 
              
              if (sensors.getDeviceCount() >= 1) {
                client.print(",\"temp_0\":");
                client.print(temp_0);              
              }
              if (sensors.getDeviceCount() >= 2) {
                client.print(",\"temp_1\":");
                client.print(temp_1);              
              }
              if (sensors.getDeviceCount() >= 3) {
                client.print(",\"temp_2\":");
                client.print(temp_2);              
              }
              if (sensors.getDeviceCount() >= 4) {
                client.print(",\"temp_3\":");
                client.print(temp_3);              
              }

              client.print("}");
              break;
            }
          }
          break;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}



void setup()
{
  Serial.begin(115200);
  
  // Initializing OneWire temperature sensors
  // ========================================
  Serial.println("Init Sensors");
  sensors.begin();

   // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  
  if (!sensors.getAddress(sensor_0, 0)) {
    Serial.println("Unable to find address for Device 0"); 
  } else {
    Serial.print("Device 0 Address: ");
    printAddress(sensor_0);
    Serial.println();    
  }
  
  if (!sensors.getAddress(sensor_1, 1)) {
    Serial.println("Unable to find address for Device 1"); 
  } else {
    Serial.print("Device 1 Address: ");
    printAddress(sensor_1);
    Serial.println();    
  }
  
  if (!sensors.getAddress(sensor_2, 2)) {
    Serial.println("Unable to find address for Device 2"); 
  } else {
    Serial.print("Device 2 Address: ");
    printAddress(sensor_2);
    Serial.println();    
  }
  
  if (!sensors.getAddress(sensor_3, 3)) {
    Serial.println("Unable to find address for Device 3"); 
  } else {
    Serial.print("Device 3 Address: ");
    printAddress(sensor_3);
    Serial.println();    
  }

 
  // set the resolution to 9 bit
  sensors.setResolution(sensor_0, TEMPERATURE_PRECISION);
  sensors.setResolution(sensor_1, TEMPERATURE_PRECISION);
  sensors.setResolution(sensor_2, TEMPERATURE_PRECISION);
  sensors.setResolution(sensor_3, TEMPERATURE_PRECISION);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(sensor_0), DEC); 
  Serial.println();

  Serial.print("Device 1 Resolution: ");
  Serial.print(sensors.getResolution(sensor_1), DEC); 
  Serial.println();
  
  Serial.print("Device 2 Resolution: ");
  Serial.print(sensors.getResolution(sensor_2), DEC); 
  Serial.println();

  Serial.print("Device 3 Resolution: ");
  Serial.print(sensors.getResolution(sensor_3), DEC); 
  Serial.println();

  
  // start the Ethernet connection and the server
  // ============================================
  Serial.println("Init ethernet...");
  
  Ethernet.begin(mac);
  server.begin();
  Serial.print("Webserver is at: ");
  Serial.println(Ethernet.localIP());
 
  delay(500);
}



void loop()
{
  listenForEthernetClients();
  
  delay(10);
}
