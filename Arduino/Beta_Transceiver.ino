/* Include header files */
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <stdlib.h>
#include <string.h>

/* RFM95 definitions */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7

/* Thermistor constants */
#define THERMISTORNOMINAL 10000
#define SERIESRESISTOR 150000   // Change this to the value of the series resistor if the resistor on the current PCB is changed

int16_t packetnum = 0;    // 16 bit integer to hold packets
char latBuffer[10];       // To hold latitude from GPS
char longBuffer[10];      // To hold longitude from GPS
String temp;              // To hold temperature from thermistor

/* Default constants for dummy data */
float Vbatt = 3.20;
float temperature = 70.42;

/* Pin declarations */
#define VBATPIN A9    // Pin for battery voltage
#define TEMPPIN A0    // Pin for temperature
#define PWMPIN  A3    // Pin for pulse width modulation
#define ANALOG1 A1    // Pin for analog reading 1
#define ANALOG2 A2    // Pin for analog reading 2

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// The name of the hardware serial port
#define GPSSerial Serial1

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);
     
// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO false

/* Start millisecond timer for GPS */
uint32_t timer = millis();


void setup()
{
  //while (!Serial);  // uncomment to have the sketch wait until Serial is ready
  pinMode(RFM95_RST, OUTPUT);   // Set initila pin mode
  digitalWrite(RFM95_RST, HIGH);    // Set initial pin mode
  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);   // Start serial at 115200 baud
  // Serial.println("Adafruit GPS library basic test!");
     
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
     
  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(100);
  /* LoRa starts */
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);    // Checking LEDs
  delay(10);
  /* Check if LoRa radio works */
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  /* Otherwise, radio has initialized */
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }

  /* Use this to set our own frequency */
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  /* The default transmitter power is 13dBm, using PA_BOOST.
     If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
     you can set transmitter powers from 5 to 23 dBm */
  rf95.setTxPower(23, false);
  delay(1000);
  /* END OF LORA */
  
  // Ask for firmware version
  GPSSerial.println(PMTK_Q_RELEASE);
}

void loop() // run over and over again
{
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  if (GPSECHO)
    if (c) Serial.print(c);
    // if a sentence is received, we can check the checksum, parse it...
  
  if (GPS.newNMEAreceived()) 
  {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    Serial.println(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }
  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis()) 
  {
    timer = millis();
  }
  // set up common variables and reading in values from analog pins
  float measuredvbat = analogRead(VBATPIN);
  float measuredtemp = analogRead(TEMPPIN);
  int16_t measureda1 = analogRead(ANALOG1);
  int16_t measureda2 = analogRead(ANALOG2);
  char radiopacket[75] = "                                                                          ";    // This is 75 spaces since it needs to hold something when initialized
  // approximately every 2 seconds or so, print out the current stats
  
  if (millis() - timer > 2000) 
  {
    timer = millis(); // reset the timer
    if (GPS.fix)  // If GPS has a fix
    {
      dtostrf(GPS.latitudeDegrees, 10, 5, latBuffer);   // Convert the values to strings to send them over LoRa
      temp += latBuffer;    // Add latitude to what is sent
      temp += ", ";   // Add delimiter
      
      dtostrf(GPS.longitudeDegrees, 10, 5, longBuffer);   // Convert the values to strings to send them over LoRa
      temp += longBuffer;   // Add longitude to what is sent
      temp += ", ";   // Add delimiter

      /* Calculate human sensible value from voltage raw data */
      measuredvbat *= 2;    
      measuredvbat *= 3.3;
      measuredvbat /= 1024;
      Vbatt = measuredvbat;
      temp += Vbatt;   // Add battery voltage to what is sent
      temp += ", ";    // Add delimiter

      /* Calculate human sensible value from thermistor raw data using the Steinhart equation*/
      measuredtemp = 1023/measuredtemp - 1;
      measuredtemp = SERIESRESISTOR / measuredtemp;
      float steinhart = measuredtemp / THERMISTORNOMINAL;
      steinhart = log(steinhart);
      steinhart /= 3950;                  // 3950 is the coeeficient of the thermistor
      steinhart += 1.0 / (25 + 273.15);   // 25 is the nominal temperature assumption
      steinhart = 1.0 / steinhart;
      steinhart -= 273.15;
      temperature =  steinhart;
      temp += temperature;  // Add temperature to what is sent
      temp += ", ";   // Add delimiter

      /* Calculate human sensible value from voltage reading 1 raw data */
      measureda1 *= 2;    
      measureda1 *= 3.3;
      measureda1 /= 1024;
      temp += measureda1;   // Add voltage reading 1 to what is sent
      temp += ", ";   // Add delimiter

      /* Calculate human sensible value from voltage reading 2 raw data */
      measureda2 *= 2;    
      measureda2 *= 3.3;
      measureda2 /= 1024;
      temp += measureda2;   // Add voltage reading 2 to what is sent
      temp += ", ";   // Add delimiter

      packetnum++;    // Increment packet count
      temp += packetnum;    // Add packet number to what is sent
      temp += ", ";   // Add delimiter
      
      delay(1000); // Wait 1 second between transmits, could also 'sleep' here!
      Serial.println("Transmitting..."); // Send a message to rf95_server

      /* Clear the whole readio packet using spaces */
      for (int i = 0; i < 74; i++)
      {
        radiopacket[i] = ' ';
      }

      /* Set temp into radio packet */
      for (int i = 0; i < temp.length(); i++)
      {
        radiopacket[i] = temp[i];
      }

      /* Debugging using serial monitor */
      Serial.print("Sending "); 
      Serial.println(radiopacket); 
      Serial.println(temp);

      /* Actually send the whole radio packet using LoRa */
      Serial.println("Sending...");
      delay(10);
      rf95.send((uint8_t *)radiopacket, 75);

      /* Wait till all the packets have been sent */
      Serial.println("Waiting for packet to complete..."); 
      delay(10);
      rf95.waitPacketSent();
      
      // Now wait for a reply
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      Serial.println("Waiting for reply...");
      if (rf95.waitAvailableTimeout(1000))
      {  
      /* Receive message using LoRa for buzzer */
      if (rf95.recv(buf, &len))
      {
        // Print received information to serial to help with debugging
        Serial.print("Got reply: ");
        Serial.println((char*)buf);
        Serial.print("RSSI: ");
        Serial.println(rf95.lastRssi(), DEC); 

        // Use received buf to determine whether to tone or not
        if(strcmp(buf, "1") == 0)   // This evaluate to true if the user wants the buzzer
        {
          tone(PWMPIN, 4699, 1000);   // We can set this frequency to be above a humans range of hearing 
                                      // but in the range of a dogs hearing. For testing purposes the frequency has been
                                      // set lower to 4699 Hz. It plays the tone for 1000 milliseconds 
        }
      }
      else
      {
        Serial.println("Receive failed");   // For debugging in serial
      }
    }
    else
    {
      Serial.println("No reply, is there a listener around?");    // If no receiving
    }
   }
 }
 temp = "";   // Reset temp to not have garbage in buffer
}
