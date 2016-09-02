
#include "RtMidi.h"
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <iostream>


using std::vector;


unsigned char snm1[9] = { 0xf0, 0x42, 0x40, 0x6e, 0x08, 0x00, 0x00, 0x01, 0xf7 };

unsigned char snm2[50] = { 0xf0, 0x42, 0x40, 0x6e, 0x08, 0x3f, 0x2a, 0x00, 0x00, 0x05, 0x05, 0x05,      \
                           0x7f, 0x7e, 0x7f, 0x7f, 0x03, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,      \
                           0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x01, 0x02, 0x03,      \
                           0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,      \
                           0x10, 0xf7 };

unsigned char snm3[18] = { 0xf0, 0x42, 0x40, 0x6e, 0x08, 0x3f, 0x0a, 0x01, 0x00, \
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x45, 0x59, 0xf7 };


void mycallback( double deltatime, std::vector< unsigned char > *message, void *userData ){

  unsigned int nBytes = message->size();

  for ( unsigned int i=0; i<nBytes; i++ )

   printf( "%2x, ", (int)message->at(i) );
  
  if ( nBytes > 0 )
    
    printf("\n");

}


int main( int argc, char **argv ){


  RtMidiOut *midiout = new RtMidiOut();

  RtMidiIn  *midiin  = new RtMidiIn();


  vector<unsigned char> message;


  unsigned int noports = midiout->getPortCount();

  unsigned int niports = midiin->getPortCount();

  printf("%d output ports available.\n", noports);

  midiout->openPort( 1 );

  midiin->openPort( 1 );

  midiin->setCallback( mycallback );

  midiin->ignoreTypes( false, true, true );  
  

  for( int i = 0; i < 9; i++ ){

    message.push_back( snm1[i] ); 

  }

  for( int i = 0; i < 50; i++ ){

    message.push_back( snm2[i] ); 

  }

  for( int i = 0; i < 18; i++ ){

    message.push_back( snm3[i] ); 

  }

  midiout->sendMessage( &message );  

  message.clear();


  while( 1 ) usleep( 1000 );


  // Clean up
  delete midiout;


  return 0;


}
