all:	live

live:	main.o blackbox.o grainengine.o soundengine.o RtMidi.o
		g++ -o live -lm -D__MACOSX_CORE__ -framework CoreAudio -framework CoreServices -framework AudioUnit \
		-framework CoreMIDI -L/usr/local/lib -ljack -I/opt/local/include -L/opt/local/lib -lsndfile -lFLAC \
		-lvorbisenc -lvorbis -lm -logg main.o blackbox.o grainengine.o soundengine.o RtMidi.o

main.o:	main.cpp blackbox.cpp blackbox.h grainengine.h grainengine.cpp RtMidi.cpp RtMidi.h
		g++ -c main.cpp

blackbox.o:	blackbox.cpp blackbox.h
		g++ -c blackbox.cpp

grainengine.o:	grainengine.cpp grainengine.h blackbox.cpp blackbox.h
		g++ -c grainengine.cpp

soundengine.o:  soundengine.cpp soundengine.h grainengine.cpp grainengine.h blackbox.cpp blackbox.h
		g++ -c soundengine.cpp

RtMidi.o:	RtMidi.cpp RtMidi.h RtError.h
		g++ -D__MACOSX_CORE__ -c RtMidi.cpp

clean:
		rm live *~ *.o *#