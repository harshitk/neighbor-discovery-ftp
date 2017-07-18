CC = gcc
MD = mkdir
CFLAGS = -g
LDLIBS = -lpthread -lssl -lcrypto
SRCDIR = src
OBJDIR = obj
EXEDIR = exe

OUT = 	$(EXEDIR)/peer
OBJS = 	$(OBJDIR)/main.o \
		$(OBJDIR)/peer.o \
		$(OBJDIR)/discovery.o \
		$(OBJDIR)/header.o

EXTRA_STATIC_LIBS= -lpthread -lssl -lcrypto

LIB_INCLUDES=./include 

$(OUT):$(OBJDIR)/main.o $(OBJDIR)/peer.o $(OBJDIR)/discovery.o $(OBJDIR)/header.o
	$(MD) -p $(EXEDIR)
	$(CC) -o $(OUT) $(OBJS) $(LDLIBS)
$(OBJDIR)/peer.o: $(SRCDIR)/peer.c
	$(CC) $(CFLAGS) -c ./src/peer.c -I$(LIB_INCLUDES) -o $(OBJDIR)/peer.o
$(OBJDIR)/main.o: $(SRCDIR)/main.c
	$(MD) -p $(OBJDIR)
	$(CC) $(CFLAGS) -c ./src/main.c -I$(LIB_INCLUDES) -o $(OBJDIR)/main.o
$(OBJDIR)/discovery.o: $(SRCDIR)/discovery.c
	$(CC) $(CFLAGS) -c ./src/discovery.c -I$(LIB_INCLUDES) -o $(OBJDIR)/discovery.o
$(OBJDIR)/header.o: $(SRCDIR)/header.c
	$(CC) $(CFLAGS) -c ./src/header.c -I$(LIB_INCLUDES) -o $(OBJDIR)/header.o

.PHONY: clean

clean:
	rm -rf $(OUT) $(OBJDIR)/*.o $(EXEDIR)
