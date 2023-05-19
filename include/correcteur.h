#pragma once
#include <stdio.h>
#include <stdint.h>

#define WORD_SIZE 8
#define POLYNOMIAL 0x07

uint8_t chg_nth_bit(int n, uint8_t m);
uint8_t set_nth_bit(int n, uint8_t m);
uint8_t get_nth_bit(int n, uint8_t m);



void print_binary_8bit(uint8_t value);
void print_binary_16bit(uint16_t value);
void print_word(int k, uint8_t value);

int cardinal_bit(uint8_t value);
int hamming_distance(uint8_t m);
uint8_t parity_bit(uint8_t m);

void bruiter(uint8_t *m);
uint8_t crcGeneration(uint8_t m);
int crcVerif(uint16_t m);
uint16_t concat(uint8_t m, uint8_t crc);