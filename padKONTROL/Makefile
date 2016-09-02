test:	main.o
	g++ -o pk -lm -framework CoreAudio -framework CoreServices -framework AudioUnit\
	 -L/usr/local/lib -ljack `pkg-config --libs --cflags sndfile` main.o

	rm *.o

main.o:	main.cpp
	g++ -c main.cpp

clean:
	rm pk *~ *.o