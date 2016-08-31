TARGET = SNC-MeisterClient
HPPFILES += ../prot/SNC_Meister_prot.h
OBJS += SNC-MeisterClient.o
OBJS += ../json/jsoncpp.o
OBJS += SNC_Meister_prot_clnt.o
OBJS += SNC_Meister_prot_xdr.o

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

../prot/SNC_Meister_prot.h: ../prot/SNC_Meister_prot.x
	rpcgen -h -N -o ../prot/SNC_Meister_prot.h ../prot/SNC_Meister_prot.x

SNC_Meister_prot_xdr.c: ../prot/SNC_Meister_prot.x
	rpcgen -c -N -DIGNORE_WARNINGS -o SNC_Meister_prot_xdr.c ../prot/SNC_Meister_prot.x

SNC_Meister_prot_clnt.c: ../prot/SNC_Meister_prot.x
	rpcgen -l -N -o SNC_Meister_prot_clnt.c ../prot/SNC_Meister_prot.x

clean:
	-rm $(TARGET) $(OBJS)
