CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -Iinclude
DEBUG_FLAGS = -g -DDEBUG

TARGET = bin/minm

SRCDIR = src
OBJDIR = obj
BINDIR = bin
INCDIR = include

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
HEADERS = $(wildcard $(INCDIR)/*.h)

.PHONY: all clean distclean rebuild run debug check info help

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@
	@echo "Build completed: $(TARGET)"

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS) | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiling: $< -> $@"

$(OBJDIR):
	@mkdir -p $(OBJDIR)
	@echo "Directory created: $(OBJDIR)"

$(BINDIR):
	@mkdir -p $(BINDIR)
	@echo "Directory created: $(BINDIR)"

clean:
	rm -rf $(OBJDIR)/*.o
	rm -f $(TARGET)
	@echo "Clean completed"

distclean: clean
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "Distclean completed"

rebuild: distclean all
	@echo "Rebuild completed"

run: rebuild
	@printf "\033[0;32m+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\033[0m\n"
	./$(TARGET)
	@printf "\033[0;32m+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\033[0m\n"

debug: CFLAGS += $(DEBUG_FLAGS)
debug: rebuild
	@echo "Debug build completed"

info:
	@echo "=== Project Information ==="
	@echo "Source files: $(SOURCES)"
	@echo "Object files: $(OBJECTS)"
	@echo "Header files: $(wildcard $(INCDIR)/*)"
	@echo "Target file: $(TARGET)"

help:
	@echo "=== Available commands ==="
	@echo "make all       - Build project"
	@echo "make clean     - Clean object files"
	@echo "make distclean - Full clean (removes bin/ and obj/)"
	@echo "make rebuild   - Full rebuild"
	@echo "make run       - Rebuild and run program"
	@echo "make debug     - Debug build"
	@echo "make info      - Project information"
	@echo "make help      - This help"