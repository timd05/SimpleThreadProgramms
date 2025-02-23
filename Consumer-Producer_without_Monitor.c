#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#define NUM_THREADS 5
#define NUM_PLACES 3

// prod gibt die Anzahl der Produzenten-Threads an
//Bedingung dafür: prod < NUM_THREADS
int prod = 2;
int done;

sem_t empty;
sem_t full;
sem_t mutex;

// initialisierung eiens Buffers für die Plätze die befüllt werden können
int last;
int buffer[NUM_PLACES];

void *Producer(void *threadid) {
    int i;
    for (i = 0; i < 10; i++) {
        sem_wait(&full);
        sem_wait(&mutex);
        // wenn der letzte Platz des Buffers belegt wurde, dann wird 
        // sleep aufgerufen, damit ein Konsument als nächstes dran ist 
        if (last == NUM_PLACES) {
            sem_post(&mutex);
            usleep(1000); // 0.001 Sekunden warten
            sem_wait(&mutex);
        }
        buffer[last] = i;
        printf("Producer %ld puts %d into buffer at place %d \n", (long)threadid, buffer[last], last);
        // platz des Buffers wurde belegt --> last wird erhöht
        last++;
        printf("The Buffer is now filled until place %d \n", last);
        sem_post(&mutex);
        sem_post(&empty);
    }
    sem_wait(&mutex);
    // wenn i-mal Produziert wurde wird done erhöht
    done++;
    sem_post(&mutex);
    sem_post(&empty);
    pthread_exit(NULL);
}

void *Consumer(void *threadid) {
    while (1) {
        sem_wait(&empty);
        sem_wait(&mutex);
        if (last > 0) { // wenn Terminierungsbedinung nicht erfüllt wurde
            printf("Consumer %ld takes %d.   ", (long)threadid, buffer[last - 1]);
            fflush(stdout);
            last--;
            printf("The Buffer is now filled until place %d \n", last);
        }
        if (done == (prod + 1) && last == 0) {
        // wenn alle Produzenten fertig sind und der Buffer leer ist
        // (Terminierungsbedingung)
            sem_post(&mutex);
            sem_post(&full);
            printf("Break!");
            exit(EXIT_SUCCESS);
        }
        sem_post(&mutex);
        sem_post(&full);
    }
    pthread_exit(NULL);
}

int main(void) {
    pthread_t threads[NUM_THREADS];
    int rc;
    long t;

    // initialisierung der Semaphoren
    sem_init(&empty, 0, 0);
    sem_init(&full, 0, NUM_PLACES);
    sem_init(&mutex, 0, 1);

    // erstellen der jeweiligen Threads (prod viele Produzenten und
    // NUM_THREAD-prod viele Konsumenten
    for (t = 0; t < NUM_THREADS; t++) {
        if (t <= prod) {
            rc = pthread_create(&threads[t], NULL, Producer, (void *)t);
        } else {
            rc = pthread_create(&threads[t], NULL, Consumer, (void *)t);
        }
        if (rc) {
            exit(-1);
        }
    }

    // warten, dass alle Threads fertig sind
    for (t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    // Zerstören der Semaphoren
    sem_destroy(&mutex);
    sem_destroy(&full);
    sem_destroy(&empty);

    return 0;
}
