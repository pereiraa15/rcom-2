CC = gcc
CFLAGS = -Wall

all: download

download: ftp_client.c
	$(CC) $(CFLAGS) -o download ftp_client.c

clean:
	rm -f download