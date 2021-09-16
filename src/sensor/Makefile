rootdir = ../..
INCLUDES += -I$(rootdir)/include
CFILES = dnstap_sensor.c \
				sensor_log.c \
				b64_encode.c \
				b64_decode.c \
				b64_buffer.c

TARGET_DNSTAP_SENSOR = dnstap-sensor

CROSS_COMPILE = 

#CFLAGS += -Wall -g -O2
CFLAGS += -Wall -g
LDFLAGS += -L$(rootdir)/lib -Wl,-rpath,../lib

CC = $(CROSS_COMPILE)gcc

OBJS = $(CFILES:%c=%o)
OBJS_DNSTAP_SENSOR = dnstap_sensor.o \
				sensor_log.o \
				b64_encode.o \
				b64_decode.o \
				b64_buffer.o

all: $(TARGET_DNSTAP_SENSOR)

$(TARGET_DNSTAP_SENSOR): $(OBJS_DNSTAP_SENSOR)
	$(CC) -o $(TARGET_DNSTAP_SENSOR) \
			$(OBJS_DNSTAP_SENSOR) $(LDFLAGS) \
			$(rootdir)/lib/libdnswire.a \
			$(rootdir)/lib/libtinyframe.a \
			-lprotobuf-c -lpthread

.c.o:
	$(CC) $(CFLAGS) -c $< $(INCLUDES)

install:
	cp -f $(TARGET_DNSTAP_SENSOR) $(rootdir)/bin

clean:
	rm -rf $(TARGET_DNSTAP_SENSOR) *.o
