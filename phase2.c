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

// Updated Account structure with mutex ( GIVEN )
typedef struct {
    int account_id ;
    double balance ;
    int transaction_count ;
    pthread_mutex_t lock ; // NEW: Mutex for this account
} Account ;
Account accounts [ NUM_ACCOUNTS ] ; // Global shared array
// GIVEN : Example of mutex initialization
void initialize_accounts () {
    for (int i = 0; i < NUM_ACCOUNTS ; i ++) {
        accounts[i].account_id = i ;
        accounts[i].balance = INITIAL_BALANCE ;
        accounts[i].transaction_count = 0;
        // Initialize the mutex
        pthread_mutex_init (& accounts [i]. lock , NULL ) ;
    }
}

void transfer_safe(int from_id, int to_id, double amount) {
    int first = (from_id < to_id) ? from_id : to_id;
    int second = (from_id < to_id) ? to_id : from_id;

    pthread_mutex_lock(&accounts[first].lock);
    pthread_mutex_lock(&accounts[second].lock);

    // ===== CRITICAL SECTION (both accounts) =====
    accounts[from_id].balance -= amount;
    accounts[to_id].balance   += amount;

    accounts[from_id].transaction_count++;
    accounts[to_id].transaction_count++;
    // ===========================================

    pthread_mutex_unlock(&accounts[second].lock);
    pthread_mutex_unlock(&accounts[first].lock);
}

// TODO 2: Update teller_thread to use safe functions
// Change : deposit_unsafe -> deposit_safe
// Change : withdrawal_unsafe -> withdrawal_safe
void * teller_thread ( void * arg ) {
    int teller_id = *( int *) arg ;

    unsigned int seed = (unsigned int)(time(NULL) ^ (unsigned long)pthread_self());
        for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
            int from = (int)(rand_r(&seed) % NUM_ACCOUNTS);
            int to;
            do {
                to = (int)(rand_r(&seed) % NUM_ACCOUNTS);
            } while (to == from);

            double amount = (double)((rand_r(&seed) % 100) + 1);

            transfer_safe(from, to, amount);

            printf("Teller %d: Transferred $%.2f from Account %d to Account %d\n",
            teller_id, amount, from, to);
        }

        return NULL ;
}

// TODO 3: Add performance timing
// Reference : Section 7.2 " Performance Measurement "
// Hint : Use clock_gettime ( CLOCK_MONOTONIC , & start );



// TODO 4: Add mutex cleanup in main ()
// Reference : man pthread_mutex_destroy
// Important : Destroy mutexes AFTER all threads complete !
void cleanup_mutexes () {
    for (int i = 0; i < NUM_ACCOUNTS ; i ++) {
        pthread_mutex_destroy (& accounts[i].lock ) ;
    }
}

int main () {
    printf ("=== Phase 2: Synchronized Demo ===\n\n") ;

    initialize_accounts(); // Initialize accounts and mutexes

    printf (" Initial State :\n") ;
        for (int i = 0; i < NUM_ACCOUNTS ; i ++) {
            printf (" Account %d: $%.2f\n", i , accounts [ i ]. balance ) ;
        }

    double expected_total = NUM_ACCOUNTS * INITIAL_BALANCE;
    printf ("\nExpected total : $%.2f\n\n", expected_total ) ;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start); // Start timing

    pthread_t threads [ NUM_THREADS ];
    int thread_ids [ NUM_THREADS ];

        for (int i = 0; i < NUM_THREADS ; i ++) {
                thread_ids [ i ] = i ;

                  int rc = pthread_create(&threads[i], NULL, teller_thread, &thread_ids[i]);
                if (rc != 0) {
                    fprintf(stderr, "Error: pthread_create failed for thread %d (code %d)\n", i, rc);
                        exit(1);
                }
        }

        for (int i = 0; i < NUM_THREADS ; i ++) {

                int rc = pthread_join(threads[i], NULL);
                if (rc != 0) {
                        fprintf(stderr, "Error: pthread_join failed for thread %d (code %d)\n", i, rc);
                        exit(1);
                }
        }
    clock_gettime(CLOCK_MONOTONIC, &end); // End timing
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;


// Calculate and display results
    printf ("\n=== Final Results ===\n") ;
    double actual_total = 0.0;
        for (int i = 0; i < NUM_ACCOUNTS ; i ++) {
            printf (" Account %d: $%.2f (%d transactions)\n",
            i , accounts [ i ]. balance , accounts [ i ]. transaction_count ) ;
            actual_total += accounts [ i ]. balance ;
        }

    printf ("\nExpected total : $%.2f\n", expected_total) ;
    printf (" Actual total : $%.2f\n", actual_total ) ;
    printf (" Difference : $%.2f\n", actual_total - expected_total ) ;

        if (expected_total != actual_total) {
            printf("\nRACE CONDITION DETECTED!\n");
        } else {
            printf("\nNo race condition detected this run.\n");
        }

    printf("\nExecution time: %.6f seconds\n", elapsed_time);

    cleanup_mutexes();

    return 0;
}


// TODO 5: Compare Phase 1 vs Phase 2 performance
// Measure execution time for both versions
// Document the overhead of synchronization
