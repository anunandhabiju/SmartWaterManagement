# Smart Water Distribution and Management System
# Makefile for GCC / Clang (Windows & POSIX)

CC ?= gcc
CFLAGS ?= -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -I./backend -O2

# Detect Windows
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
    RM = del /Q /F
    EXT = .exe
else
    LDFLAGS += -pthread -lm
    RM = rm -f
    EXT =
endif

SERVER_SRC = backend/main.c \
             backend/server.c \
             backend/tank.c \
             backend/queue.c \
             backend/linkedlist.c \
             backend/bst.c \
             backend/graph.c \
             backend/distribution.c \
             backend/storage.c \
             backend/cJSON.c \
             backend/mongoose.c

TEST_SRC = tests/test_all.c \
           backend/tank.c \
           backend/queue.c \
           backend/linkedlist.c \
           backend/bst.c \
           backend/graph.c \
           backend/distribution.c \
           backend/storage.c

SERVER_BIN = backend/server$(EXT)
TEST_BIN = tests/test_all$(EXT)

.PHONY: all test server run clean

all: test server

server: $(SERVER_BIN)

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) $(LDFLAGS) -o $(SERVER_BIN)
	@echo [OK] Built $(SERVER_BIN)

test: $(TEST_BIN)
	@echo Running automated test suite...
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CC) $(CFLAGS) $(TEST_SRC) -o $(TEST_BIN)
	@echo [OK] Built $(TEST_BIN)

run: $(SERVER_BIN)
	./$(SERVER_BIN) 8080

clean:
	$(RM) backend\server$(EXT) tests\test_all$(EXT) 2>nul || true
