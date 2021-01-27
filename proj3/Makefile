CC = gcc
CFLAGS = -Wall
OBJS = 20181611.o
TARGET = 20181611.out

$(TARGET) : $(OBJS)
	$(CC) -o $@ $(OBJS)

20181611.o : 20181611.h 20181611.c

clean :
		rm $(OBJS) $(TARGET)
