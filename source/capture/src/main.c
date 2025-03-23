#include <pcap.h>
#include <stdio.h>

void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr,
                    const u_char *packet) {
  printf("Packet captured: Length = %d bytes\n", pkthdr->len);
}

int main() {
  pcap_t *handle;
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_if_t *alldevs, *dev;

  // Find all available network devices
  if (pcap_findalldevs(&alldevs, errbuf) == -1) {
    printf("Error finding devices: %s\n", errbuf);
    return 1;
  }

  // Select the first device
  dev = alldevs;
  if (!dev) {
    printf("No devices found.\n");
    return 1;
  }

  printf("Using device: %s\n", dev->name);

  // Open the device for capturing packets
  handle = pcap_open_live(dev->name, BUFSIZ, 1, 1000, errbuf);
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

  // Clean up
  pcap_close(handle);
  pcap_freealldevs(alldevs);

  return 0;
}
