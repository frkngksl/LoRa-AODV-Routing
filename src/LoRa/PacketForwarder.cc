//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "PacketForwarder.h"
#include "inet/networklayer/ipv4/IPv4Datagram.h"
#include "inet/networklayer/contract/ipv4/IPv4ControlInfo.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/base/ApplicationPacket_m.h"
#include "LoRa/LoRaMacFrame_m.h"
namespace inet {

Define_Module(PacketForwarder);


void PacketForwarder::initialize(int stage)
{

    if (stage == 0) {
        LoRa_GWPacketReceived = registerSignal("LoRa_GWPacketReceived");
        localPort = par("localPort");
        destPort = par("destPort");
        for(unsigned int i= 0;i<20;i++){
            sourceSeqArray[i] = 0;
            broadcastIDArray[i] = 0;
        }
        //routingModule.addEntryToRouteTable(DevAddr::BROADCAST_ADDRESS,DevAddr::IKINCI,1,2);
    } else if (stage == INITSTAGE_APPLICATION_LAYER) {
        startUDP();
        getSimulation()->getSystemModule()->subscribe("LoRa_AppPacketSent", this);
    }

}


void PacketForwarder::startUDP()
{
    socket.setOutputGate(gate("udpOut"));
    const char *localAddress = par("localAddress");
    socket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
    // TODO: is this required?
    //setSocketOptions();

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;

    // Create UDP sockets to multiple destination addresses (network servers)
    while ((token = tokenizer.nextToken()) != nullptr) {
        L3Address result;
        L3AddressResolver().tryResolve(token, result);
        if (result.isUnspecified())
            EV_ERROR << "cannot resolve destination address: " << token << endl;
        else
            EV << "Got destination address: " << token << endl;
        destAddresses.push_back(result);
    }
}

void PacketForwarder::makeRREPRequest(cMessage *msg){
           // ++seqNumber;
            RREPPacket *frame = new RREPPacket("RREPPacket");
            RREQPacket *frame2 = check_and_cast<RREQPacket *>( msg);
            frame->setReceiverAddress(frame2->getTransmitterAddress());
            frame->setTransmitterAddress(frame2->getReceiverAddress());
            frame->setHopCount(0);
            frame->setDestSeqNum(frame2->getSourceSeqNum());
            frame->setPrevNodeAddr(myAddress);
            frame->setNextHopAddr(routingModule.findRoute(frame2->getTransmitterAddress()));
            frame->setLoRaTP(frame2->getLoRaTP());
            frame->setLoRaCF(frame2->getLoRaCF());
            frame->setLoRaSF(frame2->getLoRaSF());
            frame->setLoRaBW(frame2->getLoRaBW());
            frame->setLoRaCR(frame2->getLoRaCR());
            frame->setRSSI(frame2->getRSSI());
            frame->setSNIR(frame2->getSNIR());
            send(frame, "lowerLayerOut");
            sourceSeqArray[frame2->getTransmitterAddress().getInt()] = frame2->getSourceSeqNum();
}

void PacketForwarder::propagateRREP(cMessage *msg){

        RREPPacket *newRREP = check_and_cast<RREPPacket *>(msg->dup());
        RREPPacket *frame2 = check_and_cast<RREPPacket *>(msg);
        newRREP->setPrevNodeAddr(myAddress);
        newRREP->setHopCount(newRREP->getHopCount()+1);
        newRREP->setNextHopAddr(routingModule.findRoute(frame2->getReceiverAddress()));
        send(newRREP, "lowerLayerOut");
        delete msg;
}

void PacketForwarder::makeRREQRequest(cMessage *msg){
    EV << "Making RREQ Request\n";
    broadcast_id++; //To avoid broadcast Storm
    RREQPacket *frame = new RREQPacket("RREQPacket");
    LoRaMacFrame *frame2 = check_and_cast<LoRaMacFrame *>( msg);
    frame->setReceiverAddress(DevAddr::IKINCI);
    frame->setTransmitterAddress(myAddress);
    frame->setHopCount(0);
    frame->setBroadCastId(broadcast_id);
    frame->setDestSeqNum(1);
    frame->setSourceSeqNum(++sourceSeqNumber);
    frame->setPrevNodeAddr(myAddress);
    frame->setLoRaTP(frame2->getLoRaTP());
    frame->setLoRaCF(frame2->getLoRaCF());
    frame->setLoRaSF(frame2->getLoRaSF());
    frame->setLoRaBW(frame2->getLoRaBW());
    frame->setLoRaCR(frame2->getLoRaCR());
    frame->setRSSI(frame2->getRSSI());
    frame->setSNIR(frame2->getSNIR());
    broadcastIDArray[myAddress.getInt()] = broadcast_id;
    //sourceSeqArray[myAddress.getInt()] = sourceSeqNumber;
    send(frame, "lowerLayerOut");
}

void PacketForwarder::saveRouteTable(cMessage *msg){
    AODVControlPacket *frame = check_and_cast<AODVControlPacket *>(msg);
    if(frame->getPacketType() == 1){
        RREQPacket *comingRREQ = check_and_cast<RREQPacket *>(msg);
            if(!routingModule.isRouteAdded(comingRREQ->getTransmitterAddress(),comingRREQ->getPrevNodeAddr(),comingRREQ->getHopCount())){
                routingModule.addEntryToRouteTable(comingRREQ->getTransmitterAddress(),comingRREQ->getPrevNodeAddr(),comingRREQ->getHopCount());
            }
    }
    else if(frame->getPacketType() == 2){
        RREPPacket *comingRREP = check_and_cast<RREPPacket *>(msg);
        if(!routingModule.isRouteAdded(comingRREP->getTransmitterAddress(),comingRREP->getPrevNodeAddr(),comingRREP->getHopCount())){
              routingModule.addEntryToRouteTable(comingRREP->getTransmitterAddress(),comingRREP->getPrevNodeAddr(),comingRREP->getHopCount());
         }
    }

}

void PacketForwarder::propagateRREQ(cMessage *msg){
    RREQPacket *newRREQ = check_and_cast<RREQPacket *>(msg->dup());
    delete msg;
    newRREQ->setPrevNodeAddr(myAddress);
    newRREQ->setHopCount(newRREQ->getHopCount()+1);
    send(newRREQ, "lowerLayerOut");
}

void PacketForwarder::handleMessage(cMessage *msg)
{
    if (msg->arrivedOn("lowerLayerIn")) {
        AODVControlPacket *frame = check_and_cast<AODVControlPacket *>(msg);
        //EV << "Received LoRaMAC frame: " << myAddress.str();
       // LoRaMacFrame *frame = check_and_cast<LoRaMacFrame *>(PK(msg));
        if(frame->getPacketType() == 0){ // Data kısmı
            LoRaMacFrame *endNodePacket = check_and_cast<LoRaMacFrame *> (msg);
            if(endNodePacket->getReceiverAddress() == DevAddr::BROADCAST_ADDRESS){ // Means Comes From End node
                if(myAddress == DevAddr::IKINCI){
                    EV << "You have Connection\n";
                    processLoraMACPacket(PK(msg));
                }
                else{ //I am intermediate Node
                    if(routingModule.isItRecorded(DevAddr::IKINCI)){ // Has A Route
                        DevAddr nextHop = routingModule.findRoute(DevAddr::IKINCI);
                        endNodePacket->setReceiverAddress(DevAddr::IKINCI);
                        endNodePacket->setNextHopAddr(nextHop);
                        endNodePacket->setTransmitterAddress(myAddress);
                        send(endNodePacket, "lowerLayerOut");
                    }
                    else{ //Route Not Found
                         lastMessage = msg->dup();
                         makeRREQRequest(msg);
                         delete msg;
                    }
                }
            }
            else{ //Comes From Intermediate Gateway
                if(routingModule.isItRecorded(endNodePacket->getReceiverAddress())){
                    DevAddr nextHop = routingModule.findRoute(endNodePacket->getReceiverAddress());
                    endNodePacket->setNextHopAddr(nextHop);
                    send(endNodePacket, "lowerLayerOut");
                }
                else if(endNodePacket->getReceiverAddress() == myAddress){
                    EV << "We get the packet\n";
                    processLoraMACPacket(PK(msg));
                }
                else {
                    EV << "ERROR?\n";
                }
            }
        }
        else if(frame->getPacketType() == 1){ //RREQ Kismi
            RREQPacket *comingRREQ = check_and_cast<RREQPacket *>(msg);
           // EV << "ALDIK ULANNN \n";
            EV << "[RREQ] Dest Address: " << comingRREQ->getReceiverAddress().str() <<" Source Address: " << comingRREQ->getTransmitterAddress().str() <<" Previous Node: " << comingRREQ->getPrevNodeAddr().str()<<" hopCount: "<< comingRREQ->getHopCount() <<"\n";
            //delete msg;
            if(comingRREQ->getReceiverAddress() == myAddress && comingRREQ->getBroadCastId() > broadcastIDArray[comingRREQ->getTransmitterAddress().getInt()]){
                EV << "Target Node is achieved\n";
                saveRouteTable(msg);
                makeRREPRequest(msg);
            }
            else if(routingModule.isItRecorded(comingRREQ->getReceiverAddress()) && comingRREQ->getBroadCastId() > broadcastIDArray[comingRREQ->getTransmitterAddress().getInt()]){
                EV << "Intermediate Node Knows The Route\n";
                saveRouteTable(msg);
                makeRREPRequest(msg);
            }
            else if(comingRREQ->getBroadCastId() > broadcastIDArray[comingRREQ->getTransmitterAddress().getInt()]){
                EV << "Propagate\n";
                broadcastIDArray[comingRREQ->getTransmitterAddress().getInt()] = comingRREQ->getBroadCastId();
                saveRouteTable(msg);
                propagateRREQ(msg);
            }
            else{
                EV << "RREQ COME AGAIN DISCARD IT\n";
            }
            //return;
        }
        else if(frame->getPacketType() == 2){ //RREP Kismi
            RREPPacket *comingRREP = check_and_cast<RREPPacket *>(msg);
            EV << "[RREP] Dest Address: " << comingRREP->getReceiverAddress().str() <<" Source Address: " << comingRREP->getTransmitterAddress().str() <<" Previous Node: " << comingRREP->getPrevNodeAddr().str()<<" hopCount: "<< comingRREP->getHopCount() <<"\n";
            if(comingRREP->getReceiverAddress() == myAddress){
                EV << "We found the route\n";
                saveRouteTable(msg);
                routingModule.printRouteTable();
                EV << "Now send actual data\n";
                LoRaMacFrame* queuedMessage = check_and_cast<LoRaMacFrame *>(this->lastMessage);
                DevAddr nextHop = routingModule.findRoute(DevAddr::IKINCI);
                queuedMessage->setReceiverAddress(DevAddr::IKINCI);
                queuedMessage->setNextHopAddr(nextHop);
                queuedMessage->setTransmitterAddress(myAddress);
                send(queuedMessage, "lowerLayerOut");
                delete msg;
            }
            else if(comingRREP->getDestSeqNum() > sourceSeqArray[comingRREP->getTransmitterAddress().getInt()]){
                EV << "PROPOGATE RREP\n";
               // seqNumber = comingRREP->getDestSeqNum();
                sourceSeqArray[comingRREP->getReceiverAddress().getInt()] = comingRREP->getDestSeqNum();
                saveRouteTable(msg);
                routingModule.printRouteTable();
                propagateRREP(msg);
            }
            else{
                EV << "RREP COME AGAIN DISCARD IT\n";
            }
            //return;
        }
    } else if (msg->arrivedOn("udpIn")) {
        // FIXME : debug for now to see if LoRaMAC frame received correctly from network server
       // EV << "Received UDP packet HALOOOO" << endl;
        LoRaMacFrame *frame = check_and_cast<LoRaMacFrame *>(PK(msg));
        //EV << frame->getLoRaTP() << endl;
        //delete frame;
        send(frame, "lowerLayerOut");
        //
    }
}

void PacketForwarder::processLoraMACPacket(cPacket *pk)
{
    // FIXME: Change based on new implementation of MAC frame.
    emit(LoRa_GWPacketReceived, 42);
    if (simTime() >= getSimulation()->getWarmupPeriod())
        counterOfReceivedPackets++;
    LoRaMacFrame *frame = check_and_cast<LoRaMacFrame *>(pk);

    physicallayer::ReceptionIndication *cInfo = check_and_cast<physicallayer::ReceptionIndication *>(pk->getControlInfo());
    W w_rssi = cInfo->getMinRSSI();
    double rssi = w_rssi.get()*1000;
    frame->setRSSI(math::mW2dBm(rssi));
    frame->setSNIR(cInfo->getMinSNIR());
    bool exist = false;
    //EV << frame->getTransmitterAddress() << endl;
    //for (std::vector<nodeEntry>::iterator it = knownNodes.begin() ; it != knownNodes.end(); ++it)

    // FIXME : Identify network server message is destined for.
    L3Address destAddr = destAddresses[0];
    if (frame->getControlInfo())
       delete frame->removeControlInfo();

    socket.sendTo(frame, destAddr, destPort);

}

void PacketForwarder::sendPacket()
{

    /*LoRaAppPacket *mgmtCommand = new LoRaAppPacket("mgmtCommand");
    mgmtCommand->setMsgType(TXCONFIG);
    LoRaOptions newOptions;
    newOptions.setLoRaTP(uniform(0.1, 1));
    mgmtCommand->setOptions(newOptions);

    LoRaMacFrame *response = new LoRaMacFrame("mgmtCommand");
    response->encapsulate(mgmtCommand);
    response->setLoRaTP(pk->getLoRaTP());
    response->setLoRaCF(pk->getLoRaCF());
    response->setLoRaSF(pk->getLoRaSF());
    response->setLoRaBW(pk->getLoRaBW());
    response->setReceiverAddress(pk->getTransmitterAddress());
    send(response, "lowerLayerOut");*/

}

void PacketForwarder::receiveSignal(cComponent *source, simsignal_t signalID, long value, cObject *details)
{
    if (simTime() >= getSimulation()->getWarmupPeriod())
        counterOfSentPacketsFromNodes++;
}

void PacketForwarder::finish()
{
    recordScalar("LoRa_GW_DER", double(counterOfReceivedPackets)/counterOfSentPacketsFromNodes);
}



} //namespace inet
