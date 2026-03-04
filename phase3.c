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

typedef struct {
    int account_id;
    double balance;
    int transaction_count;
    pthread_mutex_t lock;
} Account;

Account accounts[NUM_ACCOUNTS];

// progress tracking for deadlock detection
volatile int progress_counter = 0;

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


// GIVEN : Conceptual example showing HOW deadlock occurs
void transfer_deadlock_example ( int from_id , int to_id , double amount ) {
    // This code WILL cause deadlock !
    // Lock source account
    pthread_mutex_lock (& accounts [ from_id ]. lock ) ;
    printf (" Thread %ld: Locked account %d\n", pthread_self () , from_id ) ;
    // Simulate processing delay
    usleep (100) ;
    // Try to lock destination account
    printf (" Thread %ld: Waiting for account %d\n", pthread_self () , to_id ) ;
    pthread_mutex_lock (& accounts [ to_id ]. lock ) ; // DEADLOCK HERE !
    // Transfer ( never reached if deadlocked )
    accounts [ from_id ]. balance -= amount ;
    accounts [ to_id ]. balance += amount ;

    pthread_mutex_unlock (& accounts [ to_id ]. lock ) ;
    pthread_mutex_unlock (& accounts [ from_id ]. lock ) ;
}
// TODO 1: Implement complete transfer function
// Use the example above as reference
// Add balance checking ( sufficient funds ?)
// Add error handling

void transfer_deadlock(int from_id, int to_id, double amount) {
    // Lock source account
    pthread_mutex_lock(&accounts[from_id].lock);
    printf("Thread %ld: Locked account %d\n", pthread_self(), from_id);

    // Simulate processing delay
    usleep(100);

    // Check for sufficient funds
    if (accounts[from_id].balance < amount) {
        printf("Thread %ld: Insufficient funds in account %d\n", pthread_self(), from_id);
        pthread_mutex_unlock(&accounts[from_id].lock);
        return;
    }

    // Try to lock destination account
    printf("Thread %ld: Waiting for account %d\n", pthread_self(), to_id);
    pthread_mutex_lock(&accounts[to_id].lock); // Potential deadlock here

    // Perform transfer
    accounts[from_id].balance -= amount;
    accounts[to_id].balance += amount;

    // Update transaction counts
    accounts[from_id].transaction_count++;
    accounts[to_id].transaction_count++;

    pthread_mutex_unlock(&accounts[to_id].lock);
    pthread_mutex_unlock(&accounts[from_id].lock);

    // Update progress counter for deadlock detection
    __sync_fetch_and_add(&progress_counter, 1);
}

typedef struct {
    int from_id;
    int to_id;
    double amount;
} TransferArgs;

// TODO 2: Create threads that will deadlock
// Thread 1: transfer (0 , 1 , amount ) // Locks 0 , wants 1
// Thread 2: transfer (1 , 0 , amount ) // Locks 1 , wants 0
// Result : Circular wait !

void *deadlock_thread(void *arg) {
    TransferArgs *t = (TransferArgs *)arg;
    transfer_deadlock(t->from_id, t->to_id, t->amount);
    return NULL;
}

int main() {
    printf("=== Phase 3: Deadlock Creation Demo ===\n\n");
    initialize_accounts();

    printf("Initial State:\n");
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        printf(" Account %d: $%.2f\n", i, accounts[i].balance);
    }
    printf("\n");

    pthread_t t1, t2;

    // Thread 1: lock 0 then want 1
    TransferArgs a1 = {0, 1, 10.0};
    // Thread 2: lock 1 then want 0
    TransferArgs a2 = {1, 0, 10.0};

    pthread_create(&t1, NULL, deadlock_thread, &a1);
    pthread_create(&t2, NULL, deadlock_thread, &a2);

    // TODO 3: Implement deadlock detection
    time_t start = time(NULL);
    int last_progress = progress_counter;

    while (1) {
        sleep(1);

        // If progress changed, reset timer baseline
        if (progress_counter != last_progress) {
            last_progress = progress_counter;
            start = time(NULL);
        }

        // If no progress for 5 seconds, suspect deadlock
        if (time(NULL) - start >= 5) {
            printf("\n*** SUSPECTED DEADLOCK: No progress for 5 seconds ***\n");
            break;
        }
    }

    cleanup_mutexes();
    return 0;
}





// TODO 4: Document the Coffman conditions
// In your report , identify WHERE each condition occurs
// Create resource allocation graph showing circular wait
