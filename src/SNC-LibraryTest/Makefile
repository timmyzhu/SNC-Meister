TARGET = SNC-LibraryTest
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
HPPFILES += SNC-LibraryTest.hpp
OBJS += SNC-LibraryTest.o
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
OBJS += searchTest.o
OBJS += MGFTest.o
OBJS += SNCOperatorsTest.o
OBJS += SNCTest.o
OBJS += NCTest.o
OBJS += priorityAlgoBySLOTest.o
OBJS += TraceReaderTest.o
OBJS += NetworkEstimatorTest.o
OBJS += ProcessedTraceTest.o

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

clean:
	-rm $(TARGET) $(OBJS)
