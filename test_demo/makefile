

INCLUDE += -I ./

LIBPATH +=
LIB += -fPIC -O0 -Wall -g -lm -pthread
ALL_C := $(wildcard ./*.c)
SOURCES := $(ALL_C) 
objects := $(SOURCES:.c=.o) 
haha:demo
demo:$(objects)
	gcc -o demo  $(objects) $(INCLUDE) $(LIBPATH) $(LIB)
%.o:%.c
	gcc $(INCLUDE) $(LIBPATH) $(LIB) -c  $< -o $@

clean:
	rm -rf *.o demo


