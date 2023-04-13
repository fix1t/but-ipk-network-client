CC = gcc
CCTEST = g++
CFLAGS  = -g -Wall
TARGET = ipkcpc
TESTS = ipkcpc_tests

.PHONY: build clean tcp udp tcpout udpout tests buildTests cleanTests

all: 
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c
	
build: clean
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

tcp: build
	./$(TARGET) -h localhost -p 2023 -m tcp

tcpout: build
	./$(TARGET) -h localhost -p 2023 -m tcp > out.log

udp: build
	./$(TARGET) -h localhost -p 2023 -m udp

udpout: build
	./$(TARGET) -h localhost -p 2023 -m udp > out.log

test: cleanTests build buildTests
	./$(TESTS) localhost 2023

buildTests: cleanTests
	$(CCTEST) $(CFLAGS) -o $(TESTS) $(TESTS).cpp

cleanTests:
	$(RM) $(TESTS)

clean:
	$(RM) $(TARGET)