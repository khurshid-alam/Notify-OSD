APP	= notification-test-cairo

CC	= gcc

CFLAGS	= -Wall -Werror -c -g -std=c99 \
	  `pkg-config --cflags gtk+-2.0`

LDFLAGS	= `pkg-config --libs gtk+-2.0`

SRCS	= main.c \

OBJS	= $(SRCS:.c=.o)

all: $(APP)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

# it is important that $(OBJS) stands _before_ $(LDFLAGS)
$(APP):	$(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o$(APP)

clean:
	rm -f *.o *~ $(APP)
