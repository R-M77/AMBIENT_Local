#include <iostream>
#include "ReaderModule.h"
//-------------- @snippet name='Connect_Basic_BasicTcpConnect' --------------
using namespace FEDM;
using namespace TagHandler;
using namespace std;

class BasicTcpConnect
{
public:
    void run()
    {
        // Create Reader Module with Request Mode UniDirectional = Advanced Protocol
        unique_ptr<ReaderModule> reader = unique_ptr<ReaderModule>(new ReaderModule(RequestMode::UniDirectional));

        int returnCode = ErrorCode::Ok;

        // TCP Port Settings
        string ipAddr = "192.168.10.10";
        int port = 10001;

        // Create a TCP Connector Object
        Connector connTCP = Connector::createTcpConnector(ipAddr, port);

        // Intro
        cout << "-----  FEIG - BasicTCPConnect Code  -----" << endl;

        // Connect TCP-Reader
        cout << "Start connection with Reader: " << connTCP.tcpIpAddress() << endl;
        returnCode = reader->connect(connTCP);

        if (returnCode < ErrorCode::Ok)
        { // Error codes from library
            cout << "Error from library - Code= " << returnCode << " - Errortext: " << reader->lastErrorText() << endl;
            exit(0);
        }
        else if (returnCode > ErrorCode::Ok)
        { // Status/Error codes from reader (find details in the annex of the reader's system manual)
            cout << "Status from reader: " << returnCode << " - Errortext: " << reader->lastStatusText() << endl;
            exit(0);
        }


        // Output ReaderType
        cout << "Reader " << reader->info().readerTypeToString() << " connected." << endl;

        //********************************************************************************
        // add Sample Code here
        //********************************************************************************

        // Disconnect Reader
        returnCode = reader->disconnect();

        if (returnCode == ErrorCode::Ok)
        {
            cout << "Reader: " << reader->info().readerTypeToString() << " disconnected." << endl;
            cout << endl;
        }
    }
};
//-------------- @/snippet --------------
int main() {
        BasicTcpConnect tcpSample;
        tcpSample.run();
        // print USB info
        // ... complete section

}