#ifndef CORRECTEUR_H
#define CORRECTEUR_H
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define WORD_SIZE 8
#define POLYNOMIAL 0xD5
#define MULTIPLE_ERROR 500


/**
 * @brief permet de bruiter un mot de 8 bits
 *
 * @param *m pointeur vers le mot de 8 bits
 */
void bruiter(uint8_t *m);

/**
 * @brief permet de changer le n-ième bit d'un mot de 8 bits
 *
 * @param n index du bit à changer
 * @param m le mot de 16 bits
 * @return uint16_t le mot de 16 bits avec le n-ième bit inversé
 */
uint16_t chg_nth_bit(int n, uint16_t m);

/**
 * @brief permet de mettre à 1 le n-ième bit d'un mot de 16 bits
 *
 * @param n index du bit à mettre à 1
 * @param m le mot de 16 bits
 * @return uint16_t le mot de 16 bits avec le n-ième bit à 1
 */
uint16_t set_nth_bit(int n, uint16_t m);

/**
 * @brief permet de récupérer le n-ième bit d'un mot de 8 bits
 *
 * @param n index du bit à récupérer
 * @param m le mot de 8 bits
 * @return uint8_t le n-ième bit
 */
uint8_t get_nth_bit(int n, uint8_t m);

/**
 * @brief permet d'afficher en binaire un mot de 8 bits
 *
 * @param value le mot de 8 bits
 */
void print_binary_8bit(uint8_t value);

/**
 * @brief permet d'afficher en binaire un mot de 16 bits
 *
 * @param value le mot de 16 bits
 */
void print_binary_16bit(uint16_t value);

/**
 * @brief permet d'afficher un mot de k bits
 *
 * @param k nombre de bits à afficher
 * @param value le message de 16 bits
 */
void print_word(int k, uint16_t value);

/**
 * @brief permet de calculer le cardinal d'un message (le nombre de bit à 1)
 *
 * @param value le message de 16 bits
 * @return int le nombre de bit à 1
 */
int cardinal_bit(uint16_t value);

/**
 * @brief permet le bit de parité
 *
 * @param m le message de 8 bits
 * @return int 1 si le message est impair, 0 si il est pair
 */
int parity_bit(uint8_t m);

/**
 * @brief permet de generer le crc d'un message
 *
 * @param m le message de 8 bits
 * @return uint8_t le crc sur 8 bits correspondant au message
 */
uint8_t crcGeneration(uint8_t m);

/**
 * @brief permet de verifier si le message contient une erreur
 *
 * @param m message de 16 bits
 * @return uint8_t le code d'erreur le syndorme et 0 si il n'y a pas d'erreur
 */
uint8_t crcVerif(uint16_t m);

/**
 * @brief permet de savoir si le message contient une erreur ou plus
 *
 * si le message contient une erreur, on renvoie l'index du bit qui est erroné
 *
 * @param m le message de 16 bits
 * @return int l'index du bit erroné ou si il y a plus d'une erreur, MULTIPLE_ERROR
 */
int crc_error_amount(uint16_t m);

/**
 * @brief concatene le message et le crc
 *
 * @param m le message de 8 bits
 * @param crc le crc de 8 bits
 * @return uint16_t le message concaténé
 */
uint16_t concat(uint8_t m, uint8_t crc);

/**
 * @brief calcule le minimum des distances de Hamming entre les messages de 8 bits et le message null
 *
 * cela definit la capacité de detection d'erreur du code
 *
 * @param m le message de 8 bits
 * @return int la capacité de detection d'erreur du code
 */
int hamming_distance(uint16_t m);

/**
 * @brief Test s'il y a une erreur dans le message et corrige si possible
 * @param *m pointeur vers le message de 8 bits
 * @param crc le crc de 8 bits
 * @return -1 si il y a une erreur, 0 sinon
 */
int test_and_correct_crc(uint8_t *m, uint8_t *crc);

#endif