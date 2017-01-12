var ref = require('ref');
var ffi = require('ffi');
var Struct = require('ref-struct')
var ArrayType = require('ref-array')

var RF24NetworkHeader = Struct({
  'from_node': 'uint16',
  'to_node': 'uint16',
  'id': 'uint8',
  'payload_length': 'uint8',
  'type': 'uint8',
  'reserved': 'uint8',
  'nextId': 'uint16',
  //'msgId': 'uint16'
});

var charArray = ArrayType('char');
var charArrayPtr = ref.refType(charArray);
var charPtr = ref.refType('char');
var headerPtr = ref.refType(RF24NetworkHeader);

var rf24lib = ffi.Library('librf24network', {
  "networkCreate": [ 'pointer', [ ] ],
  "networkBegin": [ 'void', [ 'pointer', 'uint8', 'uint16' ] ],
  "networkUpdate": [ 'uint16', [ 'pointer' ] ],
  "networkPeek": [ 'uint16', [ 'pointer', headerPtr ] ],
  "networkRead": [ 'uint16', [ 'pointer', headerPtr, charPtr, 'uint16' ] ],
  "networkWrite": [ 'uint16', [ 'pointer', headerPtr, charPtr, 'uint16' ] ],
  "networkIsValidAddress": [ 'bool', [ 'pointer', 'uint16' ] ],
  "networkAvailable": [ 'bool', [ 'pointer' ] ]
});

var RF24Network = function (address, channel = 90) {
  console.log('starting network ...');
  this.network = rf24lib.networkCreate();

  if (!rf24lib.networkIsValidAddress(this.network, address)) {
    console.log("RF24Network: invalid network address provided!");
    return;
  }

  rf24lib.networkBegin(this.network, channel, address);
  console.log('network is up ...');
};

RF24Network.prototype.available = function () {
  return rf24lib.networkAvailable(this.network);
}

RF24Network.prototype.update = function () {
  return rf24lib.networkUpdate(this.network);
}

RF24Network.prototype.peek = function (header) {
  return rf24lib.networkPeek(this.network, header);
}

RF24Network.prototype.read = function (header, data, length) {
  return rf24lib.networkRead(this.network, header, data, length);
}

RF24Network.prototype.write = function (header, data, length) {
  return rf24lib.networkWrite(this.network, header, data, length);
}

RF24Network.prototype.is_valid_address = function (address) {
  return rf24lib.networkIsValidAddress(this.network, address);
}

RF24Network.prototype.readPacket = function () {
  var header = ref.alloc(RF24NetworkHeader);

  this.peek(header);

  var headerData = header.deref();

  var data = new Buffer(headerData.payload_length);
  data.type = ref.types.char;

  this.read(header, data, headerData.payload_length);

  return { header: headerData, data: data };
}

RF24Network.prototype.writePacket = function (address, type, data) {
  var header = RF24NetworkHeader();
  header.to_node = address;
  header.type = type;

  var buffer = new Buffer(data.length + 1);
  buffer.type = ref.types.char;

  for (var i = 0; i < data.length; i++) {
    buffer[i] = typeof(data) === 'string' ? data.charCodeAt(i) : data[i];
  }
  buffer[data.length] = 0;

  this.update();

  console.log(header);
  console.log(buffer);
  
  var result = this.write(header.ref(), buffer, data.length);
  console.log(buffer);
  return result;
}



console.log('running tests ...');

var network = new RF24Network(1);

console.log('sending a packet back ...'); 
network.writePacket(0, 0, "112233445");

console.log('sending a packet back ...'); 
network.writePacket(0, 0, "112233445"); 

var func = function() {
  network.update();
  if (network.available()) {
    console.log('packet is available');
    var packet = network.readPacket();
    console.log(packet);

    console.log('sending a packet back ...'); 
    network.writePacket(0, 0, "112233445");
  }
  setTimeout(func, 100);
}
setTimeout(func, 0);


while(0) {} // prevent from exiting
