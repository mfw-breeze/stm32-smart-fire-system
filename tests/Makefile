CC     ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -O2 -I../app

BIN = test_alarm

all: test

test: $(BIN)
	./$(BIN)

$(BIN): test_alarm.c ../app/alarm.c ../app/alarm.h ../app/alarm_config.h
	$(CC) $(CFLAGS) -o $(BIN) test_alarm.c ../app/alarm.c

clean:
	rm -f $(BIN) $(BIN).exe

.PHONY: all test clean
