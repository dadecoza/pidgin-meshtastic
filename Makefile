CFLAGS = -Wall -Werror -g -O2 -fPIC -pipe `pkg-config --cflags glib-2.0 purple`
LDFLAGS = `pkg-config --libs glib-2.0 purple`

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