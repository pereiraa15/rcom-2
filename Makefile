CC = gcc
CFLAGS = -Wall

all: ftp_client

ftp_client: ftp_client.c
	$(CC) $(CFLAGS) -o ftp_client ftp_client.c

clean:
	rm -f ftp_client