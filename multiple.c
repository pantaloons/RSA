#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

#define ACCURACY 20
#define FACTOR_DIGITS 100
#define EXPONENT_MAX RAND_MAX
#define BUF_SIZE 1024

#define DATA_RADIX
#define RADIX 4294967296UL
#define HALFRADIX 2147483648

#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* http://gmplib.org/manual/Algorithms.html#Algorithms
* http://sputsoft.com/blog/2009/08/implementing-multiple-precision-arithmetic-part-2.html */

typedef unsigned int word;
typedef struct _bignum {
	int length;
	int capacity;
	word* data;
} bignum;

void bignum_iadd(bignum* source, bignum* add);
void bignum_add(bignum* result, bignum* b1, bignum* b2);
void bignum_isubtract(bignum* source, bignum* add);
void bignum_subtract(bignum* result, bignum* b1, bignum* b2);
void bignum_imultiply(bignum* source, bignum* add);
void bignum_multiply(bignum* result, bignum* b1, bignum* b2);
void bignum_idivide(bignum* source, bignum* div, bignum* remainder);
void bignum_divide(bignum* quotient, bignum* remainder, bignum* b1, bignum* b2);

word DATA0[1] = {0}; word DATA1[1] = {1}; word DATA2[1] = {2};
word DATA3[1] = {3}; word DATA4[1] = {4}; word DATA5[1] = {5};
word DATA6[1] = {6}; word DATA7[1] = {7}; word DATA8[1] = {8};
word DATA9[1] = {9}; word DATA10[1] = {10};

bignum NUMS[11] = {{1, 1, DATA0},{1, 1, DATA1},{1, 1, DATA2},
                   {1, 1, DATA3},{1, 1, DATA4},{1, 1, DATA5},
                   {1, 1, DATA6},{1, 1, DATA7},{1, 1, DATA8},
                   {1, 1, DATA9},{1, 1, DATA10}};

bignum* bignum_init() {
	bignum* b = malloc(sizeof(bignum));
	b->length = 0;
	b->capacity = 20;
	b->data = calloc(20, sizeof(word));
	return b;
}

/* Free resources used by a bignum */
void bignum_deinit(bignum* b) {
	free(b->data);
	free(b);
}

int bignum_iszero(bignum* b) {
	return b->length == 0 || (b->length == 1 && b->data[0] == 0);
}

int bignum_isnonzero(bignum* b) {
	return !bignum_iszero(b);
}

/* Copy bignum source into dest */
void bignum_copy(bignum* source, bignum* dest) {
	dest->length = source->length;
	dest->capacity = source->capacity;
	dest->data = realloc(dest->data, dest->capacity * sizeof(word));
	memcpy(dest->data, source->data, dest->length * sizeof(word));
}

/* Load a numeric string into the bignum, this will only work if radix >= 10 */
void bignum_fromstring(bignum* b, char* string) {
	int i, len = 0;
	while(string[len] != '\0') len++; /* Find string length */
	for(i = 0; i < len; i++) {
		if(i != 0) bignum_imultiply(b, &NUMS[10]); /* Base 10 multiply */
		bignum_iadd(b, &NUMS[string[i] - '0']); /* Add */
	}
}

void bignum_fromint(bignum* b, unsigned int num) {
	b->length = 1;
	if(b->capacity < b->length) {
		b->capacity = b->length;
		b->data = realloc(b->data, b->capacity * sizeof(word));
	}
	b->data[0] = num;
}

/* Print a bignum as base 10 integer, this will only work if radix >= 10 */
void bignum_print(bignum* b) {
	int cap = 100, len = 0, i;
	char* buffer = malloc(cap * sizeof(char));
	bignum *copy = bignum_init(), *remainder = bignum_init();
	if(b->length == 0 || bignum_iszero(b)) printf("0");
	else {
		bignum_copy(b, copy);
		while(bignum_isnonzero(copy)) {
			bignum_idivide(copy, &NUMS[10], remainder);
			buffer[len++] = remainder->data[0];
			if(len >= cap) {
				cap *= 2;
				buffer = realloc(buffer, cap * sizeof(char));
			}
		}
		for(i = len - 1; i >= 0; i--) printf("%d", buffer[i]);
		bignum_deinit(copy);
		bignum_deinit(remainder);
	}
	free(buffer);
}

int bignum_equal(bignum* b1, bignum* b2) {
	int i;
	if(bignum_iszero(b1) && bignum_iszero(b2)) return 1;
	else if(bignum_iszero(b1)) return 0;
	else if(bignum_iszero(b2)) return 0;
	else if(b1->length != b2->length) return 0;
	for(i = b1->length - 1; i >= 0; i--) {
		if(b1->data[i] != b2->data[i]) return 0;
	}
	return 1;
}

int bignum_greater(bignum* b1, bignum* b2) {
	int i;
	if(bignum_iszero(b1) && bignum_iszero(b2)) return 0;
	else if(bignum_iszero(b1)) return 0;
	else if(bignum_iszero(b2)) return 1;
	else if(b1->length != b2->length) return b1->length > b2->length;
	for(i = b1->length - 1; i >= 0; i--) {
		if(b1->data[i] != b2->data[i]) return b1->data[i] > b2->data[i];
	}
	return 0;
}

int bignum_less(bignum* b1, bignum* b2) {
	int i;
	if(bignum_iszero(b1) && bignum_iszero(b2)) return 0;
	else if(bignum_iszero(b1)) return 1;
	else if(bignum_iszero(b2)) return 0;
	else if(b1->length != b2->length) return b1->length < b2->length;
	for(i = b1->length - 1; i >= 0; i--) {
		if(b1->data[i] != b2->data[i]) return b1->data[i] < b2->data[i];
	}
	return 0;
}

int bignum_geq(bignum* b1, bignum* b2) {
	return !bignum_less(b1, b2);
}
int bignum_leq(bignum* b1, bignum* b2) {
	return !bignum_greater(b1, b2);
}

/* Perform an in place add into the source bignum */
void bignum_iadd(bignum* source, bignum* add) {
	bignum* temp = bignum_init();
	bignum_add(temp, source, add);
	bignum_copy(temp, source);
	bignum_deinit(temp);
}

/* Add two bignums */
void bignum_add(bignum* result, bignum* b1, bignum* b2) {
	word sum, carry = 0;
	int i, n = MAX(b1->length, b2->length);
	if(n + 1 > result->capacity) {
		result->capacity = n + 1;
		result->data = realloc(result->data, result->capacity * sizeof(word));
	}
	for(i = 0; i < n; i++) {
		sum = carry;
		if(i < b1->length) sum += b1->data[i];
		if(i < b2->length) sum += b2->data[i];
#ifdef DATA_RADIX
		result->data[i] = sum; /* Already taken mod 2^32 by unsigned wrap around */
#else
		result->data[i] = sum % RADIX;
#endif

		if(i < b1->length) { 
#ifdef DATA_RADIX
			if(sum < b1->data[i]) carry = 1; /* Result must have wrapped 2^32 so carry bit is 1 */
#else
			if(sum >= RADIX) carry = 1; /* Result >= radix so carry bit 1 */
#endif
			else carry = 0;
		}
		else {
#ifdef DATA_RADIX
			if(sum < b2->data[i]) carry = 1; /* Result must have wrapped 2^32 so carry bit is 1 */
#else
			if(sum >= RADIX) carry = 1; /* Result >= radix so carry bit 1 */
#endif
			else carry = 0;
		}
	}
	if(carry == 1) {
		result->length = n + 1;
		result->data[n] = 1;
	}
	else {
		result->length = n;
	}
}

/* Perform an in place subtract into the source bignum */
void bignum_isubtract(bignum* source, bignum* sub) {
	bignum* temp = bignum_init();
	bignum_subtract(temp, source, sub);
	bignum_copy(temp, source);
	bignum_deinit(temp);
}

/* Subtract bignum b2 from b1. Result is undefined if b2 > b1, (probably error) */
void bignum_subtract(bignum* result, bignum* b1, bignum* b2) {
	int length = 0, i;
	word carry = 0, diff, temp;
	if(b1->length > result->capacity) {
		result->capacity = b1->length;
		result->data = realloc(result->data, result->capacity * sizeof(word));
	}
	for(i = 0; i < b1->length; i++) {
		temp = carry;
		if(i < b2->length) temp = temp + b2->data[i]; /* Auto wrapped mod RADIX */
		diff = b1->data[i] - temp;
		if(temp > b1->data[i]) carry = 1;
		else carry = 0;
		result->data[i] = diff;
		if(result->data[i] != 0) length = i + 1;
	}
	result->length = length;
}

/* Perform an in place multiply into the source bignum */
void bignum_imultiply(bignum* source, bignum* mult) {
	bignum* temp = bignum_init();
	bignum_multiply(temp, source, mult);
	bignum_copy(temp, source);
	bignum_deinit(temp);
}

/* Multiply two bignums by the naive school method */
void bignum_multiply(bignum* result, bignum* b1, bignum* b2) {
	int i, j, k;
	word carry;
	unsigned long long int prod; /* Long for intermediate product... this is not portable and should probably be changed */
	if(b1->length + b2->length > result->capacity) {
		result->capacity = b1->length + b2->length;
		result->data = realloc(result->data, result->capacity * sizeof(word));
	}
	for(i = 0; i < b1->length + b2->length; i++) result->data[i] = 0;
	
	for(i = 0; i < b1->length; i++) {
		for(j = 0; j < b2->length; j++) {
			prod = (b1->data[i] * (unsigned long long int)b2->data[j]) + (unsigned long long int)(result->data[i+j]); /* This should not overflow... */
			carry = (unsigned int)(prod / RADIX);
			
			/* Add carry to the next word over, but this may cause further overflow.. propogate */
			k = 1;
			while(carry > 0) {
				unsigned int temp = result->data[i+j+k] + carry;
				if(temp < result->data[i+j+k]) carry = 1;
				else carry = 0;
				result->data[i+j+k] = temp; /* Already wrapped in unsigned arithmetic */
				k++;
			}
			
			prod = (result->data[i+j] + b1->data[i] * (unsigned long long int)b2->data[j]) % RADIX; /* Again, should not overflow... */
			result->data[i+j] = prod; /* Add */
		}
	}
	if(b1->length + b2->length > 0 && result->data[b1->length + b2->length - 1] == 0) result->length = b1->length + b2->length - 1;
	else result->length = b1->length + b2->length;
}

void bignum_idivide(bignum* source, bignum* div, bignum* remainder) {
	bignum *q = bignum_init(), *r = bignum_init();
	bignum_divide(q, r, source, div);
	bignum_copy(q, source);
	bignum_copy(r, remainder);
	bignum_deinit(q);
	bignum_deinit(r);
}

/* Divide two bignums by the naive long division method. This does integer division (b1 / b2)*/
void bignum_divide(bignum* quotient, bignum* remainder, bignum* b1, bignum* b2) {
	bignum *b2copy = bignum_init(), *b1copy = bignum_init();
	bignum *temp = bignum_init(), *temp2 = bignum_init(), *temp3 = bignum_init();
	bignum* quottemp = bignum_init();
	word carry = 0;
	int n, m, i, j, length = 0;
	unsigned long long factor = 1;
	unsigned long long gquot, gtemp, grem;
	if(bignum_less(b1, b2)) {
		quotient->length = 0;
		bignum_copy(b1, remainder);
	}
	else if(bignum_iszero(b1)) {
		quotient->length = 0;
		bignum_fromint(remainder, 0);
	}
	else if(b2->length == 1) { /* We can do simple division */
		if(quotient->capacity < b1->length) {
			quotient->capacity = b1->length;
			quotient->data = realloc(quotient->data, quotient->capacity * sizeof(word));
		}
		for(i = b1->length - 1; i >= 0; i--) {
			gtemp = carry * RADIX + b1->data[i];
			gquot = gtemp / b2->data[0];
			quotient->data[i] = gquot;
			if(quotient->data[i] != 0 && length == 0) length = i + 1;
			carry = gtemp % b2->data[0];
		}
		bignum_fromint(remainder, carry);
		quotient->length = length;
	}
	else { /* Regular long division */
		n = b1->length + 1;
		m = b2->length;
		if(quotient->capacity < n - m) {
			quotient->capacity = n - m;
			quotient->data = realloc(quotient->data, (n - m) * sizeof(word));
		}
		bignum_copy(b1, b1copy);
		bignum_copy(b2, b2copy);
		/* Normalize.. multiply by 2 until MSB >= HALFRADIX */
		while(b2copy->data[b2copy->length - 1] < HALFRADIX) {
			factor *= 2;
			bignum_imultiply(b2copy, &NUMS[2]);
		}
		if(factor > 1) {
			bignum_fromint(temp, factor);
			bignum_imultiply(b1copy, temp);
		}
		/* Ensure len + 1th word of b1 exists (zero if necessary) */
		if(b1copy->length != n) {
			b1copy->length++;
			if(b1copy->length > b1copy->capacity) {
				b1copy->capacity = b1copy->length;
				b1copy->data = realloc(b1copy->data, b1copy->capacity * sizeof(word));
			}
			b1copy->data[n - 1] = 0;
		}
		
		/* Process quotient by long division */
		for(i = n - m - 1; i >= 0; i--) {
			gtemp = RADIX * b1copy->data[i + m] + b1copy->data[i + m - 1];
			gquot = gtemp / b2copy->data[m - 1];
			if(gquot >= RADIX) gquot = UINT_MAX;
			grem = gtemp % b2copy->data[m - 1];
			while(grem < RADIX && gquot * b2copy->data[m - 2] > RADIX * grem + b1copy->data[i + m - 2]) { /* Should not overflow... ? */
				gquot--;
				grem += b2copy->data[m - 1];
			}
			quottemp->data[0] = gquot % RADIX;
			quottemp->data[1] = (gquot / RADIX);
			if(quottemp->data[1] != 0) quottemp->length = 2;
			else quottemp->length = 1;
			bignum_multiply(temp2, b2copy, quottemp);
			if(m + 1 > temp3->capacity) {
				temp3->capacity = m + 1;
				temp3->data = realloc(temp3->data, temp3->capacity * sizeof(word));
			}
			temp3->length = 0;
			for(j = 0; j <= m; j++) {
				temp3->data[j] = b1copy->data[i + j];
				if(temp3->data[j] != 0) temp3->length = j + 1;
			}
			if(bignum_less(temp3, temp2)) {
				bignum_iadd(temp3, b2copy);
				gquot--;
			}
			bignum_isubtract(temp3, temp2);
			for(j = 0; j < temp3->length; j++) b1copy->data[i + j] = temp3->data[j];
			for(j = temp3->length; j <= m; j++) b1copy->data[i + j] = 0;
			quotient->data[i] = gquot;
			if(quotient->data[i] != 0) quotient->length = i;
		}
		
		if(quotient->data[b1->length - b2->length] == 0) quotient->length = b1->length - b2->length;
		else quotient->length = b1->length - b2->length + 1;
		
		/* Divide by factor now to find final remainder */
		carry = 0;
		for(i = b1copy->length - 1; i >= 0; i--) {
			gtemp = carry * RADIX + b1copy->data[i];
			b1copy->data[i] = gtemp/factor;
			if(b1copy->data[i] != 0 && length == 0) length = i + 1;
			carry = gtemp % factor;
		}
		b1copy->length = length;
		bignum_copy(b1copy, remainder);
	}
	bignum_deinit(temp);
	bignum_deinit(temp2);
	bignum_deinit(temp3);
	bignum_deinit(b1copy);
	bignum_deinit(b2copy);
	bignum_deinit(quottemp);
}

void bignum_modpow(bignum* b1, bignum* b2, bignum* b3, bignum* result) {
	bignum *a = bignum_init(), *b = bignum_init(), *c = bignum_init();
	bignum *discard = bignum_init(), *remainder = bignum_init();
	bignum_copy(b1, a);
	bignum_copy(b2, b);
	bignum_copy(b3, c);
	bignum_fromint(result, 1);
	while(bignum_greater(b, &NUMS[0])) {
		if(b->data[0] & 1) {
			bignum_imultiply(result, a);
			bignum_divide(discard, remainder, result, c);
			bignum_copy(remainder, result);
		}
		bignum_idivide(b, &NUMS[2], discard);
		bignum_copy(a, discard);
		bignum_imultiply(a, discard);
		bignum_divide(discard, remainder, a, c);
		bignum_copy(remainder, a);
	}
	bignum_deinit(a);
	bignum_deinit(b);
	bignum_deinit(c);
	bignum_deinit(discard);
	bignum_deinit(remainder);
}

void bignum_gcd(bignum* b1, bignum* b2, bignum* result) {
	bignum *a = bignum_init(), *b = bignum_init(), *remainder = bignum_init();
	bignum *temp = bignum_init(), *discard = bignum_init();
	bignum_copy(b1, a);
	bignum_copy(b2, b);
	while(!bignum_equal(b, &NUMS[0])) {
		bignum_copy(b, temp);
		bignum_divide(discard, remainder, a, b);
		bignum_copy(remainder, b);
		bignum_copy(temp, a);
	}
	bignum_copy(a, result);
	bignum_deinit(a);
	bignum_deinit(b);
	bignum_deinit(remainder);
	bignum_deinit(temp);
	bignum_deinit(discard);
}

/* Compute a^-1 mod m, note we are finding the inverse in a finite field
   so some trickery is done to avoid subtract or negative numbers... */
void bignum_inverse(bignum* a, bignum* m, bignum* result) {
	bignum *remprev = bignum_init(), *rem = bignum_init();
	bignum *auxprev = bignum_init(), *aux = bignum_init();
	bignum *rcur = bignum_init(), *qcur = bignum_init(), *acur = bignum_init();
	bignum *discard = bignum_init();
	
	bignum_copy(m, remprev);
	bignum_copy(a, rem);
	bignum_fromint(auxprev, 0);
	bignum_fromint(aux, 1);
	while(bignum_greater(rem, &NUMS[1])) {
		bignum_divide(qcur, rcur, remprev, rem);
		bignum_subtract(acur, m, qcur);
		bignum_imultiply(acur, aux);
		bignum_iadd(acur, auxprev);
		bignum_divide(discard, remprev, acur, m);
		bignum_copy(remprev, acur);
		
		bignum_copy(rem, remprev);
		bignum_copy(aux, auxprev);
		bignum_copy(rcur, rem);
		bignum_copy(acur, aux);
	}
	
	bignum_copy(acur, result);
	
	bignum_deinit(remprev);
	bignum_deinit(rem);
	bignum_deinit(auxprev);
	bignum_deinit(aux);
	bignum_deinit(rcur);
	bignum_deinit(qcur);
	bignum_deinit(acur);
	bignum_deinit(discard);
}

int bignum_jacobi(bignum* ac, bignum* nc) {
	bignum *remainder = bignum_init(), *twos = bignum_init();
	bignum *discard = bignum_init(), *temp = bignum_init();
	bignum *a = bignum_init(), *n = bignum_init();
	int mult = 1, result = 0;
	bignum_copy(ac, a);
	bignum_copy(nc, n);
	while(bignum_greater(a, &NUMS[1]) && !bignum_equal(a, n)) {
		bignum_idivide(a, n, remainder);
		bignum_copy(remainder, a); /* a = a % n */
		if(bignum_leq(a, &NUMS[1]) || bignum_equal(a, n)) break;
		bignum_fromint(twos, 0);
		while(a->data[0] % 2 == 0) {
			bignum_iadd(twos, &NUMS[1]);
			bignum_idivide(a, &NUMS[2], remainder);
		}
		if(bignum_greater(twos, &NUMS[0]) && twos->data[0] % 2 == 1) {
			bignum_divide(discard, remainder, n, &NUMS[8]);
			if(!bignum_equal(remainder, &NUMS[1]) && !bignum_equal(remainder, &NUMS[7])) {
				mult *= -1;
			}
		}
		if(bignum_leq(a, &NUMS[1]) || bignum_equal(a, n)) break;
		bignum_divide(discard, remainder, n, &NUMS[4]);
		bignum_divide(discard, temp, a, &NUMS[4]);
		if(!bignum_equal(remainder, &NUMS[1]) && !bignum_equal(temp, &NUMS[1])) mult *= -1;
		bignum_copy(a, temp);
		bignum_copy(n, a);
		bignum_copy(temp, n);
	}
	if(bignum_equal(a, &NUMS[1])) result = mult;
	else result = 0;
	bignum_deinit(remainder);
	bignum_deinit(twos);
	bignum_deinit(discard);
	bignum_deinit(temp);
	bignum_deinit(a);
	bignum_deinit(n);
	return result;
}

/* Check whether a is a Euler witness for n */
int solovayPrime(int a, bignum* n) {
	bignum* ab = bignum_init(), *res = bignum_init(), *pow = bignum_init();
	bignum* remainder = bignum_init(), *modpow = bignum_init();
	int x, result;

	bignum_fromint(ab, a);
	x = bignum_jacobi(ab, n);
	if(x == -1) bignum_subtract(res, n, &NUMS[1]);
	else bignum_fromint(res, x);
	bignum_copy(n, pow);
	bignum_isubtract(pow, &NUMS[1]);
	bignum_idivide(pow, &NUMS[2], remainder);
	bignum_modpow(ab, pow, n, modpow);
	
	result = !bignum_equal(res, &NUMS[0]) && bignum_equal(modpow, res);
	bignum_deinit(ab);
	bignum_deinit(res);
	bignum_deinit(pow);
	bignum_deinit(remainder);
	bignum_deinit(modpow);
	return result;
}

/* Test if n is probably prime, using accuracy of k (k solovay tests) */
int probablePrime(bignum* n, int k) {
	if(bignum_equal(n, &NUMS[2])) return 1;
	else if(n->data[0] % 2 == 0 || bignum_equal(n, &NUMS[1])) return 0;
	while(k-- > 0) {
		if(n->length <= 1) { /* Prevent a > n */
			if(!solovayPrime(rand() % (n->data[0] - 2) + 2, n)) return 0;
		}
		else {
			int wit = rand() % (RAND_MAX - 2) + 2;
			if(!solovayPrime(wit, n)) return 0;
		}
	}
	return 1;
}

void randPrime(int numDigits, bignum* result) {
	char *string = malloc(numDigits+1 * sizeof(char));
	int i;
	string[0] = (rand() % 9) + '1'; /* No leading zeros */
	string[numDigits - 1] = (rand() % 5) * 2 + '1'; /* Last digit is odd */
	for(i = 1; i < numDigits - 1; i++) string[i] = (rand() % 10) + '0';
	string[numDigits] = '\0';
	bignum_fromstring(result, string);
	while(1) {
		if(probablePrime(result, ACCURACY)) {
			free(string);
			return;
		}
		bignum_iadd(result, &NUMS[2]); /* result += 2 */
	}
}

void randExponent(bignum* phi, int n, bignum* result) {
	bignum* gcd = bignum_init();
	int e = rand() % n;
	while(1) {
		bignum_fromint(result, e);
		bignum_gcd(result, phi, gcd);
		if(bignum_equal(gcd, &NUMS[1])) {
			bignum_deinit(gcd);
			return;
		}
		e = (e + 1) % n;
		if(e <= 2) e = 3;
	}
}

/* Read the file fd into an array of bytes for encoding and transmission */
int readFile(FILE* fd, char** buffer, int bytes) {
	int len = 0, cap = BUF_SIZE, r;
	char buf[BUF_SIZE];
	*buffer = malloc(BUF_SIZE * sizeof(char));
	while((r = fread(buf, sizeof(char), BUF_SIZE, fd)) > 0) {
		if(len + r >= cap) {
			cap *= 2;
			*buffer = realloc(*buffer, cap);
		}
		memcpy(&(*buffer)[len], buf, r);
		len += r;
	}
	/* Pad the last block with zeros to signal end of cryptogram. An additional block is added if there is no room */
	if(len + bytes - len % bytes > cap) *buffer = realloc(*buffer, len + bytes - len % bytes);
	do {
		(*buffer)[len] = '\0';
		len++;
	}
	while(len % bytes != 0);
	return len;
}

/* Encode message m using public exponent and modulus, c = m^e mod n */
void encode(bignum* m, bignum* e, bignum* n, bignum* result) {
	bignum_modpow(m, e, n, result);
}

/* Decode encrypted message c using private exponent and public modulus, m = c^d mod n */
void decode(bignum* c, bignum* d, bignum* n, bignum* result) {
	bignum_modpow(c, d, n, result);
}

int main(void) {
	int i, j, bytes, len;
	bignum *p = bignum_init(), *q = bignum_init(), *n = bignum_init();
	bignum *phi = bignum_init(), *e = bignum_init(), *d = bignum_init();
	bignum *x = bignum_init(), *bbytes = bignum_init(), *shift = bignum_init();
	bignum *temp1 = bignum_init(), *temp2 = bignum_init(), *temp3 = bignum_init();
	
	bignum *encoded;
	char *buffer, *decoded;
	FILE* f;
	
	srand(time(NULL));
	
	randPrime(FACTOR_DIGITS, p);
	printf("Got first prime factor, p = ");
	bignum_print(p);
	printf(" ... ");
	getchar();
	
	randPrime(FACTOR_DIGITS, q);
	printf("Got second prime factor, q = ");
	bignum_print(q);
	printf(" ... ");
	getchar();
	
	bignum_multiply(n, p, q);
	printf("Got modulus, n = pq = ");
	bignum_print(n);
	printf(" ... ");
	getchar();
	
	bignum_subtract(temp1, p, &NUMS[1]);
	bignum_subtract(temp2, q, &NUMS[1]);
	bignum_multiply(phi, temp1, temp2); /* phi = (p - 1) * (q - 1) */
	printf("Got totient, phi = ");
	bignum_print(phi);
	printf(" ... ");
	getchar();
	
	randExponent(phi, EXPONENT_MAX, e);
	printf("Chose public exponent, e = ");
	bignum_print(e);
	printf("\nPublic key is (");
	bignum_print(e);
	printf(", ");
	bignum_print(n);
	printf(") ... ");
	getchar();
	
	bignum_inverse(e, phi, d);
	printf("Calculated private exponent, d = ");
	bignum_print(d);
	printf("\nPrivate key is (");
	bignum_print(d);
	printf(", ");
	bignum_print(n);
	printf(") ... ");
	getchar();
	
	/* Compute maximum number of bytes that can be encoded in one encryption */
	bytes = -1;
	bignum_fromint(shift, 1 << 7); /* 7 bits per char */
	bignum_fromint(bbytes, 1);
	while(bignum_less(bbytes, n)) {
		bignum_imultiply(bbytes, shift); /* Shift by one byte, TODO: we use bitmask representative so this can actually be a shift... */
		bytes++;
	}

	printf("Opening file \"text.txt\" for reading\n");
	f = fopen("text.txt", "r");
	if(f == NULL) {
		printf("Failed to open file \"text.txt\". Does it exist?\n");
		return EXIT_FAILURE;
	}
	len = readFile(f, &buffer, bytes); /* len will be a multiple of bytes, to send whole chunks */
	
	printf("File \"text.txt\" read successfully, %d bytes read. Encoding byte stream in chunks of %d bytes ... ", len, bytes);
	getchar();
	printf("\n");
	
	/* Calloc works here because capacity = 0 forces a realloc by callees but we should really
	 * bignum_init() all of these */
	encoded = calloc(len/bytes, sizeof(bignum));
	for(i = 0; i < len; i += bytes) {
		bignum_fromint(x, 0);
		bignum_fromint(temp2, 128);
		bignum_fromint(temp1, 1);
		/* Compute buffer[0] + buffer[1]*128 + buffer[2]*128^2 etc (base 128 representation for characters->int encoding)*/
		for(j = 0; j < bytes; j++) {
			bignum_fromint(temp3, buffer[i + j]);
			bignum_imultiply(temp3, temp1);
			bignum_iadd(x, temp3); /*x += buffer[i + j] * (1 << (7 * j)) */
			bignum_imultiply(temp1, temp2);
		}
		encode(x, e, n, &encoded[i/bytes]);
		bignum_print(&encoded[i/bytes]);
		printf(" ");
	}
	printf("\n\nEncoding finished successfully ... ");
	getchar();
	
	printf("Decoding encoded message ... ");
	getchar();
	printf("\n");
	decoded = malloc(len * sizeof(int));
	for(i = 0; i < len/bytes; i++) {
		decode(&encoded[i], d, n, x);
		bignum_fromint(temp2, 128);
		for(j = 0; j < bytes; j++) {			
			bignum_divide(bbytes, shift, x, temp2); /* shift = x mod 128 */
			if(shift->length == 0) decoded[i*bytes + j] = (char)0;
			else decoded[i*bytes + j] = (char)(shift->data[0]);
			printf("%c", decoded[i*bytes + j]);
			bignum_idivide(x, temp2, shift); /* x = x / 128 */
		}
	}
	printf("\n\nFinished RSA demonstration ... ");
	getchar();
	
	for(i = 0; i < len/bytes; i++) free(encoded[i].data);
	free(encoded);
	free(decoded);
	free(buffer);
	bignum_deinit(p);
	bignum_deinit(q);
	bignum_deinit(n);
	bignum_deinit(phi);
	bignum_deinit(e);
	bignum_deinit(d);
	bignum_deinit(x);
	bignum_deinit(bbytes);
	bignum_deinit(shift);
	bignum_deinit(temp1);
	bignum_deinit(temp2);
	bignum_deinit(temp3);
	fclose(f);
	
	return EXIT_SUCCESS;
}