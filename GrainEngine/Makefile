test:	main.o blackbox.o grainengine.o
	g++ -o test -lm -framework CoreAudio -framework CoreServices -framework AudioUnit\
	 -L/usr/local/lib -ljack `pkg-config --libs --cflags sndfile` main.o blackbox.o grainengine.o

	rm *.o

main.o:	main.cpp blackbox.cpp blackbox.h grainengine.h grainengine.cpp
	g++ -c main.cpp

blackbox.o:	blackbox.cpp blackbox.h
		g++ -c blackbox.cpp

grainengine.o:	grainengine.cpp grainengine.h blackbox.cpp blackbox.h
		g++ -c grainengine.cpp

clean:
	rm test *~ *.o