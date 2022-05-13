#include <cstdio>
#include "../include/simulator.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

struct pkt *buffer;
int num_messages;
int current_message;
int packet_number;
int window_size;
int next_seq_a_number;
int seq_a;
int seq_b;
int window_a_start;
int compute_checksum(struct pkt packet);


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    struct pkt *packet = new pkt;
    packet->seqnum = packet_number;
    int i;
    for (i=0; i<20; i++){
        packet->payload[i] = message.data[i];
    }
    packet_number++;
    packet->checksum = compute_checksum(*packet);
    buffer[num_messages] = *packet;
    num_messages++;
    if ((next_seq_a_number < (window_a_start + window_size))){
        if (next_seq_a_number == window_a_start){
            starttimer(0, 250.0);
        }
        printf("sending new message from A\n");
        tolayer3(0, buffer[next_seq_a_number]);
        next_seq_a_number++;
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if ((compute_checksum(packet) == packet.checksum) && (packet.acknum == seq_a)) {
        if (seq_a == window_a_start){
            stoptimer(0);
        }
        seq_a++;
        current_message++;
    } else {
        printf("A received a corrupted message. Sending again\n");
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    starttimer(0, 250.0);
    int i;
    for (i=(window_a_start); i < (next_seq_a_number - 1); i++) {
        tolayer3(0, buffer[i]);
    }
    printf("A did not receive a response. Sending again\n");
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    buffer = new pkt[1000];
    window_size = getwinsize();
    num_messages = 0;
    current_message = 0;
    packet_number = 0;
    next_seq_a_number = 0;
    window_a_start = 0;
    seq_a = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if ((compute_checksum(packet) == packet.checksum) && (packet.seqnum == seq_b)) {
        printf("B received a good message. Sending ACK\n");
        tolayer5(1, packet.payload);
        struct pkt *ack = new pkt;
        ack->seqnum = packet.seqnum;
        ack->acknum = packet.seqnum;
        ack->checksum = compute_checksum(*ack);
        seq_b++;
        tolayer3(1, *ack);
    } else {
        printf("B received a corrupted packet\n");
        struct pkt *ack = new pkt;
        ack->seqnum = seq_b;
        ack->acknum = seq_b;
        ack->checksum = compute_checksum(*ack);
        tolayer3(1, *ack);
    }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    seq_b = 0;
}

int compute_checksum(struct pkt packet){
    int i;
    int generated_checksum = 0;
    for (i=0; i<sizeof(packet.payload); i++) {
        generated_checksum += int(packet.payload[i]);
    }
    generated_checksum += packet.seqnum;
    generated_checksum += packet.acknum;
    return generated_checksum;
}