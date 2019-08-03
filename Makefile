CC := gcc # This is the main compiler
# CC := clang --analyze # and comment out the linker last line for sanity
SRCDIR := src
BUILDDIR := build
TARGET := bin/valet

# Anything in the source folder will be compiled as long as it has SRCEXT
# extension.
SRCEXT := c
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
PURPLE_CFLAGS := $(shell pkg-config --cflags purple)
PURPLE_LIBS := $(shell pkg-config --libs purple)
REDIS_CFLAGS := $(shell pkg-config --cflags hiredis)
REDIS_LIBS := $(shell pkg-config --libs hiredis)
CFLAGS := -g -Wall $(PURPLE_CFLAGS) $(REDIS_CFLAGS)
LIB := -L lib $(PURPLE_LIBS) $(REDIS_LIBS)
INC := -I include

$(TARGET): $(OBJECTS)
	@echo " Linking...";
	@echo " $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo " Cleaning...";
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

# Tests
#tester:
#  $(CC) $(CFLAGS) test/tester.cpp $(INC) $(LIB) -o bin/tester

# Spikes
#ticket:
#  $(CC) $(CFLAGS) spikes/ticket.cpp $(INC) $(LIB) -o bin/ticket

.PHONY: clean
