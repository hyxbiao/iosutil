
CC = g++ -ObjC++

CPPFLAG = -I.
LDFLAGS = -framework Foundation -framework CoreFoundation -framework MobileDevice -F/System/Library/PrivateFrameworks
DEPS = common.h
OBJS = iosutil.o manager.o device.o

all: iosutil

iosutil: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(DEPS)
	$(CC) -g -c -o $@ $< $(CPPFLAGS)

clean:
	rm -rf iosutil $(OBJS)
