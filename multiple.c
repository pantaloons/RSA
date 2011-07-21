#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

#define ACCURACY 20
#define FACTOR_DIGITS 100
#define EXPONENT_MAX 1000
#define BUF_SIZE 1024

#define DATA_RADIX
#define RADIX 4294967296L
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
		b->data = calloc(b->capacity, sizeof(word));
	}
	b->data[0] = num;
}

/* Print a bignum as base 10 integer, this will only work if radix >= 10 */
void bignum_print(bignum* b) {
	bignum *copy = bignum_init(), *remainder = bignum_init();
	if(b->length == 0 || bignum_iszero(b)) printf("0");
	else {
		bignum_copy(b, copy);
		/* TODO: this prints in reverse */
		while(bignum_isnonzero(copy)) {
			bignum_idivide(copy, &NUMS[10], remainder);
			printf("%u", remainder->data[0]);
		}
		bignum_deinit(copy);
		bignum_deinit(remainder);
	}
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
		result->data = malloc(result->capacity * sizeof(word));
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
		result->data[n + 1] = 1;
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
		result->data = malloc(result->capacity * sizeof(word));
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
	int i, j, length = 0;
	word carry;
	unsigned long prod; /* Long for intermediate product... this is not portable and should probably be changed */
	if(b1->length + b2->length > result->capacity) {
		result->capacity = b1->length + b2->length;
		result->data = realloc(result->data, result->capacity * sizeof(word));
	}
	for(i = 0; i < b1->length + b2->length; i++) result->data[i] = 0;
	for(i = 0; i < b1->length; i++) {
		for(j = 0; j < b2->length; j++) {
			prod = (b1->data[i] * (long)b2->data[j]) + result->data[i+j]; /* This should not overflow... */
			carry = (int)(prod / RADIX);
			if(carry > 0 && i + j + 2 > length) length = i + j + 2;
			else if(i + j + 1 > length) length = i + j + 1;
			result->data[i+j+1] += carry; /* Carry */
			prod = (result->data[i+j] + b1->data[i] * (long)b2->data[j]) % RADIX; /* Again, should not overflow... */
			result->data[i+j] = prod; /* Add */
		}
	}
	result->length = length;
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
	word mult;
	int factor = 1, n, m, i, j, carry = 0, length = 0;
	unsigned long gquot, gtemp, grem;
	if(bignum_less(b1, b2)) {
		quotient->length = 0;
		bignum_copy(b1, remainder);
		return;
	}
	n = b1->length + 1;
	m = b2->length;
	if(quotient->capacity < n - m - 1) {
		quotient->capacity = n - m - 1;
		quotient->data = realloc(quotient->data, (n - m - 1) * sizeof(word));
	}
	bignum_copy(b1, b1copy);
	bignum_copy(b2, b2copy);
	/* Normalize.. multiply by 2 */
	while(b2copy->data[b2copy->length - 1] < HALFRADIX) {
		factor *= 2;
		for(i = 0; i < b2copy->length; i++) {
			mult = 2 * b2copy->data[i];
			b2copy->data[i] = mult; /* Already wrapped due to unsigned overflow */
			if(mult < b2copy->data[i]) { /* Wrapped around so there is a carry bit */
				b2copy->data[i+1] += 1; /* Note we do not need to realloc here because if the msb is < half radix, msb * 2 < radix */
			}
		}
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
			b1copy->data[n] = 0;
		}
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
		quottemp->length = 1;
		quottemp->data[0] = gquot % RADIX;
		quottemp->data[1] = (gquot / RADIX);
		bignum_multiply(temp2, b2copy, quottemp);
		if(temp3->length > temp3->capacity) {
			temp3->capacity = temp3->length;
			temp3->data = malloc(temp3->capacity * sizeof(word));
		}
		for(j = 0; j <= m; j++) temp3->data[j] = b1copy->data[i + j];
		if(temp3->data[m] == 0) temp3->length = m;
		else temp3->length = m + 1;
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
	for(i = b1copy->length - 1; i >= 0; i--) {
		gtemp = carry * RADIX + b1copy->data[i];
		b1copy->data[i] = gtemp/factor;
		if(b1copy->data[i] != 0 && length == 0) length = i + 1;
		carry = gtemp % factor;
	}
	b1copy->length = length;
	bignum_copy(b1copy, remainder);
	bignum_deinit(temp);
	bignum_deinit(temp2);
	bignum_deinit(temp3);
	bignum_deinit(b1copy);
	bignum_deinit(b2copy);
	bignum_deinit(quottemp);
}

int main(int argc, char** argv) {
	bignum* b1 = bignum_init();
	bignum* b2 = bignum_init();
	bignum* b3 = bignum_init();
	
	bignum_fromstring(b1, "214217125214217125214217125214217125214217125214217125214217125");
	bignum_fromstring(b2, "124792147217824197894821978124987214987421978247892412419782419");
	bignum_print(b1);
	printf("\n");
	bignum_print(b2);
	printf("\n");
	bignum_multiply(b3, b1, b2);
	bignum_print(b3);
	printf("\n");
	
	bignum_deinit(b1);
	bignum_deinit(b2);
	bignum_deinit(b3);

	return EXIT_SUCCESS;
}