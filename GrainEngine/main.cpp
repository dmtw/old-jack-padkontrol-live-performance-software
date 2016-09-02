
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sndfile.h>
#include "blackbox.h"
#include "grainengine.h"


// jack stuff

#include <jack/jack.h>
#include <jack/transport.h>

typedef jack_default_audio_sample_t sample_t;

jack_client_t *client;
jack_port_t *output_port;
jack_port_t *input_port;


// my stuff

unsigned long sr = 0;

GrainEngine* se = NULL;

Phasor* phas = NULL;


// more jack stuff

int srate (jack_nframes_t nframes, void *arg){

  sr = nframes;

  //if( se == NULL ) se = new GrainEngine( sr );

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

    if( se != NULL && phas != NULL ){
      
      float phase = phas->Process();

      se->SetPlaybackPos( phase );

      sample = se->Process();

    }

    
    out[i] = sample;


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


  // My stuff

  SNDFILE* sndfile = NULL;  

  float* mysound = NULL;

  float* frame = NULL;

  SF_INFO sinfo;

  sinfo.format = 0;



  if( argc < 4 ){

    printf("Usage: grainengine myfile.wav 1.0 20\n");

    exit( 1 );

  }

  sndfile = sf_open( argv[1], SFM_READ, &sinfo );

  if( sndfile == NULL ){

    printf("Error opening file");

    exit( 1 );

  }

  mysound = (float*)malloc( sizeof( float ) * sinfo.frames );
  
  frame = (float*)malloc( sizeof( float ) * sinfo.channels );

  for( int i = 0; i < sinfo.frames; i++ ){

    sf_read_float( sndfile, frame, sinfo.channels );

    mysound[i] = frame[0];

    for( int j = 1; j < sinfo.channels; j++ )

      mysound[i] += frame[j] / 2;
    
  }


  // Jack stuff

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


  // My stuff again

  while( sr == 0 );
  
  se = new GrainEngine( sr,
			sinfo.samplerate,
			sinfo.frames,
			mysound );

  se->SetGrainSize( atoi(argv[3]) );


  phas = new Phasor( sr );

  phas->SetFrequency( atof(argv[2]) );
  

  // Jack stuff again

  for(;;)
    sleep (1);


  jack_client_close (client);

  exit( 0 );

}
