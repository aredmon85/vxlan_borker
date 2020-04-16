#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#define SERVER "200.99.0.130"
int main() {
	char *datagram;
	struct sockaddr_in si_other;
	int s, i, slen=sizeof(si_other);
	struct vxlan_hdr
	{
		uint16_t flags;
		uint16_t policy;
		uint8_t vni[3];
		uint8_t reserved;
	};
	struct inner_eth
	{
		uint8_t dst_addr[6];
		uint8_t src_addr[6];
		uint16_t ether_type;
	};		
	struct __attribute__((__packed__)) arp_frame 
	{
		uint16_t htype;
		uint16_t ptype;
		uint8_t hlen;
		uint8_t plen;
		uint16_t oper;
		uint8_t sender_hw_addr[6];
		uint32_t sender_proto_addr;
		uint8_t target_hw_addr[6];
		uint32_t target_proto_addr;
	};
	datagram = malloc(sizeof(struct vxlan_hdr) + sizeof(struct inner_eth) + sizeof(struct arp_frame));
	memset(datagram,0,(sizeof(struct vxlan_hdr) + sizeof(struct inner_eth) + sizeof(struct arp_frame)));	
	struct vxlan_hdr *vx_hdr = (struct vxlan_hdr *) datagram;    
	vx_hdr->flags = htons(2048);
	vx_hdr->policy = 0;
	vx_hdr->vni[0] = 0;
	vx_hdr->vni[1] = 0;
	vx_hdr->vni[2] = 100;
	vx_hdr->reserved = 0;
	
	struct inner_eth *in_eth = (struct inner_eth *)(datagram + sizeof(struct vxlan_hdr));
	in_eth->src_addr[0] = 0x00;
	in_eth->src_addr[1] = 0x00;
	in_eth->src_addr[2] = 0x12;
	in_eth->src_addr[3] = 0x34;
	in_eth->src_addr[4] = 0x56;
	in_eth->src_addr[5] = 0x78;
	in_eth->dst_addr[0] = 0xff;
	in_eth->dst_addr[1] = 0xff;
	in_eth->dst_addr[2] = 0xff;
	in_eth->dst_addr[3] = 0xff;
	in_eth->dst_addr[4] = 0xff;
	in_eth->dst_addr[5] = 0xff;
	in_eth->ether_type = htons(2054);

	printf("Size of inner_eth header: %ld\n",sizeof(struct inner_eth));
	struct arp_frame *arp_frm = (struct arp_frame *)(datagram + sizeof(struct vxlan_hdr) + sizeof(struct inner_eth));
	arp_frm->htype = htons(1);
	arp_frm->ptype = htons(2048);
	arp_frm->hlen = 6;
	arp_frm->plen = 4;
	/* 1 for request, 2 for reply, everything else to bork */
	arp_frm->oper = htons(4);
        for(int i=0;i<6;i++)
	{
		arp_frm->sender_hw_addr[i] = in_eth->src_addr[i];
	}
	arp_frm->sender_proto_addr = inet_addr("10.100.0.31");
	printf("Value of sender_proto_addr: %"PRIu32"\n",arp_frm->sender_proto_addr);
	arp_frm->target_hw_addr[0] = 0;
        arp_frm->target_hw_addr[1] = 0;
        arp_frm->target_hw_addr[2] = 0;
        arp_frm->target_hw_addr[3] = 0;
        arp_frm->target_hw_addr[4] = 0;
        arp_frm->target_hw_addr[5] = 0;
	arp_frm->target_proto_addr = inet_addr("10.100.0.31");
	uint16_t packet_size = sizeof(struct vxlan_hdr) + sizeof(struct inner_eth) + sizeof(struct arp_frame);
	if((s=socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("failed to create socket");
		exit(1);
	}
	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	int port = 4789;
	si_other.sin_port = htons(port);
	if (inet_aton(SERVER , &si_other.sin_addr) == 0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	int ttl = 63;
	if(setsockopt(s, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
		perror("Failed to set socket options");
		exit(1);
	}
	puts("Sending packet");
	for(int i = 0; i<10; i++) 
	{
		sendto(s, datagram, packet_size , 0 , (struct sockaddr *) &si_other, slen);
	}
	close(s);
	return 0;
}
