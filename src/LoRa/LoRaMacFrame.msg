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

//
// TODO generated message class
//

cplusplus {{
#include "../misc/DevAddr.h"
#include <inet/common/Units.h>
}}

class noncobject DevAddr;
class noncobject inet::units::values::W;
class noncobject inet::units::values::Hz;
namespace inet;
//enum packetType {DATA = 0; RREQ = 1; RREP = 2; RRER = 3; FORWARDEDDATA = 4};

packet AODVControlPacket
{
    unsigned int packetType;
    DevAddr transmitterAddress;
    DevAddr receiverAddress;
}

class LoRaMacFrame extends AODVControlPacket{
    DevAddr nextHopAddr;
    packetType = 0;
    int sequenceNumber;
    double LoRaTP;
    inet::units::values::Hz LoRaCF;
    int LoRaSF;
    inet::units::values::Hz LoRaBW;
    int LoRaCR;
    bool LoRaUseHeader;
    double RSSI;
    double SNIR;
    int nodeID;
    
}
class RREQPacket extends AODVControlPacket{
	packetType = 1;
    unsigned int hopCount;
    unsigned int broadCastId;
    //DevAddr destAddr; ReceiverAddress
    unsigned int destSeqNum;
    //DevAddr sourceAddr; TransmitterAddress
    DevAddr prevNodeAddr;
    unsigned int sourceSeqNum;
    double LoRaTP;
    inet::units::values::Hz LoRaCF;
    int LoRaSF;
    inet::units::values::Hz LoRaBW;
    int LoRaCR;
    bool LoRaUseHeader;
    double RSSI;
    double SNIR;
}

class RREPPacket extends AODVControlPacket{
	packetType = 2;
    unsigned int hopCount;
    //DevAddr destAddr; receiverAddress
    unsigned int destSeqNum;
    //DevAddr sourceAddr; transmitterAddress
    DevAddr nextHopAddr;
    DevAddr prevNodeAddr;
    double LoRaTP;
    inet::units::values::Hz LoRaCF;
    int LoRaSF;
    inet::units::values::Hz LoRaBW;
    int LoRaCR;
    bool LoRaUseHeader;
    double RSSI;
    double SNIR;
}
