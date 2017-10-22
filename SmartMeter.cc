#include "SMAdapter.h"
#include "SmartMeter.h"
#include <string>

#ifndef SMPacket
#include <SMPacket_m.h>
#endif
#ifndef MKPacket
#include <MKPacket_m.h>
#endif

namespace smart3p {

Define_Module(SmartMeter);

void SmartMeter::initialize()
{
    //lastArrival = simTime();
    iaTimeHistogram.setName("interarrival times");
    arrivalsVector.setName("arrivals");
    arrivalsVector.setInterpolationMode(cOutVector::NONE);

    energyDataQueue = new cQueue("energyDataQueue");
    startSendingEnergyData = new cMessage("startSendingEnergyData");
    
    stackSizeSignal = registerSignal("stackSize");
    sessionKeyDecryptionTimeSignal = registerSignal("sessionKeyDecryptionTime");
    sessionKeyEncryptionTimeSignal = registerSignal("sessionKeyEncryptionTime");
    dataEncryptionTimeSignal = registerSignal("dataEncryptionTime");
    
    //EV << "Packet Size id: " << packetSizeSignal << endl;
}

void SmartMeter::addSimTime(double time)
{
    curDelay = simTime()+time;
    /*
    simtime_t_cref st = sim->getSimTime();
    EV << "Simtime before adding: " << simTime() << endl;
    sim->setSimTime(SimTime(st.raw()+(time*st.getScale()))); //advances simtime forward
    EV << "Simtime after adding:  " << simTime() << endl;
    */
    
}
    
void SmartMeter::log(simsignal_t id, double value) //simsignal_t is just type def'ed as an int
{
    emit(id,value);
}

void SmartMeter::log(char* tag, double value)
{
    if (!strcmp(tag, "stackSize"))
    {
	log(stackSizeSignal,value);
    }
    else if (!strcmp(tag, "sessionKeyDecryptionTime"))
    {
	log(sessionKeyDecryptionTimeSignal,value);
    }
    else if (!strcmp(tag, "sessionKeyEncryptionTime"))
    {
	log(sessionKeyEncryptionTimeSignal,value);
    }
    else if (!strcmp(tag, "dataEncryptionTime"))
    {
	log(dataEncryptionTimeSignal,value);
    }
    else
    {
	EV << "Tag: \"" << tag << "\" failed to correspond to an already existing tag" << endl;
    }
}
    
void SmartMeter::sendQueueDataToCollector()
{
    if (!energyDataQueue->isEmpty())
    {
        // Send the energy measurement to the collector
        cMessage *frontDatum = check_and_cast<cMessage*> (energyDataQueue->front());

	cMessage *datumClone = frontDatum->dup();
	SMPacket *p = check_and_cast<SMPacket*> (datumClone->getObject("packet"));

	if (dblrand() < par("per").doubleValue())
	{
	    p->setBitError(true);
	}
            
	send(datumClone, "radio$o");

	log("stackSize",energyDataQueue->getLength());
	
        // Set the timeout to know whether the message was received by
        // the collector or not
	cancelEvent(timeoutEvent);
        scheduleAt(simTime() + waitForDelivery, timeoutEvent);
    }
    else
    {
        // Continue sending the rest of the energy queue data
	cancelEvent(timeoutEvent);
	cancelEvent(startSendingEnergyData);
	scheduleAt(simTime() + 1, startSendingEnergyData);
    }
}

void SmartMeter::timedHandleMessage(cMessage *msg)
{
    if (msg == timeoutEvent)
    {
	/*
        // Timeout expired, re-sending the data...
        // Send the energy measurement to the collector from the energy data queue
        cMessage *frontDatum = check_and_cast<cMessage*> (energyDataQueue->front());
        send(frontDatum->dup(), "radio$o");
        // Set the timeout to know whether the message was received by
        // the collector or not
        scheduleAt(simTime() + waitForDelivery, timeoutEvent);
        // Return from the function
	*/
        sendQueueDataToCollector();
    }
    else if (!strcmp(msg->getName(), "ACK"))
    {
        EV << "Packet is successfully received: SM --> Collector" << endl;
        // Delete the energy consumption message from the top of the energy data queue

	if (!energyDataQueue->isEmpty()) delete energyDataQueue->pop();

	delete msg;
        sendQueueDataToCollector();
    }
    else if (!strcmp(msg->getName(), "startSendingEnergyData"))
    {
        sendQueueDataToCollector();
    }
    // Receiving a message from generator
    else if (!strcmp(msg->getName(), "smartMeterInit"))
    {
        smartMeterInit(msg);
    }
    // Receiving a ready message from the utility company
    else if (!strcmp(msg->getName(), "ready"))
    {
        registerAtUC();
    }
    // Receiving register info from the utility company
    else if (!strcmp(msg->getName(), "registerInfoFromUC"))
    {
        registerInfoFromUC(msg);
    }
    else if (!strcmp(msg->getName(), "sessionKeyStartFromTTP"))
    {
    	startSessionKeyExchange(msg);
    }
    // End of the session key exchange phase
    else if (!strcmp(msg->getName(), "sessionKey"))
    {
        endOfSessionKeyExchange(msg);
        // Start sending data to the collector in 5 seconds
        scheduleAt(simTime() + 5, startSendingEnergyData);
    }
    // Done getting data from the generator
    else if (!strcmp(msg->getName(), "done"))
    {
        delete msg;
        cancelEvent(startSendingEnergyData);
        cancelEvent(timeoutEvent);
    }
    else if (!strcmp(msg->getName(), "energyConsumption"))
    {
        sendEnergyConsumption(msg); //puts a message in the queue
	sendQueueDataToCollector(); //dequeues that message and starts attempting to send it
    }
    else if (!strcmp(msg->getName(), "delayedMessage"))
    {
	send(check_and_cast<cMessage*>(msg->getParList().remove(0)),"radio$o");
	delete msg;
    }
    else if (!strcmp(msg->getName(), "delayedEnqueue"))
    {
	energyDataQueue->insert(check_and_cast<cMessage*>(msg->getParList().remove(0)));
	delete msg;
    }
    else // unknown message
    {
        EV << "Message " << msg->getName() << " received" << endl;
    }
}

void SmartMeter::finish()
{
    cancelEvent(timeoutEvent);
    cancelEvent(startSendingEnergyData);
    recordStatistic(&iaTimeHistogram);
    energyDataQueue->clear();
    delete energyDataQueue;
    delete startSendingEnergyData;
}

void SmartMeter::startSessionKeyExchange(cMessage *msg)
{    
    // Starting the session key exchange phase
    EV << "Starting session key exchange phase" << endl;

    //cQueue contains every message of the split information
    cQueue* queue = sm->startSessionKeyExchange(msg);

    while (!queue->isEmpty())
    {
	cMessage* out = check_and_cast<cMessage*> (queue->pop());
	send(out,"radio$o");
    }
}

void SmartMeter::sendEnergyConsumption(cMessage *msg)
{
    simtime_t d = simTime() - lastArrival;

    iaTimeHistogram.collect(d);
    arrivalsVector.record(1);
    
    // Add message to the energy data queue
    cMessage* out = sm->sendEnergyConsumption(msg);

    delete msg;
    
    cMessage* delayed = new cMessage("delayedEnqueue");
    delayed->addObject(out);
    scheduleAt(curDelay,delayed);
    //send(out,"radio$o");
}

void SmartMeter::endOfSessionKeyExchange(cMessage *msg)
{
    // Save the session key
    EV << "Saving session key" << endl;

    if (!sm->endOfSessionKeyExchange(msg))
    {
	EV << "Failed to verify the session key" << endl;
	
	return;
    }

    EV << "Successfully verified the session key" << endl;
    
    // Send ready message to the data generator
    cMessage *m = new cMessage("ready");
    SMPacket *p = new SMPacket("packet");

    p->setId(smID);
    m->addObject(p);
    
    send(m, "generatorLine$o");
    delete msg;
}

void SmartMeter::registerInfoFromUC(cMessage *msg)
{
    // Save the registration information
    EV << "Received register info" << endl;

    sm->registerInfoFromUC(msg);
}

void SmartMeter::registerAtUC()
{
    // Start the registration phase
    EV << "Starting the registration phase for SM# " << smID << endl;

    cMessage *reg = new cMessage("registerFromSM");
    SMPacket* p = new SMPacket("packet");
    reg->addObject(p);
    
    p->setId(smID);
    
    send(reg, "radio$o");
}

void SmartMeter::smartMeterInit(cMessage *msg)
{
    SMPacket *p = dynamic_cast<SMPacket*> (msg->getObject("packet"));
    smID = p->getId();

    EV << "ID: " << smID << endl;
    
    sm = new SMAdapter(smID, this);
    //sm = new SMAdapter(smID);
    
    // Generate a secret symmetric key AES-128 for SM-UC communication
    // Attach the key to msg
    p->setExtraInfo("jkjhasf78hy8h374hd878nnfis34i");

    send(msg, "radio$o");
}

}; // namespace
