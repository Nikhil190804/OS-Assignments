/*
 * No changes are allowed in this file
 */
char array[5001];
int fib(int n) {
  if(n<2) return n;
  else return fib(n-1)+fib(n-2);
}

int _start() {
	int val = fib(40);
  array[5000]='a';
	return val;
}
