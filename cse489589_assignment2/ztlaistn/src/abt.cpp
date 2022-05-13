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
bool waiting;
int packet_number;
int seq_a;
int seq_b;
int compute_checksum(struct pkt packet);
void change_sequence(int AorB);


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    struct pkt *packet = new pkt;
    int i;
    for (i=0; i<20; i++){
        packet->payload[i] = message.data[i];
    }
    packet->seqnum = packet_number;
    packet->checksum = compute_checksum(*packet);
    if (packet_number == 0){
        packet_number = 1;
    } else {
        packet_number = 0;
    }
    buffer[num_messages] = *packet;
    num_messages++;
    if (!waiting){
        waiting = true;
        tolayer3(0, buffer[current_message]);
        starttimer(0, 50.0);
        printf("sending new message from A\n");
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    waiting = false;
    if ((compute_checksum(packet) == packet.checksum) && (packet.acknum == seq_a)) {
        stoptimer(0);
        change_sequence(0);
        current_message++;
    } else {
        printf("A received a corrupted message\n");
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    printf("A did not receive a response. Sending again\n");
    starttimer(0, 50.0);
    waiting = true;
    tolayer3(0, buffer[current_message]);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    buffer = new pkt[1000];
    num_messages = 0;
    current_message = 0;
    waiting = false;
    packet_number = 0;
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
        change_sequence(1);
        tolayer3(1, *ack);
    } else {
        printf("B received an incorrect packet\n");
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

void change_sequence(int AorB) {
    if (AorB == 0){
        if (seq_a == 0){
            seq_a = 1;
        } else {
            seq_a = 0;
        }
    }
    if (AorB == 1){
        if (seq_b == 0){
            seq_b = 1;
        } else {
            seq_b = 0;
        }
    }
}