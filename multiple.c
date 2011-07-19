#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define ACCURACY 20
#define SINGLE_MAX 10000
#define EXPONENT_MAX 1000
#define BUF_SIZE 1024

typedef unsigned char word;
typedef struct _bignum {
	int length;
	int capacity;
	word* data;
} bignum;

bignum* bignum_init() {
	bignum* b = malloc(sizeof(bignum));
	b->length = 0;
	b->capacity = 20;
	b->data = calloc(20, sizeof(word));
	return b;
}

/* Load a numeric string into the bignum */
void bignum_fromstring(bignum* b, char* string) {
	int len = 0, i, length = 0;
	while(string[len] != '\0') len++; /* Find string length */
	if(len > b->capacity) {
		b->capacity = len;
		b->data = calloc(len, sizeof(word));
	}
	
	for(i = len - 1; i >= 0; i--) {
		b->data[len - i - 1] = string[i] - '0';
		if(b->data[i] > 0) length = len - i;
	}
	b->length = length;
}

void bignum_print(bignum* b) {
	int i;
	if(b->length == 0) printf("0");
	else {
		for(i = b->length - 1; i >= 0; i--) printf("%d", b->data[i]);
	}
}

/* Free resources used by a bignum */
void bignum_deinit(bignum* b) {
	free(b->data);
	free(b);
}

/* Multiply two bignums by the naive school method */
void bignum_multiply(bignum* result, bignum* b1, bignum* b2) {
	int i, j;
	if(b1->length + b2->length + 1 > result->capacity) {
		result->capacity = b1->length + b2->length + 1;
		result->data = realloc(result->data, result->capacity * sizeof(word));
	}
	for(i = 0; i < b1->length + b2->length + 1; i++) result->data[i] = 0;
	for(i = 0; i < b1->length; i++) {
		for(j = 0; j < b2->length; j++) {
			result->data[i+j+1] += (result->data[i+j] + b1->data[i] * b2->data[j]) / 10; /* Carry */
			result->data[i+j] = (result->data[i+j] + b1->data[i] * b2->data[j]) % 10; /* Add */
		}
	}
	for(i = b1->length + b2->length + 1; i >= 0; i--) { /* Find new length */
		if(result->data[i] != 0) break;
		result->length = i;
	}
}

int main(int argc, char** argv) {
	bignum* b1 = bignum_init();
	bignum* b2 = bignum_init();
	bignum* b3 = bignum_init();
	
	bignum_fromstring(b1, "99999999999999999999999999999999999999999999999999999");
	bignum_fromstring(b2, "99999999999999999999999999999999999999999999999999991");
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