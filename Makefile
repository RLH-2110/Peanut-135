TARGET = peanut135
OBJDIR = obj/
CFLAGS += -Werror
CFLAGS += $(shell pkg-config --cflags libdrm)
LIBDRM += $(shell pkg-config --libs libdrm)

all: $(TARGET)


$(TARGET): $(OBJDIR)main.o $(OBJDIR)rom.o $(OBJDIR)ram.o $(OBJDIR)util.o $(OBJDIR)lcd.o  $(OBJDIR)drm.o $(OBJDIR)input.o

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
	
$(OBJDIR)drm.o: drm.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) $(LIBDRM) 

$(OBJDIR)main.o: main.c westonkill.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)input.o: input.c touch_regions.c finger.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) 

$(OBJDIR)%.o: %.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET)
	rm -rdf $(OBJDIR)

.PHONY: all clean
