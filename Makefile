
.PHONY : clean

CFLAGS = -W -Wall -O3

sim65_test : sim65_test.c cJSON.c 6502.c memory.c error.c
clean :
	$(RM) *~ sim65_test
