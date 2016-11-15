CC = g++
TARGET = main

$(TARGET): main.o Normsinv.o
	$(CC) -lm -o $@ $@.o Normsinv.o

main.o: main.cpp Normsinv.cpp Normsinv.h
	$(CC) -g -c $*.cpp Normsinv.cpp

Normsinv.o: Normsinv.cpp Normsinv.h
	$(CC) -g -c $*.cpp
