#define DEBUG
#include "Arduino.h"
#include "RF24Network.h"
#include <RF24.h>
#include <SPI.h>
#include <EEPROM.h>
#include "stekar.h"
#include "audio.h"


RF24 radio(7, 8);                       // nRF24L01(+) radio attached using Getting Started board 
RF24Network network(radio);             // Network uses that radio
StekarAudio audio;

const uint16_t channel = 90;            // Channel
const uint16_t rootNodeAddress = 00;
uint16_t maxSlaveAddress = 5;
uint16_t freeSlaveAddress = 1;

int deviceIdEEPROMAddress = 112;
int deviceId;
int parentNodeEEPROMAddress = 108;
uint16_t parentNode = 0;               // should be -1 with new device
int encryptionKeyEEPROMAddress = 104;
uint32_t encryptionKey = 0x12345678;   // hardcoded encryption key, locks device to specific network
int thisNodeEEPROMAddress = 100;
uint16_t thisNodeAddress = 00;     // Address of our node

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
  Serial.print(" size: "); Serial.print((*packet).header.payload_length);
  //Serial.print(" next_id: "); Serial.println((*packet).header.next_id);
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

// should ping the parent and wait for response
bool ping(uint16_t address) {
  return writePacket(address, PACKET_TYPE::PING, NULL, 0);
}

uint16_t findn(uint16_t num)
{
    uint16_t n = 0;
    while(num) {
        num /= 10;
        n++;
    }
    return n;
}

uint16_t pow(uint16_t num, uint16_t pow) {
  if (pow == 0) return 1;
  uint16_t result = num;
  for (byte i = 1; i < pow; i++) {
    result *= num;
  }
}

void get_children(uint16_t address, uint16_t* result) {
  if (address == 0) {
    for (uint16_t i = 0; i < 5; i++) {
      result[i] = i;
    }
    return;
  }
  else {
    uint16_t mult = findn(address);
    if (mult > 5) {
      result = NULL;
      return;
    }
    mult = pow(10, mult);
    for (uint16_t i = 0; i < 5; i++) result[i] = i*mult + address;
  }
}

uint16_t _discover(uint16_t parent) {
  uint16_t foundAddress = -1;
  uint16_t address_list[5];
  get_children(parent, address_list);
  if (address_list == NULL) return -1;

  for (int i = 0; i < 5; i++) {
    uint16_t testAddress = address_list[i];
    network.updateAddress(testAddress);
    if (ping(0)) {
      return testAddress;
    }
    foundAddress = _discover(testAddress);
    if (foundAddress != -1) return foundAddress;
  }
  return foundAddress;
}

// gets the parent address or -1 in case no parent was found
uint16_t discover() {
  // try to use last possible address on every level (4 levels down)
  uint16_t address = -1;
  byte deviceIdBytes[2] = { (byte)(deviceId >> 8), (byte)(deviceId & 0x00ff) };

  address = _discover(0);
  if (address == -1) return -1;

  // at this point we have a working address with which we can ping
  // lets ask master now to appoint as an address of his choosing
  writePacket(rootNodeAddress, PACKET_TYPE::NEW_NODE, (char*)deviceIdBytes, 2);
  packet_t packet;
  while (!packet.header.type != PACKET_TYPE::NEW_NODE_REPLY) {
    network.update();
    if (network.available()) {
      readPacket(&packet);
    }
  }

  if (packet.data[0] == 0x50 && packet.data[1] == 0xff) {
    address == (packet.data[2] << 8) + packet.data[3];
  }

  if (address != -1) {
    EEPROM.update(thisNodeEEPROMAddress, address);  
  }
  
  #ifdef DEBUG
    Serial.print("DEBUG: new address: "); Serial.println(address);
  #endif
  return address;
}

// gets the parent address or -1 in case no parent was found
uint16_t reroute() {
  // same as discover except that it tells the master that node was rerouted
  return discover();
}

void Stekar::printStatus(uint16_t address) {
  char data[256];
  char num[10];
  strcpy(data, "");
  for (int i = 0; i < 8; i++) {
    strcat(data, itoa(i, num, 10));
    strcat(data, "-");
    strcat(data, func[i].name);
    strcat(data, "\r\n");
  }
  writePacket(address, PACKET_TYPE::REPLY, data, strlen(data) + 1);
  strcpy(data, "");
  for (int i = 0; i < 8; i++) {
    strcat(data, itoa(i, num, 10));
    strcat(data, "-");
    strcat(data, sensor[i].name);
    strcat(data, "\r\n");
  }
  writePacket(address, PACKET_TYPE::REPLY, data, strlen(data) + 1);
  strcpy(data, "");
  for (int i = 0; i < 8; i++) {
    strcat(data, itoa(i, num, 10));
    strcat(data, "-");
    strcat(data, byte[i].name);
    strcat(data, "\r\n");
  }
  writePacket(address, PACKET_TYPE::REPLY, data, strlen(data) + 1);
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

Stekar::Stekar(void) {}

void Stekar::send(char* data, int length) {
  writePacket(rootNodeAddress, PACKET_TYPE::AUDIO, data, length);
}

void Stekar::init()
{
  Serial.begin(115200);
  SPI.begin();
  radio.begin();
  thisNodeAddress = EEPROM.read(thisNodeEEPROMAddress); 
  deviceId = EEPROM.read(deviceIdEEPROMAddress); 

  Serial.print("Stekar 0.2.0alpha - "); Serial.println(thisNodeAddress == 0 ? "master" : "slave");
  network.begin(channel, thisNodeAddress);
  audio.init(this, 0); // init audio library on adc 0

  //encryptionKey == EEPROM.read(encryptionKeyEEPROMAddress);
  if (thisNodeAddress == 0) return;

  // new device should come with EEPROM address set to -1. 
  // If this address is detected node runs auto discovery.
  while (thisNodeAddress == -1 || !network.is_valid_address(thisNodeAddress)) {
    #ifdef DEBUG
      Serial.println("DEBUG: running discover");
    #endif
    thisNodeAddress = discover();
    delay(500);
  }

  // node pings its parent. if parent doesn't respond node 
  // should run the reroute command. this should also happen if
  // transmittion is interupted for N minutes any time in the process
  int counter = 0;
  while (!ping(rootNodeAddress) && counter++ < 100) delay(100);
  if (!ping(rootNodeAddress)) {
    #ifdef DEBUG
      Serial.println("DEBUG: running reroute");
    #endif
    do {
      parentNode = reroute();
      delay(500);
    } while (parentNode == -1);
  }
}

void Stekar::update(void){
  bool response;
  bool typeIsDiscover;
  char result;
  char bytesArray[8];
  packet_t packet;
  int intArray[8];
  network.update();                  // Check the network regularly
  audio.update();                     // let audio library do its processing

  while ( Serial.available() > 0 ) {
    uint16_t cmd = Serial.parseInt();
    uint16_t address = Serial.parseInt();
    char bytes[8] = {};
    Serial.print("sending command ["); Serial.print(cmd, DEC); Serial.print("] to: "); Serial.println(address, DEC);
    switch (cmd) {
      // send ping
      case PACKET_TYPE::PING:
        response = writePacket(address, PACKET_TYPE::PING, (char*)"0123456789", 11);
        break;  
      case 1:
        response = writePacket(address, PACKET_TYPE::PING, (char*)"012345678901234567890123", 25);
        break;
      // ask for sensor values
      case PACKET_TYPE::SENSOR:
        response = writePacket(address, PACKET_TYPE::SENSOR, NULL, 0);
        break;
      case PACKET_TYPE::WRITE_BYTE:
      case PACKET_TYPE::WRITE_FLAG:
        bytes[0] = Serial.parseInt();
        bytes[1] = Serial.parseInt();
        response = writePacket(address, cmd, bytes, 3);
        break;
      case PACKET_TYPE::COMMAND:
        bytes[0] = Serial.parseInt();
        bytes[1] = Serial.parseInt();
        bytes[2] = Serial.parseInt();
        response = writePacket(address, cmd, bytes, 3);
      default:
        response = writePacket(address, cmd, NULL, 0);
    }
    Serial.print("Got response: ");
    Serial.println(response, HEX);
  }

  while ( network.available() ) {     // Is there anything ready for us? (handle requests --> reply)
    Serial.print("received packet:");
    readPacket(&packet);
    Serial.print(" type: "); Serial.print(packet.header.type);
    Serial.print(" from: "); Serial.print(packet.header.from_node);
    Serial.print(" size: "); Serial.println(packet.header.payload_length);
    switch (packet.header.type) {
      // we received a reponse to issued command
      case PACKET_TYPE::REPLY:
        Serial.print("first byte: "); Serial.println(packet.data[0], HEX);
        Serial.println(packet.data);
        break;
      // when ping is recieved just acknowledge the package (happens automatically)
      case PACKET_TYPE::PING:
        break;  
      // when new node connects it tries to auto discover its parent by sending 
      // DISCOVER packet to the list of preset addresses. The address that replies
      // and has the strongest signal is used as parent. 
      case PACKET_TYPE::DISCOVER:
      case PACKET_TYPE::REROUTE:
        // reply with confirmation to the node and inform master of new node
        // master can then display list of all nodes and which one sees which one
        // and allows us to reorder topology on-the-fly from the master
        // todo: this should not send back its address but the free address that can be used
        // slave can derive parent address from that
        typeIsDiscover = packet.header.type == PACKET_TYPE::DISCOVER;
        if (freeSlaveAddress < maxSlaveAddress) {
          // todo: tale ack ne poslje podatka nazaj ... treba write uporabit
          //radio.writeAckPayload(pipeNumber, &freeSlaveAddress, sizeof(freeSlaveAddress));
          if (thisNodeAddress == rootNodeAddress) {
            // send over Serial to host
            
          } else {
            // send notification to master
            writePacket(rootNodeAddress, typeIsDiscover ? PACKET_TYPE::NEW_NODE : PACKET_TYPE::MOVED_NODE, (char*)freeSlaveAddress, 1);
            // todo: free slave address ... kk ga doloci po rebootu ? flash ? reroute ?
            // fajn bi blo da po rebootu se zmeri ve kdo je prklucen na njega.
            freeSlaveAddress++;
          }
        }
        break;
      case PACKET_TYPE::STATUS:
        printStatus(packet.header.from_node);
        break;
      // return sensors values to parent
      case PACKET_TYPE::DATA:
        //writePacket(packet.header.from, PACKET_TYPE::REPLY, dataStruct);
        break;
      case PACKET_TYPE::SENSOR:
        for (int i = 0; i < 8; i++) {
          intArray[i] = *sensor[i].var; 
        }
        writePacket(packet.header.from_node, PACKET_TYPE::REPLY, (char*)intArray, sizeof(intArray));
        break;
      case PACKET_TYPE::BYTES:
        for (int i = 0; i < sizeof(bytesArray); i++) {
          bytesArray[i] = *byte[i].var; 
        }
        writePacket(packet.header.from_node, PACKET_TYPE::REPLY, bytesArray, sizeof(bytesArray));
        break;
      case PACKET_TYPE::FLAGS:
        writePacket(packet.header.from_node, PACKET_TYPE::REPLY, (char*)byte[8].var, 1);
        break;
      case PACKET_TYPE::WRITE_BYTE:
        if (packet.data[0] < 8) {
          *byte[packet.data[0]].var = packet.data[1];
        }
        break;
      case PACKET_TYPE::WRITE_FLAG:
        if (packet.data[0] < 8) {
          if ((bool)packet.data[1])
            *byte[packet.data[0]].var |= 1 << packet.data[0];
          else
            *byte[packet.data[0]].var &= ~(1 << packet.data[0]);
        }
        break;
      case PACKET_TYPE::WRITE_BYTES:
        for (int i = 0; i < 8; i++) {
          *byte[i].var = packet.data[i]; 
        }
        break;
      case PACKET_TYPE::WRITE_FLAGS:
        *byte[8].var = packet.data[0];
        break;
      // begining of audo transmittion  
      case PACKET_TYPE::AUDIO:
        break;
      case PACKET_TYPE::AUDIO_START:
        audio.run();
        break;
      case PACKET_TYPE::AUDIO_STOP:
        audio.stop();
        break;
      case PACKET_TYPE::COMMAND:
        result = (*func[packet.data[0]].func)(packet.data[1], packet.data[2]);
        writePacket(packet.header.from_node, PACKET_TYPE::REPLY, &result, 1);
        break;
      case PACKET_TYPE::SET_ENCRYPTION_KEY:
        //encryptionKey = packet.data[0] << 24 & packet.data[1] << 16 & packet.data[2] << 8 & packet.data[3];
        EEPROM.update(encryptionKeyEEPROMAddress, encryptionKey);
        break;
      case PACKET_TYPE::SET_ADDRESS:
        EEPROM.update(thisNodeEEPROMAddress, packet.data[0]);
        resetFunc();
        break;
      case PACKET_TYPE::REPROGRAM:
      case PACKET_TYPE::RESTART:
        resetFunc();
        break;
      case PACKET_TYPE::NEW_NODE:
        // we should check the table of known nodes ... is this a known one ?
        // if so is its route valid/live ?
        // if yes return old address
        // if no, return a new one 
        bytesArray[0] = 0x50;
        bytesArray[1] = 0xff;
        bytesArray[2] = 0;
        bytesArray[3] = freeSlaveAddress;
        writePacket(packet.header.from_node, PACKET_TYPE::NEW_NODE_REPLY, bytesArray, 4);
        break;
      case PACKET_TYPE::MOVED_NODE:
        break;
    }
  }
}
