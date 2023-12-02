LUA         = 5.1
LUA_CFLAGS  = $(shell pkg-config --cflags lua$(LUA))
LUA_LIBS    = $(shell pkg-config --libs lua$(LUA))

CFLAGS      = -Wall -fPIC -shared $(LUA_CFLAGS) -Isrc/
LIBS        = -lrt -lcurl $(LUA_LIBS)

PREFIX      = /usr/local
LIBDIR      = $(PREFIX)/lib/lua/$(LUA)

SRC = src/libcurl_async.c src/libcurl_helpers.c src/libcurl_logger.c \
	src/libcurl_lua.c src/libcurl_multi.c
OBJ= $(SRC:.c=.o)

all: lua_async_http.so

lua_async_http.so: $(OBJ)
	$(CC) $(CFLAGS) -shared -o $@ $(OBJ) $(LIBS)

clean:
	rm -f src/*.o *.so

install:
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 lua_async_http.so $(DESTDIR)$(LIBDIR)/lua_async_http.so

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/lua_async_http.so

.PHONY: lua_async_http.so
