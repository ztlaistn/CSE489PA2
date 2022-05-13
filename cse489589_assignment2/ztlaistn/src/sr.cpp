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
struct logical_timer{
    struct pkt packet;
    double time_remaining;
    bool timer_active;
};

struct pkt *buffer;
int num_messages;
int packet_number;
int window_size;
int next_seq_a_number;
int successful_messages_in_window_a;
int successful_messages_in_window_b;
struct pkt *received_message_buffer;
int *received_message_buffer_indices;
int received_message_index;
struct logical_timer *timer_buffer;
int *timer_buffer_indices;
int timer_buffer_index;
int seq_b;
int window_a_start;
int window_b_start;
int b_window_seq;
int compute_checksum(struct pkt packet);


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    //create packet from message
    struct pkt *packet = new pkt;
    packet->seqnum = packet_number;
    int i;
    for (i=0; i<20; i++){
        packet->payload[i] = message.data[i];
    }
    packet_number++;
    packet->checksum = compute_checksum(*packet);

    //add packet to buffer
    buffer[num_messages] = *packet;
    num_messages++;

    //start logical timer for packet and send if window not full
    if ((next_seq_a_number < (window_a_start + window_size))){
        printf("sending new message from A\n");

        timer_buffer[timer_buffer_index].packet = buffer[next_seq_a_number];
        timer_buffer[timer_buffer_index].time_remaining = 250.0;
        timer_buffer[timer_buffer_index].timer_active = true;
        timer_buffer_indices[timer_buffer_index] = packet->seqnum;
        tolayer3(0, timer_buffer[timer_buffer_index].packet);
        next_seq_a_number++;
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if ((compute_checksum(packet) == packet.checksum) && (packet.acknum >= window_a_start) && (packet.acknum < (window_a_start + window_size))) {
        //disable timer
        int i;
        for (i=0; i <window_size; i++) {
            if (timer_buffer_indices[i] == packet.acknum) {
                timer_buffer[i].time_remaining = 0.0;
                timer_buffer[i].timer_active = false;
            }
        }

        //count messages successfully received in the window, shift window if all successfully received
        if (successful_messages_in_window_a < window_size){
            successful_messages_in_window_a++;
        } else {
            successful_messages_in_window_a = 0;
            timer_buffer_index = 0;
            window_a_start = window_a_start + window_size;
        }

    } else {
        printf("A received a corrupted message\n");
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
    int i;
    for (i=0; i <window_size; i++) {
        //check if logical timer for packet is active
        if (timer_buffer[i].timer_active) {
            //check if time is remaining
            if (timer_buffer[i].time_remaining > 0.0){
                //decrease time left
                timer_buffer[i].time_remaining = timer_buffer[i].time_remaining - 5.0;
            } else {
                //restart logical timer
                timer_buffer[i].time_remaining = 250.0;
                printf("A did not receive a response. Sending again\n");
                tolayer3(0, timer_buffer[i].packet);
            }
        }
    }
    //restart hardware timer
    starttimer(0, 5.0);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    buffer = new pkt[1000];
    window_size = getwinsize();
    timer_buffer = new logical_timer[window_size];
    timer_buffer_indices = new int[window_size];
    num_messages = 0;
    packet_number = 0;
    next_seq_a_number = 0;
    window_a_start = 0;
    successful_messages_in_window_a = 0;
    timer_buffer_index = 0;
    starttimer(0, 1.0);
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    //check if packet was not already sent
    printf("%s\n", packet.payload);
    int i;
    bool packet_already_present = false;
    for (i = 0; i < window_size; i++) {
        if (received_message_buffer[i].seqnum == packet.seqnum) {
            packet_already_present = true;
        }
    }

    //check if packet is not corrupted and sequence number is within window
    if ((compute_checksum(packet) == packet.checksum) && (packet.seqnum >= window_b_start) && (packet.seqnum < (window_b_start + window_size - 1)) && !packet_already_present) {
        printf("B received a good message. Sending ACK\n");

        //add packet to received message buffer
        successful_messages_in_window_b++;
        received_message_buffer[received_message_index] = packet;
        received_message_buffer_indices[received_message_index] = packet.seqnum;
        received_message_index++;
        if (received_message_index == window_size){

            successful_messages_in_window_b = 0;

            //print packet numbers (for debug purposes)
            int j;
            for (i = 0; i < received_message_index; i++) {
                printf("%s\n", received_message_buffer[i].payload);
            }

            //reorder packets
            for (i = 0; i < (received_message_index - 1); i++) {
                for (j = 0; j < (received_message_index - i - 1); j++) {
                    if (received_message_buffer[j].seqnum > received_message_buffer[j + 1].seqnum) {
                        struct pkt *temp = new pkt;
                        *temp = received_message_buffer[j];
                        received_message_buffer[j] = received_message_buffer[j + 1];
                        received_message_buffer[j + 1] = *temp;
                    }
                }
            }
            for (i = 0; i < received_message_index; i++) {
                //printf("%s\n", received_message_buffer[i].payload);
                tolayer5(1, received_message_buffer[i].payload);
            }
            received_message_index = 0;
        }

        if (b_window_seq == (window_b_start + window_size)) {
            window_b_start = window_b_start + window_size;
        }
        b_window_seq++;

        //create ACK packet
        struct pkt *ack = new pkt;
        ack->seqnum = packet.seqnum;
        ack->acknum = packet.seqnum;
        ack->checksum = compute_checksum(*ack);
        //shift window if full, sending ordered messages to B's application layer


        //send ACK
        tolayer3(1, *ack);
    }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    window_size = getwinsize();
    seq_b = 0;
    successful_messages_in_window_b = 0;
    received_message_buffer = new pkt[window_size];
    received_message_buffer_indices = new int[window_size];
    received_message_index = 0;
    window_b_start = 0;
    b_window_seq = 0;
    int k;
    for (k = 0; k < window_size; k++) {
        received_message_buffer[k].seqnum = -1;
    }
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