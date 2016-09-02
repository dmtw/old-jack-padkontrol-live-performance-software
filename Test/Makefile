test:	main.o blackbox.o
	g++ -o test -lm -framework CoreAudio -framework CoreServices -framework AudioUnit -L/usr/local/lib -ljack main.o blackbox.o

main.o:	main.cpp blackbox.cpp blackbox.h
	g++ -c main.cpp

blackbox.o:	blackbox.cpp blackbox.h
		g++ -c blackbox.cpp

clean:
	rm test *~ *.o