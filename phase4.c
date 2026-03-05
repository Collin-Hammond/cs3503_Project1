/* STRATEGY 1: Lock Ordering ( RECOMMENDED )
*
* ALGORITHM :
* To prevent circular wait , always acquire locks in consistent order
*
* Step 1: Identify which account ID is lower
* Step 2: Lock lower ID first
* Step 3: Lock higher ID second
* Step 4: Perform transfer
* Step 5: Unlock in reverse order
*
* WHY THIS WORKS :
* - Thread 1: transfer (0 ,1) locks 0 then 1
* - Thread 2: transfer (1 ,0) locks 0 then 1 ( SAME ORDER !)
* - No circular wait possible
*
* WHICH COFFMAN CONDITION DOES THIS BREAK ?
* Answer in your report !
*/// TODO : Implement safe_transfer_ordered (from , to , amount )
// Use the algorithm description above
// Hint : int first = ( from < to) ? from : to;

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define NUM_ACCOUNTS 2
#define INITIAL_BALANCE 1000.0

typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock;
} Account;

Account accounts[NUM_ACCOUNTS];

void initialize_accounts() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;
        pthread_mutex_init(&accounts[i].lock, NULL);
    }
}

void cleanup_mutexes() {
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_destroy(&accounts[i].lock);
    }
}

// Phase 4: deadlock prevention using lock ordering
void safe_transfer_ordered(int from_id, int to_id, double amount) {
    if (from_id == to_id) return;

    int first = (from_id < to_id) ? from_id : to_id;
    int second = (from_id < to_id) ? to_id : from_id;

    pthread_mutex_lock(&accounts[first].lock);
    // delay to prove it still won't deadlock
    usleep(100);
    pthread_mutex_lock(&accounts[second].lock);

    // ===== CRITICAL SECTION (both accounts) =====
    if (accounts[from_id].balance >= amount) {
        accounts[from_id].balance -= amount;
        accounts[to_id].balance += amount;
        accounts[from_id].transaction_count++;
        accounts[to_id].transaction_count++;
    } else {
        printf("Thread %lu: Insufficient funds in account %d\n",
               (unsigned long)pthread_self(), from_id);
    }
    // ===========================================

    pthread_mutex_unlock(&accounts[second].lock);
    pthread_mutex_unlock(&accounts[first].lock);
}

typedef struct {
    int from_id;
    int to_id;
    double amount;
} TransferArgs;

void *ordered_thread(void *arg) {
    TransferArgs *t = (TransferArgs *)arg;
    safe_transfer_ordered(t->from_id, t->to_id, t->amount);
    printf("Thread %lu: Completed transfer %d -> %d\n",
           (unsigned long)pthread_self(), t->from_id, t->to_id);
    return NULL;
}

int main() {
    printf("=== Phase 4: Deadlock Resolution (Lock Ordering) ===\n\n");
    initialize_accounts();

    printf("Initial State:\n");
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        printf(" Account %d: $%.2f\n", i, accounts[i].balance);
    }
    printf("\n");

    pthread_t t1, t2;

    // Same scenario as Phase 3, but should NOT deadlock now
    TransferArgs a1 = {0, 1, 10.0};
    TransferArgs a2 = {1, 0, 10.0};

    pthread_create(&t1, NULL, ordered_thread, &a1);
    pthread_create(&t2, NULL, ordered_thread, &a2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("\n=== Final Results ===\n");
    double total = 0.0;
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        printf(" Account %d: $%.2f (%d transactions)\n",
               i, accounts[i].balance, accounts[i].transaction_count);
        total += accounts[i].balance;
    }
    printf("Total in system: $%.2f\n", total);

    cleanup_mutexes();
    return 0;
}
