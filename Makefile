
.PHONY : default clean

CFLAGS = -W -Wall -O3

default: sim65-test sim65-fixed-test

sim65-test : sim65-test.c cJSON.c sim65-testcase.c 6502.c

sim65-fixed-test : sim65-test.c cJSON.c sim65-testcase.c 6502-fixed.c

clean :
	$(RM) *~ sim65-test
