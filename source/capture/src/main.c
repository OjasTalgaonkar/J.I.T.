#include <C:/Program Files/Npcap/npcap-sdk-1.15/Include/pcap.h>
#include <stdio.h>

void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr,
                    const u_char *packet) {
  printf("Packet captured: Length = %d bytes\n", pkthdr->len);
}

int main() {
  pcap_t *handle;
  char errbuf[PCAP_ERRBUF_SIZE];
  const char *dev;

  dev = pcap_lookupdev(errbuf);
  if (dev == NULL) {
    printf("Error finding device: %s\n", errbuf);
    return 1;
  }

  printf("Using device: %s\n", dev);

  // Open the device for capturing packets
  handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
  if (handle == NULL) {
    printf("Error opening device: %s\n", errbuf);
    return 1;
  }

  // Start capturing packets
  printf("Starting packet capture... Press Ctrl+C to stop.\n");
  if (pcap_loop(handle, 10, packet_handler, NULL) < 0) {
    printf("Error capturing packets: %s\n", pcap_geterr(handle));
    return 1;
  }

  // Close the capture handle
  pcap_close(handle);

  return 0;
}
