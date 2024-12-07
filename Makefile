
.PHONY : default clean

CFLAGS = -W -Wall -O3

default: sim65-original-test sim65-fixed-test

sim65-original-test : sim65-test.c cJSON.c sim65-testcase.c 6502.c
	$(CC) $(CFLAGS) -DFIX_SIM65=0 $^ -o $@

sim65-fixed-test : sim65-test.c cJSON.c sim65-testcase.c 6502-fixed.c adc_and_sbc.c
	$(CC) $(CFLAGS) -DFIX_SIM65=1 $^ -o $@

clean :
	$(RM) *~ sim65-original-test sim65-fixed-test *.test-out test_summary.html
