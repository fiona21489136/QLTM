/*
 * Q_learn.h
 *
 *  Created on: 2024¦~6¤ë13¤é
 *      Author: User
 */

#ifndef Q_LEARN_H_
#define Q_LEARN_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>


// (clause_acc, clause_len)
#define N_QSTATES 8
#define N_ACTIONS 4  // full run, prun run, skip, halt

#define EPISODES 40 // # of sampled data
#define GAMMA 0.85
#define DIS_RATE 0.95 // discount rate . future is more important
#define L_RATE 0.75 // learning rate .


// exploration parameters
#define EPSILON 0.85
#define MAX_EPSILON 1.0
#define MIN_EPSILON 0.01
#define DECAY_RATE 0.005


extern volatile float epsilon;


extern volatile float Q_table[N_QSTATES][N_ACTIONS];
extern volatile float Q_table_learned[N_QSTATES][N_ACTIONS];
extern volatile float rewards[N_QSTATES][N_ACTIONS];

void init_Q();
void print_Qval();
int8_t get_cur_state(uint8_t, uint8_t);
int8_t get_rand_state();
int8_t get_random_action();
int8_t choose_action( int8_t );
void update_Q_table(int8_t , int8_t , int8_t , float );



#endif /* Q_LEARN_H_ */
