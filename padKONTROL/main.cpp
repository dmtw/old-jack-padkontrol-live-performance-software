
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sndfile.h>


// jack stuff

#include <jack/jack.h>
#include <jack/transport.h>
#include <jack/midiport.h>

typedef jack_default_audio_sample_t sample_t;

jack_client_t *client;
jack_port_t *output_port;
jack_port_t *input_port;


unsigned int sampc = 0;

unsigned int sampn = 2000;


// Enter native mode: f0 42 40 6e 08 00 00 01 F7

unsigned char snm1[9] = { 0xf0, 0x42, 0x40, 0x6e, 0x08, 0x00, 0x00, 0x01, 0xf7 };

// F0 42 40 6E 08 3F 2A 00 00 05 05 05 7F 7E 7F 7F 03 0A 0A 0A 0A 0A 0A 0A 0A 
// 0A 0A 0A 0A 0A 0A 0A 0A 01 02 03 04 05 06 07 08 09 0A 0B 0C 0d 0E 0F 10 F7

unsigned char snm2[50] = { 0xf0, 0x42, 0x40, 0x6e, 0x08, 0x3f, 0x2a, 0x00, 0x00, 0x05, 0x05, 0x05,	\
                           0x7f, 0x7e, 0x7f, 0x7f, 0x03, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,	\
                           0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x01, 0x02, 0x03,	\
                           0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,	\
                           0x10, 0xf7 };

// F0 42 40 6E 08 3F 0A 01 00 00 00 00 00 00 29 29 29 F7

unsigned char snm3[18] = { 0xf0, 0x42, 0x40, 0x6e, 0x08, 0x3f, 0x0a, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                           0x53, 0x45, 0x59, 0xf7 };


typedef enum {

  IS_PADKONTROL_CONNECTED,
  START_NATIVE_MODE_1,
  START_NATIVE_MODE_2,
  START_NATIVE_MODE_3,
  RUNNING

} midi_out_mode;


midi_out_mode mostatus = IS_PADKONTROL_CONNECTED;

bool padkontrol_connected = false;


void midi_in_process( jack_nframes_t nframes,
		      void* arg ){

  void* port_buf = jack_port_get_buffer( input_port, nframes );

  jack_midi_event_t in_event;

  jack_nframes_t event_count = jack_midi_get_event_count( port_buf );


  for( int i = 0; i < event_count; i++ ){


    jack_midi_event_get( &in_event, port_buf, i );

    for( int i = 0; i < in_event.size; i++ ){

      if( in_event.size == 9 )

	if( in_event.buffer[5] != 0x40 )
	  
	  printf( "%2x ", in_event.buffer[i] );

    }

    if( in_event.size == 9 )
      
      if( in_event.buffer[5] != 0x40 )
      
	printf( "\n" );

  }

}


void midi_out_process( jack_nframes_t nframes,
		   void* arg ){

  void* port_buf = jack_port_get_buffer( output_port, nframes );

  unsigned char* buffer;

  jack_midi_clear_buffer( port_buf );


  switch( mostatus ){

  case IS_PADKONTROL_CONNECTED:

    if( padkontrol_connected ){

      printf("Successfully connected to padKONTROL.\n");

      mostatus = START_NATIVE_MODE_1;

    }

    break;

  case START_NATIVE_MODE_1:
    
    buffer = jack_midi_event_reserve( port_buf, 0, 9 );

    memcpy( buffer, snm1, 9 );

    printf("Native Mode Start 1/3\n");

    mostatus = START_NATIVE_MODE_2;
    
    break;

  case START_NATIVE_MODE_2:

    buffer = jack_midi_event_reserve( port_buf, 0, 50 );

    memcpy( buffer, snm2, 50 );

    printf("Native Mode Start 2/3\n");

    mostatus = START_NATIVE_MODE_3;

    break;

  case START_NATIVE_MODE_3:

    buffer = jack_midi_event_reserve( port_buf, 0, 18 );

    memcpy( buffer, snm3, 18 );

    printf("Native Mode Start 3/3\n");

    mostatus = RUNNING;

    break;

  case RUNNING:

    break;

  }

}


void audio_process( jack_nframes_t nframes,
		    void* arg ){

  

}


int process( jack_nframes_t nframes,
	     void* arg ){

  midi_in_process( nframes, arg );

  midi_out_process( nframes, arg );

  audio_process( nframes, arg );

  return 0;

}

void error (const char *desc){

  fprintf (stderr, "JACK error: %s\n", desc);

}

void jack_shutdown (void *arg){

  exit (1);

}

int main( int argc,
	  char** argv ){



  // Jack stuff


  const char** ports;



  client = jack_client_open( "padKONTROL Test", JackNullOption, NULL, NULL );

  if( client == NULL ){

    printf("Error starting client. Is the Jack server running?");

    exit( 1 );

  }


  jack_set_process_callback( client, process, NULL );


  jack_on_shutdown( client, jack_shutdown, NULL );

  
  input_port = jack_port_register( client,
				   "SYSEX IN",
				   JACK_DEFAULT_MIDI_TYPE,
				   JackPortIsInput,
				   0 );

  output_port = jack_port_register( client,
				    "SYSEX OUT",
				    JACK_DEFAULT_MIDI_TYPE,
				    JackPortIsOutput,
				    0 );

  if( input_port == NULL || output_port == NULL ){

    printf("Error opening ports");

    exit( 1 );

  }


  if( jack_activate( client ) ){

    printf("Error during client activation");

    exit( 1 );

  }


  
  // Connect to output ports

  ports = jack_get_ports( client,
			  NULL,
			  NULL,
			  JackPortIsPhysical | JackPortIsInput );

  if( ports == NULL ){

    printf("Could not find any ports to send data to.\n");

    exit( 1 );

  }

  int i = 0;

  while( ports[i] != NULL ){

    if( jack_connect( client, jack_port_name( output_port ), ports[i] ) ){

      printf("Could not connect to port #%d.\n", i );

    }

    i++;

  }

  
  free( ports );

  
  // Connect to input ports

  ports = jack_get_ports( client,
			  NULL,
			  NULL,
			  JackPortIsPhysical | JackPortIsOutput );

  if( ports == NULL ){

    printf("Could not find any ports to send data to.\n");

    exit( 1 );

  }


  i = 0;


  while( ports[i] != NULL ){

    if( jack_connect( client, ports[i], jack_port_name( input_port ) ) ){

      printf("Could not connect to output port #%d.\n", i );

    }

    i++;

  }

  free( ports );

  // ---

  padkontrol_connected = true;


  // Jack stuff again

  for(;;)
    sleep (1);


  jack_client_close (client);

  exit( 0 );

}
