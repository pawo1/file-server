
.PHONY: all clean 
all: 
	/usr/bin/g++ -g main.cpp -o server -O0 -std=c++2a -Wall -Wextra -lssl -lcrypto

clean:
	rm -r server


