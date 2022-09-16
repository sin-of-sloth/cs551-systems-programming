TARGET=ppmcvt
CC=gcc -g


.PHONY: cleanall cleanobj gdb

all: $(TARGET)

ppmcvt: pbm.o pbm_aux.o ppmcvt.o

cleanobj:
	$(RM) pbm.o pbm_aux.o ppmcvt.o

cleanall: cleanobj
	$(RM) $(TARGET)