/*
 Update 2014 - TMRh20
 */

/**
 * Simplest possible example of using RF24Network,
 *
 * RECEIVER NODE
 * Listens for messages from the transmitter and prints them out.
 */

#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <time.h>


/**
 * g++ -L/usr/lib main.cc -I/usr/include -o main -lrrd
 **/
//using namespace std;

// CE Pin, CSN Pin, SPI Speed

// Setup for GPIO 22 CE and GPIO 25 CSN with SPI Speed @ 1Mhz
//RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_18, BCM2835_SPI_SPEED_1MHZ);

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 4Mhz
//RF24 radio(RPI_V2_GPIO_P1_15, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_4MHZ); 

// Setup for GPIO 22 CE and CE1 CSN with SPI Speed @ 8Mhz

RF24Network network;

// Address of our node in Octal format
const uint16_t this_node = 01;

// Address of the other node in Octal format (01,021, etc)
const uint16_t other_node = 00;

const unsigned long interval = 2000; //ms  // How often to send 'hello world to the other unit

unsigned long last_sent;             // When did we last send?
unsigned long packets_sent;          // How many have we sent already


struct packet_t {
  RF24NetworkHeader header;
  char data[257];
};

enum PACKET_TYPE : unsigned char {
  PING = 0,                // pings the parent
  DISCOVER = 1,            // finds parent and gets address
  REROUTE = 2,             // finds new parentNode
  STATUS = 5,
  DATA = 10,               // read data (bytes + flags + sensors)
  BYTES = 11,              // read bytes data
  FLAGS = 12,              // read flags data
  SENSOR = 13,             // read sensor data
  WRITE_BYTE = 15,         // write specific byte (address,value)
  WRITE_FLAG = 16,         // write specific flag (address,value)
  WRITE_FLAGS = 17,        // write all flags
  WRITE_BYTES = 18,        // write all bytes
  COMMAND = 20,            // command to execute
  AUDIO = 30,              // beginning of audo transmittion
  AUDIO_START = 31,
  AUDIO_STOP = 32,
  REPROGRAM = 71,          // update sketch over the air
  RESTART = 70,
  SET_ENCRYPTION_KEY = 80, // sets new encryption key
  SET_ADDRESS = 81,
  NEW_NODE = 85,           // notifies master of new node
  NEW_NODE_REPLY = 86,
  MOVED_NODE = 87,         // notifies master that node was rerouted
  REPLY = 99               // reply to a previous request
};

void readPacket(packet_t *packet) {
  #ifdef DEBUG 
  Serial.println("DEBUG: reading packet: "); 
  #endif

  network.peek((*packet).header);

  #ifdef DEBUG
  Serial.print(" type: "); Serial.print((*packet).header.type);
  Serial.print(" from: "); Serial.print((*packet).header.from_node);
  Serial.print(" to: "); Serial.print((*packet).header.to_node); 
  Serial.print(" size: "); Serial.println((*packet).header.payload_length);
  #endif

  network.read((*packet).header, (*packet).data, (*packet).header.payload_length);
  (*packet).data[(*packet).header.payload_length] = 0;

  #ifdef DEBUG 
  Serial.print("DEBUG: payload: "); Serial.write((*packet).data, (*packet).header.payload_length); Serial.print("\r\n");
  #endif
}

bool writePacket(uint16_t address, unsigned char type, char* data, int length) {
  #ifdef DEBUG
  Serial.print("DEBUG: sending packet: ");
  Serial.print(" type: "); Serial.print(type);
  Serial.print(" from: "); Serial.print(thisNodeAddress);
  Serial.print(" to: "); Serial.print(address); 
  Serial.print(" size: "); Serial.println(length);
  if (length) {
    Serial.print("DEBUG: payload: "); Serial.write(data, length); Serial.print("\r\n");
  }
  #endif

  network.update();
  RF24NetworkHeader header(address, type);
  bool result = network.write(header, data, length);

  #ifdef DEBUG
    Serial.print("DEBUG: result: "); Serial.println(result);
  #endif
  return result;
}

int main(int argc, char** argv) 
{
  printf("starting stekar test\r\n");

  packet_t packet;
  delay(5);
  network.begin(/*channel*/ 90, /*node address*/ this_node);

  while(1)
  {
    printf("waiting for data ...\r\n");
    network.update();
    while ( network.available() ) {     // Is there anything ready for us?
      readPacket(&packet);  

      printf("Received packet from %u to %u type %u size %u \n", packet.header.from_node, packet.header.to_node, packet.header.type, packet.header.payload_length);

      switch (packet.header.type) {
      // we received a reponse to issued command
        case PACKET_TYPE::REPLY:
          printf("first byte: %u", packet.data[0]); 
          break;
      }

    }     
    delay(1000);
    writePacket(0, PACKET_TYPE::PING, "123456789", 10);
    delay(500);
  }

  return 0;
}

