#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "cache.h"
#include "utility.h"

// Funkcija za inicijalizaciju cache memorije
// Parametri :
// size - veličina određenog nivoa cache-a
// association - red asocijativnosti - stepen broja 2 ( 1- direktno mapiran cache)
// inclussion - 0 non-inclusive, 1 inclusive i 2  exclusive

void Initialization_of_cache(uint32_t *size, uint32_t *association, uint32_t *inclusion)
{
	uint32_t i;
	CACHE = (CACHE_STRUCT *)malloc(sizeof(CACHE_STRUCT)*num_of_levels);
	if (CACHE == NULL)
		printf("Greska pri alokaciji memorije. \n");
	offset_width = Log_2(blocksize);
	for (i = 0; i < num_of_levels; i++)
	{
		if (size[i] == 0)
			break;

		// Inicijalizujmo prvo karakteristike cache-a
		CACHE[i].cache_features.cache_size= size[i];
		CACHE[i].cache_features.association = association[i];
		CACHE[i].cache_features.inclusion = inclusion[i];
		CACHE[i].cache_features.set_number = CACHE[i].cache_features.cache_size / (CACHE[i].cache_features.association * blocksize);

		CACHE[i].cache_features.index_width = Log_2(CACHE[i].cache_features.set_number);
		CACHE[i].cache_features.tag_width = 64 - CACHE[i].cache_features.index_width - offset_width;

		// Alokacija prostora za set-ove (uključujući blokove i niz tag-ova)
		CACHE[i].set_of_blocks = (SET *)malloc(sizeof(SET) * CACHE[i].cache_features.set_number);
		if (CACHE[i].set_of_blocks == NULL)
			printf("Greska pri alokaciji memorije. \n");
		uint32_t j;
		for (j = 0; j < CACHE[i].cache_features.set_number; j++)
		{
		    // broj blokova u jednom setu je jednak asocijativnosti datog nivoa cache-a
			CACHE[i].set_of_blocks[j].blocks = (CACHE_BLOCK *)malloc(sizeof(CACHE_BLOCK) * CACHE[i].cache_features.association);
			if (CACHE[i].set_of_blocks[j].blocks == NULL)
				printf("Greska pri alokaciji memorije. \n");
			memset(CACHE[i].set_of_blocks[j].blocks, 0, sizeof(CACHE_BLOCK) * CACHE[i].cache_features.association);
			CACHE[i].set_of_blocks[j].cache_ranks = (uint64_t *)malloc(sizeof(uint64_t) * CACHE[i].cache_features.association);
			if (CACHE[i].set_of_blocks[j].cache_ranks == NULL)
				printf("Greska pri alokaciji memorije. \n");
			memset(CACHE[i].set_of_blocks[j].cache_ranks, 0, sizeof(uint64_t) * CACHE[i].cache_features.association);
		}
		// Inicijalizacija strukture parametara datog nivoa, koje pratimo
		memset(&(CACHE[i].cache_parameters), 0, sizeof(CACHE_PARAMETERS));

	}
}

// Funkcija za ekstrakciju tag-a i index-a iz adrese
// Parametri :
// level - nivo cache-a
// address - 64-obitna adresa u virtual address space (VAS)
// tag - argument koji računamo i vraćamo preko pokazivača *tag
// index - argument koji računamo i vraćamo preko pokazivača *index

void Extraction_from_address(uint32_t level, uint64_t address, uint64_t *tag, uint64_t *index)
{
	uint32_t tag_width = CACHE[level].cache_features.tag_width;
	*tag = address >> (64 - tag_width);
	*index = (address << tag_width) >> (tag_width + offset_width);
}

// Funkcija za kreiranje 64-bitne adrese na osnovu tag-a i index-a
// Parametri :
// level - nivo cache-a
// tag - dio adrese tag
// index - dio adrese index
// Povratna vrijednost : kreirana adresa

uint64_t Creation_of_address(uint32_t level, uint64_t tag, uint64_t index)
{
	uint64_t address = 0;
	address |= (tag << (CACHE[level].cache_features.index_width + offset_width));
	address |= (index << offset_width);
	return address;
}
// Funkcija za traženje bloka u cache nivou "level"
// koja vraća indeks bloka u nizu blocks
// ( preko pokazivača ulaznog argumenta *serial_number)
// čiji tag je jednak ulaznom argumentu tag
// i koji se nalazi u nizu blocks u setu blokova na lokaciji index
// Parametri :
// level - nivo cache-a
// tag - identifikator memorijskog bloka koji tražimo
// index - indeks u nizu set_of_blocks bloka
// serial number - redni broj bloka u nizu blocks
// Povratna vrijednost : 0 - HIT (pogodak) ili 1- MISS (promašaj)

uint8_t Searching_of_cache(uint32_t level, uint64_t tag, uint64_t index, uint32_t *serial_number)
{
	uint32_t i, k = CACHE[level].cache_features.association;
	for (i = 0; i < CACHE[level].cache_features.association; i++)
        // ima smisla tražiti samo validne - važeće lokacije
		if (CACHE[level].set_of_blocks[index].blocks[i].valid_bit == VALID && CACHE[level].set_of_blocks[index].blocks[i].tag == tag)
		{
			k = i;
			break;
		}
    // Ako smo došli do kraja niza blocks i nismo našli
	if (k == CACHE[level].cache_features.association)
		return MISS;
	else
	{
		*serial_number = k;
		return HIT;
	}
}

// Funkcija za postavljanje rank-a bloku u cache nivou "level"
// koji se u setu blokova (set_of_blocks) (nizu) nalazi na lokaciji index
// a u nizu blokova (blocks) tog seta nalazi se na lokaciji serial_number
// čime se održava niz rankova blokova
// Parametri :
// level - nivo cache-a
// index - indeks u nizu set_of_blocks
// serial number - redni broj bloka u nizu blocks
// result - HIT ili MISS
// rank_value - nova vrijednost ranka na koju postavljamo rank traženog bloka

void Maintaining_of_rank(uint32_t level, uint64_t index, uint32_t serial_num, uint8_t result, uint64_t rank_value)
{
	// Postavljanje niza rank na niz rankova blokova proslijeđenog seta blokova
	uint64_t *rank = CACHE[level].set_of_blocks[index].cache_ranks;
	switch (replacement)
	{
	case FIFO:
		// FIFO: Ažuriranje samo ako su podaci tek smješteni
		// (desio se cache miss)
		if (result == MISS)
			rank[serial_num] = rank_value;
		break;
	default:
		rank[serial_num] = rank_value;
		break;
	}
}

// Funkcija koja vraća redni broj bloka koji je kandidat za izbacivanje
// ili smiještanje
// Parametri :
// level - nivo cache-a
// index - indeks u nizu set_of_blocks
// Povratna vrijednost : redni broj bloka (u nizu rank-ova) koji
// je nevalidan (ako ima takvih) ili onaj koji je validan (ako nema nevalidnih)
// a sa najmanjom vrijednosti ranka (najmanje korišten - LRU)

uint32_t Top_rank(uint32_t level, uint64_t index)
{
	uint32_t i, association = CACHE[level].cache_features.association;
	// Prvi kandidati za izbacivanje su nevalidne lokacije
	// Na početku su sve lokacije proglašene kao INVALID
	for (i = 0; i < association; i++)
		if (CACHE[level].set_of_blocks[index].blocks[i].valid_bit == INVALID)
			return i;
	uint64_t *rank = CACHE[level].set_of_blocks[index].cache_ranks;

	// U suprotnom koristimo tehniku zamjene
	// Sljedeće linije koda važe i za FIFO i za LRU tehnike zamjene
        uint32_t k = 0;
		for (i = 0; i < association; i++)
			if (rank[i] < rank[k]) // tražimo minimalan rank
				k = i;
		return k;
}

// Funkcija koja izbacuje jedan blok iz cache nivoa "level"
// seta blokova na lokaciji "index" kako bi se napravio prostor za
// blok koji će se smjestiti
// Parametri :
// level - nivo cache-a
// index - indeks u nizu set_of_blocks
// Povratna vrijednost : redni broj prvog nevalidnog bloka na nivou "level"
// u set_of_blocks na lokaciji index (ako ih ima)
// a inače, blok kandidat sa najmanjom vrijednosti ranka (prema FIFO ili LRU)

uint32_t Eviction_of_cache(uint32_t level, uint64_t index)
{
	uint32_t serial_num = Top_rank(level, index);
	CACHE_BLOCK *tmp = &(CACHE[level].set_of_blocks[index].blocks[serial_num]);
	// Ako je lokacija validna
	if (tmp->valid_bit == VALID)
	{
		uint64_t address = Creation_of_address(level, tmp->tag, index);
		// Ako je lokacija dirty-bila izmijenjena, mora se izvršiti write back u viši nivo cache-a
		if (tmp->dirty_bit == DIRTY)
		{
			CACHE[level].cache_parameters.number_of_write_backs++;
			Operation_write(level + 1, address, DIRTY, CACHE[level].set_of_blocks[index].cache_ranks[serial_num]);
		}
		else
		{
			// Ako su ovaj i naredni level u vezi exclusive
			// nad bilo kojim evicted blokom treba izvršiti write back
			if (CACHE[level].cache_features.inclusion == EXCLUSIVE)
			{
				Operation_write(level + 1, address, CLEAN, CACHE[level].set_of_blocks[index].cache_ranks[serial_num]);
			}
		}
		// Ako su ovaj i viši nivo cache-a u vezi inclusive
		// onda treba da označimo blok kao invalid na nižem nivou cache-a
		// To radi f-ja Invalidation()
		if (level > L1)
			Invalidation(level - 1, address);
	}
	return serial_num;
}

// Funkcija za alokaciju ( smiještanje ili zamjenu) bloka blk na cache nivou "level"
// u setu blokova na lokaciji "index", rednog broja serial_num u nizu blocks
// Parametri :
// level - nivo cache-a
// index - indeks u nizu set_of_blocks
// serial_num - redni broj bloka u nizu blocks
// blk - blok koji se smiješta na gore-pomenutu lokaciju

void Replacement_of_cache(uint32_t level, uint64_t index, uint32_t serial_num, CACHE_BLOCK blk)
{
	CACHE[level].set_of_blocks[index].blocks[serial_num].valid_bit = VALID;
	CACHE[level].set_of_blocks[index].blocks[serial_num].tag = blk.tag;
	CACHE[level].set_of_blocks[index].blocks[serial_num].dirty_bit = blk.dirty_bit;
}

// Funkcija za slanje signala nevalidnosti na niži nivo cache-a "level"
// kako bi se blok na adresi address učinio nevalidnim
// i to radimo samo ako su nivo sa kog dolazi "signal invalidnosti" - viši nivo (L2)
// i niži nivo (L1) u vezi inkluzivnosti
// jer ako podatak nije pronađen na višem nivou (što je sigurno ako smo dospjeli u
// f-ju Eviction_of_cache()) ne može da bude važeći ako postoji na nižem nivou
// jer je u vezi inkluzivnosti L1 podskup od L2, pa ako podatka nema u većem skupu,
// ne može postojati ni u manjem
// Parametri :
// level - nivo cache-a
// address - adresa čiji blok se označava kao invalid

void Invalidation(uint32_t level, uint64_t address)
{
	// Sačuvamo podatak o nivou cache-a sa kog dolazi invalidation signal,
	// jer treba da upišemo "dirty" podatke na taj nivo cache-a
	uint32_t level_invalidation_signal_from = level + 1;
	while (level >= L1)
	{
		switch (CACHE[level].cache_features.inclusion)
		{
		// Samo ukoliko je inkluzivnost tipa inclusive treba da invalid-ujemo podatke
		// na nižim nivoima cache-a (koji su validni na višem nivou)
		case INCLUSIVE:
		{
			uint64_t tag, index;
			uint32_t serial_num;
			Extraction_from_address(level, address, &tag, &index);
			uint8_t result = Searching_of_cache(level, tag, index, &serial_num);
			if (result == HIT)
			{
				CACHE[level].set_of_blocks[index].blocks[serial_num].valid_bit = INVALID;

				// Ako je taj blok dirty, treba da bude write back u cache odakle signal dolazi
				if (CACHE[level].set_of_blocks[index].blocks[serial_num].dirty_bit == DIRTY)
					Operation_write(level_invalidation_signal_from, address, DIRTY, CACHE[level].set_of_blocks[index].cache_ranks[serial_num]);
			}
			if (level == L1)
				// L1 je najniži nivo, pa dostizanje L1 znači da je proces invalidacije završen
				return;
			else
				// Ako nije dostignut L1, treba da idemo na niži nivo cache-a
				level--;
			break;
		}
		default:
			return;
		}
	}
}

// Funkcija koja vrši operaciju čitanja bloka memorije
// Parametri :
// level - nivo cache-a
// address - adresa memorijskog bloka koja će se ekstrahovati
// kako bi se na osnovu tag-a i index-a u cache nivou "level" pronašla lokacija
// bloka koji se želi pročitati, ako postoji
// blk - blok u koji se smiješta sadržaj pročitanog bloka (preko pokazivača)
// rank_value - rank bloka koji predstavlja zapravo broj pristupa nivou "level" cache-a
// Povratna vrijednost : serial number - redni broj pronađenog bloka u nizu blokova
// ako se desio cache hit (što znači da blok postoji u cache-u na nivou level)
// inače vraća redni broj lokacije u nizu blokova na datom nivou na koji se smiješta
// blok ako se desio cache miss (što znači da blok nije postojao u cache-u,
// a to znači da ga onda treba "useliti" u taj nivo cache-a)
// Napomena: f-ja vraća 0 kada je proslijeđeni "level" veći od L2

uint32_t Operation_read(uint32_t level, uint64_t address, CACHE_BLOCK *blk, uint64_t rank_value)
{
	// Ovo se dešava samo kada je level > L2, jer je L2 najviši nivo cache-a koji imamo
	// moramo obezbijediti ovaj slučaj kada se desi da Operation_write()
	// pozove f-ju read kako bi pribavila blok sa višeg nivoa,a taj viši nivo je veći od
	// ukupnog broja nivoa (od L2)
	if (level >= num_of_levels)
	{
		blk->dirty_bit = CLEAN;
		blk->valid_bit = VALID;
		return 0;
	}
	// Ažuriranje parametara koje pratimo za  CACHE[level]
	CACHE[level].cache_parameters.number_of_access++;
	CACHE[level].cache_parameters.number_of_reads++;

	// rank_value treba biti vrijeme pristupa ovog specificiranog nivoa cache-a
	rank_value = CACHE[level].cache_parameters.number_of_access;

	// Pretraživanje ovog nivoa cache-a
	// Tj. tražimo blok čija je adresa "address"
	uint64_t tag, index;
	uint32_t serial_num = 0;
	Extraction_from_address(level, address, &tag, &index);
	uint8_t result = Searching_of_cache(level, tag, index, &serial_num);

	// Ako je pronađen blok
	if (result == HIT)
	{
		// Uzimanje bloka i smiještanje u promjenjivu "blk"
		*blk = CACHE[level].set_of_blocks[index].blocks[serial_num];

		// Ako je operacija read() pozvana iz f-ja Operation_read() ili Operation_write()
		// To znači da ovaj nivo (L2) nije top operation level
		// U tom slučaju, ako su ovaj i niži nivo cache-a u vezi exclusive
		// To znači da je skup lokacija na ta 2 nivoa disjunktan =>
		// Pa moramo proglasiti ovaj blok kao INVALID na ovom nivou cache-a (L2)
		if (level > L1 && CACHE[level - 1].cache_features.inclusion == EXCLUSIVE)
		{
			CACHE[level].set_of_blocks[index].blocks[serial_num].valid_bit = INVALID;
			return serial_num;
		}
		// U suprotnom, učitavamo blok ovog nivoa i označavamo blok višeg nivoa kao CLEAN
		blk->dirty_bit = CLEAN;

		// Ažuriranje niza rank-ova
		Maintaining_of_rank(level, index, serial_num, HIT, rank_value);
	}
	else
	{
		// Ako blok nije pronađen (cache MISS)
		// Ako blok nije pronađen na L1 nivou, moraće se smjestiti na taj nivo cache-a
		// A da li će se smjestiti na viši nivo cache-a zavisi od inkluzivnosti
		CACHE[level].cache_parameters.number_of_read_misses++;

        // Ako je ovo L1 ili ovaj nivo i niži nivo NISU u vezi exclusive
		// (u drugom slučaju to radimo jer za slučaj ekskluzivnosti lokacije na L1 i L2
        // su disjunktne)
		// treba da smjestimo blok na ovaj nivo cache-a

		// Ako treba da smjestimo blok na ovaj nivo, prvo moramo napraviti prostor
		// gdje ćemo ga smjestiti (f-ja Eviction_of_cache())
		if (level == L1 || CACHE[level - 1].cache_features.inclusion != EXCLUSIVE)
			serial_num = Eviction_of_cache(level, index);

		// Treba da pročitamo i sljedeći (viši) nivo cache-a
		Operation_read(level + 1, address, blk, rank_value);

		// Ažuriranje tag-a bloka
		blk->tag = tag;

		// Ako je ovo L1 ili ovaj nivo i niži nivo NISU u vezi exclusive
		// smiještamo blok na ovaj nivo cache-a
		if (level == L1 || CACHE[level - 1].cache_features.inclusion != EXCLUSIVE)
		{
			Replacement_of_cache(level, index, serial_num, *blk);
			// Održavanje niza rank-ova
			Maintaining_of_rank(level, index, serial_num, MISS, rank_value);
		}
	}
	return serial_num;
}

// Funkcija koja vrši operaciju upisa/modifikacije (postavljanja dirty bit-a bloka)
// Parametri :
// level - nivo cache-a
// address - adresa memorijskog bloka koja će se ekstrahovati
// kako bi se na osnovu tag-a i index-a u cache nivou "level" pronašla lokacija
// bloka koji se želi modifikovati, ako postoji
// dirty bit - DIRTY ili CLEAN ( 1 ILI 0) vrijednost
// rank_value - rank bloka koji predstavlja zapravo broj pristupa nivou "level" cache-a


void Operation_write(uint32_t level, uint64_t address, uint8_t dirty_bit, uint64_t rank_value)
{
	// Ažuriranje parametara koje pratimo za cache[level]
	CACHE[level].cache_parameters.number_of_access++;
	CACHE[level].cache_parameters.number_of_writes++;

	// rank_value treba biti vrijeme pristupa ovog specificiranog nivoa cache-a
	rank_value = CACHE[level].cache_parameters.number_of_access;

	// Pretraživanje ovog nivoa cache-a
	uint64_t tag, index;
	uint32_t serial_num;
	Extraction_from_address(level, address, &tag, &index);
	uint8_t result = Searching_of_cache(level, tag, index, &serial_num);
	// Ako je pogodak samo izmijenimo podatak (postavimo dirty bit)
	if (result == HIT)
	{
		// Operacija upisa - izmjena bloka
		CACHE[level].set_of_blocks[index].blocks[serial_num].dirty_bit = dirty_bit;

		// Održavanje niza rank-ova
		Maintaining_of_rank(level, index, serial_num, HIT, rank_value);
	}
	else
	{
		// Ako je promašaj
		CACHE[level].cache_parameters.number_of_write_misses++;

		// Treba da smjestimo blok na ovaj nivo
		// Ali prvo moramo napraviti prostor za blok
		serial_num = Eviction_of_cache(level, index);

		// Zatim, čitamo sljedeći nivo cache-a
		CACHE_BLOCK *blk = (CACHE_BLOCK*)malloc(sizeof(CACHE_BLOCK));
		// Uzimam blok sa višeg nivoa ako postoji
		Operation_read(level + 1, address, blk, rank_value);
		// Ažuriranje tag-a bloka
		blk->tag = tag;

		// Smiještanje bloka
		Replacement_of_cache(level, index, serial_num, *blk);
		free(blk);

		// Operacija upisa
		CACHE[level].set_of_blocks[index].blocks[serial_num].dirty_bit = dirty_bit;
		// Održavanje niza rank-ova
		Maintaining_of_rank(level, index, serial_num, MISS, rank_value);
	}
}

// Funkcija za oslobađanje keš memorije ( uključujući sve set-ove i block-ove)

void Freeing_of_cache()
{
	uint32_t i;
	for (i = 0; i < num_of_levels; i++)
	{
		uint32_t j;
		for (j = 0; j < CACHE[i].cache_features.set_number; j++)
		{
			free(CACHE[i].set_of_blocks[j].blocks);
			free(CACHE[i].set_of_blocks[j].cache_ranks);
		}
		free(CACHE[i].set_of_blocks);
	}
	free(CACHE);
}
