#include "DataGenerator.h"
#include <string>
#include <fstream>

#ifndef SMPacket
#include <SMPacket_m.h>
#endif

namespace smart3p {

using namespace omnetpp;

Define_Module(DataGenerator);

void DataGenerator::initialize()
{
    readyList = new bool[gateSize("smLine")]; //keeps track of which smart meters are ready
    
    timer = new cMessage("timer");
        
    dataFile=par("dataFileName");
    
    cMessage *job = new cMessage("startTheJob");
    scheduleAt(simTime(), job);
}

bool DataGenerator::readData()
{
    std::ifstream file(dataFile);
    int counter = 0;
    int key = -1; // Smart meter ID
    char c; // Separator
    double value = -1.0; // Energy consumption
    EV << "Start" << endl;
    if (!file.is_open())
    {
	EV << "Data file was not open." << endl;
	return false;
    }

    //only add more data to an already existing queue
    if (outputQueue == NULL) outputQueue = new std::queue<double>();
    
    while (file >> key >> c >> value)
    {
	counter++;
	/*
	if (ECdata.find(key) != ECdata.end())
	{
	    ECdata[key].push(value);
	}
	else
	{
	    std::queue<double> q;
	    ECdata.insert(std::pair<int, std::queue<double> > (key, q));
	    ECdata[key].push(value);
	}
	*/
	outputQueue->push(value);
    }
    EV << "Finish " << counter << " times." << endl;
    
    return true;
}


bool DataGenerator::sendDataToSmartMeters()
{
/*
    std::map<int, std::queue<double> >::iterator it = ECdata.begin();

    int howManySMareDone = 0;
    for (int i = 0; i < gateSize("smLine"); i++)
    {
        cMessage *energyConsumption = new cMessage("energyConsumption");
        SMPacket *packet = new SMPacket("packet");

        // Set packet error rate
        if (dblrand() < par("per").doubleValue())
        {
            packet->setBitError(true);
            EV << "Packet lost for meter #" << i << endl;
        }

        packet->setId(it->first);
        // Send energy consumption until we ran out of data
        if(!it->second.empty())
        {
            packet->setValue(it->second.front());
            it->second.pop();
        }
        // Send done message to the smart meter that we do not have any data
        else
        {
            energyConsumption->setName("done");
            howManySMareDone++;
        }
        it++;
        energyConsumption->addObject(packet);

        cGate *g = gate("smLine$o", i);
        send(energyConsumption, g);
    }
    // If all smart meters ran out of data, then stop simulation
    if (howManySMareDone == gateSize("smLine"))
        return true;
*/
    if (outputQueue->size() < gateSize("smLine")) readData(); //re-read the data to fill up the queue
    
    for (int i = 0; i < gateSize("smLine"); i++)
    {
	if (!readyList[i]) continue;
	
	cMessage *energyConsumption = new cMessage("energyConsumption");
        SMPacket *packet = new SMPacket("packet");
	energyConsumption->addObject(packet);

	//send different data to every smart meter
	double value = outputQueue->front();
	// packet->setValue(value);
	outputQueue->pop();

        cGate *g = gate("smLine$o", i);
        send(energyConsumption, g);
    }
    
    return true;
}


void DataGenerator::sendDataToSmartMeter(cMessage* msg)
{
    cGate *g = gate("smLine$o", msg->getArrivalGate()->getIndex());

    if (outputQueue->empty())
    {
//	send(new cMessage("done", g)); //we want to run indefinetly
//	return;
	if (!readData()) return;
    }

    SMPacket *p = dynamic_cast<SMPacket*> (msg->getObject("packet"));

    p->setValue(outputQueue->front());
    outputQueue->pop();
    
    msg->setName("energyConsumption");
    send(msg, g);
}
   
void DataGenerator::handleMessage(cMessage *msg)
{
    if (!strcmp(msg->getName(), "startTheJob"))
    {
        // Read the energy consumption data into memory
        if (!readData())
            return;

        // Send ID to every smart meter
        //std::map<int, std::queue<double> >::iterator it = ECdata.begin();
        for (int i = 0; i < gateSize("smLine"); i++)
        {
            cMessage *smartMeterInit = new cMessage("smartMeterInit");
            SMPacket *packet = new SMPacket("packet");
            //packet->setId(it->first);
            //it++;

//EV << i << "/" << gateSize("smLine") << endl;
	    packet->setId(i);
            smartMeterInit->addObject(packet);

            cGate *g = gate("smLine$o", i);
            send(smartMeterInit, g);
        }

        delete msg;
        return;
    }

    else if (!strcmp(msg->getName(), "ready"))
    {
	SMPacket *p = dynamic_cast<SMPacket*> (msg->getObject("packet"));
	//smCounter++;
	int id = p->getId();
	readyList[id] = true;
	
        delete msg;
        // Check if we received a ready message from all smart meters
        // If not, return from the function
	for (int i=0;i<gateSize("smLine");i++)
	{
	    if (!readyList[i])
	    {
		EV << "SM #" << i << " has not sent ready signal" << endl;
	    }
	}
	
	if (!timer->isScheduled()) scheduleAt(simTime() + par("freqOfSendingData").doubleValue(), timer);
    }
    
    else if (!strcmp(msg->getName(), "sendData") || !strcmp(msg->getName(), "timer"))
    {
	sendDataToSmartMeters();
	scheduleAt(simTime() + par("freqOfSendingData").doubleValue(), timer);
	return;
    }
    
    // if (!isDone)
    // {
    //     scheduleAt(simTime() + par("freqOfSendingData").doubleValue(), timer);
    // }
}

void DataGenerator::finish()
{
    //delete timer;
}

} //namespace
