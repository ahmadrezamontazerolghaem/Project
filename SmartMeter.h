
#ifndef __SMART3P_SMARTMETER_H
#define __SMART3P_SMARTMETER_H
#include "Unit.h"
#include <omnetpp.h>


class SMAdapter;

namespace smart3p {

using namespace omnetpp;

//! Simulation Smart Meter class
class SmartMeter : public Unit
{
  private:
    //state
    /*! 
      Contains the amount of time to delay the simulation.
      \sa addSimTime(double)
    */
    simtime_t curDelay;

    /*! Simulation time of the last successful outgoing message */
    simtime_t lastArrival;

    long int smID;

    /*! Energy data output queue */
    cQueue *energyDataQueue;

    /*! Internal message to start sending more energy consumption data. 
     \sa sendEnergyConsumption(cMessage *msg) */
    cMessage *startSendingEnergyData;
    
    // statistics
    /*! \sa log(simsignal_t id, double value) and log(char* tag, double value)*/
    simsignal_t stackSizeSignal;
    /*! \sa log(simsignal_t id, double value) and log(char* tag, double value)*/
    simsignal_t sessionKeyDecryptionTimeSignal;
    /*! \sa log(simsignal_t id, double value) and log(char* tag, double value)*/
    simsignal_t sessionKeyEncryptionTimeSignal;
    /*! \sa log(simsignal_t id, double value) and log(char* tag, double value)*/
    simsignal_t dataEncryptionTimeSignal;
    /*! \sa log(simsignal_t id, double value) and log(char* tag, double value)*/
    cDoubleHistogram iaTimeHistogram;
    /*! \sa log(simsignal_t id, double value) and log(char* tag, double value)*/
    cOutVector arrivalsVector;

    // functionality
    //! Initilization
    /*! Initilizes all the variables and signals for logging */
    void smartMeterInit(cMessage *msg);

    /*! Registers the smart3p::SmartMeter with smart3p::UtilityCompany. */
    void registerAtUC();
    /*! Registers the key data from smart3p::UtilityCompany. */
    void registerInfoFromUC(cMessage *msg);
    /*! Recieves the data from TTP and sends the new session key back. */
    void startSessionKeyExchange(cMessage *msg);
    /*! Finishes session key registration and verification. */
    void endOfSessionKeyExchange(cMessage *msg);
    /*! Puts the next message onto the energyDataQueue. */
    void sendEnergyConsumption(cMessage *msg);
    /*! Encapsulates energy data and sends it to smart3p::Collector */    
    void sendQueueDataToCollector();

    //! adapter
    /*! Pointer to the adapter. */
    SMAdapter* sm;

  protected:
    //! Initializer
    virtual void initialize();
    //! Message Handler
    virtual void timedHandleMessage(cMessage *msg);
    //! Deconstructor
    virtual void finish();

public:
    //emit
    /*!
      Emits statistical data to be registered by the simulation.
      \param id simsignal_t id of the data collection to put value into.
      \param value The value to add to the data collection identified by id.
      \sa log(char* tag, double value)
    */
    void log(simsignal_t id, double value); //simsignal_t is just type def'ed as an int
    /*! 
      Emits statistical data to be registered by the simulation.
      \param tag The c-string which corresponds to the id of the data collection to add data to.
      \param value The value to add to the data collection identified by tag.
      \sa log(simsignal_t id, double value)
    */
    void log(char* tag, double value);

    /*!
      Adds additional time to the simulation based upon the amount of time taken to perform protocol-level processing.

      Omnet++ treats all computations as taking no time at all in the simulation. So to properly record statistical data we have to add additional time to the simulation.
      \param t Time to add to the simulation.
     */
    void addSimTime(double t); //sets the delay to add to sim time for next scheduled event
};

}; // namespace

#endif
