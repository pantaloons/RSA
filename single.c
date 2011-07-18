#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define ACCURACY 20
#define SINGLE_MAX 10000
#define EXPONENT_MAX 1000

/* Computes a^b mod c */
int modpow(long a, long b, long c) {
	long res = 1;
	while(b > 0) {
		if(b & 1) res = (res * a) % c; /* Need longs else this will overflow... */
		b = b >> 1;
		a = (a * a) % c;
	}
	return (int)res;
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
	if(n == 2) return 1;
	else if(n % 2 == 0 || n == 1) return 0;
	while(k-- > 0) {
		if(!solovayPrime(rand() % (n - 2) + 2, n)) return 0;
	}
	return 1;
}

/* Find a random (probable) prime between 3 and n - 1, this distribution is nowhere near uniform, see prime gaps */
int randPrime(int n) {
	int prime = rand() % n;
	n += n % 2; /* n needs to be even so modulo wrapping preserves oddness */
	prime += 1 - prime % 2;
	while(1) {
		if(probablePrime(prime, ACCURACY)) return prime;
		prime = (prime + 2) % n;
	}
}

/* Find a random exponent x between 3 and n - 1 such that gcd(x, phi) = 1, this distribution is similarly nowhere near uniform */
int randExponent(int phi, int n) {
	int e = rand() % n;
	while(1) {
		if(gcd(e, phi) == 1) return e;
		e = (e + 1) % n;
		if(e <= 2) e = 3;
	}
}

/* Compute gcd(a, b) */
int gcd(int a, int b) {
	int temp;
	while(b != 0) {
		temp = b;
		b = a % b;
		a = temp;
	}
	return a;
}

/* Compute n^-1 mod m */
int inverse(int n, int modulus) {
	int a = n, b = modulus;
	int x = 0, y = 1, x0 = 1, y0 = 0, q, temp;
	while(b != 0) {
		q = a / b;
		temp = a % b;
		a = b;
		b = temp;
		temp = x; x = x0 - q * x; x0 = temp;
		temp = y; y = y0 - q * y; y0 = temp;
	}
	if(x0 < 0) x0 += modulus;
	return x0;
}

/* Read the file fd into an array of bytes for encoding and transmission */
int readFile(FILE* fd, char** buffer) {
	int len = 0, cap = 1024, i, r, next;
	char buf[1024];
	*buffer = malloc(1024 * sizeof(char));
	while((r = fread(buf, sizeof(char), 1024, fd)) > 0) {
		if(len + r >= cap) {
			cap *= 2;
			*buffer = realloc(*buffer, cap);
		}
		memcpy(&(*buffer)[len], buf, r);
		len += r;
	}
	*buffer = realloc(*buffer, cap * 2);
	do {
		(*buffer)[len] = '\0';
		len++;
	}
	while(len % 4 != 0);
	return len;
}

/* Encode message m using public exponent and modulus, c = m^e mod n */
int encode(int m, int e, int n) {
	printf("enc: %d %d %d %d\n", m, e, n, modpow(m, e, n));
	return modpow(m, e, n);
}

/* Decode encrypted message c using private exponent and public modulus, m = c^d mod n */
int decode(int c, int d, int n) {
	printf("dec: %d %d %d %d\n", c, d, n, modpow(c, d, n));
	return modpow(c, d, n);
}

int main(int argc, char** argv) {
	int p, q, n, phi, e, d, i, x;
	int *encoded, *decoded;
	srand(time(NULL));
	p = randPrime(SINGLE_MAX);
	p = 3359;
	printf("Got first prime factor, p = %d ... ", p);
	getchar();
	q = randPrime(SINGLE_MAX);
	q = 9907;
	printf("Got second prime factor, q = %d ... ", q);
	getchar();
	n = p * q;
	printf("Got modulus, n = pq = %d ... ", n);
	getchar();
	phi = (p - 1) * (q - 1);
	printf("Got totient, phi = %d ... ", phi);
	getchar();
	e = randExponent(phi, EXPONENT_MAX);
	e = 853;
	printf("Chose public exponent, e = %d\nPublic key is (%d, %d) ... ", e, e, n);
	getchar();
	d = inverse(e, phi);
	printf("Calculated private exponent, d = %d\nPrivate key is (%d, %d) ... ", d, d, n);
	getchar();
	
	printf("Opening file \"text.txt\" for reading\n");
	FILE* f = fopen("text.txt", "r");
	if(f == NULL) {
		printf("Failed to open file \"text.txt\". Does it exist?\n");
		return EXIT_FAILURE;
	}
	char* buffer;
	int len = readFile(f, &buffer); /* len will be a multiple of 4, to send integer chunks */
	
	printf("File \"text.txt\" read successfully, %d bytes read. Encoding byte stream ... ", len);
	getchar();
	encoded = malloc((len/4) * sizeof(int));
	for(i = 0; i < len; i += 4) {
		encoded[i/4] = encode(buffer[i] + buffer[i + 1]*256 + buffer[i + 2]*65536 + buffer[i + 3]*16777216, e, n);
		printf("%d ", encoded[i/4]);
	}
	printf("\nEncoding finished successfully ... ");
	getchar();
	
	printf("Decoding encoded message ... ");
	getchar();
	decoded = malloc(len * sizeof(int));
	for(i = 0; i < len/4; i++) {
		int x = decode(encoded[i], d, n);
		decoded[i*4] = x % 256;
		decoded[i*4 + 1] = (x >> 8) % 256;
		decoded[i*4 + 2] = (x >> 16) % 256;
		decoded[i*4 + 3] = (x >> 24) % 256;
	}
	for(i = 0; i < len; i++) {
		if(decoded[i] == '\0') break;
		printf("%d: %c\n", decoded[i], (char)decoded[i]);
	}
	
	free(encoded);
	free(buffer);
}