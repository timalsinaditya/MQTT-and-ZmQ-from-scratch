package mqtt;

import inet.applications.contract.IApp;

simple MqttApp like IApp
{
	parameters:
		@display("i=block/app");
		@class(MqttApp);
		@lifecycleSupport;
		
		string localAddress = default("");
		string broadcastAddress = default("255.255.255.255");
		
		int localPort = default(8000);
		int destPort = default(-1);
		
		double retransmissionInterval @unit(s) = default(10s);
		int maxRetransmissions = default(3);
		
		double packetBER = default(0.1);
		
	gates:
	    input socketIn;
	    output socketOut;
}