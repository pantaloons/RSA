#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

#define ACCURACY 20
#define SINGLE_MAX 10000
#define EXPONENT_MAX 1000
#define BUF_SIZE 1024

#define DATA_RADIX
#define RADIX 4294967296L

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

word DATA0[1] = {0};
word DATA1[1] = {1};
word DATA2[1] = {2};
word DATA3[1] = {3};
word DATA4[1] = {4};
word DATA5[1] = {5};
word DATA6[1] = {6};
word DATA7[1] = {7};
word DATA8[1] = {8};
word DATA9[1] = {9};
word DATA10[1] = {10};
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

/* Print a bignum as base 10 integer, this will only work if radix >= 10 */
void bignum_print(bignum* b) {
	int i;
	bignum *copy = bignum_init(), *remainder = bignum_init();
	if(b->length == 0 || bignum_iszero(b)) printf("0");
	else {
		bignum_copy(b, copy);
		/* TODO: this prints in reverse */
		//while(bignum_isnonzero(copy)) {
			//bignum_idivide(copy, &NUMS[10], remainder);
			//printf("%d", remainder->data[0]);
		//}
		printf("%u", b->data[0]);
		bignum_deinit(copy);
		bignum_deinit(remainder);
	}
}

int bignum_iszero(bignum* b) {
	return b->length == 0 || b->length == 1 && b->data[0] == 0;
}

int bignum_isnonzero(bignum* b) {
	return !bignum_iszero(b);
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
	int sum, carry = 0, i;
	int n = MAX(b1->length, b2->length);
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
	int diff, carry = 0, length = 0, i;
	if(b1->length > result->capacity) {
		result->capacity = b1->length;
		result->data = malloc(result->capacity * sizeof(word));
	}
	for(i = 0; i < b1->length; i++) {
		diff = b1->data[i] - carry;
		if(i < b2->length) diff -= b2->data[i];
#ifdef DATA_RADIX
		if(diff < 0) diff = UINT_MAX - (-diff + 1); /* diff % radix */
#else
		if(diff < 0) diff = RADIX - (-diff);
#endif
		result->data[i] = diff;
		if(result->data[i] != 0) length = i + 1;
		if(i < b2->length && b1->data[i] < b2->data[i] + carry) carry = 1; /* Carry if lower digit is bigger (wrap under 0) */
		else carry = 0;
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
	int i, j, length = 0, carry;
	long prod;
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
	return;
}

int main(int argc, char** argv) {
	bignum* b1 = bignum_init();
	bignum* b2 = bignum_init();
	bignum* b3 = bignum_init();
	
	bignum_fromstring(b1, "99999999999999999999992147021782178942197824978217841297819278422");
	bignum_fromstring(b2, "99999999999999999999999999999999999999999999999999999999999999999");
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