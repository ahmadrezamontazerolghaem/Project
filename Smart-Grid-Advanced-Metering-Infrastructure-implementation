
package smart3p;

//
// Sends energy consumption to collectors
//
simple SmartMeter
{
    parameters:
        double per = default(0.1);
		
	@signal[stackSize](type="double");
	@statistic[stackSize](title="Size of the outbound message stack"; source="stackSize"; record=histogram,stats; interpolationmode=none);

	@signal[sessionKeyDecryptionTime](type="double");
	@statistic[sessionKeyDecryptionTime](title="Decryption time using std::chrono"; source="sessionKeyDecryptionTime"; record=histogram,stats; interpolationmode=none);

	@signal[sessionKeyEncryptionTime](type="double");
	@statistic[sessionKeyEncryptionTime](title="Encryption time using std::chrono"; source="sessionKeyEncryptionTime"; record=histogram,stats; interpolationmode=none);

	@signal[dataEncryptionTime](type="double");
	@statistic[dataEncryptionTime](title="Encryption time using std::chrono"; source="dataEncryptionTime"; record=histogram,stats; interpolationmode=none);
	
        @display("i=block/sink");
        volatile double sendInterval @unit(s) = default(exponential(1s));
    gates:
        inout generatorLine;
        inout radio;
}
