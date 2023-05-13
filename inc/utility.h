#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED

#define _is_power_of_2(x) (((x) == 0) || ((x) > 0 && !((x) & ((x) - 1))))

extern char *REPLACEMENT_NAME[];
extern char *INCLUSION_NAME[];

uint32_t Log_2(uint32_t num);

void Parsing_commandline_arguments(int argc, char* argv[], uint32_t *size, uint32_t *assoc, uint32_t *inclusion);

void Print_output_parameters();

#endif // UTILITY_H_INCLUDED
