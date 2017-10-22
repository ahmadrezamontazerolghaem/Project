#include "UCAdapter.h"
#include "crypto/UtilityCompany.h"
#include <stdlib.h> //for itoa

#ifndef SMPacket
#include <SMPacket_m.h>
#endif

UCAdapter::UCAdapter(smart3p::Unit* o) : Adapter(o)
{
    uc = new SMImp::UtilityCompany();
}

UCAdapter::UCAdapter() : Adapter()
{
    uc = new SMImp::UtilityCompany();
}

UCAdapter::~UCAdapter()
{
    delete uc;
}

omnetpp::cMessage* UCAdapter::registerSM(omnetpp::cMessage *msg)
{
    
    smart3p::SMPacket *p = omnetpp::check_and_cast<smart3p::SMPacket*> (msg->getObject("packet"));
    CryptoPP::Integer* id = new CryptoPP::Integer(p->getId());
    //CryptoPP::Integer* key = check_and_cast<CryptoPP::Integer> (msg->getObject("key"));
    
    uc->registerSM(id);
    
    uc->addSMData(id,new CryptoPP::Integer(p->getSmGateID()));
    uc->addSMData(id,new CryptoPP::Integer(msg->getArrivalGate()->getIndex())); //collgate
    
    // Send the info back to the smart meter
    
    omnetpp::cMessage* message = new omnetpp::cMessage("registerInfoFromUC");
    smart3p::SMPacket* output = new smart3p::SMPacket("packet");
    message->addObject(output);

    output->setSmGateID(p->getSmGateID());

    List<CryptoPP::Integer>* sm = uc->getSM(*id);
    ::cInteger* anonid = new ::cInteger("anonid", sm->get(1));
    
    SMImp::Payload payload = uc->generatePartialKeys(*dynamic_cast<CryptoPP::Integer*>(anonid));
    uc->addSMData(id,&payload.pub); 
    
    output->addObject(anonid);
    output->addObject(new ::cInteger("SMUCKey",sm->get(2)));
    output->addObject(new ::cInteger("p",&payload.params.p));
    output->addObject(new ::cInteger("q",&payload.params.q));
    output->addObject(new ::cInteger("g",&payload.params.g));
    output->addObject(new ::cInteger("x",&payload.params.x));
    output->addObject(new ::cInteger("priv",&payload.priv));
    output->addObject(new ::cInteger("pub",&payload.pub));

    return message;
}

omnetpp::cMessage* UCAdapter::registerTTP(omnetpp::cMessage *msg)
{
    smart3p::SMPacket *p = omnetpp::check_and_cast<smart3p::SMPacket*> (msg->getObject("packet"));
    CryptoPP::Integer* id = new CryptoPP::Integer(p->getId());

    uc->registerTTP(id);

//    uc->addTTPData(id,new CryptoPP::Integer(msg->getArrivalGate()->getIndex()));
    
    delete msg;

    // Send the info back to the smart meter
    //List<CryptoPP::Integer>* ttp = uc->getTTP(*id);
    SMImp::Payload payload = uc->generatePartialKeys(*id);

    omnetpp::cMessage* message = new omnetpp::cMessage("registerInfoFromUC");
    smart3p::SMPacket* output = new smart3p::SMPacket("packet");
    message->addObject(output);
    
    output->addObject(new ::cInteger("p",&payload.params.p));
    output->addObject(new ::cInteger("q",&payload.params.q));
    output->addObject(new ::cInteger("g",&payload.params.g));
    output->addObject(new ::cInteger("x",&payload.params.x));
    output->addObject(new ::cInteger("priv",&payload.priv));
    output->addObject(new ::cInteger("pub",&payload.pub));

    return message;
}

omnetpp::cMessage* UCAdapter::sessionKeyStartFromTTP(omnetpp::cMessage* msg)
{
    smart3p::SMPacket *p = omnetpp::check_and_cast<smart3p::SMPacket*> (msg->getObject("packet"));

    //need to de-anonymize the id
    CryptoPP::Integer* anonId = dynamic_cast<CryptoPP::Integer*>(p->getObject("smid"));
    List<CryptoPP::Integer>* sm = uc->getSMbyAnonId(anonId);
    
    //set gates for transmission
    //CryptoPP::Integer* id = sm->get(0); //unnessisary
    p->setSmGateID(sm->get(3)->ConvertToLong());
    p->setCollGateID(sm->get(4)->ConvertToLong());

    return msg;
}

bool UCAdapter::sessionKeyExchangeFromTTP(omnetpp::cMessage *msg)
{
    smart3p::SMPacket* data = omnetpp::check_and_cast<smart3p::SMPacket*> (msg->getObject("packet"));
        
    SMImp::HMACPayload pl;

    pl.id = *dynamic_cast<CryptoPP::Integer*>(data->getObject("id"));
    pl.hmac = *dynamic_cast<CryptoPP::Integer*>(data->getObject("hmac"));
    pl.c1 = *dynamic_cast<CryptoPP::Integer*>(data->getObject("c1"));
    pl.c2 = *dynamic_cast<CryptoPP::Integer*>(data->getObject("c2"));
    pl.timeStamp = *dynamic_cast<CryptoPP::Integer*>(data->getObject("timeStamp"));

    CryptoPP::Integer* anonId = dynamic_cast<CryptoPP::Integer*>(data->getObject("smId"));

    if (uc->verifyHMAC(*anonId, pl))
    {
	List<CryptoPP::Integer>* sm = uc->getSMbyAnonId(anonId);
	data->setSmGateID(sm->get(3)->ConvertToLong());
	data->setCollGateID(sm->get(4)->ConvertToLong());

	return true;
    }

    return false;
}

bool UCAdapter::sessionKeyExchangeFromSM(omnetpp::cMessage *msg)
{
    smart3p::SMPacket* data = omnetpp::check_and_cast<smart3p::SMPacket*> (msg->getObject("packet"));
    
    SMImp::HMACPayload pl;

    pl.id = *dynamic_cast<CryptoPP::Integer*>(data->getObject("id"));
    pl.hmac = *dynamic_cast<CryptoPP::Integer*>(data->getObject("hmac"));
    pl.c1 = *dynamic_cast<CryptoPP::Integer*>(data->getObject("c1"));
    pl.c2 = *dynamic_cast<CryptoPP::Integer*>(data->getObject("c2"));
    pl.timeStamp = *dynamic_cast<CryptoPP::Integer*>(data->getObject("timeStamp"));

    List<CryptoPP::Integer>* list = uc->getSM(pl.id);
    
    if (uc->verifyHMAC(*list->get(2),pl))
    {
	data->removeObject("id");
	data->addObject(new ::cInteger("id",list->get(1)));

	return true;
    }
    return false;
}

bool UCAdapter::energyConsumptionProcessing(omnetpp::cMessage *msg)
{
    omnetpp::cQueue* dataQueue = omnetpp::check_and_cast<omnetpp::cQueue*> (msg->removeObject("dataToUC"));

    omnetpp::cQueue* outputQueue = new omnetpp::cQueue("queue");

    while (!dataQueue->isEmpty())
    {
	omnetpp::cMessage* message = omnetpp::check_and_cast<omnetpp::cMessage*> (dataQueue->pop());
	if (sessionKeyExchangeFromSM(message)) outputQueue->insert(message);
	//else delete message;
    }
    
    delete dataQueue;

    msg->addObject(outputQueue);
    
    return !outputQueue->isEmpty();
}
