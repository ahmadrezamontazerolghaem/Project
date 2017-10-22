
#ifndef __SMART3P_DATAGENERATOR_H_
#define __SMART3P_DATAGENERATOR_H_

#include <omnetpp.h>
#include <map>
#include <queue>

namespace smart3p {

using namespace omnetpp;

//! Generates data for smart3p::SmartMeter
class DataGenerator : public omnetpp::cSimpleModule
{
  private:
    /*!
      List of booleans where every index refers to the index of a smart3p::SmartMeter and its ready state.
    */
    bool* readyList;

    /*!
      Broadcasts data to any ready smart3P::SmartMeter.
    */
    bool sendDataToSmartMeters();

    /*!
      Filename of the file to read data from. Specified in DataGenerator.ned.
      \sa readData(char* f)
    */
    char* dataFile;
    
    /*!
      Reads data from file to the output stream which will then be sent broadcast to any ready smart3P::SmartMeter.
    */
    bool readData(char* f);

    /*!
      Data send timer.
    */
    cMessage *timer;
    /*!
      Keeps track of the number of smart3p::SmartMeter.
    */
    int smCounter;
    // map<ID, energy_measurements>
    /*!
      Currently unused, for the future will enable more complex data to be transmitted to smart3p::SmartMeter.
    */
    std::map<int, std::queue<double> > ECdata;

    /*! 
      Output queue containing the information to be sent to smart3p::SmartMeter.
    */
    std::queue<double>* outputQueue;

    /*!
      Sends data at the front of the outputQueue to all ready smart3p::SmartMeter.
    */
    void sendDataToSmartMeter(cMessage *msg);
    
  protected:
    //! Class initilization.
    virtual void initialize();
    //! Main message handler.
    virtual void handleMessage(cMessage *msg);
    //! Called when simulation is shut down.
    virtual void finish();
};

} //namespace

#endif
