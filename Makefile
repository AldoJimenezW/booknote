CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -O0
LIBS = -lsqlite3
TARGET = booknote

# Source files
SRCS = src/main.c \
       src/utils/error.c \
       src/core/book.c \
       src/core/note.c \
       src/database/db.c \
       src/database/schema.c \
       src/database/queries.c \
       src/cli/commands.c

OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo "Build complete: ./$(TARGET)"

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Clean complete"

# Run
run: $(TARGET)
	./$(TARGET)

# Install (requires sudo)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Help
help:
	@echo "booknote Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the project (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  run       - Build and run"
	@echo "  install   - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  help      - Show this message"

.PHONY: all clean run install uninstall help
