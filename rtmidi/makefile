
rt:	main.o RtMidi.o
	g++ -o rt main.o RtMidi.o -D__MACOSX_CORE__ -framework CoreMIDI -framework CoreFoundation -framework CoreAudio

RtMidi.o:	RtMidi.cpp RtMidi.h RtError.h
		g++ -D__MACOSX_CORE__ -c RtMidi.cpp

main.o:		main.cpp
		g++ -c main.cpp