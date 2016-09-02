
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sndfile.h>
#include <jack/jack.h>
#include <jack/transport.h>

#include <string>
#include <vector>

#include "RtMidi.h"

#include "blackbox.h"
#include "grainengine.h"
#include "soundengine.h"


#define PADS	0x45
#define BUTTONS	0x48
#define WHEEL	0x43


typedef jack_default_audio_sample_t sample_t;


struct appdata {


  struct {


    int SampleRate;

    SoundEngine* Engine;

    bool Ready;


  } SoundEngineData;


  struct {


    jack_client_t *Client;
    

    jack_port_t *AudioIn;
    
    jack_port_t *AudioOut;


  } JackData;


  struct {


    RtMidiIn *MidiIn;

    RtMidiOut *MidiOut;

    vector<unsigned char> MidiMessage;

    bool IsConnected;


    unsigned char LightData[8];


    unsigned char OutPadData[16];

    unsigned char OutNumData[3];

    unsigned char OutBtnData[19];


    unsigned char InPadData[16][2]; // [n][0]: Press/Release, [n][1]: Velocity

    unsigned char InBtnData[19][2];

    unsigned char InKnbData[2];


    int SettingSectionState;

    int ParameterSectionState;

    int XYPadSectionState;

    int TriggerPadSectionState;

    int RotaryDelta;


  } PadKontrolData;


  struct {


    int BPM;
    
    int SelectedTrack;

    unsigned char TrackEditMode;

    bool SetTrackEdit;

    
  } SequencerData;


  bool Exit;


} AppData;


typedef enum buttondatatypes {


  BUTTON_SCENE   = 0,
  BUTTON_MESSAGE = 1,
  BUTTON_SETTING = 2,
  BUTTON_NOTECC  = 3,
  BUTTON_MIDICH  = 4,
  BUTTON_SWTYPE  = 5,
  BUTTON_RELVAL  = 6,
  BUTTON_VELCTY  = 7,
  BUTTON_PORT    = 8,
  BUTTON_FIXVEL  = 9,
  BUTTON_PRGCHG  = 10,
  BUTTON_X       = 11,
  BUTTON_Y       = 12,
  BUTTON_KNOB1   = 13,
  BUTTON_KNOB2   = 14,
  BUTTON_PEDAL   = 15,
  BUTTON_ROLL    = 16,
  BUTTON_FLAM    = 17,
  BUTTON_HOLD    = 18
 

} ButtonDataTypes;


unsigned char snm1[9] = { 0xf0, 0x42, 0x40, 0x6e, 0x08, 0x00, 0x00, 0x01, 0xf7 };

// F0 42 40 6E 08 3F 2A 00 00 05 05 05 7F 7E 7F 7F 03 0A 0A 0A 0A 0A 0A 0A 0A
// 0A 0A 0A 0A 0A 0A 0A 0A 01 02 03 04 05 06 07 08 09 0A 0B 0C 0d 0E 0F 10 F7

unsigned char snm2[50] = { 0xf0, 0x42, 0x40, 0x6e, 0x08, 0x3f, 0x2a, 0x00, 0x00, 0x05, 0x05, 0x05,      \
                           0x7f, 0x7e, 0x7f, 0x7f, 0x03, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,      \
                           0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x01, 0x02, 0x03,      \
                           0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,      \
                           0x10, 0xf7 };

// F0 42 40 6E 08 3F 0A 01 00 00 00 00 00 00 29 29 29 F7

unsigned char snm3[18] = { 0xf0, 0x42, 0x40, 0x6e, 0x08, 0x3f, 0x0a, 0x01, 0x00, \
			   0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x45, 0x59, 0xf7 };


void clearMessage();

void addToMessage( unsigned char* data, int size );

void sendMessage();


void MyError( const char *desc );

void SysexOutProcess();

void SysexInProcess( std::vector<unsigned char> *message );

int  Process( jack_nframes_t nframes, void *arg);

int  GetSampleRate( jack_nframes_t nframes, void *arg );

void Error( const char *desc );

void JackShutdown( void *arg );

void Init( int argc, char **argv );


int main( int argc,
	  char **argv ){


  Init( argc, argv );


  // Don't exit
  
  while( AppData.Exit == false ){

    usleep( 20000 );

    SysexOutProcess();

  }


  // Exit

  jack_client_close( AppData.JackData.Client );

  exit( 0 );
  

}


void clearMessage(){

  AppData.PadKontrolData.MidiMessage.clear();

}

void addToMessage( unsigned char* data, int size ){

  for( int i = 0; i < size; i++ )

    AppData.PadKontrolData.MidiMessage.push_back( data[i] );

}

void sendMessage(){

  AppData.PadKontrolData.MidiOut->sendMessage( &AppData.PadKontrolData.MidiMessage );

}



void MyError( const char *desc ){

  printf("%s\n", desc);

  exit( 1 );

}


void AudioInProcess( jack_nframes_t nframes, void *arg ){

}


void AudioOutProcess( jack_nframes_t nframes, void *arg ){

  
  sample_t *out = (sample_t*)jack_port_get_buffer( AppData.JackData.AudioOut, nframes );

  for( int i = 0; i < nframes; i++ ){

    out[i] = AppData.SoundEngineData.Engine->Process();

  }


}


int Process( jack_nframes_t nframes, void *arg ){


  if( AppData.SoundEngineData.Ready ){
    
    AudioInProcess( nframes, arg );
    
    AudioOutProcess( nframes, arg );
    
  }


  return 0;


}


int  GetSampleRate( jack_nframes_t nframes, void *arg ){

  if( AppData.SoundEngineData.SampleRate == 0 ){

    AppData.SoundEngineData.SampleRate = nframes;

  } else MyError( "Cannot change sample rate while running!" );

  return 0;

}


void Error( const char *desc ){

  printf( "%s\n", desc );

}


void JackShutdown( void *arg ){

  exit( 0 );

}


void SetNum( unsigned int num ){


  int val = num;


  for( int i = 0; i < 3; i++ ){

    AppData.PadKontrolData.OutNumData[i] = ( val % 10 ) + 0x30;

    val = val / 10;

  }


}


int GetPadNum( int a, int b ){

	int n = -1;

	for( int i = b; i >= a; i-- ){

		if( AppData.PadKontrolData.InPadData[i][0] > 0 )

			n = i;

	}

	for( int i = b; i >= a; i-- ){

		if( AppData.PadKontrolData.InPadData[i][0] > 1 )

			n = i;

	}

	return n;

}


// --- Begin Input Finite State Machines --- //


enum{

	SSSM_BPMGLOBAL	= 0,
	SSSM_BPMPRCNT	= 1,
	SSSM_GRAINSIZE	= 2

};

void SettingSectionSMI(){

	int temp = 0;


	AppData.PadKontrolData.SettingSectionState = \

			( AppData.PadKontrolData.InBtnData[ BUTTON_X		][0] > 0 ? 1 : 0 ) << 0 | \
			( AppData.PadKontrolData.InBtnData[ BUTTON_Y		][0] > 0 ? 1 : 0 ) << 1 | \
			( AppData.PadKontrolData.InBtnData[ BUTTON_PEDAL	][0] > 0 ? 1 : 0 ) << 2 | \
			( AppData.PadKontrolData.InBtnData[ BUTTON_SETTING	][0] > 0 ? 1 : 0 ) << 3 ;


	switch( AppData.PadKontrolData.SettingSectionState ){

	case SSSM_BPMGLOBAL:

		AppData.SequencerData.BPM += AppData.PadKontrolData.RotaryDelta * 5;

		if( AppData.SequencerData.BPM < 0 )

			AppData.SequencerData.BPM = 0;

		AppData.SoundEngineData.Engine->SetTempo( AppData.SequencerData.BPM );

		SetNum( AppData.SequencerData.BPM );

		break;

	case SSSM_BPMPRCNT:

		if( AppData.SoundEngineData.Engine->Tracks.size() )

			temp =	AppData.SoundEngineData.Engine->\
					Tracks[ AppData.SequencerData.SelectedTrack ]->TempoPercent;

			temp += AppData.PadKontrolData.RotaryDelta;

			if( temp < 0 ) temp = 0;

			AppData.SoundEngineData.Engine->\
			Tracks[ AppData.SequencerData.SelectedTrack ]->\
			SetTempoPercent( temp );

			SetNum( temp );

		break;

	case SSSM_GRAINSIZE:

		if( AppData.SoundEngineData.Engine->Tracks.size() )

			temp =	AppData.SoundEngineData.Engine->\
					Tracks[ AppData.SequencerData.SelectedTrack ]->GrainSize;

			temp += AppData.PadKontrolData.RotaryDelta;

			if( temp < 1 ) temp = 1;

			AppData.SoundEngineData.Engine->\
			Tracks[ AppData.SequencerData.SelectedTrack ]->\
			SetGrainSize( temp );

			SetNum( temp );

		break;

	}

}

void SettingSectionSMO(){

	AppData.PadKontrolData.OutBtnData[ BUTTON_X		]	= ( AppData.PadKontrolData.SettingSectionState >> 0 ) & 1;
	AppData.PadKontrolData.OutBtnData[ BUTTON_Y		]	= ( AppData.PadKontrolData.SettingSectionState >> 1 ) & 1;
	AppData.PadKontrolData.OutBtnData[ BUTTON_PEDAL	]	= ( AppData.PadKontrolData.SettingSectionState >> 2 ) & 1;
	AppData.PadKontrolData.OutBtnData[ BUTTON_SETTING]	= ( AppData.PadKontrolData.SettingSectionState >> 3 ) & 1;


	switch( AppData.PadKontrolData.SettingSectionState ){

	case SSSM_BPMGLOBAL:
		break;

	case SSSM_BPMPRCNT:
		break;

	case SSSM_GRAINSIZE:
		break;

	}

}

void ParameterSectionSMI(){

}

void ParameterSectionSMO(){

}

void XYPadSectionSMI(){

}

void XYPadSectionSMO(){

}

enum{

	TPSM_DEFAULT	= 0,
	TPSM_MUTE		= 3,
	TPSM_SOLO		= 12,
	TPSM_CUE		= 6,
	TPSM_SELPAGE	= 7,
	TPSM_SELTRACK	= 1,
	TPSM_STEPSKIP	= 5,
	TPSM_LASTSTEP	= 4,
	TPSM_STARTSTEP	= 2,
	TPSM_DIVISIONS	= 9

};

void TriggerPadSectionSMI(){


	AppData.PadKontrolData.TriggerPadSectionState = ( AppData.PadKontrolData.InBtnData[BUTTON_SCENE][0]		> 0 ? 1 : 0 ) << 0 |
													( AppData.PadKontrolData.InBtnData[BUTTON_MESSAGE][0]	> 0 ? 1 : 0 ) << 1 |
													( AppData.PadKontrolData.InBtnData[BUTTON_FIXVEL][0]	> 0 ? 1 : 0 ) << 2 |
													( AppData.PadKontrolData.InBtnData[BUTTON_PRGCHG][0]	> 0 ? 1 : 0 ) << 3;

	switch( AppData.PadKontrolData.TriggerPadSectionState ){

	case TPSM_DEFAULT:

		if( GetPadNum( 0, 15 ) >= 0 ){

			if( AppData.SoundEngineData.Engine->Tracks.size() > AppData.SequencerData.SelectedTrack ){

					AppData.SoundEngineData.Engine->Tracks[ AppData.SequencerData.SelectedTrack ]->SetStep( GetPadNum( 0, 15 ) );

			}

		}

		break;

	case TPSM_MUTE:
		break;

	case TPSM_SOLO:
		break;

	case TPSM_CUE:
		break;

	case TPSM_SELPAGE:
		break;

	case TPSM_SELTRACK:

		if( GetPadNum( 0, 15 ) >= 0 && GetPadNum( 0, 15 ) < AppData.SoundEngineData.Engine->Tracks.size() )

			AppData.SequencerData.SelectedTrack = GetPadNum( 0, 15 );

		break;

	case TPSM_STEPSKIP:
		break;

	case TPSM_LASTSTEP:

		if( AppData.SoundEngineData.Engine->Tracks.size() > AppData.SequencerData.SelectedTrack ){

			if( GetPadNum( 0, 15 ) >= 0 ){

				AppData.SoundEngineData.Engine->\
				Tracks[ AppData.SequencerData.SelectedTrack ]->\
				SetLastStep( GetPadNum( 0, 15 ) );

			}

		}

		break;

	case TPSM_STARTSTEP:

		if( GetPadNum( 0, 15 ) >= 0 )

			AppData.SoundEngineData.Engine->Tracks[ AppData.SequencerData.SelectedTrack ]->StartStep = GetPadNum( 0, 15 );

		break;

	case TPSM_DIVISIONS:
		break;

	}

}

void TriggerPadSectionSMO(){

	int temp	= 0;
	int cstep	= 0;
	int divs	= 0;
	int sstep	= 0;


	AppData.PadKontrolData.OutBtnData[ BUTTON_SCENE ]	= ( AppData.PadKontrolData.TriggerPadSectionState >> 0 ) & 1;
	AppData.PadKontrolData.OutBtnData[ BUTTON_MESSAGE ]	= ( AppData.PadKontrolData.TriggerPadSectionState >> 1 ) & 1;
	AppData.PadKontrolData.OutBtnData[ BUTTON_FIXVEL ]	= ( AppData.PadKontrolData.TriggerPadSectionState >> 2 ) & 1;
	AppData.PadKontrolData.OutBtnData[ BUTTON_PRGCHG ]	= ( AppData.PadKontrolData.TriggerPadSectionState >> 3 ) & 1;

	switch( AppData.PadKontrolData.TriggerPadSectionState ){

	case TPSM_DEFAULT:

		if( AppData.SoundEngineData.Engine->Tracks.size() > AppData.SequencerData.SelectedTrack ){

			cstep	= AppData.SoundEngineData.Engine->Tracks[ AppData.SequencerData.SelectedTrack ]->CurrentStep;
			divs	= AppData.SoundEngineData.Engine->Tracks[ AppData.SequencerData.SelectedTrack ]->Divisions;
			sstep	= AppData.SoundEngineData.Engine->Tracks[ AppData.SequencerData.SelectedTrack ]->StartStep;

			AppData.PadKontrolData.OutPadData[ ( sstep + cstep ) % divs ] = 1;

		}

		break;

	case TPSM_MUTE:
		break;

	case TPSM_SOLO:
		break;

	case TPSM_CUE:
		break;

	case TPSM_SELPAGE:
		break;

	case TPSM_SELTRACK:

		if( AppData.SoundEngineData.Engine->Tracks.size() > AppData.SequencerData.SelectedTrack ){

			AppData.PadKontrolData.OutPadData[ AppData.SequencerData.SelectedTrack ] = 1;

		}

		break;

	case TPSM_STEPSKIP:
		break;

	case TPSM_LASTSTEP:

		for( int i = 0; i <= AppData.SoundEngineData.Engine->Tracks[ AppData.SequencerData.SelectedTrack ]->LastStep; i++)

		AppData.PadKontrolData.OutPadData[ i ] = 1;

		break;

	case TPSM_STARTSTEP:

		temp = AppData.SoundEngineData.Engine->Tracks[ AppData.SequencerData.SelectedTrack ]->StartStep;

		AppData.PadKontrolData.OutPadData[ temp ] = 1;

		break;

	case TPSM_DIVISIONS:
		break;

	}

}


// --- End Input Finite State Machines --- //


void ProcessOutput(){

	SettingSectionSMO();

	ParameterSectionSMO();

	XYPadSectionSMO();

	TriggerPadSectionSMO();

}


// All lights off F0 42 40 6E 08 3F 0A 01 00 00 00 00 00 00 29 29 29 F7

unsigned char setlights[18] = { 0xf0, 0x42, 0x40, 0x6e, 0x08, 0x3f, 0x0a, 0x01, \
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x29, 0x29, 0xf7 };


void SysexOutProcess(){


	memset( AppData.PadKontrolData.OutBtnData, 0, 19 );

	memset( AppData.PadKontrolData.OutPadData, 0, 16 );

	memset( AppData.PadKontrolData.LightData, 0, 5 );


	ProcessOutput();


	for( int i = 0; i < 19; i++ )

	AppData.PadKontrolData.LightData[ ( i + 16 ) / 7 ] |=

	  ( AppData.PadKontrolData.OutBtnData[i] > 0 ? 1 : 0 ) << ( ( i + 16 ) % 7 );


	for( int i = 0; i < 16; i++ )

	AppData.PadKontrolData.LightData[ i / 7 ] |=

	  ( AppData.PadKontrolData.OutPadData[i] > 0 ? 1 : 0 ) << ( i % 7 );


	//


	clearMessage();

	addToMessage( setlights, 18 );

	AppData.PadKontrolData.MidiMessage[8 ] = AppData.PadKontrolData.LightData[0];
	AppData.PadKontrolData.MidiMessage[9 ] = AppData.PadKontrolData.LightData[1];
	AppData.PadKontrolData.MidiMessage[10] = AppData.PadKontrolData.LightData[2];
	AppData.PadKontrolData.MidiMessage[11] = AppData.PadKontrolData.LightData[3];
	AppData.PadKontrolData.MidiMessage[12] = AppData.PadKontrolData.LightData[4];

	AppData.PadKontrolData.MidiMessage[14] = AppData.PadKontrolData.OutNumData[0];
	AppData.PadKontrolData.MidiMessage[15] = AppData.PadKontrolData.OutNumData[1];
	AppData.PadKontrolData.MidiMessage[16] = AppData.PadKontrolData.OutNumData[2];

	sendMessage();


}


void StoreInput( std::vector<unsigned char> *message ){
  

  // F0 42 40 6E 08 -- PadKONTROL Sysex Message 

  if( message->at( 0 ) == 0xf0 && \
      message->at( 1 ) == 0x42 && \
      message->at( 2 ) == 0x40 && \
      message->at( 3 ) == 0x6e && \
      message->at( 4 ) == 0x08    ){
    
    
    switch( message->at( 5 ) ){

      int val;
      
      int index;
      

    case PADS:
      
      if( message->at( 6 ) & 0x40 ){

    	  if( AppData.PadKontrolData.InPadData[ message->at( 6 ) - 0x40 ][0] < 2 )

    		  AppData.PadKontrolData.InPadData[ message->at( 6 ) - 0x40 ][0]++;

    	  AppData.PadKontrolData.InPadData[ message->at( 6 ) - 0x40 ][0] = message->at( 7 );

      } else {

    	  AppData.PadKontrolData.InPadData[ message->at( 6 ) ][0] = 0;

    	  AppData.PadKontrolData.InPadData[ message->at( 6 ) ][1] = 0;

      }

      break;
      
      
    case BUTTONS: // ( Or pad?)

    	if( message->at( 6 ) < 19 ){

			if( message->at( 7 ) > 0 && AppData.PadKontrolData.InBtnData[ message->at(6) ][0] < 2 ){

				AppData.PadKontrolData.InBtnData[ message->at(6) ][0]++;

			} else {

				AppData.PadKontrolData.InBtnData[ message->at(6) ][0] = 0;

			}

    	}

      break;
      

    case WHEEL:
      
		AppData.PadKontrolData.RotaryDelta += ( message->at(7) == 0x01 ? 1 : -1 );

		break;

	  }

  }
    
}


void UpdateInBtnsPadsRotary(){

	for( int i = 0; i < 16; i++ )

		if( AppData.PadKontrolData.InPadData[i][0] < 2 && \
			AppData.PadKontrolData.InPadData[i][0] > 0 )

			AppData.PadKontrolData.InPadData[i][0]++;

	for( int i = 0; i < 19; i++ )

		if( AppData.PadKontrolData.InBtnData[i][0] < 2 && \
			AppData.PadKontrolData.InBtnData[i][0] > 0 )

			AppData.PadKontrolData.InBtnData[i][0]++;

	AppData.PadKontrolData.RotaryDelta = 0;

}


void ProcessInput(){

	SettingSectionSMI();

	ParameterSectionSMI();

	XYPadSectionSMI();

	TriggerPadSectionSMI();


	UpdateInBtnsPadsRotary();

}


void SysexInProcess( std::vector<unsigned char> *message ){

  StoreInput( message );

  ProcessInput();

}


void MidiCallback( double deltatime, std::vector< unsigned char > *message, void *userData ){

/*
  unsigned int nBytes = message->size();

  for ( unsigned int i=0; i<nBytes; i++ )

    printf( "%2x, ", (int)message->at(i) );  // Some debug code

  if ( nBytes > 0 )

    printf("\n");
*/

  SysexInProcess( message );


}


void Init( int argc, char **argv ){


  // Check args

  if( argc < 4 ) MyError("Useage: live <bpm> <grainsize> <firstfile.wav> ... <lastfile.wav>");


  // Init AppData struct

  AppData.Exit = false;

  AppData.PadKontrolData.IsConnected = false;

  AppData.SoundEngineData.SampleRate = 0;

  AppData.SoundEngineData.Engine = NULL;

  AppData.SoundEngineData.Ready = false;

  // --- //

  AppData.PadKontrolData.SettingSectionState = 0;

  AppData.PadKontrolData.ParameterSectionState = 0;

  AppData.PadKontrolData.XYPadSectionState = 0;

  AppData.PadKontrolData.TriggerPadSectionState = 0;

  AppData.PadKontrolData.RotaryDelta = 0;

  for( int i = 0; i < 8; i++ )

    AppData.PadKontrolData.LightData[i] = 0;

  // --- //

  AppData.SequencerData.BPM = atoi(argv[1]);
  
  AppData.SequencerData.SelectedTrack = 0;

  AppData.SequencerData.SetTrackEdit = false;


  // Make a new Jack client

  AppData.JackData.Client = jack_client_open( "LiveAlpha", JackNullOption, NULL, NULL );

  if( AppData.JackData.Client == NULL ) MyError( "Could not open client" );


  // Set Jack callbacks

  jack_set_sample_rate_callback( AppData.JackData.Client, GetSampleRate, NULL );

  jack_set_process_callback( AppData.JackData.Client, Process, NULL );

  jack_on_shutdown( AppData.JackData.Client, JackShutdown, NULL );

  jack_set_error_function( Error );


  // Set up ports

  AppData.JackData.AudioIn = jack_port_register( AppData.JackData.Client,
						 "Record",
						 JACK_DEFAULT_AUDIO_TYPE,
						 JackPortIsInput,
						 0 );

  AppData.JackData.AudioOut = jack_port_register( AppData.JackData.Client,
						  "Playback",
						  JACK_DEFAULT_AUDIO_TYPE,
						  JackPortIsOutput,
						  0 );

  if( AppData.JackData.AudioIn  == NULL || AppData.JackData.AudioOut == NULL )
    
    MyError( "Error opening ports." );
  

  // Activate client

  if( jack_activate( AppData.JackData.Client ) )

    MyError( "Unable to activate client." );


  // Connect to output ports                                                                              

  int i = 0;

  const char **ports = jack_get_ports( AppData.JackData.Client,
				 NULL,
				 "audio",
				 JackPortIsPhysical | JackPortIsOutput );

  if( ports == NULL ) MyError( "Could not find input audio ports." );

  while( ports[i] != NULL ){

    jack_connect( AppData.JackData.Client,
		  ports[i],
		  jack_port_name( AppData.JackData.AudioIn ) );

    i++;

  }

  free( ports );

  i = 0;

  ports = jack_get_ports( AppData.JackData.Client,
			  NULL,
			  "audio",
			  JackPortIsPhysical | JackPortIsInput );

  if( ports == NULL ) MyError( "Could not find input audio ports." );

  while( ports[i] != NULL ){

    jack_connect( AppData.JackData.Client,
		  jack_port_name( AppData.JackData.AudioOut ),
		  ports[i] );

    i++;

  }

  free( ports );

  
  // Configure rtmidi

  try {

    AppData.PadKontrolData.MidiIn  = new RtMidiIn();

  }

  catch ( RtError &error ) {

    error.printMessage();

    exit( EXIT_FAILURE );

  }


  try {

    AppData.PadKontrolData.MidiOut = new RtMidiOut();

  }
  
  catch ( RtError &error ) {
    
    error.printMessage();
    
    exit( EXIT_FAILURE );

  }
  

  unsigned int niports = AppData.PadKontrolData.MidiIn->getPortCount();

  unsigned int noports = AppData.PadKontrolData.MidiOut->getPortCount();

  
  bool connected = false;
  
  for( int i = 0; i < niports; i++ ){
    
    if( AppData.PadKontrolData.MidiIn->getPortName( i ).compare( std::string( "padKONTROL PORT A" ) ) == 0 ){
      
      AppData.PadKontrolData.MidiIn->openPort( i );
      
      connected = true;
      
    }
    
  }
  
  AppData.PadKontrolData.MidiIn->setCallback( MidiCallback );

  AppData.PadKontrolData.MidiIn->ignoreTypes( false, true, true );

  if( connected == false ) MyError("Could not connect to input MIDI port.");

  connected = false;
  
  for( int i = 0; i < noports; i++ ){

    if( AppData.PadKontrolData.MidiOut->getPortName( i ).compare( std::string( "padKONTROL CTRL" ) ) == 0 ){

      AppData.PadKontrolData.MidiOut->openPort( i );

      connected = true;

    }

  }
  
  if( connected == false ) MyError("Could not connect to output MIDI port.");
  

  // Set up padKONTROL

  clearMessage();

  addToMessage( snm1, 9 );

  addToMessage( snm2, 50 );

  addToMessage( snm3, 18 );

  sendMessage();


  // Allow open connection to padKONTROL

  AppData.PadKontrolData.IsConnected = true;


  // Set up sound engine

  while( AppData.SoundEngineData.SampleRate == 0 )

    usleep( 1000 );

  AppData.SoundEngineData.Engine = new SoundEngine( AppData.SoundEngineData.SampleRate,
						    AppData.SequencerData.BPM,
						    8,
						    60,
						    atoi(argv[2]) );

  if( argc > 3 ){

    for( int i = 2; i < argc; i++ ){

      AppData.SoundEngineData.Engine->NewTrack( argv[i] );

    }

  }


  AppData.SoundEngineData.Ready = true;


}
