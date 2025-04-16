//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package mqtt;

import mqtt.MqttApp;

module MqttClient extends MqttApp
{
	parameters:
	    @class(MqttClient);
	    
	    int keepAlive @unit(s) = default(30s);
	    bool cleanSession = default(false);
	    double reconnectInterval @unit(s) = default(5s);
    	double publishInterval @unit(s) = default(1s);
    	double subscriptionInterval @unit(s) = default(1s);
    	double unsubscriptionInterval @unit(s) = default(10s);
    	
    	int publishDataSize = default(100); // bytes
    	
    	int serverPort;
    	string serverAddress;

	    string items;
}