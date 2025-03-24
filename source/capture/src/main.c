#include <pcap.h>
#include <stdio.h>
#include <winsock2.h>

// Define Ethernet header structure
struct ethernet_header {
  u_char dest_mac[6];
  u_char src_mac[6];
  u_short ether_type;
};

// Define IPv4 header structure
struct ip_header {
  u_char version_ihl;
  u_char tos;
  u_short total_length;
  u_short id;
  u_short flags_offset;
  u_char ttl;
  u_char protocol;
  u_short checksum;
  struct in_addr src_ip;
  struct in_addr dst_ip;
};

// Define TCP header structure
struct tcp_header {
  u_short src_port;
  u_short dst_port;
  u_int seq_num;
  u_int ack_num;
  u_char data_offset;
  u_char flags;
  u_short window;
  u_short checksum;
  u_short urgent_ptr;
};

// Packet handler with detailed output
void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr,
                    const u_char *packet) {
  printf("\nPacket captured: Length = %d bytes\n", pkthdr->len);
  struct ethernet_header *eth = (struct ethernet_header *)packet;
  printf("Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", eth->src_mac[0],
         eth->src_mac[1], eth->src_mac[2], eth->src_mac[3], eth->src_mac[4],
         eth->src_mac[5]);
  printf("Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", eth->dest_mac[0],
         eth->dest_mac[1], eth->dest_mac[2], eth->dest_mac[3], eth->dest_mac[4],
         eth->dest_mac[5]);

  if (ntohs(eth->ether_type) == 0x0800) {
    struct ip_header *ip = (struct ip_header *)(packet + 14);
    int ip_header_len = (ip->version_ihl & 0x0F) * 4;
    printf("Source IP: %s\n", inet_ntoa(ip->src_ip));
    printf("Destination IP: %s\n", inet_ntoa(ip->dst_ip));
    printf("Protocol: ");
    switch (ip->protocol) {
    case 1:
      printf("ICMP\n");
      break;
    case 6:
      printf("TCP\n");
      break;
    case 17:
      printf("UDP\n");
      break;
    default:
      printf("Unknown (%d)\n", ip->protocol);
      break;
    }
    if (ip->protocol == 6 || ip->protocol == 17) {
      struct tcp_header *tcp =
          (struct tcp_header *)(packet + 14 + ip_header_len);
      printf("Source Port: %d\n", ntohs(tcp->src_port));
      printf("Destination Port: %d\n", ntohs(tcp->dst_port));
    }
  } else {
    printf("Non-IPv4 packet (EtherType: 0x%04x)\n", ntohs(eth->ether_type));
  }
}

int main() {
  pcap_t *handle;
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_if_t *alldevs, *dev;
  int i = 0;

  // Initialize Winsock
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    printf("WSAStartup failed: %d\n", WSAGetLastError());
    return 1;
  }

  // Find all available network devices
  if (pcap_findalldevs(&alldevs, errbuf) == -1) {
    printf("Error finding devices: %s\n", errbuf);
    WSACleanup();
    return 1;
  }

  // Print all devices
  printf("Available devices:\n");
  for (dev = alldevs; dev != NULL; dev = dev->next) {
    printf("%d: %s (%s)\n", ++i, dev->name,
           dev->description ? dev->description : "No description");
  }
  if (i == 0) {
    printf("No devices found.\n");
    pcap_freealldevs(alldevs);
    WSACleanup();
    return 1;
  }

  // Select the Wi-Fi adapter (device 5 in your list)
  dev = alldevs;
  for (int j = 1; j < 5; j++) { // Move to the 5th device (index 1-based)
    if (dev->next)
      dev = dev->next;
  }
  printf("\nUsing device: %s\n", dev->name);

  // Open the device for capturing packets
  handle = pcap_open_live(dev->name, BUFSIZ, 1, 1000, errbuf);
  if (handle == NULL) {
    printf("Error opening device: %s\n", errbuf);
    pcap_freealldevs(alldevs);
    WSACleanup();
    return 1;
  }

  // Start capturing packets
  printf("Starting packet capture... Press Ctrl+C to stop.\n");
  int result = pcap_loop(handle, 0, packet_handler, NULL);
  if (result < 0) {
    printf("Error capturing packets: %s\n", pcap_geterr(handle));
  } else {
    printf("Capture stopped.\n");
  }

  // Clean up
  pcap_close(handle);
  pcap_freealldevs(alldevs);
  WSACleanup();

  return 0;
}