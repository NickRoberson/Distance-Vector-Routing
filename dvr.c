#include "dvr.h"

#include <stdio.h>
#include <stdlib.h>

// Event types
#define FROM_LAYER2 2
#define LINK_CHANGE 10
#define INFINITY 9999

// Declarations for external functions in each node's implementation file
extern void rtinit0();
extern void rtinit1();
extern void rtinit2();
extern void rtinit3();
extern void rtupdate0(struct rtpkt *rcvdpkt);
extern void rtupdate1(struct rtpkt *rcvdpkt);
extern void rtupdate2(struct rtpkt *rcvdpkt);
extern void rtupdate3(struct rtpkt *rcvdpkt);
extern void linkhandler0(int linkid, int newcost);
extern void linkhandler1(int linkid, int newcost);

// A node in the event list
struct event {
  float evtime;           /* event time */
  int evtype;             /* event type code */
  int eventity;           /* entity where event occurs */
  struct rtpkt *rtpktptr; /* ptr to packet (if any) assoc w/ this event */
  struct event *prev;
  struct event *next;
};

void init();
void insertevent(struct event *p);
float jimsrand();

// Trace level to use
int TRACE = 1;

// The event list
struct event *evlist = NULL;

// The current simulated time
float clocktime = 0.000;

/**
 * Print a distance table for a given node. This should really use loops, but
 * this is still preferable to separate duplicated code in each node*.c file.
 *
 * \param from  The id of the node whose distance table is being printed
 * \param dt    A pointer to the distance table
 */
void printdt(int from, struct distance_table* dt) {
  if(from == 0) {
    printf("                via     \n");
    printf("   D0 |    1     2    3 \n");
    printf("  ----|-----------------\n");
    printf("     1|  %3d   %3d   %3d\n", dt->costs[1][1],
                                         dt->costs[1][2],
                                         dt->costs[1][3]);
    printf("dest 2|  %3d   %3d   %3d\n", dt->costs[2][1],
                                         dt->costs[2][2],
                                         dt->costs[2][3]);
    printf("     3|  %3d   %3d   %3d\n", dt->costs[3][1],
                                         dt->costs[3][2],
                                         dt->costs[3][3]);

  } else if(from == 1) {
    printf("             via   \n");
    printf("   D1 |    0     2 \n");
    printf("  ----|-----------\n");
    printf("     0|  %3d   %3d\n", dt->costs[0][0], dt->costs[0][2]);
    printf("dest 2|  %3d   %3d\n", dt->costs[2][0], dt->costs[2][2]);
    printf("     3|  %3d   %3d\n", dt->costs[3][0], dt->costs[3][2]);

  } else if(from == 2) {
    printf("                via     \n");
    printf("   D2 |    0     1    3 \n");
    printf("  ----|-----------------\n");
    printf("     0|  %3d   %3d   %3d\n", dt->costs[0][0],
                                         dt->costs[0][1],
                                         dt->costs[0][3]);
    printf("dest 1|  %3d   %3d   %3d\n", dt->costs[1][0],
                                         dt->costs[1][1],
                                         dt->costs[1][3]);
    printf("     3|  %3d   %3d   %3d\n", dt->costs[3][0],
                                         dt->costs[3][1],
                                         dt->costs[3][3]);

  } else if(from == 3) {
    printf("             via     \n");
    printf("   D3 |    0     2 \n");
    printf("  ----|-----------\n");
    printf("     0|  %3d   %3d\n", dt->costs[0][0], dt->costs[0][2]);
    printf("dest 1|  %3d   %3d\n", dt->costs[1][0], dt->costs[1][2]);
    printf("     2|  %3d   %3d\n", dt->costs[2][0], dt->costs[2][2]);

  } else {
    fprintf(stderr, "Invalid ID from=%d\n", from);
    abort();
  }
}

/**
 * Get the current simulated time
 *
 * \returns A floating point representation of the current time
 */
float get_time() {
  return clocktime;
}

void creatertpkt(struct rtpkt *initrtpkt, int srcid, int destid,
                 int *mincosts) {
  int i;
  initrtpkt->sourceid = srcid;
  initrtpkt->destid = destid;
  for (i = 0; i < 4; i++) {
    initrtpkt->mincost[i] = mincosts[i];
  }
}

int main() {
  struct event *eventptr;

  init();

  while (1) {
    eventptr = evlist; /* get next event to simulate */

    if (eventptr == NULL) {
      printf("\nSimulator terminated at t=%f, no packets in medium\n", clocktime);
      return 0;
    }

    evlist = evlist->next; /* remove this event from event list */

    if (evlist != NULL) {
      evlist->prev = NULL;
    }

    if (TRACE > 1) {
      printf("MAIN: rcv event, t=%.3f, at %d", eventptr->evtime,
             eventptr->eventity);
      if (eventptr->evtype == FROM_LAYER2) {
        printf(" src:%2d,", eventptr->rtpktptr->sourceid);
        printf(" dest:%2d,", eventptr->rtpktptr->destid);
        printf(" contents: %3d %3d %3d %3d\n", eventptr->rtpktptr->mincost[0],
               eventptr->rtpktptr->mincost[1], eventptr->rtpktptr->mincost[2],
               eventptr->rtpktptr->mincost[3]);
      }
    }
    clocktime = eventptr->evtime; /* update time to next event time */
    if (eventptr->evtype == FROM_LAYER2) {
      if (eventptr->eventity == 0) {
        rtupdate0(eventptr->rtpktptr);
      } else if (eventptr->eventity == 1) {
        rtupdate1(eventptr->rtpktptr);
      } else if (eventptr->eventity == 2) {
        rtupdate2(eventptr->rtpktptr);
      } else if (eventptr->eventity == 3) {
        rtupdate3(eventptr->rtpktptr);
      } else {
        printf("Panic: unknown event entity\n");
        exit(0);
      }
    } else if (eventptr->evtype == LINK_CHANGE) {
      if (clocktime < 10001.0) {
        linkhandler0(1, 20);
        linkhandler1(0, 20);
      } else {
        linkhandler0(1, 1);
        linkhandler1(0, 1);
      }
    } else {
      printf("Panic: unknown event type\n");
      exit(0);
    }
    if (eventptr->evtype == FROM_LAYER2) {
      free(eventptr->rtpktptr); /* free memory for packet, if any */
    }
    free(eventptr);             /* free memory for event struct   */
  }
}

/**
 * Initialize the simulator
 */
void init() {
  int i;
  float sum, avg;
  struct event *evptr;

  printf("Enter TRACE:");
  scanf("%d", &TRACE);

  srand(9999); /* init random number generator */
  sum = 0.0;   /* test random number generator for students */

  for (i = 0; i < 1000; i++) {
    sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
  }

  avg = sum / 1000.0;
  if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n");
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0); /* Updated for gcc. JJW 1/14/14 */
  }

  clocktime = 0.0; /* initialize time to 0.0 */
  rtinit0();
  rtinit1();
  rtinit2();
  rtinit3();

  /* initialize future link changes */
  if (LINKCHANGES == 1) {
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtime = 10000.0;
    evptr->evtype = LINK_CHANGE;
    evptr->eventity = -1;
    evptr->rtpktptr = NULL;
    insertevent(evptr);
    evptr = (struct event *)malloc(sizeof(struct event));
    evptr->evtype = LINK_CHANGE;
    evptr->evtime = 20000.0;
    evptr->rtpktptr = NULL;
    insertevent(evptr);
  }
}

/**
 * Generate a random number in [0, 1]
 *
 * \returns A pseudorandom number in [0, 1]
 */
float jimsrand() {
  float x = rand() / ((float)RAND_MAX);
  return x;
}

/**
 * Add an event to the event list
 *
 * \param event The event to add
 */
void insertevent(struct event *p) {
  struct event *q, *qold;

  if (TRACE > 3) {
    printf("            INSERTEVENT: time is %lf\n", clocktime);
    printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
  }
  q = evlist;      /* q points to header of list in which p struct inserted */
  if (q == NULL) { /* list is empty */
    evlist = p;
    p->next = NULL;
    p->prev = NULL;
  } else {
    for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next) {
      qold = q;
    }
    if (q == NULL) { /* end of list */
      qold->next = p;
      p->prev = qold;
      p->next = NULL;
    } else if (q == evlist) { /* front of list */
      p->next = evlist;
      p->prev = NULL;
      p->next->prev = p;
      evlist = p;
    } else { /* middle of list */
      p->next = q;
      p->prev = q->prev;
      q->prev->next = p;
      q->prev = p;
    }
  }
}

/**
 * Print the current event list.
 */
void printevlist() {
  struct event *q;
  printf("--------------\nEvent List Follows:\n");
  for (q = evlist; q != NULL; q = q->next) {
    printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype,
           q->eventity);
  }
  printf("--------------\n");
}

/**
 * Send a packet to a node via layer 2
 *
 * \param packet  The packet being sent
 */
void tolayer2(struct rtpkt packet) {
  struct rtpkt *mypktptr;
  struct event *evptr, *q;
  float lastime;
  int i;

  int connectcosts[4][4];

  /* initialize by hand since not all compilers allow array initilization */
  connectcosts[0][0] = 0;
  connectcosts[0][1] = 1;
  connectcosts[0][2] = 3;
  connectcosts[0][3] = 7;
  connectcosts[1][0] = 1;
  connectcosts[1][1] = 0;
  connectcosts[1][2] = 1;
  connectcosts[1][3] = 999;
  connectcosts[2][0] = 3;
  connectcosts[2][1] = 1;
  connectcosts[2][2] = 0;
  connectcosts[2][3] = 2;
  connectcosts[3][0] = 7;
  connectcosts[3][1] = 999;
  connectcosts[3][2] = 2;
  connectcosts[3][3] = 0;

  /* be nice: check if source and destination id's are reasonable */
  if (packet.sourceid < 0 || packet.sourceid > 3) {
    printf("WARNING: illegal source id in your packet, ignoring packet!\n");
    return;
  }
  if (packet.destid < 0 || packet.destid > 3) {
    printf("WARNING: illegal dest id in your packet, ignoring packet!\n");
    return;
  }
  if (packet.sourceid == packet.destid) {
    printf("WARNING: source and destination id's the same, ignoring packet!\n");
    return;
  }
  if (connectcosts[packet.sourceid][packet.destid] == 999) {
    printf("WARNING: source and destination not connected, ignoring packet!\n");
    return;
  }

  /* make a copy of the packet student just gave me since he/she may decide */
  /* to do something with the packet after we return back to him/her */
  mypktptr = (struct rtpkt *)malloc(sizeof(struct rtpkt));
  mypktptr->sourceid = packet.sourceid;
  mypktptr->destid = packet.destid;

  for (i = 0; i < 4; i++) {
    mypktptr->mincost[i] = packet.mincost[i];
  }

  if (TRACE > 2) {
    printf("    TOLAYER2: source: %d, dest: %d\n              costs:",
           mypktptr->sourceid, mypktptr->destid);
    for (i = 0; i < 4; i++) printf("%d  ", mypktptr->mincost[i]);
    printf("\n");
  }

  /* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype = FROM_LAYER2;     /* packet will pop out from layer3 */
  evptr->eventity = packet.destid; /* event occurs at other entity */
  evptr->rtpktptr = mypktptr;      /* save ptr to my copy of packet */

  /* finally, compute the arrival time of packet at the other end.
     medium can not reorder, so make sure packet arrives between 1 and 10
     time units after the latest arrival time of packets
     currently in the medium on their way to the destination */
  lastime = clocktime;
  for (q = evlist; q != NULL; q = q->next) {
    if ((q->evtype == FROM_LAYER2 && q->eventity == evptr->eventity)) {
      lastime = q->evtime;
    }
  }
  evptr->evtime = lastime + 2. * jimsrand();

  if (TRACE > 2) {
    printf("    TOLAYER2: scheduling arrival on other side\n");
  }

  insertevent(evptr);
}

struct distance_table dt0, dt1, dt2, dt3;
struct rtpkt packet;
extern void rtinit0(){
	//Init min cost
	int i,j;
	for(i = 0; i< 4; i++){
		packet.mincost[i] = INFINITY;
		for(j = 0; j<4;j++){
			dt0.costs[i][j] = INFINITY;
		}
	}
	//save the distance
	dt0.costs[1][1] = 1;
	dt0.costs[2][2] = 3;
	dt0.costs[3][3] = 7;
	//find and save the minmum costs if they exist
	for(i = 0; i< 4; i++){
		for(j = 0; j<4;j++){
			if(packet.mincost[i] > dt0.costs[i][j]){
				packet.mincost[i] = dt0.costs[i][j];
			}
		}
	}
	packet.sourceid = 0;
	packet.destid = 1;
	tolayer2(packet);

	packet.sourceid = 0;
	packet.destid = 2;
	tolayer2(packet);

	packet.sourceid = 0;
	packet.destid = 3;
	tolayer2(packet);
}
extern void rtinit1(){
	//Init min cost
	int i,j;
	for(i = 0; i< 4; i++){
		packet.mincost[i] = INFINITY;
		for(j = 0; j<4;j++){
			dt1.costs[i][j] = INFINITY;
		}
	}
	//save the distance
	dt1.costs[0][0] = 1;
	dt1.costs[2][2] = 1;
	//find and save the minmum costs if they exist
	for(i = 0; i< 4; i++){
		for(j = 0; j<4;j++){
			if(packet.mincost[i] > dt1.costs[i][j]){
				packet.mincost[i] = dt1.costs[i][j];
			}
		}
	}
	packet.sourceid = 1;
	packet.destid = 0;
	tolayer2(packet);

	packet.sourceid = 1;
	packet.destid = 2;
	tolayer2(packet);
}
extern void rtinit2(){
	//Init min cost
	int i,j;
	for(i = 0; i< 4; i++){
		packet.mincost[i] = INFINITY;
		for(j = 0; j<4;j++){
			dt2.costs[i][j] = INFINITY;
		}
	}
	//save the distance
	dt2.costs[0][0] = 3;
	dt2.costs[1][1] = 1;
	dt2.costs[3][3] = 2;
	//find and save the minmum costs if they exist
	for(i = 0; i< 4; i++){
		for(j = 0; j<4;j++){
			if(packet.mincost[i] > dt2.costs[i][j]){
				packet.mincost[i] = dt2.costs[i][j];
			}
		}
	}
	packet.sourceid = 2;
	packet.destid = 0;
	tolayer2(packet);

	packet.sourceid = 2;
	packet.destid = 1;
	tolayer2(packet);

	packet.sourceid = 2;
	packet.destid = 3;
	tolayer2(packet);
}
extern void rtinit3(){
	//Init min cost
	int i,j;
	for(i = 0; i< 4; i++){
		packet.mincost[i] = INFINITY;
		for(j = 0; j<4;j++){
			dt3.costs[i][j] = INFINITY;
		}
	}
	//save the distance
	dt3.costs[0][0] = 7;
	dt3.costs[2][2] = 2;
	//find and save the minmum costs if they exist
	for(i = 0; i< 4; i++){
		for(j = 0; j<4;j++){
			if(packet.mincost[i] > dt3.costs[i][j]){
				packet.mincost[i] = dt3.costs[i][j];
			}
		}
	}
	packet.sourceid = 3;
	packet.destid = 0;
	tolayer2(packet);

	packet.sourceid = 3;
	packet.destid = 2;
	tolayer2(packet);
}
extern void rtupdate0(struct rtpkt *rcvdpkt){
	int needUpdate = 0;
	int i,j;
	for(i = 0; i< 4; i++){
	packet.mincost[i] = INFINITY;
	}
	for(i = 0; i< 4; ++i){
		if(rcvdpkt->mincost[i] + dt0.costs[rcvdpkt->sourceid][rcvdpkt->sourceid] < dt0.costs[i][rcvdpkt->sourceid]){
			needUpdate = 1;
			dt0.costs[i][rcvdpkt->sourceid] = rcvdpkt->mincost[i] + dt0.costs[rcvdpkt->sourceid][rcvdpkt->sourceid];
		}
	}
	if(needUpdate){
		for(i = 0; i< 4; i++){
			for(j = 0; j<4 ; j++){
				if(packet.mincost[i]>dt0.costs[i][j]){
					packet.mincost[i]=dt0.costs[i][j];
				}
			}
		}
		packet.sourceid = 0;
		packet.destid = 1;
		tolayer2(packet);

		packet.sourceid = 0;
		packet.destid = 2;
		tolayer2(packet);

		packet.sourceid = 0;
		packet.destid = 3;
		tolayer2(packet);
	}
	printdt(0, &dt0);
}
extern void rtupdate1(struct rtpkt *rcvdpkt){
	int needUpdate = 0;
	int i,j;
	for(i = 0; i< 4; i++){
	packet.mincost[i] = INFINITY;
	}
	for(i = 0; i< 4; ++i){
		if(rcvdpkt->mincost[i] + dt1.costs[rcvdpkt->sourceid][rcvdpkt->sourceid] < dt1.costs[i][rcvdpkt->sourceid]){
			needUpdate = 1;
			dt1.costs[i][rcvdpkt->sourceid] = rcvdpkt->mincost[i] + dt1.costs[rcvdpkt->sourceid][rcvdpkt->sourceid];
		}
	}
	if(needUpdate){
		for(i = 0; i< 4; i++){
			for(j = 0; j<4 ; j++){
				if(packet.mincost[i]>dt1.costs[i][j]){
					packet.mincost[i]=dt1.costs[i][j];
				}
			}
		}
		packet.sourceid = 1;
		packet.destid = 0;
		tolayer2(packet);

		packet.sourceid = 1;
		packet.destid = 2;
		tolayer2(packet);

		packet.sourceid = 0;
		packet.destid = 3;
		tolayer2(packet);
	}
	printdt(1, &dt1);

}
extern void rtupdate2(struct rtpkt *rcvdpkt){
	int needUpdate = 0;
	int i,j;
	for(i = 0; i< 4; i++){
	packet.mincost[i] = INFINITY;
	}
	for(i = 0; i< 4; ++i){
		if(rcvdpkt->mincost[i] + dt2.costs[rcvdpkt->sourceid][rcvdpkt->sourceid] < dt2.costs[i][rcvdpkt->sourceid]){
			needUpdate = 1;
			dt2.costs[i][rcvdpkt->sourceid] = rcvdpkt->mincost[i] + dt2.costs[rcvdpkt->sourceid][rcvdpkt->sourceid];
		}
	}
	if(needUpdate){
		for(i = 0; i< 4; i++){
			for(j = 0; j<4 ; j++){
				if(packet.mincost[i]>dt2.costs[i][j]){
					packet.mincost[i]=dt2.costs[i][j];
				}
			}
		}
		packet.sourceid = 2;
		packet.destid = 0;
		tolayer2(packet);

		packet.sourceid = 2;
		packet.destid = 1;
		tolayer2(packet);

		packet.sourceid = 2;
		packet.destid = 3;
		tolayer2(packet);
	}
	printdt(2, &dt2);
}
extern void rtupdate3(struct rtpkt *rcvdpkt){
	int needUpdate = 0;
	int i,j;
	for(i = 0; i< 4; i++){
	packet.mincost[i] = INFINITY;
	}
	for(i = 0; i< 4; ++i){
		if(rcvdpkt->mincost[i] + dt3.costs[rcvdpkt->sourceid][rcvdpkt->sourceid] < dt3.costs[i][rcvdpkt->sourceid]){
			needUpdate = 1;
			dt3.costs[i][rcvdpkt->sourceid] = rcvdpkt->mincost[i] + dt3.costs[rcvdpkt->sourceid][rcvdpkt->sourceid];
		}
	}
	if(needUpdate){
		for(i = 0; i< 4; i++){
			for(j = 0; j<4 ; j++){
				if(packet.mincost[i]>dt3.costs[i][j]){
					packet.mincost[i]=dt3.costs[i][j];
				}
			}
		}
		packet.sourceid = 3;
		packet.destid = 0;
		tolayer2(packet);

		packet.sourceid = 3;
		packet.destid = 2;
		tolayer2(packet);
	}
	printdt(3, &dt3);
}
extern void linkhandler0(int linkid, int newcost){
	dt0.costs[0][linkid] = newcost;
	packet.sourceid = 0;
	packet.destid = 1;
	packet.mincost[linkid] = newcost;
	tolayer2(packet);

	packet.sourceid = 0;
	packet.destid = 2;
	packet.mincost[linkid] = newcost;
	tolayer2(packet);

	packet.sourceid = 0;
	packet.destid = 3;
	packet.mincost[linkid] = newcost;
	tolayer2(packet);

}
extern void linkhandler1(int linkid, int newcost){
	dt1.costs[1][linkid] = newcost;

	packet.sourceid = 1;
	packet.destid = 1;
	packet.mincost[linkid] = newcost;
	tolayer2(packet);

	packet.sourceid = 1;
	packet.destid = 2;
	packet.mincost[linkid] = newcost;
	tolayer2(packet);
}
