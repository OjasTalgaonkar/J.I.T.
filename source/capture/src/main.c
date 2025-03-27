#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>

#define SHM_NAME "JIT_CORE_OPEN_MEM"
#define SHM_SIZE 16384
#define QUEUE_SIZE 30
#define PACKET_SIZE 512

typedef struct {
  HANDLE hMapFile;
  char *shared_mem;
} SharedMemory;

typedef struct {
  int head;
  int tail;
  int count;
  char packets[QUEUE_SIZE][PACKET_SIZE];
  HANDLE mutex;
} SharedQueue;

void enqueue_packets(SharedQueue *queue, const char *data, int len) {
  WaitForSingleObject(queue->mutex, INFINITE);
  if (queue->count < QUEUE_SIZE) {
    snprintf(queue->packets[queue->tail], PACKET_SIZE, "Packet: %.*s", len,
             data);
    queue->tail = (queue->tail + 1) % QUEUE_SIZE;
    queue->count++;
    printf("Enqueued packet at %d, count: %d\n", queue->tail, queue->count);
  } else {
    printf("Queue is full, dropping packet.\n");
  }
}

void packet_handler(u_char *user, const struct pcap_pkthdr *pkthdr,
                    const u_char *packet) {
  SharedQueue *queue = (SharedQueue *)user;

  if (queue) {
    enqueue_packets(queue, (const char *)packet, pkthdr->len);
  }
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
  queue->mutex = CreateMutex(NULL, FALSE, NULL);

  // Find all available network adapters
  if (pcap_findalldevs(&alldevs, errbuf) == -1) {
    printf("Error finding devices: %s\n", errbuf);
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
    return 1;
  }

  // Get user selection
  printf("Select an interface (1-%d): ", i);
  scanf("%d", &choice);

  dev = alldevs;
  for (i = 1; i < choice && choice < 10; i++)
    dev = dev->next;

  printf("Using device: %s\n", dev->name);

  handle = pcap_open_live(dev->name, BUFSIZ, 1, 1000, errbuf);
  if (handle == NULL) {
    printf("Error opening device: %s\n", errbuf);
    pcap_freealldevs(alldevs);
    return 1;
  }

  printf("Capturing packets... Press Ctrl+C to stop.\n");
  pcap_loop(handle, 0, &packet_handler, (u_char *)queue);

  pcap_freealldevs(alldevs);
  UnmapViewOfFile(queue);
  CloseHandle(hMapFile);
  return 0;
}
