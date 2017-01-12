/* 
	Stekar.h - Library for Stekar
	Created by mr. Solata 2017
	All rights reserved
*/

#ifndef Stekar_h
#define Stekar_h
//#define DEBUG

#include "Arduino.h"

struct func_setting_t {
  char* name;
  byte(* func) (byte a, byte b);
};

struct sensor_setting_t {
  char* name;
  int* var;
};

struct byte_setting_t {
  char* name;
  byte* var;
};

class Stekar
{
  public:
    Stekar();
    void init();
    void update();
    void send(char* data, int length);
    sensor_setting_t sensor[8] = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}};
	func_setting_t func[8] = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}};
	byte_setting_t byte[9] = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {(char*)"flags", 0}};

  private:
  	void printStatus(uint16_t address);
    int _pin;
};

#endif