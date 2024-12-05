CFLAGS = -Wall -Werror -g -O2 -fPIC -pipe `pkg-config --cflags glib-2.0 purple`
LDFLAGS = `pkg-config --libs glib-2.0 purple`
PLUGIN_DIR = `pkg-config --variable=plugindir purple`
DATA_DIR = `pkg-config --variable=datadir purple`

include  ./nanopb/extra/nanopb.mk


##
## INCLUDE PATHS
##
INCLUDE_PATHS += \
        -I. \
        -I$(NANOPB_DIR)

##
##  SOURCES, OBJECTS
##
CSRC =	\
meshtastic/mesh.pb.c \
meshtastic/telemetry.pb.c \
meshtastic/config.pb.c \
meshtastic/channel.pb.c \
meshtastic/xmodem.pb.c \
meshtastic/device_ui.pb.c \
meshtastic/module_config.pb.c \
meshtastic/admin.pb.c \
meshtastic/connection_status.pb.c \
$(NANOPB_DIR)/pb_encode.c \
$(NANOPB_DIR)/pb_decode.c \
$(NANOPB_DIR)/pb_common.c \
mtstrings.c \
meshtastic.c

libmeshtastic.so: $(C_SRC)
	$(CC) $(INCLUDE_PATHS) $(CSRC) -o $@ $(CFLAGS) $(LDFLAGS) -olibmeshtastic.so -shared
clean:
	rm -f *.so *.o
install: libmeshtastic.so
	install -d $(PLUGIN_DIR)
	install -m 644 libmeshtastic.so $(PLUGIN_DIR)
	install -d $(DATA_DIR)/pixmaps/pidgin/emblems/16
	install -m 644 pixmaps/pidgin/emblems/16/meshtastic-signal-0.png $(DATA_DIR)/pixmaps/pidgin/emblems/16
	install -m 644 pixmaps/pidgin/emblems/16/meshtastic-signal-1.png $(DATA_DIR)/pixmaps/pidgin/emblems/16
	install -m 644 pixmaps/pidgin/emblems/16/meshtastic-signal-2.png $(DATA_DIR)/pixmaps/pidgin/emblems/16
	install -m 644 pixmaps/pidgin/emblems/16/meshtastic-signal-3.png $(DATA_DIR)/pixmaps/pidgin/emblems/16
	install -m 644 pixmaps/pidgin/emblems/16/meshtastic-signal-4.png $(DATA_DIR)/pixmaps/pidgin/emblems/16
	install -d $(DATA_DIR)/pixmaps/pidgin/protocols/16
	install -m 644 pixmaps/pidgin/protocols/16/meshtastic.png $(DATA_DIR)/pixmaps/pidgin/protocols/16/meshtastic.png
	install -d $(DATA_DIR)/pixmaps/pidgin/protocols/22
	install -m 644 pixmaps/pidgin/protocols/22/meshtastic.png $(DATA_DIR)/pixmaps/pidgin/protocols/22/meshtastic.png
	install -d $(DATA_DIR)/pixmaps/pidgin/protocols/48
	install -m 644 pixmaps/pidgin/protocols/48/meshtastic.png $(DATA_DIR)/pixmaps/pidgin/protocols/48/meshtastic.png
	