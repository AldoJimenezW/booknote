CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -O0
LIBS = -lsqlite3

# GTK and Poppler flags
GUI_CFLAGS = $(shell pkg-config --cflags gtk+-3.0 poppler-glib)
GUI_LIBS = $(shell pkg-config --libs gtk+-3.0 poppler-glib)

# Targets
TARGET_CLI = booknote
TARGET_GUI = booknote-gui

# CLI source files
CLI_SRCS = src/main.c \
           src/utils/error.c \
           src/core/book.c \
           src/core/note.c \
           src/database/db.c \
           src/database/schema.c \
           src/database/queries.c \
           src/cli/commands.c

# GUI source files  
GUI_SRCS = src/gui/main.c \
           src/utils/error.c \
           src/core/book.c \
           src/core/note.c \
           src/database/db.c \
           src/database/schema.c \
           src/database/queries.c

CLI_OBJS = $(CLI_SRCS:.c=.o)
GUI_OBJS = $(GUI_SRCS:.c=.o)

# Default target
all: $(TARGET_CLI) $(TARGET_GUI)

# CLI binary
$(TARGET_CLI): $(CLI_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo "CLI build complete: ./$(TARGET_CLI)"

# GUI binary
$(TARGET_GUI): $(GUI_OBJS)
	$(CC) $(CFLAGS) $(GUI_CFLAGS) -o $@ $^ $(LIBS) $(GUI_LIBS)
	@echo "GUI build complete: ./$(TARGET_GUI)"

# Pattern rule for GUI objects
src/gui/%.o: src/gui/%.c
	$(CC) $(CFLAGS) $(GUI_CFLAGS) -c $< -o $@

# Pattern rule for all other objects
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(CLI_OBJS) $(GUI_OBJS) $(TARGET_CLI) $(TARGET_GUI)
	@echo "Clean complete"

# Run CLI
run: $(TARGET_CLI)
	./$(TARGET_CLI)

# Run GUI
run-gui: $(TARGET_GUI)
	./$(TARGET_GUI)

# Install
install: $(TARGET_CLI) $(TARGET_GUI)
	install -m 755 $(TARGET_CLI) /usr/local/bin/
	install -m 755 $(TARGET_GUI) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET_CLI)
	rm -f /usr/local/bin/$(TARGET_GUI)

# Help
help:
	@echo "booknote Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build CLI and GUI (default)"
	@echo "  booknote     - Build CLI only"
	@echo "  booknote-gui - Build GUI only"
	@echo "  clean        - Remove build artifacts"
	@echo "  run          - Build and run CLI"
	@echo "  run-gui      - Build and run GUI"
	@echo "  install      - Install both to /usr/local/bin"
	@echo "  uninstall    - Remove from /usr/local/bin"

.PHONY: all clean run run-gui install uninstall help
