CC = gcc
INCFLAGS = -I.
DEBUG_CCFLAGS = $(INCFLAGS) -c -g -D_DEBUG
RELEASE_CCFLAGS = $(INCFLAGS) -c -O2 -DNDEBUG
DEBUG_BINDIR = ../Bin/Debug
RELEASE_BINDIR = ../Bin/Release
DEBUG_TARGET = $(DEBUG_BINDIR)/libBMCOMM.a
RELEASE_TARGET = $(RELEASE_BINDIR)/libBMCOMM.a
DEBUG_OBJDIR = ../Bin/Debug/o
DEBUG_OBJECTS = $(DEBUG_OBJDIR)/BMBase.o $(DEBUG_OBJDIR)/BMCRC.o \
	$(DEBUG_OBJDIR)/BMBuffer.o $(DEBUG_OBJDIR)/BMTick.o
RELEASE_OBJECTS = $(RELEASE_OBJDIR)/BMBase.o $(RELEASE_OBJDIR)/BMCRC.o \
	$(RELEASE_OBJDIR)/BMBuffer.o $(RELEASE_OBJDIR)/BMTick.o

debug	:	$(DEBUG_TARGET)
$(DEBUG_TARGET)	:	$(DEBUG_OBJECTS)
	$(AR) -r -s $@ $^

$(DEBUG_OBJDIR)/BMBase.o	:	BMBase.c BMBase.h
	mkdir -p $(DEBUG_OBJDIR)
	$(CC) $(DEBUG_CCFLAGS) -o $@ $<

$(DEBUG_OBJDIR)/BMCRC.o	:	BMCRC.c BMCRC.h BMBase.h
	mkdir -p $(DEBUG_OBJDIR)
	$(CC) $(DEBUG_CCFLAGS) -o $@ $<

$(DEBUG_OBJDIR)/BMBuffer.o	:	BMBuffer.c BMBuffer.h BMBase.h
	mkdir -p $(DEBUG_OBJDIR)
	$(CC) $(DEBUG_CCFLAGS) -o $@ $<

$(DEBUG_OBJDIR)/BMTick.o	:	BMTick.c BMTick.h BMBase.h
	mkdir -p $(DEBUG_OBJDIR)
	$(CC) $(DEBUG_CCFLAGS) -o $@ $<

$(DEBUG_OBJDIR)/BMSched.o	:	BMSched.c BMSched.h BMBase.h
	mkdir -p $(DEBUG_OBJDIR)
	$(CC) $(DEBUG_CCFLAGS) -o $@ $<

release	:	$(RELEASE_TARGET)
$(RELEASE_TARGET)	:	$(RELEASE_OBJECTS)
	$(AR) -r -s $@ $^

$(RELEASE_OBJDIR)/BMBase.o	:	BMBase.c BMBase.h
	mkdir -p $(RELEASE_OBJDIR)
	$(CC) $(RELEASE_CCFLAGS) -o $@ $<

$(RELEASE_OBJDIR)/BMCRC.o	:	BMCRC.c BMCRC.h BMBase.h
	mkdir -p $(RELEASE_OBJDIR)
	$(CC) $(RELEASE_CCFLAGS) -o $@ $<

$(RELEASE_OBJDIR)/BMBuffer.o	:	BMBuffer.c BMBuffer.h BMBase.h
	mkdir -p $(RELEASE_OBJDIR)
	$(CC) $(RELEASE_CCFLAGS) -o $@ $<

$(RELEASE_OBJDIR)/BMTick.o	:	BMTick.c BMTick.h BMBase.h
	mkdir -p $(RELEASE_OBJDIR)
	$(CC) $(RELEASE_CCFLAGS) -o $@ $<

$(RELEASE_OBJDIR)/BMSched.o	:	BMSched.c BMSched.h BMBase.h
	mkdir -p $(RELEASE_OBJDIR)
	$(CC) $(RELEASE_CCFLAGS) -o $@ $<

clean_debug	:
	rm -f $(DEBUG_TARGET) $(DEBUG_OBJECTS)

clean_release	:
	rm -f $(RELEASE_TARGET) $(RELEASE_OBJECTS)
