/*
 * LoRaAODV.h
 *
 *  Created on: Mar 16, 2020
 *      Author: ns3
 */

#ifndef LORAAODV_LORAAODV_H_
#define LORAAODV_LORAAODV_H_

#include <string.h>
#include <omnetpp.h>
#include "misc/DevAddr.h"
#include <vector>
#include <map>

namespace inet {


using namespace std;

typedef struct routeTableEntry{
    DevAddr destinationAddress;
    DevAddr nextHop;
    //int sequenceNumber;
    int hopCount;
} routeTableEntry;

class LoRaAODV {
private:
    vector<routeTableEntry> routingTable;
public:
    void addEntryToRouteTable(DevAddr dest,DevAddr nextHop, int hopC);
    void printRouteTable();
    bool isItRecorded(DevAddr dest);
    bool isRouteAdded(DevAddr dest, DevAddr nextHop, int hopC);
    DevAddr findRoute(DevAddr dest);
};


}



#endif /* LORAAODV_LORAAODV_H_ */

