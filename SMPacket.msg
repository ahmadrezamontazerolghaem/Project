namespace smart3p;

//
// Smart Meter Packet
//
packet SMPacket {
    int id;
    double value;
    int smGateID;
    int collGateID;
    string extraInfo;
    string extraInfoSize;
}
