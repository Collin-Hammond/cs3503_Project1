#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Configuration
#define NUM_ACCOUNTS 2
#define NUM_THREADS 4
#define TRANSACTIONS_PER_THREAD 10
#define INITIAL_BALANCE 1000.0

// Account data structure (GIVEN)
typedef struct {
    int account_id;
    double balance;
    int transaction_count;
} Account;

// Per-thread stats to compute correct expected total
typedef struct {
    int teller_id;
    double deposited;
    double withdrawn;
} ThreadStats;

// Global shared array - THIS CAUSES RACE CONDITIONS!
Account accounts[NUM_ACCOUNTS];

// GIVEN: deposit function WITH race condition
void deposit_unsafe(int account_id, double amount) {
    // READ
    double current_balance = accounts[account_id].balance;

    // MODIFY (simulate processing time)
    usleep(1);

    double new_balance = current_balance + amount;

    // WRITE
    accounts[account_id].balance = new_balance;
    accounts[account_id].transaction_count++;
}

// withdrawal WITH race condition (same pattern)
void withdrawal_unsafe(int account_id, double amount) {
    // READ
    double current_balance = accounts[account_id].balance;

    // MODIFY (simulate processing time)
    usleep(1);

    double new_balance = current_balance - amount;

    // WRITE
    accounts[account_id].balance = new_balance;
    accounts[account_id].transaction_count++;
}

// Thread function
void *teller_thread(void *arg) {
    ThreadStats *stats = (ThreadStats *)arg;
    int teller_id = stats->teller_id;

    unsigned int seed = (unsigned int)(time(NULL) ^ (unsigned long)pthread_self());

    stats->deposited = 0.0;
    stats->withdrawn = 0.0;

    for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
        int account_idx = (int)(rand_r(&seed) % NUM_ACCOUNTS);
        double amount = (double)((rand_r(&seed) % 100) + 1);
        int operation = (int)(rand_r(&seed) % 2); // 1 = deposit, 0 = withdraw

        if (operation == 1) {
            deposit_unsafe(account_idx, amount);
            stats->deposited += amount;

            printf("Teller %d: Deposited $%.2f to Account %d\n",
                   teller_id, amount, account_idx);
        } else {
            withdrawal_unsafe(account_idx, amount);
            stats->withdrawn += amount;

            printf("Teller %d: Withdrew   $%.2f from Account %d\n",
                   teller_id, amount, account_idx);
        }
    }

    return NULL;
}

int main() {
    printf("=== Phase 1: Race Conditions Demo ===\n\n");

    // Initialize accounts
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;
    }

    // Display initial state
    printf("Initial State:\n");
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        printf(" Account %d: $%.2f\n", i, accounts[i].balance);
    }

    // Timing start
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Create threads
    pthread_t threads[NUM_THREADS];
    ThreadStats stats[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        stats[i].teller_id = i;

        int rc = pthread_create(&threads[i], NULL, teller_thread, &stats[i]);
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_create failed for thread %d (code %d)\n", i, rc);
            exit(1);
        }
    }

    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        int rc = pthread_join(threads[i], NULL);
        if (rc != 0) {
            fprintf(stderr, "Error: pthread_join failed for thread %d (code %d)\n", i, rc);
            exit(1);
        }
    }

    // Timing end
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) +
                          (end.tv_nsec - start.tv_nsec) / 1e9;

    // Compute correct expected total
    double total_deposited = 0.0;
    double total_withdrawn = 0.0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_deposited += stats[i].deposited;
        total_withdrawn += stats[i].withdrawn;
    }

    double initial_total = NUM_ACCOUNTS * INITIAL_BALANCE;
    double expected_total = initial_total + total_deposited - total_withdrawn;

    // Final results
    printf("\n=== Final Results ===\n");
    double actual_total = 0.0;

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        printf(" Account %d: $%.2f (%d transactions)\n",
               i, accounts[i].balance, accounts[i].transaction_count);
        actual_total += accounts[i].balance;
    }

    printf("\nInitial total:   $%.2f\n", initial_total);
    printf("Total deposited: $%.2f\n", total_deposited);
    printf("Total withdrawn: $%.2f\n", total_withdrawn);
    printf("Expected total:  $%.2f\n", expected_total);
    printf("Actual total:    $%.2f\n", actual_total);
    printf("Difference:      $%.2f\n", actual_total - expected_total);

    double diff = actual_total - expected_total;
    if (diff > 0.0001 || diff < -0.0001) {
        printf("\nRACE CONDITION DETECTED!\n");
        printf("Run this program multiple times (./phase1) to see different results.\n");
    } else {
        printf("\nNo race condition detected this run (run again).\n");
    }

    printf("\nExecution time: %.6f seconds\n", elapsed_time);

    return 0;
}
