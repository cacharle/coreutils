BINDIR = bin
SRCDIR = src

CC = gcc
CCFLAGS = -Wall -Wextra -Wpedantic

all: prebuild $(BINDIR)/basename $(BINDIR)/chown $(BINDIR)/cut $(BINDIR)/head \
		$(BINDIR)/mkdir $(BINDIR)/mv $(BINDIR)/rm $(BINDIR)/seq \
		$(BINDIR)/shuf $(BINDIR)/tee $(BINDIR)/tr \
		$(BINDIR)/shuf_trand

prebuild:
	mkdir -vp $(BINDIR)

$(BINDIR)/basename: $(SRCDIR)/basename.c
	$(CC) $(CCFLAGS) -o $@ $^

$(BINDIR)/chown: $(SRCDIR)/chown.c
	$(CC) $(CCFLAGS) -o $@ $^

$(BINDIR)/cut: $(SRCDIR)/cut.c
	$(CC) $(CCFLAGS) -o $@ $^

$(BINDIR)/head: $(SRCDIR)/head.c
	$(CC) $(CCFLAGS) -o $@ $^

$(BINDIR)/mkdir: $(SRCDIR)/mkdir.c
	$(CC) $(CCFLAGS) -o $@ $^

$(BINDIR)/mv: $(SRCDIR)/mv.c
	$(CC) $(CCFLAGS) -o $@ $^

$(BINDIR)/rm: $(SRCDIR)/rm.c
	$(CC) $(CCFLAGS) -o $@ $^

$(BINDIR)/seq: $(SRCDIR)/seq.c
	$(CC) $(CCFLAGS) -o $@ $^

$(BINDIR)/shuf: $(SRCDIR)/shuf.c
	$(CC) $(CCFLAGS) -o $@ $^

$(BINDIR)/tee: $(SRCDIR)/tee.c
	$(CC) $(CCFLAGS) -o $@ $^

$(BINDIR)/tr: $(SRCDIR)/tr.c
	$(CC) $(CCFLAGS) -o $@ $^

#### shuf using trand
TRAND_PATH = trand

$(BINDIR)/shuf_trand: $(SRCDIR)/shuf.c
	make --no-print-directory -C $(TRAND_PATH)
	$(CC) $(CCFLAGS) -o $@ $^ -DUSE_TRAND -I$(TRAND_PATH) -L$(TRAND_PATH) -ltrand -lpthread

clean:
	rm -vf $(BINDIR)/*

re: clean all

.PHONY: prebuild all clean re
