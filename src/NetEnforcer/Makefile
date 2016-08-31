TARGET = NetEnforcer
HPPFILES += ../prot/NetEnforcer_prot.h
OBJS += NetEnforcer.o
OBJS += NetEnforcer_prot_xdr.o

CC = g++
#CFLAGS = -Wall -Werror -DDEBUG -g # debug flags
CFLAGS = -Wall -Werror -g -O2 # release flags
CFLAGS += -I..
LDFLAGS += -lm

default:	$(TARGET)
all:		$(TARGET)

$(TARGET):	$(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HPPFILES)
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp $(HPPFILES)
	$(CC) $(CFLAGS) -c -o $@ $<

../prot/NetEnforcer_prot.h: ../prot/NetEnforcer_prot.x
	rpcgen -h -N -o ../prot/NetEnforcer_prot.h ../prot/NetEnforcer_prot.x

NetEnforcer_prot_xdr.c: ../prot/NetEnforcer_prot.x
	rpcgen -c -N -DIGNORE_WARNINGS -o NetEnforcer_prot_xdr.c ../prot/NetEnforcer_prot.x

clean:
	-rm $(TARGET) $(OBJS)
