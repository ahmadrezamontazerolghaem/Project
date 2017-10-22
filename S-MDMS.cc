#include "TrustedThirdParty.h"
//#include "cqueue.h"

#include "TTPAdapter.h"

#ifndef SMPacket
#include <SMPacket_m.h>
#endif

namespace smart3p {

Define_Module(TrustedThirdParty);

void TrustedThirdParty::initialize()
{
    cMessage *reg = new cMessage("register");
    scheduleAt(simTime(), reg);
    scheduleAt(simTime()+5, new cMessage("startSessionKeyExchange"));
    //todo Add id to adapter initialization
    //ttp = new TTPAdapter(100000,this);
    ttp = new TTPAdapter(100000);
}
    
void TrustedThirdParty::timedHandleMessage(cMessage *msg)
{
    // Check if the message is for registration at UC
    if (!strcmp(msg->getName(), "register"))
    {
        registerAtUC(msg);

        return;
    }
    // Receiving registration information from the utility company
    else if (!strcmp(msg->getName(), "registerInfoFromUC"))
    {
        registerInfoFromUC(msg);

        return;
    }
    //starting session key exchange
    else if (!strcmp(msg->getName(), "startSessionKeyExchange"))
    {
        startSessionKeyExchange();

        return;
    }
    // Receiving request for a session key exchange phase
    else if (!strcmp(msg->getName(), "sessionKeyExchange"))
    {
        finishSessionKeyExchange(msg);

        return;
    }
    else if (!strcmp(msg->getName(), "registerSM"))
    {
	registerSM(msg);

        return;
    }
    processDataFromSM(msg);
}

void TrustedThirdParty::registerSM(cMessage *msg)
{
    ttp->registerSM(msg);

    delete msg;
}
    
void TrustedThirdParty::startSessionKeyExchange()
{
    cQueue* queue = ttp->startSessionKeyExchange();
    while (!queue->isEmpty())
    {
	cMessage* out = check_and_cast<cMessage*>(queue->pop());
	send(out, "ucLine$o");
    }
}
    
void TrustedThirdParty::finishSessionKeyExchange(cMessage *msg)
{
    // Generate the session key
    cMessage* out = ttp->finishSessionKeyExchange(msg);

    delete msg;
    
    if (out == NULL)
    {
	EV << "Have not finished session key exchange yet" << endl;
	return;
    }

    send(out, "ucLine$o");
}

void TrustedThirdParty::registerInfoFromUC(cMessage *msg)
{
    // Save the registration information
    EV << "Received register info" << endl;

    ttp->registerInfoFromUC(msg);

    delete msg;
}
void TrustedThirdParty::registerAtUC(cMessage *msg)
{
    delete msg;
    // Start the registration phase
    EV << "Starting the registration phase for TTP" << endl;

    cMessage* reg = ttp->registerAtUC();
    
    send(reg, "ucLine$o");
}

void TrustedThirdParty::processDataFromSM(cMessage *msg)
{
    cQueue *queue = dynamic_cast<cQueue*> (msg->getObject("queue"));
    
    while (!queue->isEmpty())
    {
	cMessage *message = dynamic_cast<cMessage*> (queue->pop());
	ttp->processDataFromSM(message);
    }
    
    /*
    // Get a queue of messages from different smart meters for a particular collector
    cArray *queueArray = dynamic_cast<cArray*> (msg->getObject("data"));
    // Retrieve every object from the queue
    for(int i = 0; i < queueArray->size(); i++)
    {
        cQueue *cQ = dynamic_cast<cQueue *> (queueArray->get(i));
        while (!cQ->isEmpty())
        {
            cMessage *cMsg = dynamic_cast<cMessage *> (cQ->pop());
            SMPacket *p = dynamic_cast<SMPacket*> (cMsg->getObject("packet"));
            long int id = p->getId();
            double value = p->getValue();
            EV << "ID: " << id << ", value = " << value << endl;
            delete cMsg;
        }
        queueArray->remove(i);
        delete cQ;
    }
    */
    delete msg;
}

} //namespace
