
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include "blackbox.h"

#include <jack/jack.h>
#include <jack/transport.h>

typedef jack_default_audio_sample_t sample_t;

jack_client_t *client;
jack_port_t *output_port;
jack_port_t *input_port;

unsigned long sr;

SinOsc* osc = NULL;

float delayline[5000];

int delaylineindex = 0;


int srate (jack_nframes_t nframes, void *arg){

  sr = nframes;

  if( osc == NULL ) osc = new SinOsc( sr );

  osc->SetFrequency( 111.0 );

  return 0;

}

int process( jack_nframes_t nframes,
	     void* arg ){

  int i = 0;

  jack_default_audio_sample_t *out = 
    (jack_default_audio_sample_t *) 
    jack_port_get_buffer (output_port, nframes);

  jack_default_audio_sample_t *in = 
    (jack_default_audio_sample_t *) 
    jack_port_get_buffer (input_port, nframes);

  for( i = 0; i < nframes; i++ ){

    float sample = 0;

    if( osc != NULL ){

      osc->SetFrequency( 440.0 );

      sample = osc->Process() * in[i];

    }

    sample = sample + delayline[ delaylineindex ];

    delayline[ delaylineindex ] = sample;

    out[i] = sample;


    delayline[ delaylineindex ] = sample * 0.7;

    delaylineindex++;

    if( delaylineindex >= 5000 ) delaylineindex = 0;

  }

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

  client = jack_client_open( "test", JackNullOption, NULL, NULL );

  if( client == NULL ){

    printf("Error starting client. Is the Jack server running?");

    exit( 1 );

  }


  jack_set_process_callback( client, process, NULL );


  jack_set_sample_rate_callback (client, srate, 0);


  jack_on_shutdown( client, jack_shutdown, NULL );

  
  input_port = jack_port_register( client,
				   "input",
				   JACK_DEFAULT_AUDIO_TYPE,
				   JackPortIsInput,
				   0 );

  output_port = jack_port_register( client,
				    "output",
				    JACK_DEFAULT_AUDIO_TYPE,
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



  for(;;)
    sleep (1);


  jack_client_close (client);

  exit( 0 );

}
