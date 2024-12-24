
.PHONY : default clean

CFLAGS = -W -Wall -O3

sim65-test : sim65-test.c cJSON.c sim65-testcase.c 6502.c memory.c
	$(CC) $(CFLAGS) $^ -o $@

clean :
	$(RM) *~ sim65-test *.test-out test_summary.html
