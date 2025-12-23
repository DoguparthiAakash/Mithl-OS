CC = gcc
# Use pkg-config for CFLAGS and LIBS
# Add libinput and libudev to flags
CFLAGS = -Wall -O2 $(shell pkg-config --cflags libdrm libinput libudev)
LDFLAGS = $(shell pkg-config --libs libdrm libinput libudev)

# Source files
SRC = compositor.c drm_backend.c window_manager.c input.c

all: compositor

compositor: $(SRC)
	$(CC) $(CFLAGS) -o compositor $(SRC) $(LDFLAGS)

clean:
	rm -f compositor
