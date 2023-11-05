all:
	gcc  -m32 -no-pie -nostdlib -o fib fib.c
	gcc -m32 -no-pie -nostdlib -o sum sum.c
	gcc -m32 -no-pie -nostdlib -o prime prime.c
	gcc -m32 -o SimpleSmartLoader SimpleSmartLoader.c

clean:
	-@rm -f fib SimpleSmartLoader sum prime
