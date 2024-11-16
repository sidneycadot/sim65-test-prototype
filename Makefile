
.PHONY : clean

CFLAGS = -W -Wall -O3

sim65-test : sim65-test.c cJSON.c sim65-testcase.c 6502.c

clean :
	$(RM) *~ sim65-test
