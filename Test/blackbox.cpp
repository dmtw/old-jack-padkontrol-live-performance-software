
#include "blackbox.h"
#include <math.h>

SinOsc::SinOsc( int samplerate ){

  sr = samplerate;

  this->phase = 0;

  this->delta = 0;

}

float SinOsc::Process(){
  
  float sample = 0;

  sample = sin( this->phase * 2 * M_PI );

  this->phase += this->delta;

  while( this->phase >= 1.0 )

    this->phase -= 1.0;

  return sample;
  
}

void SinOsc::SetFrequency( float frequency ){

  this->delta = 1.0 / (((double)this->sr) / ((double)frequency));

}
