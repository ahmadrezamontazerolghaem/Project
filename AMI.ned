
package smart3p.simulations;

import smart3p.SmartMeter;
import smart3p.Collector;
import smart3p.UtilityCompany;
import smart3p.TrustedThirdParty;
import smart3p.DataGenerator;

//
// AMI network
//
network AMI
{
	parameters:
		int smNum = default(10);
		int colNum = default(2);
		volatile int smDelay @unit(ms) = default(exponential(100ms));
	types:
		// datarate is 250 kpbs max for ZigBee, average is ~163 kpsb on 70m distance
		channel ZigBee extends ned.DatarateChannel
		{
			delay = smDelay;
			datarate = 163kbps;
			per = 0.1;
		}
		channel Wireless extends ned.DatarateChannel
		{
			delay = 1s;
			datarate = 30Mbps;
		}
    submodules:
        sms[smNum]: SmartMeter;
        colls[colNum]: Collector;
        uc: UtilityCompany;
        ttp: TrustedThirdParty;
        gen: DataGenerator;
	
    connections:
        for i=0..smNum-1
        {
        	sms[i].generatorLine <--> gen.smLine++;
        	sms[i].radio <--> ZigBee <--> colls[floor(i/(smNum/colNum))].radio++;
        }
        for i=0..colNum-1
        {
        	colls[i].ucLine <--> Wireless <--> uc.radio++;
        }
        uc.ttpLine <--> ttp.ucLine;
}
