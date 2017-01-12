#include "RF24Network.h"

typedef void CRF24Network;

#ifdef __cplusplus
extern "C" {
#endif

  CRF24Network* networkCreate();
  void networkDelete(CRF24Network*);
  void networkBegin(CRF24Network*, uint8_t channel, uint16_t address);
  uint16_t networkUpdate(CRF24Network*);
  bool networkAvailable(CRF24Network*);
  uint16_t networkRead(CRF24Network*, RF24NetworkHeader&, void*, uint16_t length);
  uint16_t networkRead(CRF24Network*, RF24NetworkHeader&, void*, uint16_t length);
  uint16_t networkPeek(CRF24Network*, RF24NetworkHeader&);
  bool networkIsValidAddress(CRF24Network*, uint16_t address);

#ifdef __cplusplus
}
#endif
