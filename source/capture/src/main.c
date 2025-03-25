#include <pcap.h>
#include <stdio.h>
#include <winsock2.h>

void packet_handler(u_char *user, const struct pcap_pkthdr *pkthdr,
                    const u_char *packet) {
  printf(user);
  printf("\nPacket captured: %d bytes\n", pkthdr->len);
}

int main() {
  pcap_if_t *alldevs, *dev;
  pcap_t *handle;
  char errbuf[PCAP_ERRBUF_SIZE];
  int i = 0, choice;

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
  pcap_loop(handle, 0, packet_handler, NULL);

  pcap_freealldevs(alldevs);
  return 0;
}