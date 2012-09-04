#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>

#define EL128_BLOCK_SIZE 1024
#define EL128_BLOCK_BYTES (EL128_BLOCK_SIZE * sizeof(uint64_t))

// ln2 in base 2**64

#define EL128_A0 0xb17217f7d1cf79abULL
#define EL128_B0 0xc9e3b39803f2f6afULL
#define EL128_C0 0x40f343267298b62dULL
#define EL128_D0 0x8a0d175b8baafa2bULL

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

	X = el128->u64_block;
	A = el128->A;
	B = el128->B;
	C = el128->C;
	D = el128->D;	

	for(i = 0; i < EL128_BLOCK_SIZE / 8; ++i)
	{
		A ^= (D << 7) + (D >> 11) + *X++;
		B ^= A ^ *X++;
		C ^= (B << 5) + (B >> 23) + *X++;
		D ^= A + C + *X++;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= C;
		C ^= B;
		D ^= A ^ *X++;
		A ^= (D << 12) + (D >> 5) + *X++;
		B ^= A ^ *X++;
		C ^= (B << 9) + (B >> 20) + *X++;
		D ^= A;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A += D >> 13;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A += D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
		A ^= D;
		B ^= A;
		C ^= B;
		D ^= C;
	}
	el128->A = A;
	el128->B = D;
	el128->C = B;
	el128->D = C;
	
	el128->bfilled = 0;
	el128->bytes += EL128_BLOCK_BYTES;
}

void el128_update(struct el128_t *el128, const char *data, int len)
{
	int ptr = 0, remain;

	// this is just stupid, but we'll allow it
	
	if (len == 0)
	{
		return;
	}

	// special case; buffer non-empty

	if (el128->bfilled)
	{
		remain = EL128_BLOCK_BYTES - el128->bfilled;
		if (len >= remain)
		{
			// buffer the first remain bytes to fill up the block
			memcpy(&el128->uc_block[el128->bfilled], data, remain);
			_el128_block(el128);
			len -= remain;
			ptr += remain;
		}
		else
		{
			// buffer all len bytes of *data, without filling up to one block
		
			memcpy(&el128->uc_block[el128->bfilled], data, len);
			el128->bfilled += len;
			return; // all of the input consumed
		}
	}

	// at this point the buffer will be empty
	// process as many whole blocks as we can
	
	//assert(el128->bfilled == 0);
	
	while (len >= EL128_BLOCK_BYTES)
	{
		memcpy(el128->uc_block, &data[ptr], EL128_BLOCK_BYTES);
		_el128_block(el128);
		len -= EL128_BLOCK_BYTES;
		ptr += EL128_BLOCK_BYTES;
	}
	
	// buffer the last len (< EL128_BLOCK_BYTES) remaining bytes
	
	//assert(el128->bfilled == 0);

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
	
	_el128_byte(el128, bytes & 0xff); bytes >>= 8;
	_el128_byte(el128, bytes & 0xff); bytes >>= 8;
	_el128_byte(el128, bytes & 0xff); bytes >>= 8;
	_el128_byte(el128, bytes & 0xff); bytes >>= 8;
	_el128_byte(el128, bytes & 0xff); bytes >>= 8;
	_el128_byte(el128, bytes & 0xff); bytes >>= 8;
	_el128_byte(el128, bytes & 0xff); bytes >>= 8;
	_el128_byte(el128, bytes & 0xff); bytes >>= 8;
	
	while (el128->bfilled)
	{
		_el128_byte(el128, 0);
	}
	checksum[0] = el128->C;
	checksum[1] = el128->D;
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

int main(int argc, char **argv)
{
	int i, len, ret = 0;
	char *buffer;
	uint64_t checksum[2], c[2], x[2];
	struct el128_t el128;
	FILE *fp;

	el128_init(&el128);	
	buffer = malloc(65536);
		
	for (i = 1; i < argc; ++i)
	{
		fp = fopen(argv[i], "rb");
		if (fp)
		{
			el128_start(&el128);
			while ((len = fread(buffer, 1, 65536, fp)) > 0)
			{
				el128_update(&el128, buffer, len);
			}

			fclose(fp);
			el128_finish(&el128, checksum);

			if (i == 1)
			{
				c[0] = checksum[0];
				c[1] = checksum[1];
			}

			x[0] = checksum[0] ^ c[0];
			x[1] = checksum[1] ^ c[1];

			printf("%08x-%08x-%08x-%08x [%08x-%08x-%08x-%08x] %s\n",
				(int)(checksum[0] >> 32), (int)(checksum[0] & 0xffffffff),
				(int)(checksum[1] >> 32), (int)(checksum[1] & 0xffffffff),
				(int)(x[0] >> 32), (int)(x[0] & 0xffffffff),
				(int)(x[1] >> 32), (int)(x[1] & 0xffffffff),
				argv[i]);
		}
		else
		{
			printf("**** : \"%s\" not found\n", argv[i]);
			ret = -1;
			goto early_fail;
		}
	}

early_fail:
	free(buffer);
	el128_destroy(&el128);
	return ret;
}
