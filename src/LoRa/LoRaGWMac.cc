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

#include "LoRaGWMac.h"
#include "inet/common/ModuleAccess.h"
#include "inet/physicallayer/contract/packetlevel/IRadio.h"
#include "PacketForwarder.h"


namespace inet {

Define_Module(LoRaGWMac);

void LoRaGWMac::initialize(int stage)
{
    MACProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        // subscribe for the information of the carrier sense
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        //radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);
        waitingForDC = false;
        dutyCycleTimer = new cMessage("Duty Cycle Timer");
        const char *addressString = par("address");
        GW_forwardedDown = 0;
        GW_droppedDC = 0;
        if (!strcmp(addressString, "auto")) {
            // assign automatic address
            address = DevAddr::generateAutoAddress();
            // change module parameter from "auto" to concrete address
            par("address").setStringValue(address.str().c_str());
            PacketForwarder* loraApp = dynamic_cast<PacketForwarder *>(getParentModule()->getParentModule()->getSubmodule("packetForwarder"));
            loraApp->myAddress = address;
           // this->getParentModule()->getParentModule()->getSubmo=address;
        }
        else
            address.setAddress(addressString);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        radio->setRadioMode(IRadio::RADIO_MODE_TRANSCEIVER);
    }
}

void LoRaGWMac::finish()
{
    recordScalar("GW_forwardedDown", GW_forwardedDown);
    recordScalar("GW_droppedDC", GW_droppedDC);
    cancelAndDelete(dutyCycleTimer);
}


InterfaceEntry *LoRaGWMac::createInterfaceEntry()
{
    InterfaceEntry *e = new InterfaceEntry(this);

    // data rate
    //e->setDatarate(bitrate);

    // generate a link-layer address to be used as interface token for IPv6
    //e->setMACAddress(address);
    //e->setInterfaceToken(address.formInterfaceIdentifier());

    // capabilities
    //e->setMtu(par("mtu"));
    //e->setMulticast(true);
    //e->setBroadcast(true);
    //e->setPointToPoint(false);

    return e;
}

void LoRaGWMac::handleSelfMessage(cMessage *msg)
{
    if(msg == dutyCycleTimer){
        waitingForDC = false;
        if(waitingPacket != nullptr){
            EV <<"SENDING THE PACKET KNOW DUE TO DUTY CYCYLE\n";
            sendDown(waitingPacket);
            waitingPacket = nullptr;
        }
    }
}

void LoRaGWMac::handleUpperPacket(cPacket *msg)
{
    if(waitingForDC == false)
    {
        AODVControlPacket *frame2 = check_and_cast<AODVControlPacket *>(msg);
        if(frame2->getPacketType() == 0){
                   LoRaMacFrame *frame = check_and_cast<LoRaMacFrame *>(msg);
                    /*frame->removeControlInfo();
                    LoRaMacControlInfo *ctrl = new LoRaMacControlInfo();
                    ctrl->setSrc(address);
                    ctrl->setDest(frame->getReceiverAddress());
                    frame->setControlInfo(ctrl);*/
                    sendDown(msg);
                    waitingForDC = true;
                    double delta;
                    if(frame->getLoRaSF() == 7) delta = 0.61696; //spre
                    if(frame->getLoRaSF() == 8) delta = 1.23392;
                    if(frame->getLoRaSF() == 9) delta = 2.14016;
                    if(frame->getLoRaSF() == 10) delta = 4.28032;
                    if(frame->getLoRaSF() == 11) delta = 7.24992;
                    if(frame->getLoRaSF() == 12) delta = 14.49984;
                    scheduleAt(simTime() + delta, dutyCycleTimer);
                    lastDelta = delta;
                    GW_forwardedDown++;

        }
        else if(frame2->getPacketType() == 1){
            EV << "SENDING RREQ DOWN";
            RREQPacket *frame = check_and_cast<RREQPacket *> (msg);
            sendDown(msg);
            waitingForDC = true;
            double delta;
            if(frame->getLoRaSF() == 7) delta = 0.61696; //spre
            if(frame->getLoRaSF() == 8) delta = 1.23392;
            if(frame->getLoRaSF() == 9) delta = 2.14016;
            if(frame->getLoRaSF() == 10) delta = 4.28032;
            if(frame->getLoRaSF() == 11) delta = 7.24992;
            if(frame->getLoRaSF() == 12) delta = 14.49984;
            scheduleAt(simTime() + delta, dutyCycleTimer);
            lastDelta = delta;
            GW_forwardedDown++;


        }
        else if(frame2->getPacketType() == 2){
            EV << "SENDING RREP DOWN";
            RREPPacket *frame = check_and_cast<RREPPacket *> (msg);
            sendDown(msg);
            waitingForDC = true;
            double delta;
            if(frame->getLoRaSF() == 7) delta = 0.61696; //spre
            if(frame->getLoRaSF() == 8) delta = 1.23392;
            if(frame->getLoRaSF() == 9) delta = 2.14016;
            if(frame->getLoRaSF() == 10) delta = 4.28032;
            if(frame->getLoRaSF() == 11) delta = 7.24992;
            if(frame->getLoRaSF() == 12) delta = 14.49984;
            lastDelta = delta;
            scheduleAt(simTime() + delta, dutyCycleTimer);
            GW_forwardedDown++;
        }
    }
    else
    {
        EV << "WAITING DUE TO DUTY CYCLE";
        waitingPacket = msg;
        GW_droppedDC++; // NOW IT REPRESENTS NUMBER PACKETS THAT WAITS
        //delete msg;
    }
}

void LoRaGWMac::handleLowerPacket(cPacket *msg)
{
    AODVControlPacket *frame = check_and_cast<AODVControlPacket *>(msg);
    //if(frame->getReceiverAddress() == DevAddr::BROADCAST_ADDRESS)
    if(frame->getPacketType() == 0){
        LoRaMacFrame *frame2 = check_and_cast<LoRaMacFrame *>(msg);
        if(frame2->getNextHopAddr() != address && frame2->getReceiverAddress() != DevAddr::BROADCAST_ADDRESS){
            EV << "MESSAGE DELETED BY " << address.str() << " \n";
            delete msg;
        }
        else{
            sendUp(msg);
        }
    }
    else if(frame->getPacketType() == 2){
        RREPPacket *comingRREP = check_and_cast<RREPPacket *>(msg);
        if(comingRREP->getNextHopAddr() != address){
            delete msg;
        }
        else{
            sendUp(msg);
        }
    }
    else{
            sendUp(msg);
        }
   // sendUp(msg);
    //else
      //  delete frame;
}

void LoRaGWMac::sendPacketBack(LoRaMacFrame *receivedFrame)
{
    EV << "sending Data frame back" << endl;
    LoRaMacFrame *frameToSend = new LoRaMacFrame("BackPacket");
    frameToSend->setReceiverAddress(receivedFrame->getTransmitterAddress());
    sendDown(frameToSend);
}

void LoRaGWMac::createFakeLoRaMacFrame()
{

}

void LoRaGWMac::receiveSignal(cComponent *source, simsignal_t signalID, long value, cObject *details)
{
    Enter_Method_Silent();
    if (signalID == IRadio::transmissionStateChangedSignal) {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE) {
            //transmissin is finished
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        }
        transmissionState = newRadioTransmissionState;
    }
}

DevAddr LoRaGWMac::getAddress()
{
    return address;
}

}