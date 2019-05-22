CXX := g++
CXXFLAGS := -g
LIBS := -lncursesw

TARGET := a.out
SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS := $(patsubst %.cpp,%.d,$(SOURCES))

-include $(DEPENDS)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $< $(LIBS)

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPENDS)
