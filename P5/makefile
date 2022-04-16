#  Author: Michael Trani
#  April 2022      

LIBS = # -lpthread

CC=g++

CFLAGS=-g -Wall -std=c++11

all: oss user

oss: oss.o
	$(CC) oss.o -o oss $(LIBS)
user: user.o
	$(CC) user.o -o user $(LIBS)
oss.o: oss.cpp
	$(CC) -c -g oss.cpp
user.o: user.cpp
	$(CC) -c -g user.cpp
clean:
	find . -type f ! -iname "*.cpp" ! -iname "makefile" ! -iname "README" ! -iname "*.h" -delete
