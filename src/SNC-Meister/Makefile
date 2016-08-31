TARGET = SNC-Meister
HPPFILES += ../SNC-Library/Estimator.hpp
HPPFILES += ../SNC-Library/TraceReader.hpp
HPPFILES += ../SNC-Library/ProcessedTrace.hpp
HPPFILES += ../SNC-Library/priorityAlgoBySLO.hpp
HPPFILES += ../SNC-Library/NC.hpp
HPPFILES += ../SNC-Library/MGF.hpp
HPPFILES += ../SNC-Library/SNCOperators.hpp
HPPFILES += ../SNC-Library/SNC.hpp
HPPFILES += ../SNC-Library/search.hpp
HPPFILES += ../SNC-Library/time.hpp
HPPFILES += ../SNC-Library/serializeJSON.hpp
HPPFILES += ../prot/SNC_Meister_prot.h
HPPFILES += ../prot/NetEnforcer_prot.h
OBJS += SNC-Meister.o
OBJS += ../json/jsoncpp.o
OBJS += ../SNC-Library/Estimator.o
OBJS += ../SNC-Library/NetworkEstimator.o
OBJS += ../SNC-Library/TraceReader.o
OBJS += ../SNC-Library/ProcessedTrace.o
OBJS += ../SNC-Library/priorityAlgoBySLO.o
OBJS += ../SNC-Library/NC.o
OBJS += ../SNC-Library/MGF.o
OBJS += ../SNC-Library/SNCOperators.o
OBJS += ../SNC-Library/SNC.o
OBJS += ../SNC-Library/search.o
OBJS += SNC_Meister_prot_xdr.o
OBJS += NetEnforcer_prot_clnt.o
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

../prot/SNC_Meister_prot.h: ../prot/SNC_Meister_prot.x
	rpcgen -h -N -o ../prot/SNC_Meister_prot.h ../prot/SNC_Meister_prot.x

SNC_Meister_prot_xdr.c: ../prot/SNC_Meister_prot.x
	rpcgen -c -N -DIGNORE_WARNINGS -o SNC_Meister_prot_xdr.c ../prot/SNC_Meister_prot.x

../prot/NetEnforcer_prot.h: ../prot/NetEnforcer_prot.x
	rpcgen -h -N -o ../prot/NetEnforcer_prot.h ../prot/NetEnforcer_prot.x

NetEnforcer_prot_xdr.c: ../prot/NetEnforcer_prot.x
	rpcgen -c -N -DIGNORE_WARNINGS -o NetEnforcer_prot_xdr.c ../prot/NetEnforcer_prot.x

NetEnforcer_prot_clnt.c: ../prot/NetEnforcer_prot.x
	rpcgen -l -N -o NetEnforcer_prot_clnt.c ../prot/NetEnforcer_prot.x

clean:
	-rm $(TARGET) $(OBJS)
