#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 5
#define NUM_PLACES 3

int prod = 2;              // Anzahl der Produzenten
int done = 0;              // Zähler für fertige Produzenten
pthread_mutex_t mutex;     // Mutex zur Absicherung kritischer Abschnitte
pthread_cond_t not_full;   // Signalisiert, dass der Puffer nicht voll ist
pthread_cond_t not_empty;  // Signalisiert, dass der Puffer nicht leer ist

int buffer[NUM_PLACES];    // Buffer erstellen
int last = 0;              // Zeiger für die nächste Schreibposition
int take = 0;              // Zeiger für die nächste Leseposition

void produzieren(void *threadid) {
    for (int i = 0; i < 10; i++) {
        pthread_mutex_lock(&mutex);

        // Warten, falls der Buffer voll ist
        while (buffer[last] != -1) {
            pthread_cond_wait(&not_full, &mutex);
        }

        // Daten in den Buffer schreiben
        buffer[last] = i;
        printf("Producer %ld puts %d into buffer at place %d\n", (long)threadid, buffer[last], last);
        last = (last + 1) % NUM_PLACES;

        // Konsumenten signalisieren
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_lock(&mutex);
    done++;  // Produzent meldet sich als fertig
    pthread_cond_broadcast(&not_empty);  // Konsumenten wecken, falls sie terminieren sollen
    pthread_mutex_unlock(&mutex);
}

void konsumieren(void *threadid) {
    while (1) {
        pthread_mutex_lock(&mutex);

        // Warten, falls der Buffer leer ist und Produzenten noch aktiv sind
        while (buffer[take] == -1 && done < prod) {
            pthread_cond_wait(&not_empty, &mutex);
        }

        // Beenden, wenn der Buffer leer ist und alle Produzenten fertig sind
        if (buffer[take] == -1 && done == prod) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Generieren einer zufälligen Zahl zwischen 1 und NUM_PLACES
        int min = 1;
        int max = NUM_PLACES;
        int randomNumber = min + rand() % (max - min + 1);

        // Konsumieren der zufälligen Anzahl an Elementen
        while (randomNumber > 0) {
            if (buffer[take] == -1) {  // Prüfen, ob der Buffer leer ist
                break;
            }
            printf("Consumer %ld takes %d from place %d\n", (long)threadid, buffer[take], take);
            buffer[take] = -1;
            take = (take + 1) % NUM_PLACES;
            randomNumber--;
        }

        // Produzenten signalisieren
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mutex);
    }
}

void *Producer(void *threadid) {
    produzieren(threadid);
    pthread_exit(NULL);
}

void *Consumer(void *threadid) {
    konsumieren(threadid);
    pthread_exit(NULL);
}

int main(void) {
    for (int i = 0; i < NUM_PLACES; i++) {
        buffer[i] = -1; // Buffer initialisieren (leer = -1)
    }

    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&not_full, NULL);
    pthread_cond_init(&not_empty, NULL);

    // Threads erstellen
    for (long t = 0; t < NUM_THREADS; t++) {
        if (t < prod) {
            pthread_create(&threads[t], NULL, Producer, (void *)t);
        } else {
            pthread_create(&threads[t], NULL, Consumer, (void *)t);
        }
    }

    // Auf Threads warten
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    // Ressourcen freigeben
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_full);
    pthread_cond_destroy(&not_empty);

    return 0;
}
