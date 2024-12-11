CC = gcc
CFLAGS = -Wall
SRC_DIR = ftp_client
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/url_parser.c $(SRC_DIR)/socket_ops.c $(SRC_DIR)/ftp_protocol.c
OBJS = $(SRCS:.c=.o)

all: download

download: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o download

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f download $(OBJS)
	rm -rf downloads/*