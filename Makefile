CC = gcc
LINKER = gcc

LFLAGS = -Wall -I. -lrt -llua -lcurl -shared -fPIC
CFLAGS = -Wall -lrt -llua -lcurl -shared -fPIC

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = lua_async_http.so

$(BINDIR)/$(TARGET): $(OBJECTS)
		@mkdir -p $(@D) 
		$(LINKER) -o $@ $(LFLAGS) $(OBJECTS)
		@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
		@mkdir -p $(@D) 
		$(CC) $(CFLAGS) -c $< -o $@
		@echo "Compiled "$<" successfully!"

clean:
	rm -f $(OBJDIR)/*.o $(OBJDIR)/*.so
