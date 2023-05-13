#ifndef CACHE_H_INCLUDED
#define CACHE_H_INCLUDED

// Dostupni nivoi cache-a
#define L1 0
#define L2 1

// Dostupne tehnike zamjene
#define LRU 0
#define FIFO 1

// Inkluzivnost
#define NON_INCLUSIVE 0
#define INCLUSIVE 1
#define EXCLUSIVE 2

// Mogući pristupi memoriji
#define OPERATION_READ 'r'
#define OPERATION_WRITE 'w'

// Pogodak ili promašaj
#define HIT 0
#define MISS 1

// Biti modifikacije/nemodifikacije podataka
#define DIRTY 1
#define CLEAN 0

// Biti važenja/nevaženja
#define VALID 1
#define INVALID 0

// Definisanje tipova
typedef char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// Strukture podataka cache-a
typedef struct cache_block
{
	uint64_t tag; // jedinstveni identifikator memorijskog bloka koji se kešira u datu cache liniju
	uint8_t dirty_bit;
	uint8_t valid_bit;
}CACHE_BLOCK;

typedef struct set
{
	CACHE_BLOCK *blocks; //niz blokova koji se kesiraju u jednu kes liniju
	uint64_t *cache_ranks; // niz rankova za eviction memorijskih blokova iz datog seta
}SET;

typedef struct cache_parameters
{
	uint64_t number_of_access; // broj pristupa odg nivou cache-a (treba nam zbog rank-a bloka)
	uint64_t number_of_reads; // broj čitanja odg nivou cache-a
	uint64_t number_of_writes; // broj upisa odg nivou cache-a
	uint64_t number_of_read_misses; // broj promašaja čitanja odg nivou cache-a
	uint64_t number_of_write_misses; // broj promašaja upisa odg nivou cache-a
	uint64_t number_of_write_backs; // broj write back-ova odg nivou cache-a
}CACHE_PARAMETERS;

typedef struct cache_features
{
	uint32_t cache_size; // veličina određenog nivoa cache-a
	uint32_t association; // asocijacijativnost određenog nivoa cache-a
	uint32_t inclusion; // inclusive-nost određenog nivoa cache-a
    uint32_t set_number; // cache_size / (association * blocksize)
	uint32_t tag_width;  // broj bita tag-a u adresi (64 - index_width - offset_width)
	uint32_t index_width; // broj bita index-a u adresi (log_2 set_number)
}CACHE_FEATURES;

typedef struct cache
{
	SET *set_of_blocks;
	CACHE_FEATURES cache_features;
	CACHE_PARAMETERS cache_parameters;
}CACHE_STRUCT;

extern CACHE_STRUCT* CACHE;

extern uint32_t num_of_levels; // broj cache novoa (L1,L2)
extern uint32_t blocksize; // veličina bloka
extern uint32_t replacement; // tehnika zamjene blokova
extern uint32_t offset_width; // broj bita offset-a (pomjeraja unutar adrese) log_2(blocksize)

extern char* input_file; // naziv ulaznog fajla sa komandama za upis i čitanje
extern uint64_t input_file_count; // broj linija ulaznog fajla

void Initialization_of_cache(uint32_t *size, uint32_t *association, uint32_t *inclusion);

void Extraction_from_address(uint32_t level, uint64_t address, uint64_t *tag, uint64_t *index);

uint64_t Creation_of_address(uint32_t level, uint64_t tag, uint64_t index);

uint8_t Searching_of_cache(uint32_t level, uint64_t tag, uint64_t index, uint32_t *serial_num);

void Maintaining_of_rank(uint32_t level, uint64_t index, uint32_t serial_num, uint8_t result, uint64_t rank_value);

uint32_t Top_rank(uint32_t level, uint64_t index);

uint32_t Eviction_of_cache(uint32_t level, uint64_t index);

void Replacement_of_cache(uint32_t level, uint64_t index, uint32_t serial_num, CACHE_BLOCK blk);

void Invalidation(uint32_t level, uint64_t address);

uint32_t Operation_read(uint32_t level, uint64_t address, CACHE_BLOCK *blk, uint64_t rank_value);

void Operation_write(uint32_t level, uint64_t address, uint8_t dirty_bit, uint64_t rank_value);

void Freeing_of_cache();

#endif // CACHE_H_INCLUDED
