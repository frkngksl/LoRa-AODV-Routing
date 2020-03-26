/*
 * LoRaAODV.cc
 *
 *  Created on: Mar 16, 2020
 *      Author: ns3
 */

#include "LoRaAODV.h"

namespace inet{



void LoRaAODV::addEntryToRouteTable(DevAddr dest, DevAddr nextHop, int hopC){
        routeTableEntry entryTemp;
        entryTemp.destinationAddress = dest;
        entryTemp.nextHop = nextHop;
        //entryTemp.sequenceNumber = seq;
        entryTemp.hopCount = hopC;
        routingTable.push_back(entryTemp);
    };

void LoRaAODV::printRouteTable(){
        for(unsigned int i=0;i<routingTable.size();i++){
            EV << "For Destination: " << routingTable[i].destinationAddress << "Send To: " << routingTable[i].nextHop << "\n";
        }
    };

bool LoRaAODV::isRouteAdded(DevAddr dest, DevAddr nextHop, int hopC){
    for(unsigned int i=0;i<routingTable.size();i++){
            if(routingTable[i].destinationAddress == dest && routingTable[i].nextHop == nextHop && routingTable[i].hopCount == hopC){
                return true;
            }
        }
    return false;
}

bool LoRaAODV::isItRecorded(DevAddr dest){
    for(unsigned int i=0;i<routingTable.size();i++){
        if(routingTable[i].destinationAddress == dest){
            return true;
        }
    }
    //EV << "Don't Found \n";
    return false;
};

DevAddr LoRaAODV::findRoute(DevAddr dest){
    int minCount=1238213823;
    DevAddr nextHop;
    for(unsigned int i=0;i<routingTable.size();i++){
        if(routingTable[i].destinationAddress == dest && routingTable[i].hopCount < minCount){
            nextHop = routingTable[i].nextHop;
        }
    }
    return nextHop;
}
}

