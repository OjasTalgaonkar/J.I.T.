#include <pcap.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>

#define SHM_NAME "JIT_CORE_OPEN_MEM"
#define SHM_SIZE 16384
#define QUEUE_SIZE 30
#define PACKET_SIZE 512
#define SEM_PROD "Global\\SemProd"
#define SEM_CONS "Global\\SemCons"

typedef struct {
  int head;
  int tail;
  int count;
  char packets[QUEUE_SIZE][PACKET_SIZE];
  int shutdown; // Shutdown flag for Go program
} SharedQueue;

void enqueue_packets(SharedQueue *queue, const char *data, int len,
                     HANDLE sem_prod, HANDLE sem_cons) {
  WaitForSingleObject(sem_prod, INFINITE); // Wait for empty slot
  if (queue->count < QUEUE_SIZE) {
    snprintf(queue->packets[queue->tail], PACKET_SIZE, "Packet: %.*s", len,
             data);
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;
    queue->count++;
    printf("Enqueued packet at %d, count: %d\n", queue->tail, queue->count);
  } else {
    printf("Queue is full, dropping packet.\n");
  }
  ReleaseSemaphore(sem_cons, 1, NULL); // Signal consumer
}

void packet_handler(u_char *user, const struct pcap_pkthdr *pkthdr,
                    const u_char *packet) {
  SharedQueue *queue = (SharedQueue *)user;
  HANDLE *semaphores = (HANDLE *)(queue + 1); // Semaphores stored after queue
  enqueue_packets(queue, (const char *)packet, pkthdr->len, semaphores[0],
                  semaphores[1]);
}

int main() {
  pcap_if_t *alldevs, *dev;
  pcap_t *handle;
  char errbuf[PCAP_ERRBUF_SIZE];
  int i = 0, choice;

  // Create shared memory
  HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
                                      PAGE_READWRITE, 0, SHM_SIZE, SHM_NAME);
  if (hMapFile == NULL) {
    printf("CreateFileMapping failed (%lu)\n", GetLastError());
    return 1;
  }

  SharedQueue *queue = (SharedQueue *)MapViewOfFile(
      hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHM_SIZE);
  if (queue == NULL) {
    printf("MapViewOfFile failed (%lu)\n", GetLastError());
    CloseHandle(hMapFile);
    return 1;
  }

  queue->head = 0;
  queue->tail = 0;
  queue->count = 0;
  queue->shutdown = 0;

  HANDLE sem_prod =
      CreateSemaphore(NULL, QUEUE_SIZE, QUEUE_SIZE, TEXT(SEM_PROD));
  if (sem_prod == NULL) {
    printf("CreateSemaphore (SemProd) failed (%lu)\n", GetLastError());
    UnmapViewOfFile(queue);
    CloseHandle(hMapFile);
    return 1;
  }

  HANDLE sem_cons = CreateSemaphore(NULL, 0, QUEUE_SIZE, TEXT(SEM_CONS));
  if (sem_cons == NULL) {
    printf("CreateSemaphore (SemCons) failed (%lu)\n", GetLastError());
    CloseHandle(sem_prod);
    UnmapViewOfFile(queue);
    CloseHandle(hMapFile);
    return 1;
  }

  // Store semaphores in shared memory (after queue)
  HANDLE *semaphores = (HANDLE *)(queue + 1);
  semaphores[0] = sem_prod;
  semaphores[1] = sem_cons;

  // Find all available network adapters
  if (pcap_findalldevs(&alldevs, errbuf) == -1) {
    printf("Error finding devices: %s\n", errbuf);
    CloseHandle(sem_prod);
    CloseHandle(sem_cons);
    UnmapViewOfFile(queue);
    CloseHandle(hMapFile);
    return 1;
  }

  printf("Available network interfaces:\n");
  for (dev = alldevs; dev != NULL; dev = dev->next) {
    printf("%d: %s", ++i, dev->name);
    if (dev->description)
      printf(" (%s)", dev->description);
    printf("\n");
  }

  if (i == 0) {
    printf("No interfaces found.\n");
    pcap_freealldevs(alldevs);
    CloseHandle(sem_prod);
    CloseHandle(sem_cons);
    UnmapViewOfFile(queue);
    CloseHandle(hMapFile);
    return 1;
  }

  // Get user selection
  printf("Select an interface (1-%d): ", i);
  scanf("%d", &choice);

  // Validate choice
  if (choice < 1 || choice > i) {
    printf("Invalid choice.\n");
    pcap_freealldevs(alldevs);
    CloseHandle(sem_prod);
    CloseHandle(sem_cons);
    UnmapViewOfFile(queue);
    CloseHandle(hMapFile);
    return 1;
  }

  dev = alldevs;
  for (i = 1; i < choice; i++)
    dev = dev->next;

  printf("Using device: %s\n", dev->name);

  // Launch the Go program after semaphores are created
  SHELLEXECUTEINFO sei = {0};
  sei.cbSize = sizeof(SHELLEXECUTEINFO);
  sei.fMask = SEE_MASK_NOCLOSEPROCESS;
  sei.lpVerb = TEXT("open");
  sei.lpFile = TEXT("build\\packet_processor.exe");
  sei.lpDirectory = NULL;
  sei.nShow = SW_SHOWNORMAL; // Open in a new console window to see Go output

  if (!ShellExecuteEx(&sei)) {
    printf("Failed to launch Go program (%lu)\n", GetLastError());
    pcap_freealldevs(alldevs);
    CloseHandle(sem_prod);
    CloseHandle(sem_cons);
    UnmapViewOfFile(queue);
    CloseHandle(hMapFile);
    return 1;
  }
  printf("Launched Go program (packet_processor.exe)\n");

  handle = pcap_open_live(dev->name, BUFSIZ, 1, 1000, errbuf);
  if (handle == NULL) {
    printf("Error opening device: %s\n", errbuf);
    pcap_freealldevs(alldevs);
    CloseHandle(sem_prod);
    CloseHandle(sem_cons);
    UnmapViewOfFile(queue);
    CloseHandle(hMapFile);
    return 1;
  }

  printf("Capturing packets... Press Ctrl+C to stop.\n");
  pcap_loop(handle, 0, &packet_handler, (u_char *)queue);

  queue->shutdown = 1;                 // Signal Go program to exit
  ReleaseSemaphore(sem_cons, 1, NULL); // Wake up Go program to check shutdown
  Sleep(1000);

  // end handlers
  pcap_close(handle);
  pcap_freealldevs(alldevs);
  CloseHandle(sem_prod);
  CloseHandle(sem_cons);
  UnmapViewOfFile(queue);
  CloseHandle(hMapFile);
  return 0;
}