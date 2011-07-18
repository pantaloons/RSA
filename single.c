#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Computes a^b mod c */
int modpow(int a, int b, int c) {
	int res = 1;
	while(b > 0) {
		if(b & 1) res = (res * a) % c;
		b = b >> 1;
		a = (a * a) % c;
	}
	return res;
}

/* Computes the Jacobi symbol, (a, n) */
int jacobi(int a, int n) {
	int twos, temp;
	int mult = 1;
	while(a > 1 && a != n) {
		a = a % n;
		if(a <= 1 || a == n) break;
		twos = 0;
		while(a % 2 == 0 && ++twos) a /= 2; /* Factor out multiples of 2 */
		if(twos > 0 && twos % 2 == 1) mult *= (n % 8 == 1 || n % 8 == 7) * 2 - 1;
		if(a <= 1 || a == n) break;
		if(n % 4 != 1 && a % 4 != 1) mult *= -1; /* Coefficient for flipping */
		temp = a;
		a = n;
		n = temp;
	}
	if(a == 0) return 0;
	else if(a == 1) return mult;
	else return 0; /* a == n => gcd(a, n) != 1 */
}

/* Check whether a is a Euler witness for n */
int solovayPrime(int a, int n) {
	int x = jacobi(a, n);
	if(x == -1) x = n - 1;
	return x != 0 && modpow(a, (n - 1)/2, n) == x;
}

/* Test if n is probably prime, using accuracy of k (k solovay tests) */
int probablePrime(int n, int k) {
	while(k-- > 0) {
		if(!solovayPrime(rand() % (n - 2) + 2, n)) return 0;
	}
	return 1;
}

int main(int argc, char** argv) {
	int i = 0;
	srand(time(NULL));
	for(i = 3; i < 100; i += 2) {
		if(probablePrime(i, 10)) printf("%d is probably prime\n", i);
		else printf("%d is not prime\n", i);
	}
	return 0;
}