#include <string.h>
#include <stdio.h>
#include <inttypes.h> // (u)int64_t
#include <stdlib.h> // malloc()
#include <assert.h>
#include <sys/types.h> // stat()
#include <sys/stat.h>
#include <unistd.h>
#include <time.h> // clock()



// el128 is a 128 bit hashing algorithm based on the Merkle-Damgård construction
// el128 is designed to run fast, not to be cryptographically strong

#define EL128_BLOCK_SIZE 128
#define EL128_BLOCK_BYTES (EL128_BLOCK_SIZE * sizeof(uint64_t))

// 64 hex digits of ln 2

#define EL128_A0 0xb17217f7d1cf79abULL
#define EL128_B0 0xc9e3b39803f2f6afULL
#define EL128_C0 0x40f343267298b62dULL
#define EL128_D0 0x8a0d175b8baafa2bULL

// 96 hex digits of euler's constant - 2

#define XYZZY_0 0xb7e151628aed2a6aULL
#define XYZZY_1 0xbf7158809cf4f3c7ULL
#define XYZZY_2 0x62e7160f38b4da56ULL
#define XYZZY_3 0xa784d9045190cfefULL
#define XYZZY_4 0x324e7738926cfbe5ULL
#define XYZZY_5 0xf4bf8d8d8c31d763ULL

struct el128_t
{
	uint64_t A, B, C, D;
	unsigned char *uc_block;
	uint64_t *u64_block;
	int bfilled;
	uint64_t bytes;
};

void el128_init(struct el128_t *el128)
{
	el128->u64_block = malloc(EL128_BLOCK_SIZE * sizeof(uint64_t));
	el128->uc_block = (unsigned char *)el128->u64_block;
}

void el128_destroy(struct el128_t *el128)
{
	free(el128->u64_block);
}

static void _el128_block(struct el128_t *el128)
{
	int i;
	uint64_t *X, A, B, C, D;

//	fprintf(stderr, "_el128_block(), bytes=%lu\n", el128->bfilled + el128->bytes);
	X = el128->u64_block;
	A = el128->A;
	B = el128->B;
	C = el128->C;
	D = el128->D;

	for(i = 0; i < EL128_BLOCK_SIZE / 8; ++i)
	{
		A ^= (D << 7) + (D >> 11) + *X++;
		B += A ^ *X++;
		C += (B << 5) + (B >> 23) + *X++;
		D += (A >> 6) + C + *X++;

		A ^= (D << 27) + XYZZY_0;
		B += A;	C += B;	D += C;

		A ^= ((D << 12) + (D >> 19)) | ~C;
		B += A;
		C += B;
		D += C ^ *X++;

		A ^= D >> 16;
		B += A;	C += B;	D += C;

		A ^= (D >> 22) | B;
		B += A;	C += B;	D += C;

		A ^= (D >> 13) + XYZZY_1;
		B += A;	C += B;	D += C;

		A ^= (D << 10) + (D >> 5) + *X++;
		B += A ^ *X++;
		C += (B << 9) + (B >> 20) + *X++;
		D += C;

		A ^= D >> 31;
		B += A;	C += B;	D += C;

		A ^= (D << 4) + XYZZY_2;
		B += A;	C += B;	D += C;

		A ^= D >> 7;
		B += A;	C += B;	D += C;

		A ^= D << 13;
		B += A;	C += B;	D += C;

		A ^= (D >> 5) + XYZZY_3;
		B += A;	C += B;	D += C;

		A ^= (D << 17);
		B += A;	C += B;	D += C;

		A ^= D >> 23;
		B += A; C += B; D += C;

		A ^= (D << 9) + XYZZY_4;
		B += A;	C += B;	D += C;

		A ^= D >> 18;
		B += A;	C += B;	D += C;

		A ^= (D << 21) + XYZZY_5;
		B += A;	C += B;	D += C;
	}
	el128->A += A;
	el128->B += D;
	el128->C += B;
	el128->D += C;

	el128->bfilled = 0;
	el128->bytes += EL128_BLOCK_BYTES;
}

void el128_update(struct el128_t *el128, const char *data, int len)
{
	int ptr = 0;

	// this is just stupid, but we'll allow it

	if (len == 0)
	{
		return;
	}

	// at this point the buffer will be empty
	// process as many whole blocks as we can

	assert(el128->bfilled == 0);

	while (len >= EL128_BLOCK_BYTES)
	{
		memcpy(el128->uc_block, &data[ptr], EL128_BLOCK_BYTES);
		_el128_block(el128);
		len -= EL128_BLOCK_BYTES;
		ptr += EL128_BLOCK_BYTES;
	}

	// buffer the last len (< EL128_BLOCK_BYTES) remaining bytes

	assert(el128->bfilled == 0);

	if (len)
	{
		memcpy(el128->uc_block, &data[ptr], len);
		el128->bfilled = len;
	}
}

static void _el128_byte(struct el128_t *el128, unsigned char byte)
{
	el128->uc_block[el128->bfilled++] = byte;
	if (el128->bfilled == EL128_BLOCK_BYTES)
	{
		_el128_block(el128);
	}
}

void el128_finish(struct el128_t *el128, uint64_t checksum[2])
{
	uint64_t bytes;

	bytes = el128->bytes + el128->bfilled;

	// Merkle-Damgård compliant padding

	_el128_byte(el128, 0x80);

	do
	{
		_el128_byte(el128, 0);
	} while (el128->bfilled != (EL128_BLOCK_BYTES - 8));

	_el128_byte(el128, (bytes & 0xff00000000000000ULL) >> 56);
	_el128_byte(el128, (bytes & 0x00ff000000000000ULL) >> 48);
	_el128_byte(el128, (bytes & 0x0000ff0000000000ULL) >> 40);
	_el128_byte(el128, (bytes & 0x000000ff00000000ULL) >> 32);
	_el128_byte(el128, (bytes & 0x00000000ff000000ULL) >> 24);
	_el128_byte(el128, (bytes & 0x0000000000ff0000ULL) >> 16);
	_el128_byte(el128, (bytes & 0x000000000000ff00ULL) >>  8);
	_el128_byte(el128, (bytes & 0x00000000000000ffULL) >>  0);

	checksum[0] = el128->C;
	checksum[1] = el128->D;

	assert (el128->bfilled == 0);
}

void el128_start(struct el128_t *el128)
{
	el128->A = EL128_A0;
	el128->B = EL128_B0;
	el128->C = EL128_C0;
	el128->D = EL128_D0;
	el128->bfilled = 0;
	el128->bytes = 0;
}

#define IOBUF_SIZE EL128_BLOCK_BYTES * 16

int main(int argc, char **argv)
{
	int i, len;
	char *buffer;
	uint64_t checksum[2];
	struct el128_t el128;
	FILE *fp;

	el128_init(&el128);
	buffer = malloc(IOBUF_SIZE);

	for (i = 1; i < argc; ++i)
	{
		struct stat sb;

		if (!strcmp(argv[i], "-t"))
		{
			int i, n;
			char *tbuffer;
			clock_t t_start, t_end;

			tbuffer = calloc(EL128_BLOCK_BYTES, 1);

			el128_start(&el128);


			n = 256 * 1024 * 1024;
			fprintf(stderr, "%s, timing _el128_byte (%d)... ", argv[0], n);

			t_start = clock();

			for (i = 0; i < n; ++i)
			{
				_el128_byte(&el128, 0x00);
			}

			t_end = clock();

			el128_start(&el128);

			fprintf(stderr, "%f us\n", 1e+6 * (double)(t_end - t_start) / (double)(n * CLOCKS_PER_SEC));

			n = 8 * 1024 * 1024;
			fprintf(stderr, "%s, timing el128_update (%d)... ", argv[0], n);

			t_start = clock();

			for (i = 0; i < n; ++i)
			{
				el128_update(&el128, tbuffer, EL128_BLOCK_BYTES);
			}

			t_end = clock();

			free(tbuffer);

			fprintf(stderr, "%f us\n", 1e+6 * (double)(t_end - t_start) / (double)(n * CLOCKS_PER_SEC));
		}
		else if (-1 == lstat(argv[i], &sb))
		{
			fprintf(stderr, "%s: can't stat() \"%s\"\n", argv[0], argv[i]);
		}
		else
		{
			if ((sb.st_mode & S_IFMT) != S_IFREG)
			{
				fprintf(stderr, "%s: not a regulat file \"%s\"\n", argv[0], argv[i]);
			}
			else
			{
				fp = fopen(argv[i], "rb");
				if (fp)
				{
					el128_start(&el128);
					while ((len = fread(buffer, 1, IOBUF_SIZE, fp)) > 0)
					{
						el128_update(&el128, buffer, len);
					}

					el128_finish(&el128, checksum);

					printf("%08x-%08x-%08x-%08x %s\n",
						(int)(checksum[0] >> 32), (int)(checksum[0] & 0xffffffff),
						(int)(checksum[1] >> 32), (int)(checksum[1] & 0xffffffff),
						argv[i]);

					fclose(fp);
				}
				else
				{
					fprintf(stderr, "%s: file not found \"%s\"\n", argv[0], argv[i]);
				}
			}
		}
	}

	free(buffer);
	el128_destroy(&el128);

	return 0;
}
