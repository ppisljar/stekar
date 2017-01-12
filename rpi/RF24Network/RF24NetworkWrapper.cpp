#include "RF24Network.h"
#include "RF24NetworkWrapper.h"

extern "C" {
CRF24Network* networkCreate() {
  return (CRF24Network*)(new RF24Network());
}

void networkDelete(CRF24Network* _network) {
  RF24Network* network = (RF24Network*)_network;
  delete network;
}

void networkBegin(CRF24Network* _network, uint8_t channel, uint16_t address) {
  RF24Network* network = (RF24Network*)_network;
  return network->begin(channel, address);
}

uint16_t networkUpdate(CRF24Network* _network) {
  RF24Network* network = (RF24Network*)_network;
  return network->update();
}

bool networkAvailable(CRF24Network* _network) {
  RF24Network* network = (RF24Network*)_network;
  return network->available();
}

uint16_t networkPeek(CRF24Network* _network, RF24NetworkHeader& header) {
  RF24Network* network = (RF24Network*)_network;
  return network->peek(header);
}

uint16_t networkRead(CRF24Network* _network, RF24NetworkHeader& header, void* data, uint16_t length) {
  RF24Network* network = (RF24Network*)_network;
  return network->read(header, data, length);
}

uint16_t networkWrite(CRF24Network* _network, RF24NetworkHeader& header, void* data, uint16_t length) {
  RF24Network* network = (RF24Network*)_network;
  return network->write(header, data, length);
}

bool networkIsValidAddress(CRF24Network* _network, uint16_t address) {
  RF24Network* network = (RF24Network*)_network;
  return network->is_valid_address(address);
}
}
