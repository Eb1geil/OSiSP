#include <windows.h>
#include <stdio.h>

// Structure to store thread information
struct ThreadData {
    int id;
    int data;
    HANDLE threadHandle;
    HANDLE finishedEvent;
};

// Global variables
HANDLE hSemaphore;
HANDLE hMutex;
ThreadData thread1Data;
ThreadData thread2Data;

// Thread function for the first thread
DWORD WINAPI ThreadFunction1(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;

    for (int i = 0; i < 5; i++) {
        WaitForSingleObject(hSemaphore, INFINITE);
        WaitForSingleObject(hMutex, INFINITE);

        data->data++;
        printf("Thread %d: Incremented data to %d\n", data->id, data->data);

        ReleaseMutex(hMutex);
        ReleaseSemaphore(hSemaphore, 1, NULL);
        Sleep(1000); // Simulate some work
    }

    SetEvent(data->finishedEvent); // Signal that the thread has finished
    return 0;
}

// Thread function for the second thread
DWORD WINAPI ThreadFunction2(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;

    for (int i = 0; i < 5; i++) {
        WaitForSingleObject(hSemaphore, INFINITE);
        WaitForSingleObject(hMutex, INFINITE);

        data->data--;
        printf("Thread %d: Decremented data to %d\n", data->id, data->data);

        ReleaseMutex(hMutex);
        ReleaseSemaphore(hSemaphore, 1, NULL);
        Sleep(1000); // Simulate some work
    }

    SetEvent(data->finishedEvent); // Signal that the thread has finished
    return 0;
}

int main() {
    // Initialize the semaphore and mutex
    hSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
    hMutex = CreateMutex(NULL, FALSE, NULL);

    if (hSemaphore == NULL || hMutex == NULL) {
        printf("Failed to create synchronization objects\n");
        return 1;
    }

    // Initialize thread data and events
    thread1Data.id = 1;
    thread2Data.id = 2;
    thread1Data.data = 0;
    thread2Data.data = 0;
    thread1Data.finishedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    thread2Data.finishedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // Create two threads
    thread1Data.threadHandle = CreateThread(NULL, 0, ThreadFunction1, &thread1Data, 0, NULL);
    thread2Data.threadHandle = CreateThread(NULL, 0, ThreadFunction2, &thread2Data, 0, NULL);

    if (thread1Data.threadHandle == NULL || thread2Data.threadHandle == NULL) {
        printf("Failed to create threads\n");
        return 2;
    }

    // Wait for threads to finish
    WaitForSingleObject(thread1Data.finishedEvent, INFINITE);
    WaitForSingleObject(thread2Data.finishedEvent, INFINITE);

    // Cleanup
    CloseHandle(thread1Data.threadHandle);
    CloseHandle(thread2Data.threadHandle);
    CloseHandle(thread1Data.finishedEvent);
    CloseHandle(thread2Data.finishedEvent);
    CloseHandle(hSemaphore);
    CloseHandle(hMutex);

    printf("Thread 1's final data: %d\n", thread1Data.data);
    printf("Thread 2's final data: %d\n", thread2Data.data);

    return 0;
}