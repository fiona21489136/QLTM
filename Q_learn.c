#include "Q_learn.h"
#include "clause_acc_data.h"
#include "mementos.h"

#pragma PERSISTENT(Q_table)
#pragma DATA_SECTION(Q_table_learned, ".cinit")
#pragma DATA_SECTION(rewards, ".cinit")

volatile float epsilon = EPSILON;  // backup

volatile float Q_table[N_QSTATES][N_ACTIONS] = { 0.0 };  // backup

volatile float Q_table_learned[N_QSTATES][N_ACTIONS] = {
{0.0920544, 0.1167428, 0.0997489, 0.0},
{0.1187878, 0.1046250, 0.0941442, 0.0},
{0.0994577, 0.1088349, 0.0943923, 0.0},
{0.1320452, 0.1235244, 0.0870153, 0.0},
{0.0920544, 0.1167428, 0.0997489, 0.0},
{0.1187878, 0.1046250, 0.0941442, 0.0},
{0.0994577, 0.1088349, 0.0943923, 0.0},
{0.1320452, 0.1235244, 0.0870153, 0.0},
};

/*
 * {0.920544, 1.167428, 0.997489},
{1.187878, 1.046250, 0.941442},
{0.994577, 1.088349, 0.943923},
{1.320452, 1.235244, 0.870153}*/


/*
0.968840 1.218334 1.212086
1.242702 1.211189 0.959409
1.103225 1.142422 0.995587
1.388953 1.185182 0.953447
 * */


//float Q_table_learned[N_QSTATES][N_ACTIONS] = {
//{0.175952, 0.278982, 0.267440},
//{0.209110, 0.295435, 0.123890},
//{0.234722, 0.280408, 0.174475},
//{0.364271, 0.270529, 0.164777},
//{0.175952, 0.278982, 0.267440},
//{0.209110, 0.295435, 0.123890},
//{0.234722, 0.280408, 0.174475},
//{0.364271, 0.270529, 0.164777},
//};

// ! given more reasonable !
volatile float rewards[N_QSTATES][N_ACTIONS] = {
    {  -0.09,  0.01,  0.04, 0.02},
    {  0.01,  0.01,  0.03, 0.02 },
    {  0.01,  0.03, -0.03, 0.05},
    {  0.025,  0.03,  -0.05, 0.05}, //
    {  -0.09,  0.02,  0.04, -0.01},
    {  -0.09,  0.02,  0.01, -0.017 },
    {  0.01,  0.03, -0.03, -0.03},
    {  0.04,  0.01,  -0.09, -0.09},
};

/*
 * {  -0.09,  0.04,  0.02, },
    {  0.01,  0.05,  -0.1, },
    {  0.01,  0.05, -0.09, },
    {  0.33,  0.01,  -0.09, },
 * */


void init_Q(){
    //printf("init Q \n");
    int s, a;
    for(s = 0; s < N_QSTATES; s++){
        for(a = 0; a < N_ACTIONS; a++){
            Q_table[s][a] = Q_table_learned[s][a];
        }
    }
}

void print_Qval(){
    int s, a;
    printf("Q_table:\n");
    for(s = 0; s < N_QSTATES; s++){
        for(a = 0; a < N_ACTIONS; a++){
            printf("%f ",Q_table[s][a]);
        }
        printf("\n");
    }
    printf("\n");
}
// Bellman ford Equation:
// Q'(s,a) = Q(s,a) + alpha*( reward(s,a) + gamma* Max{ Q(s',ALL a)} - Q(s,a) );

int8_t get_rand_state(){
    return rand()%N_QSTATES;
}

// !! read system indicators
int8_t get_cur_state(uint8_t class, uint8_t clause){
    // factors:  (1) clause acc , (2) clause len
    // Q-learning : can learn from the experience that agent react to the environment
    // *action : each step that you react to the env
    // *state : >> from the (1)(2)
    if(  avg_acc_Lv0[class] - clause_acc_Lv0[class][clause] > acc_std_dev_Lv0[class]
    && (clause_lens_lv0[class][clause] - clause_len_avg_lv0[class]) > clasuse_len_std_dev_lv0[class]
    && (TA0CCR0 - TA0R) < (CYCLE_TIME - TB0R)){
        return 0; // worst
    }
    else if( avg_acc_Lv0[class] - clause_acc_Lv0[class][clause] > acc_std_dev_Lv0[class]
    && clause_lens_lv0[class][clause] < clause_len_avg_lv0[class]
    && (TA0CCR0 - TA0R) < (CYCLE_TIME - TB0R)){
        return 1;
    }
    else if(clause_acc_Lv0[class][clause] >= avg_acc_Lv0[class]
    && (clause_lens_lv0[class][clause] - clause_len_avg_lv0[class]) > clasuse_len_std_dev_lv0[class]
    && (TA0CCR0 - TA0R) < (CYCLE_TIME - TB0R)){
        return 2;
    }
    else if (clause_acc_Lv0[class][clause] >= avg_acc_Lv0[class]
    && clause_lens_lv0[class][clause] < clause_len_avg_lv0[class]
    && (TA0CCR0 - TA0R) < (CYCLE_TIME - TB0R)){
        return 3;
    }
    else if( avg_acc_Lv0[class] - clause_acc_Lv0[class][clause] > acc_std_dev_Lv0[class]
    && (clause_lens_lv0[class][clause] - clause_len_avg_lv0[class]) > clasuse_len_std_dev_lv0[class]
    && (TA0CCR0 - TA0R) >= (CYCLE_TIME - TB0R)){
        return 4;
    }
    else if(avg_acc_Lv0[class] - clause_acc_Lv0[class][clause] > acc_std_dev_Lv0[class]
    && clause_lens_lv0[class][clause] < clause_len_avg_lv0[class]
    && (TA0CCR0 - TA0R) >= (CYCLE_TIME - TB0R)){
        return 5;
    }
    else if(clause_acc_Lv0[class][clause] >= avg_acc_Lv0[class]
    && (clause_lens_lv0[class][clause] - clause_len_avg_lv0[class]) > clasuse_len_std_dev_lv0[class]
    && (TA0CCR0 - TA0R) >= (CYCLE_TIME - TB0R)){
        return 6;
    }
    else if (clause_acc_Lv0[class][clause] >= avg_acc_Lv0[class]
    && clause_lens_lv0[class][clause] < clause_len_avg_lv0[class]
    && (TA0CCR0 - TA0R) >= (CYCLE_TIME - TB0R)){
        return 7; // best
    }
    else if( (TA0CCR0 - TA0R) < (CYCLE_TIME - TB0R) ){
        return rand() % 4;
    }
    else{
        return rand() % 4 + 4;
    }
}

int8_t get_random_action(){
    int rand_num = rand()%100;
    if( rand_num < 30){
        return 0;
    }
    else if( rand_num < 85){
        return 1;
    }
    else{
        return 2;
    }
    //return rand()%N_ACTIONS;
}

int8_t choose_action( int8_t cur_state){
    // choose the max expect action with max Q value


    //printf("e: %f \n",epsilon);
    if( cur_state >= N_QSTATES || (float)rand()/(float)RAND_MAX <= epsilon){
        //printf("get rand action!\n");
        return get_random_action();
    }
    else{
        float max_Qval = Q_table[cur_state][0];
        int best_action = 0; // default: full run
        int i;
        //print_Qval();
        for(i=1;i<N_ACTIONS;i++){
            if( Q_table[cur_state][i] >= max_Qval){
                max_Qval = Q_table[cur_state][i];
                best_action = i;
            }
        }
        //printf("get best action %d from max_Qval: %f\n",best_action, max_Qval);
        return best_action;
    }

}

void update_Q_table(int8_t cur_state, int8_t next_state, int8_t action, float reward){
    if(next_state == -1){
        // til this class end
        return;
    }
    uint8_t a;
    double max_next_Q = Q_table[next_state][0];
    for(a = 1; a < N_ACTIONS; a++){
        if(Q_table[next_state][a] > max_next_Q){
            max_next_Q = Q_table[next_state][a];
        }
    }
    //printf("updateing... max_next_Q_val %f\n",max_next_Q);
    Q_table[cur_state][action] = Q_table[cur_state][action] + L_RATE*( reward + GAMMA*max_next_Q - Q_table[cur_state][action] );
}

// Bellman ford Equation:
// Q(s,a) = Q(s,a) + alpha*( reward(s,a) + gamma* Max{ Q(next states ,ALL a)} );
