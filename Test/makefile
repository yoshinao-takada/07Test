CC = gcc
INC_FLAGS = -I../Lib
DEBUG_CC_FLAGS = -c -g -pthread $(INC_FLAGS) -D_DEBUG
RELEASE_CC_FLAGS = -c -pthread -O2 $(INC_FLAGS) -DNDEBUG
DEBUG_BINDIR = ../Bin/Debug
RELEASE_BINDIR = ../Bin/Release
DEBUG_OBJDIR = $(DEBUG_BINDIR)/o
RELEASE_OBJDIR = $(RELEASE_OBJDIR)/o
DEBUG_TARGET = $(DEBUG_BINDIR)/Test.exe
RELEASE_TARGET = $(RELEASE_BINDIR)/Test.exe
DEBUG_MAINOBJ = $(DEBUG_OBJDIR)/Test.o
RELEASE_MAINOBJ = $(RELEASE_OBJDIR)/Test.o
DEBUG_OBJECTS = $(DEBUG_OBJDIR)/Test.o \
	$(DEBUG_OBJDIR)/TickUT.o $(DEBUG_OBJDIR)/EvPoolUT.o \
	$(DEBUG_OBJDIR)/BufferUT.o $(DEBUG_OBJDIR)/CommUT.o	\
	$(DEBUG_OBJDIR)/RingBufferUT.o $(DEBUG_OBJDIR)/LockNUnlockUT.o \
	$(DEBUG_OBJDIR)/STickUT.o $(DEBUG_OBJDIR)/CommThUT.o \
	$(DEBUG_OBJDIR)/EvQPoolUT.o

debug	:	$(DEBUG_TARGET)
$(DEBUG_TARGET)	:	$(DEBUG_OBJECTS) $(DEBUG_BINDIR)/libBMCOMM.a
	$(CC) -pthread -o $@ $(DEBUG_OBJECTS) -L$(DEBUG_BINDIR) -lBMCOMM -lpthread -lm

$(DEBUG_OBJDIR)/Test.o	:	Test.c
	$(CC) $(DEBUG_CC_FLAGS) -o $@ $<

$(DEBUG_OBJDIR)/TickUT.o	:	TickUT.c
	$(CC) $(DEBUG_CC_FLAGS) -o $@ $<

$(DEBUG_OBJDIR)/EvPoolUT.o	:	EvPoolUT.c
	$(CC) $(DEBUG_CC_FLAGS) -o $@ $<

$(DEBUG_OBJDIR)/BufferUT.o	:	BufferUT.c
	$(CC) $(DEBUG_CC_FLAGS) -o $@ $<

$(DEBUG_OBJDIR)/CommUT.o	:	CommUT.c
	$(CC) $(DEBUG_CC_FLAGS) -o $@ $<

$(DEBUG_OBJDIR)/CommThUT.o	:	CommThUT.c
	$(CC) $(DEBUG_CC_FLAGS) -o $@ $<

$(DEBUG_OBJDIR)/RingBufferUT.o	:	RingBufferUT.c
	$(CC) $(DEBUG_CC_FLAGS) -o $@ $<

$(DEBUG_OBJDIR)/STickUT.o	:	STickUT.c
	$(CC) $(DEBUG_CC_FLAGS) -o $@ $<

$(DEBUG_OBJDIR)/EvQPoolUT.o	:	EvQPoolUT.c
	$(CC) $(DEBUG_CC_FLAGS) -o $@ $<

$(DEBUG_OBJDIR)/LockNUnlockUT.o	:	LockNUnlockUT.c
	$(CC) $(DEBUG_CC_FLAGS) -o $@ $<

clean_debug	:
	rm -rf $(DEBUG_TARGET) $(DEBUG_OBJECTS)
