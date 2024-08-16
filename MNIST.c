#include <stdio.h>
#include <msp430fr5994.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "encoded.h"
#include "datafile.h"
#include "clause_acc_data.h"
#include "Q_learn.h"
#include "mementos.h"

#pragma PERSISTENT(check_timerA)
volatile int check_timerA = 0;

volatile int power_fail_flag = 0;


volatile uint8_t error_cnt = 0;
volatile float acc_rate = 0.0;

// for sync
volatile int8_t cur_state, next_state;
volatile int class_index[CLASSES][PRUN_LV] = {0};
volatile int clause_index[CLASSES][PRUN_LV] = {0};


// test data

#define FILTER_L 0x3ff
#define FILTER_M 0xffc00
#define FILTER_H 0x3ff00000

#define SHIFT_L 0
#define SHIFT_M 10
#define SHIFT_H 20

#define SIGN_BIT 9



void sync_clause_index(uint8_t class){
    uint8_t l;
    for(l=0;l<PRUN_LV;l++){

        int jump_pos = 0;
        if(encoded_arr[l][clause_index[class][l]] != 0)
            jump_pos = ((encoded_arr[l][clause_index[class][l]]-1)/3)+1;

        clause_index[class][l] = clause_index[class][l] + jump_pos + 1;
        //printf("jump %u",((encoded_arr[l][clause_index[class][l]]-1)/3)+1);
        //printf("clause_inde[0][] = %u \n", clause_index[class][l]);
    }

}



void pre_infer(){
    uint8_t c, l;
    for( l=0; l <PRUN_LV; l++){
        uint32_t class_pos = 0;
        for(c = 0; c < CLASSES; c++){
            class_index[c][l] = class_pos;
            clause_index[c][l] = class_pos + 1;
            uint16_t next_class_shift = encoded_arr[l][class_pos];
            class_pos += next_class_shift;
        }
    }
}

int8_t chunking_infer_for_Q(const uint32_t *features_c){
    uint8_t c;
    unsigned int clause;
    int8_t sign;
    int8_t max_class = -1;
    volatile int max_votes = -10000;
    int votes;
    for(c = 0; c<CLASSES; c++){
        //progress_class = c;
        //printf("class:%d\n",c);
        votes = 0;
        for(clause = 0; clause< CLAUSES; clause++){
            //progress_clause = clause;
            //printf("%d-",clause);
            //printf("CHECK timer A into cpu on cycle times:%d, hibernate check:%d\n",cycle, check_val);
            sign = 1 - 2*(clause & 1);
            uint8_t clause_val = 1;
            uint32_t cl;
            volatile uint32_t pos;

            cur_state = get_cur_state(c, clause);
            int8_t action = choose_action(cur_state);
            // 0 : full run
            // 1 : prun run
            // 2 : skip
            // 3 : halt right away
            if(action == 0){

                unsigned int clause_len = encoded_arr[0][clause_index[c][0]];
                pos = clause_index[c][0]; // clause_len index position, real include feature is pos+1
                int sign_bit;
                unsigned int fea_idx;
                for(cl = 0; cl < clause_len; cl++){
                    if(cl%3 == 0){
                        // HIGH POS
                        pos++;
                        fea_idx = (FILTER_H & encoded_arr[0][pos]) >> SHIFT_H;

                        sign_bit = (fea_idx >> SIGN_BIT) & 1;
                        if(sign_bit)
                            fea_idx -= 512;
                        //printf("fea_idx: %u\n", fea_idx);
                    }
                    else if(cl%3 == 1){
                        // MID POS
                        fea_idx = (FILTER_M & encoded_arr[0][pos]) >> SHIFT_M ;   // may be +/-

                        sign_bit = (fea_idx >> SIGN_BIT) & 1;
                        if(sign_bit)
                            fea_idx -= 512;
                        //printf("fea_idx: %u\n", fea_idx);
                    }
                    else{
                        // LOW POS
                        fea_idx = (FILTER_L & encoded_arr[0][pos]) >> SHIFT_L ;   // may be +/-
                        //printf("merged fea_idx: %6\n", (FILTER_L & encoded_arr[0][pos]));
                        sign_bit = (fea_idx >> SIGN_BIT) & 1;
                        if(sign_bit)
                            fea_idx -= 512;
                        //printf("fea_idx: %u\n", fea_idx);
                    }
                    int chunk_num, chunk_pos;
                    chunk_num = (fea_idx-1)/INT_SIZE;
                    if (chunk_num == FEATURE_CHUNKS-1)
                        chunk_pos = 3 - (fea_idx-1)%INT_SIZE;
                    else
                        chunk_pos = 31 - (fea_idx-1)%INT_SIZE;

                    if(sign_bit){
                        clause_val &= !( (features_c[chunk_num] >> chunk_pos) & 1);
                    }
                    else{
                        clause_val &= ((features_c[chunk_num] >> chunk_pos) & 1);
                    }
                    if(clause_val == 0)
                        break;
                }
                if(clause_len != 0){
                    votes += clause_val * sign;
                }

            }
            else if(action == 1){
                unsigned int clause_len = encoded_arr[1][clause_index[c][1]];
                pos = clause_index[c][1]; // clause_len index position, real include feature is pos+1
                int sign_bit;
                unsigned int fea_idx;
                for(cl = 0; cl < clause_len; cl++){
                    if(cl%3 == 0){
                        // HIGH POS
                        pos++;
                        fea_idx = (FILTER_H & encoded_arr[1][pos]) >> SHIFT_H;

                        sign_bit = (fea_idx >> SIGN_BIT) & 1;
                        if(sign_bit)
                            fea_idx -= 512;
                        //printf("fea_idx: %u\n", fea_idx);
                    }
                    else if(cl%3 == 1){
                        // MID POS
                        fea_idx = (FILTER_M & encoded_arr[1][pos]) >> SHIFT_M ;   // may be +/-

                        sign_bit = (fea_idx >> SIGN_BIT) & 1;
                        if(sign_bit)
                            fea_idx -= 512;
                        //printf("fea_idx: %u\n", fea_idx);
                    }
                    else{
                        // LOW POS
                        fea_idx = (FILTER_L & encoded_arr[1][pos]) >> SHIFT_L ;   // may be +/-
                        //printf("merged fea_idx: %6\n", (FILTER_L & encoded_arr[0][pos]));
                        sign_bit = (fea_idx >> SIGN_BIT) & 1;
                        if(sign_bit)
                            fea_idx -= 512;
                        //printf("fea_idx: %u\n", fea_idx);
                    }
                    int chunk_num, chunk_pos;
                    chunk_num = (fea_idx-1)/INT_SIZE;
                    if (chunk_num == FEATURE_CHUNKS-1)
                        chunk_pos = 3 - (fea_idx-1)%INT_SIZE;
                    else
                        chunk_pos = 31 - (fea_idx-1)%INT_SIZE;

                    if(sign_bit){
                        clause_val &= !( (features_c[chunk_num] >> chunk_pos) & 1);
                    }
                    else{
                        clause_val &= ((features_c[chunk_num] >> chunk_pos) & 1);
                    }
                    if(clause_val == 0)
                        break;
                }
                if(clause_len != 0){
                    votes += clause_val * sign;
                }
            }
            else if(action == 2){
                // skip this clause
                //printf("skip\n");
            }
            else if (action == 3){
                //printf("halt\n");
                TA0CCTL0 |= CCIFG;
                clause -= 1;
            }
            if( clause == CLAUSES -1)
                next_state = -1;
            else
                next_state = get_cur_state(c,clause+1);
            update_Q_table(cur_state, next_state, action, rewards[cur_state][action]);
            if(action != 3)
                sync_clause_index(c);
        }

        if(votes > max_votes){
            max_votes = votes;
            max_class = c;
        }
    }

    return max_class;
}

// main cpu on/off time control
void init_timerA(){
    TA0CTL = TACLR;  // clear TAR
    TA0CCR0 = cycles_on[cycle];
    TA0CTL = TASSEL_1 |MC_1; // ACLK, count-up mode
    TA0CCTL0 |= CCIE; // enable interrupt, and it will auto clear when interrupt occurred
    __bis_SR_register(GIE);  // enable general interrupt
    __no_operation(); // delay 1 cycle
}



int main(void)
{

    WDTCTL = WDTPW | WDTHOLD;   // stop watch-dog timer
    //PM5CTL0 &= ~LOCKLPM5;
    SetUp();  // system clock
    //IsReset();
    int i;
    // Timer setup
    init_timerA();
    init_timerB();
    init_Q();
    printf("init OK!\n");
    srand(time(NULL));

    // INFERENCE MODE START
    for(i=0;i<40;i++){
        pre_infer();
        if( chunking_infer_for_Q(X_Test[i]) != Y_Test[i] ){
            error_cnt += 1;
        }
        epsilon = MIN_EPSILON + (MAX_EPSILON-MIN_EPSILON) * exp(-1.0*DECAY_RATE*i);
    }

    //print_Qval();
    //TA0CCTL0 &= ~CCIE;
    //TB0CCTL0 &= ~CCIE;
    printf("cycle = %d,check_val = %d, error = %d\n",cycle, check_val, error_cnt);
    acc_rate = 1.0 - 1.0*error_cnt/40;
    printf("Acc: %f, error count = %d\n", acc_rate, error_cnt);

    return 0;
}



// timer 0 A0 ISR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    // after power fail
    if(power_fail_flag == 1){

        __bic_SR_register_on_exit(LPM3_bits); // back from LPM
        power_fail_flag = 0;
        cycle++;
        if(cycle>=1000)
            cycle = 0;

        TA0R = 0;
        TA0CCR0 = cycles_on[cycle];
        TA0CCTL0 |= CCIE;
        TB0R = 0;
        TB0CTL |= MC_1;  //restart timer B
        Restore();

    }
    else{
        check_timerA ++;
        //prepare into LPM3
        TA0CCR0 = FAIL_CYCLE;//power fail cycle
        TA0R = 0;
        power_fail_flag = 1;
        TB0R = 0;
        TB0CTL &= ~MC_3;  //stop timer B
        __bis_SR_register_on_exit(LPM3_bits + GIE); // into LPM
    }

}
