/* 
	Stekar.h - Library for Stekar
	Created by mr. Solata 2017
	All rights reserved
*/

#ifndef StekarAudio_h
#define StekarAudio_h

#include "Arduino.h"
#include "stekar.h"


class StekarAudio
{
  public:
    StekarAudio();
    void init(Stekar* stekar, byte adc);
    void update();
    void run();
    void stop();
    void send(char* data, int length);
    
  private:
    Stekar* stekar;
};

#endif