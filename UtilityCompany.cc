
#include "UtilityCompany.h"
#include <map>
#include <string>

#ifndef SMPacket
#include <SMPacket_m.h>
#endif

namespace smart3p {

Define_Module(UtilityCompany);

UtilityCompany::UtilityCompany()
{
    data = new cArray("data");
    uc = new ::UCAdapter();
    //uc = new ::UCAdapter(this); //enables debug printing
}

UtilityCompany::~UtilityCompany()
{
    delete data;
}

void UtilityCompany::initialize()
{
    prevTime = simTime();
}

void UtilityCompany::addPacketToDB(SMPacket *p)
{
    MeterData save;
    save.anonymID = p->getId() + 100000;
    save.smGate = p->getSmGateID();
    save.collGate = p->getCollGateID();
    save.secretSMtoUCkey = p->getExtraInfo();

    EV << "UC data from ID# " << p->getId() << ", C# " << save.collGate << ", S# " << save.smGate <<
            ", key: " << p->getExtraInfo() << endl;

    meters.insert(std::pair<int, MeterData>(p->getId(), save));
}

void UtilityCompany::timedHandleMessage(cMessage *msg)
{
    // Receiving a message from generator, passed by the smart meter and collector
    if (!strcmp(msg->getName(), "smartMeterInit"))
    {
        smartMeterInit(msg);

        return;
    }
    // Receiving a registration message from SM
    else if (!strcmp(msg->getName(), "registerFromSM"))
    {
        registerSM(msg);

        return;
    }
    // Receiving a registration message from TTP
    else if (!strcmp(msg->getName(), "registerFromTTP"))
    {
        registerTTP(msg);

        return;
    }
    else if (!strcmp(msg->getName(), "registerSMWithTTP"))
    {
        registerSMWithTTP(msg);

        return;
    }
    //Recieving request to start session key exchange from TTP
    else if (!strcmp(msg->getName(), "sessionKeyStartFromTTP"))
    {
        sessionKeyStartFromTTP(msg);

        return;
    }
    // Receiving a request for a session key exchange from SM to TTP
    else if (!strcmp(msg->getName(), "sessionKeyExchange"))
    {
        sessionKeyExchangeFromSM(msg);

        return;
    }
    // Receiving a request for a session key exchange from TTP to SM
    else if (!strcmp(msg->getName(), "sessionKey"))
    {
        sessionKeyExchangeFromTTP(msg);

        return;
    }

    energyConsumptionProcessing(msg);
}

void UtilityCompany::sessionKeyStartFromTTP(cMessage* msg)
{
    //Starting session key exchange
    uc->sessionKeyStartFromTTP(msg);

    SMPacket *p = check_and_cast<SMPacket*> (msg->getObject("packet"));

    cGate *g = gate("radio$o", p->getCollGateID());
    send(msg, g);

}
    
void UtilityCompany::sessionKeyExchangeFromTTP(cMessage *msg)
{
    if (!uc->sessionKeyExchangeFromTTP(msg))
    {
	EV << "HMAC Verification Failed!" << endl;
	return;
    }
    
    // Set the ID for the SM we are sending to
    SMPacket *p = check_and_cast<SMPacket*> (msg->getObject("packet"));
    cGate *g = gate("radio$o", p->getCollGateID());
    send(msg, g);
}
    
void UtilityCompany::sessionKeyExchangeFromSM(cMessage *msg)
{
    // Change the ID to anonymized ID
    if (!uc->sessionKeyExchangeFromSM(msg))
    {
	EV << "HMAC Verification failed!" << endl;
	return;
    }
    
    // Link the collector with the ID
    SMPacket *p = check_and_cast<SMPacket*> (msg->getObject("packet"));
    p->setCollGateID(msg->getArrivalGate()->getIndex());
    
    // Forward the data to TTP
    send(msg, "ttpLine$o");
}

void UtilityCompany::energyConsumptionProcessing(cMessage *msg)
{
    EV << "UC Received " << msg->getName() << endl;
    
    if (!uc->energyConsumptionProcessing(msg))
    {
	EV << "HMAC Verification failed!" << endl;
	delete msg;
	return;
    }

    send(msg, "ttpLine$o"); //msg contains a queue name "queue" which holds all the verified data
}
    
void UtilityCompany::registerSMWithTTP(cMessage *msg)
{
    SMPacket *p = check_and_cast<SMPacket*> (msg->getObject("packet"));
    
    //msg's packet contains anonid
    cMessage* out = new cMessage("registerSM");
    SMPacket* pac = new SMPacket("packet");
    out->addObject(pac);

    ::cInteger* id = check_and_cast<::cInteger*>(p->getObject("anonid"));
    
    pac->setId(id->ConvertToLong());

    delete msg;
    
    send(out, "ttpLine$o");
}

void UtilityCompany::registerTTP(cMessage *msg)
{
    SMPacket *p = check_and_cast<SMPacket*> (msg->getObject("packet"));
    int ttpid = p->getId();
    EV << "Starting registration phase for ttpId# " << ttpid << endl;
    
    // Registration phase for a trusted third party
    cMessage* info = uc->registerTTP(msg);
        
    // Send the info back to the trusted third party
    send(info, "ttpLine$o"); //msg is deleted in the register function
}
   
void UtilityCompany::registerSM(cMessage *msg)
{
    //SMPacket *p = check_and_cast<SMPacket*> (msg->getObject("packet"));
        
    cGate *g = gate("radio$o", msg->getArrivalGate()->getIndex());
    
    // Registration phase for a smart meter
    cMessage* info = uc->registerSM(msg);    

    delete msg;

    cMessage* toTTP = info->dup();
    toTTP->setName("registerSMWithTTP");
    scheduleAt(simTime()+0.05, toTTP);
    
    send(info, g); //registerInfoFromUC
}

void UtilityCompany::smartMeterInit(cMessage *msg)
{
    // Link the smart meter with the ID
    SMPacket *p = check_and_cast<SMPacket*> (msg->getObject("packet"));
    p->setCollGateID(msg->getArrivalGate()->getIndex());

    // Add packet to the local DB
    addPacketToDB(p);

    // Send a ready message back to the smart meter
    // It will allow the smart meter to start registering at UC
    cMessage *ready = new cMessage("ready");
    SMPacket *info = new SMPacket("packet");
    info->setSmGateID(p->getSmGateID());
    ready->addObject(info);

    cGate *g = gate("radio$o", msg->getArrivalGate()->getIndex());
    send(ready, g);

    delete msg;
}

} //namespace
