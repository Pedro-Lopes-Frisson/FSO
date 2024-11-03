CXX = g++
CXXFLAGS = -Wall -ggdb -pthread

TARGETS = bb-unsafe bb-safe question4

OBJS = thread.o utils.o

.PHONY:	all clean cleanall

all: $(TARGETS)

question4: question4.o BufferPool.o fifo-mon-safe.o $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

bb-unsafe: main.o fifo-unsafe.o $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

bb-safe: main.o fifo-mon-safe.o $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS)

*.o:	*.h

clean:
	rm -f *.o

cleanall:	clean
	rm -f $(TARGETS) 
