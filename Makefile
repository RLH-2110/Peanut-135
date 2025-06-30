TARGET = peanut135
OBJDIR = obj/
CFLAGS += -Werror
CFLAGS += $(shell pkg-config --cflags libdrm)
LIBDRM += $(shell pkg-config --libs libdrm)

INICODE = $(OBJDIR)iniparser.o $(OBJDIR)dictionary.o

all: $(TARGET)

$(TARGET): $(OBJDIR)tests.o $(OBJDIR)main.o $(OBJDIR)rom.o $(OBJDIR)ram.o $(OBJDIR)util.o $(OBJDIR)lcd.o  $(OBJDIR)drm.o $(OBJDIR)input.o $(OBJDIR)rini.o $(INICODE)

	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBDRM)
	$(info    #############################################################################)
	$(info    $(CC))
	$(info    -----------------------------------------------------------------------------)
	$(info    $(CFLAGS))
	$(info    -----------------------------------------------------------------------------)
	$(info    $^) 
	$(info    -----------------------------------------------------------------------------)
	$(info    -o $@) 
	$(info    -----------------------------------------------------------------------------)
	$(info    $(LDFLAGS))
	$(info    -----------------------------------------------------------------------------)
	$(info    $(LIBDRM))
	$(info    #############################################################################)
	
$(OBJDIR)rini.o: librini/src/rini.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) $(LIBDRM) 

$(OBJDIR)drm.o: drm.c peanut.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) $(LIBDRM) 

$(OBJDIR)main.o: main.c westonkill.c peanut_gb.h main.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)input.o: input.c touch_regions.c finger.c peanut.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) 

$(OBJDIR)tests.o: forktest/tests.c peanut.h forktest/test_* main.h 
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) 


$(OBJDIR)%.o: %.c peanut.h forktest/test_*
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET)
	rm -rdf $(OBJDIR)

.PHONY: all clean


$(OBJDIR)iniparser.o: iniparser/src/iniparser.c iniparser/src/iniparser.h iniparser/src/dictionary.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) 

$(OBJDIR)dictionary.o: iniparser/src/dictionary.c iniparser/src/dictionary.h iniparser/src/iniparser.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) 
