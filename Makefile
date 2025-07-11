include lvgl/lvgl.mk

TARGET = peanut135
OBJDIR = obj/
CFLAGS += -Werror
CFLAGS += $(shell pkg-config --cflags libdrm)
LIBDRM += $(shell pkg-config --libs libdrm)

INICODE = $(OBJDIR)iniparser.o $(OBJDIR)dictionary.o

LDFLAGS += -lblkid

CURR_DIR = $(shell pwd)
LVGL_SRCFILES := $(ASRCS) $(CSRCS)
# Convert absolute paths to relative paths under obj/
LVGL_OBJFILES := $(patsubst $(CURR_DIR)/%.c,obj/%.o,$(filter %.c,$(CSRCS))) \
                 $(patsubst $(CURR_DIR)/%.S,obj/%.o,$(filter %.S,$(ASRCS)))

# CFLAGS += -g -O0

all: $(TARGET)

$(TARGET): $(OBJDIR)main.o $(OBJDIR)rom.o $(OBJDIR)ram.o $(OBJDIR)util.o $(OBJDIR)lcd.o  $(OBJDIR)drm.o $(OBJDIR)input.o $(OBJDIR)blockmnt.o $(INICODE) $(LVGL_OBJFILES)

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
	
$(OBJDIR)drm.o: drm.c headers/peanut.h drm_draw.c headers/main.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) $(LIBDRM) 

$(OBJDIR)main.o: main.c westonkill.c headers/peanut_gb.h headers/main.h lvgl.c device_tree.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)input.o: input.c touch_regions.c finger.c headers/peanut.h headers/main.h
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS) 



$(OBJDIR)%.o: %.c headers/peanut.h headers/main.h lv_conf.h
	#mkdir -p $(OBJDIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ $(LDFLAGS)

$(OBJDIR)%.o: %.S lv_conf.h
	#mkdir -p $(OBJDIR) 
	mkdir -p $(dir $@)
	$(CC) $(AFLAGS) -c $< -o $@ $(LDFLAGS)

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
