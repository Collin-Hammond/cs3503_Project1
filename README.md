## Phase 1 – Race Condition Demonstration

This program simulates multiple bank teller threads accessing shared
accounts without synchronization. Because multiple threads update the
same account balances at the same time, race conditions can occur and
produce incorrect final totals.

## Compilation

gcc phase1.c -o phase1 -pthread

## Run

./phase1

---

## Phase 2 – Resource Protection with Mutexes

This phase adds mutex locks to protect shared account data. Each account
has its own mutex, and threads must acquire the lock before modifying
the account balance. This ensures that only one thread can update an
account at a time and prevents race conditions.

## Compilation

gcc phase2.c -o phase2 -pthread

## Run

./phase2

---

## Phase 3 – Deadlock Creation

This phase demonstrates how deadlock can occur when multiple threads
lock resources in different orders. Two threads attempt to transfer
money between accounts while locking them in opposite orders, which
creates a circular wait condition and causes deadlock. The program
includes a timeout mechanism to detect when threads are stuck.

## Compilation

gcc phase3.c -o phase3 -pthread

## Run

./phase3

---

## Phase 4 – Deadlock Resolution (Lock Ordering)

This phase resolves the deadlock problem by enforcing a consistent lock
ordering strategy. Threads always lock the account with the lower ID
first and the higher ID second. This prevents circular wait and allows
both threads to complete their transfers successfully.

## Compilation

gcc phase4.c -o phase4 -pthread

## Run

./phase4
