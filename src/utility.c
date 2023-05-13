#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include "utility.h"

char *REPLACEMENT_NAME[] = { "LRU","FIFO" };
char *INCLUSION_NAME[] = { "non-inclusive", "inclusive", "exclusive" };

// Pomoćna funkcija koja računa logaritam po bazi 2 ulaznog argumenta
// Parametri :
// value - vrijednost čiji se logaritam računa
// Povratna vrijednost : vrijednost logaritma ulaznog argumenta
// Napomena: Log_2(0) = 0

uint32_t Log_2(uint32_t value)
{
	uint32_t res = 0, tmp = value;
	while (tmp >>= 1)
		res++;
	return res;
}

// Pomoćna funkcija za parsiranje argumenata komandne linije i provjeru ulaza
// Parametri :
// argc - broj argumenata komandne linije
// argv - niz argumenata komandne linje
// size - alocirani niz za veličine svakog od nivoa cache-a
// association - alocirani niz za red asocijativnosti svakog od nivoa cache-a
// inclusion - alocirani niz za inkluzivnosti svakog od nivoa cache-a
// Posljednja 3 gore-navedena niza popunjavaju se vrijednostima u ovoj f-ji preko pokazivača

void Parsing_commandline_arguments(int argc, char* argv[], uint32_t *size, uint32_t *association, uint32_t *inclusion)
{
	uint32_t i;
	blocksize = atoi(argv[1]);
	size[L1] = atoi(argv[2]);
	association[L1] = atoi(argv[3]);
	inclusion[L1] = atoi(argv[7]);
	if (num_of_levels == 2)
	{
		size[L2] = atoi(argv[4]);
		association[L2] = atoi(argv[5]);
		inclusion[L2] = NON_INCLUSIVE; // posljednji nivo cache-a (L2) je non-inclusive
	}
	replacement = atoi(argv[6]);
	input_file = argv[8];

	// Provjera ulaza
	if((replacement =! LRU) || (replacement =! FIFO))
			printf("Pogresan unos tehnike zamjene ( dostupne: FIFO i LRU ).\n");
	for (i = 0; i < num_of_levels; i++)
	{
		if(!_is_power_of_2(size[i]))
			printf("Pogresan unos velicine cache-a ( mora biti stepen od 2 ).\n");
		if(!_is_power_of_2(association[i]))
			printf("Pogrešan unos asocijativnosti ( mora biti stepen od 2 ).\n");
		if(inclusion[i] < NON_INCLUSIVE || inclusion[i] > EXCLUSIVE)
			printf("Pogresan unos inkluzivnosti ( dostupne: 0 - non-inclusive, 1 - inclusive, 2 - exclusive ).\n");

	if(num_of_levels== 1 && inclusion[L1] != NON_INCLUSIVE)
		printf("Pogresan unos inkluzivnosti ( posljednji nivo cache-a (L1) mora biti non_inclusive ).\n");
    }
}

// Pomoćna funkcija koja najprije ispisuje konfigurisane vrijednosti cache-a
// A zatim ispisuje parametre cache-a, koje smo pratili, na standardni izlaz
// ( Nakon izvršavanja komandi čitanja i upisa zadatih u input.txt fajlu)

void Print_output_parameters()
{
	printf("===== KONFIGURISANE KARAKTERISTIKE KES MEMORIJE =====\n");
	printf("Velicina kes linije:  %u\n", blocksize);
	printf("Velicina L1 nivoa kes-a:  %u\n", CACHE[L1].cache_features.cache_size);
	printf("Asocijativnost L1 kes-a:  %u\n", CACHE[L1].cache_features.association);
	uint64_t check_L2 = (num_of_levels == 1) ? 0 : CACHE[L2].cache_features.cache_size;
	printf("Velicina L2 kes-a:  %llu\n", check_L2);
	check_L2 = (num_of_levels == 1) ? 0 : CACHE[L2].cache_features.association;
	printf("Asocijativnost L2 kes-a:  %llu\n", check_L2);
	printf("Tehnika zamjene blokova:  %s\n", REPLACEMENT_NAME[replacement]);
	printf("Inkluzivnost izmedju L1 i L2:  %s\n", INCLUSION_NAME[CACHE[L1].cache_features.inclusion]);
	printf("Ulazni fajl:  %s\n", input_file);
	printf("\n");
	printf("===== PRACENI PARAMETRI SIMULACIJE =====\n");
	printf("1. Broj citanja L1 kes-a:  %llu\n", CACHE[L1].cache_parameters.number_of_reads);
	printf("2. Broj promasaja pri citanje L1 kes-a:  %llu\n", CACHE[L1].cache_parameters.number_of_read_misses);
	printf("3. Broj promasaja pri upisu L1 kes-a:  %llu\n", CACHE[L1].cache_parameters.number_of_write_misses);
	double miss_rate_L1 = ((double)CACHE[L1].cache_parameters.number_of_read_misses + (double)CACHE[L1].cache_parameters.number_of_write_misses) / ((double)CACHE[L1].cache_parameters.number_of_reads + (double)CACHE[L1].cache_parameters.number_of_writes);
	printf("4. Ukupna ocjena promasaja L1 kes-a:  %f\n", miss_rate_L1);
	printf("5. Broj writeback-ova L1 kes-a:  %llu\n", CACHE[L1].cache_parameters.number_of_write_backs);
	check_L2 = (num_of_levels == 1) ? 0 : CACHE[L2].cache_parameters.number_of_reads;
	printf("6. Broj citanja L2 kes-a:  %llu\n", check_L2);
	check_L2 = (num_of_levels == 1) ? 0 : CACHE[L2].cache_parameters.number_of_read_misses;
	printf("7. Broj promasaja pri citanju L2 kes-a:  %llu\n", check_L2);
	check_L2 = (num_of_levels == 1) ? 0 : CACHE[L2].cache_parameters.number_of_writes;
	printf("8. Broj upisa L2 kes-a:  %llu\n", check_L2);
	check_L2 = (num_of_levels == 1) ? 0 : CACHE[L2].cache_parameters.number_of_write_misses;
	printf("9. Broj promasaja upisa L2:  %llu\n", check_L2);
	check_L2 = (num_of_levels == 1) ? 0 : CACHE[L2].cache_parameters.number_of_write_misses;
	double miss_rate_L2 = ((double)CACHE[L2].cache_parameters.number_of_read_misses + (double)CACHE[L2].cache_parameters.number_of_write_misses) / ((double)CACHE[L2].cache_parameters.number_of_reads + (double)CACHE[L2].cache_parameters.number_of_writes);
	if(num_of_levels == 1)
		printf("10. Ukupna ocjena promasaja L2 kes-a:  %d\n", 0);
	else
		printf("10. Ukupna ocjena promasaja L2 kes-a:  %f\n", miss_rate_L2);
	check_L2 = (num_of_levels == 1) ? 0 : CACHE[L2].cache_parameters.number_of_write_backs;
	printf("11. Broj writeback-ova L2 kes-a:  %llu\n", check_L2);
}
