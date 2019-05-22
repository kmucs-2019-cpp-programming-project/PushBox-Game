CXX := g++
CXXFLAGS := -g
LDFLAGS := $(CXXFLAGS) -MMD -MP -c
LIBS := -lncursesw

Q := @
ECHO := $(Q) echo

OUT := build
SRC := src

TARGET := $(OUT)/a.out
SOURCES := $(wildcard $(SRC)/*.cpp)
OBJECTS := $(subst $(SRC),$(OUT),$(patsubst %.cpp,%.o,$(SOURCES)))
DEPENDS := $(patsubst %.o,%.d,$(OBJECTS))

.PHONY: all clean vars

all: $(OUT) $(TARGET)

vars:
	$(ECHO) 'TARGET - $(TARGET), OBJECTS - $(OBJECTS), DEPENDS - $(DEPENDS), SOURCES - $(SOURCES)'

$(OUT):
	$(ECHO) "'$(OUT)' folder is not exists. creating folder."
	$(Q) mkdir -p $(OUT)

$(TARGET): $(OBJECTS)
	$(ECHO) '[LD] $^ => $@'
	$(Q) $(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

$(OUT)/%.o: $(SRC)/%.cpp Makefile
	$(ECHO) '[CXX] $< => $@'
	$(Q) $(CXX) $(LDFLAGS) -o $@ $< $(LIBS)

clean:
	$(ECHO) '[DEL] $(TARGET)'
	$(Q)rm -f $(TARGET) 
	$(ECHO) '[DEL] $(OBJECTS)'
	$(Q)rm -f $(OBJECTS)
	$(ECHO) '[DEL] $(DEPENDS)'
	$(Q)rm -f $(DEPENDS)

run:
	$(Q) $(TARGET)

-include $(DEPENDS)
