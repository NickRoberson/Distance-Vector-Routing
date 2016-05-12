#ifndef __DVR_H__
#define __DVR_H__

// Change this definition to 1 if you want to enable link changes for the
// advanced assignment.
#define LINKCHANGES 1

// A distance table used within each node
struct distance_table {
  int costs[4][4];
};

// A packet sent from one routing process to another via a call to tolayer2
struct rtpkt {
  int sourceid;   // id of sending router sending this pkt
  int destid;     // id of router to which pkt being sent (must be a neighbor)
  int mincost[4]; // min cost to node 0 ... 3
};

/**
 * Send a packet to another node via layer 2.
 *
 * \param packet  The packet being sent
 */
void tolayer2(struct rtpkt packet);

/**
 * Print a distance table used by a given node.
 *
 * \param from  The id of the node that controls this distance table
 * \param dt    A pointer to the distance table struct
 */
void printdt(int from, struct distance_table* dt);

/**
 * Get the current simulated time.
 *
 * \returns A floating point representation of the current time
 */
float get_time();

#endif
