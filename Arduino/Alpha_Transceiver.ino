// Feather9x_RX
// -*- mode: C++ -*-


/* Include header files  */
#include <SPI.h>
#include <RH_RF95.h>

/* Define pin on the arduino that the switch is connected to */
#define SWITCHPIN  A0

/* for Feather32u4 RFM9x */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blink this LED when alpha receives
#define LED 13

void setup()
{
  // Initialize pins
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // Start serial
  Serial.begin(115200);

  // Delay for startup
  delay(100);

  // Manual Reset to Check
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  // Print error message if LoRa fails
  while (!rf95.init()) {
    //Serial.println("LoRa radio init failed");
    while (1);
  }
  // Otherwise print OK message
  //Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    //Serial.println("setFrequency failed");
    while (1);
  }
  // Use to manually set frequency while testing in serial monitor
  // Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);   // Use to boost power
}

void loop()
{
  if (rf95.available())   // If radio is available
  {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);    // Setting buffer

    if (rf95.recv(buf, &len))   // Whatever is received will be in buf
    {
      digitalWrite(LED, HIGH);    // To indicate that somethign was received
      // Serial.print("Got: ");
      Serial.println((char*)buf);  // Send what was received to serial monitor for Arduino debugging
      /* Use to print signal strength to test how far we can take the beta transceiver */
      // Serial.print("RSSI: ");   
      // Serial.println(rf95.lastRssi(), DEC);

      // Send a reply
      if(digitalRead(SWITCHPIN) == HIGH)    // This means the user wants the beta transceiver to buzz
      {
        uint8_t data[] = "1";   // Send buzz
        rf95.send(data, sizeof(data));  // Send using LoRa radio
        rf95.waitPacketSent();    // Wait till all the packets are sent
        // Serial.println("Sent a reply");
        digitalWrite(LED, LOW);   // Physical indication of sent signal
      }
      else    // If the user doesn't want the beta transceiver to buzz
      {
        uint8_t data[] = "0";   // Send no buzz
        rf95.send(data, sizeof(data));  // Send using LoRa radio
        rf95.waitPacketSent();  // Wait till all the packets are sent
        // Serial.println("Sent a reply");
        digitalWrite(LED, LOW); // Physical indication of sent signal
      }
    }
    else
    {
      /* Check here to know if Receive is failing */
      // Serial.println("Receive failed");
    }
  }
}
