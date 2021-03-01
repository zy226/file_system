cc = gcc
prom = shell
deps = shell.h file.c
obj = main.o disk.o shell.o file.o

$(prom): $(obj)
	$(cc) -o $(prom) $(obj)

%.o: %.c $(deps)
	$(cc) -c $< -o $@

clean:
	-rm -rf $(obj) $(prom)