#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "utility.h"

uint32_t num_of_levels;
uint32_t blocksize, replacement, offset_width;
char *input_file;
uint64_t input_file_count;

CACHE_STRUCT *CACHE;

int main(int argc, char* argv[])
{
	if (argc != 9)
	{
		printf("Argumenti komandne linije moraju biti navedeni sljedecim redoslijedom:\n");
        printf("%s Cache_Line_Size L1_Size L1_Association L2_Size L2_Association Replacement Inclusion Input_File\n", argv[0]);
		exit(EXIT_FAILURE);
	}
    // Ako je argument L2_Size = 0 => postoji samo L1 nivo cache-a
	num_of_levels = ((atoi(argv[4])) == 0) ? 1 : 2;

	// Alokacija memorije za nizove size, association, inclusion
	// gdje je broj elemenata svakog od tih nizova jednak
	// ukupnom broju nivoa cache-a
	uint32_t *size, *association, *inclusion;
	size = (uint32_t *)malloc(sizeof(uint32_t) * num_of_levels);
	if (size == NULL)
		printf("Greška pri alokaciji memorije.\n");
	association = (uint32_t *)malloc(sizeof(uint32_t) * num_of_levels);
	if (association == NULL)
		printf("Greska pri alokaciji memorije.\n");
	inclusion = (uint32_t *)malloc(sizeof(uint32_t) * num_of_levels);
	if (inclusion == NULL)
		printf("Greska pri alokaciji memorije.\n");

    // Parsiranje elemenata komandne linije
    // I popunjavanje elemenata nizova size, association i inclusion
    // za odgovarajuće nivoe cache-a
	Parsing_commandline_arguments(argc, argv, size, association, inclusion);

	// Inicijalizacija cache-a
	Initialization_of_cache(size, association, inclusion);

	free(size);
	free(association);
	free(inclusion);

	// Otvaranje ulaznog fajla u režimu čitanja
	FILE *input_file_fp = fopen(input_file, "r");
	if (input_file_fp == NULL)
		printf("Greska prilikom otvaranja ulazne datoteke.\n");

	while (1)
	{
		int result;
		uint8_t line;
		uint8_t operation;
		uint64_t address;
		result = fscanf(input_file_fp, "%c %llx%c", &operation, &address, &line);
		input_file_count++;
		uint64_t rank_value = input_file_count;
		if (result == EOF)
			// Ako smo došli do kraja ulaznog fajla prekidamo postupak
			break;
		switch (operation)
		{
		case OPERATION_READ:
		{
			CACHE_BLOCK *blk = (CACHE_BLOCK *)malloc(sizeof(CACHE_BLOCK));
			Operation_read(L1, address, blk, rank_value);
			//printf("%x \n", blk->tag);
			free(blk);
			break;
		}
		case OPERATION_WRITE:
			Operation_write(L1, address, DIRTY, rank_value);
			break;
		default:
			printf("Greska: Pogresan tip operacije. Dostupne operacije u fajlu su: citanje - 'r' i pisanje - 'w'.\n");
			break;
		}
	}
	fclose(input_file_fp);
	// Ispis praćenih parametara cache-a
	Print_output_parameters();

    // Oslobađanje cache memorije
	Freeing_of_cache();
	return 0;
}
