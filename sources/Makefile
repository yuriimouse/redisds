VERSION = 2.3.1

# # Library name
LIB_NAME = redisds
# # Install path
INSTALL_PATH = /usr/local/

CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g -std=gnu99 -DVERSION=\"$(VERSION)\" -I$(INSTALL_PATH) -I/usr/include 
LDFLAGS = -shared

STATIC_LIB = lib$(LIB_NAME).a
TARGET_LIB = lib$(LIB_NAME).so

.DEFAULT_GOAL := default
default: all clean

.PHONY: all
all: $(TARGET_LIB)

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_LIB): $(OBJECTS)
	$(CC) ${LDFLAGS} -o $@ $^
	ar rcs $(STATIC_LIB) $(OBJECTS)

.PHONY: clean
clean:
	rm -f *.o
	rm -f $(TARGET_LIB) $(STATIC_LIB)

.PHONY: install
install: all copy clean

.PHONY: copy
copy:
	/bin/mkdir -p $(INSTALL_PATH)include/$(LIB_NAME)/
	cp -f *.h $(INSTALL_PATH)include/$(LIB_NAME)/
	/bin/mkdir -p $(INSTALL_PATH)lib
	cp -f $(TARGET_LIB) $(INSTALL_PATH)lib/
	cp -f $(STATIC_LIB) $(INSTALL_PATH)lib/

