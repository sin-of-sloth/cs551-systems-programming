TARGET=main_demo
CC=gcc -g


.PHONY: cleanall cleanobj gdb

all: $(TARGET)

main_demo: my_malloc.o

cleanobj:
	$(RM) my_malloc.o

cleanall: cleanobj
	$(RM) $(TARGET)