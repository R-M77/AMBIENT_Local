#include <iostream>
#include <sstream>
#include "include/fedm/ReaderModule.h"
#include "include/fedm/UsbManager.h"

using namespace FEDM;
using namespace FEDM::Utility;
using namespace std;



class UsbManagerConnect : IUsbListener
{
public:
    static UsbManager usbManager;

    static void SampleMain()
    {
        UsbManagerConnect usbManager;
        usbManager.Run();
    }

    void Run()
    {
        // Start USB Discover
        usbManager.startDiscover(this);
        cout << "Waiting for Connection with first USB Reader...." << endl;;

        // Wait for first BLE device
        while (usbManager.readerCount() == 0)
        {
        }

        // Get the Connector by BLE Manager
        UsbScanInfo usbScanInfo = usbManager.popDiscover();
        usbManager.stopDiscover();

        if (!usbScanInfo.isValid())
        {
            cout << "Error: Invalid usb scan info" << endl;
            return;
        }

        Connector connector = usbScanInfo.connector();
        if (connector.isValid())
        {
            // Create Reader Module with Request Mode UniDirectional = Advanced Protocol
            ReaderModule* reader = new ReaderModule(RequestMode::UniDirectional);
            cout << "Start connection with Reader: " << usbScanInfo.readerTypeToString() << " (" << connector.usbDeviceIdToHexString() << ")..." << endl;
            // Connect
            int state = reader->connect(connector);

            cout << "Connect: " << reader->lastErrorStatusText() << endl;
            // Error handling
            if (state != ErrorCode::Ok)
            {
                return;
            }

            // ********************************************************************************
            // THIS PART IS THE PLACEHOLDER FOR TAG ITEM
            // INSERT CODE HERE
            //********************************************************************************

            // Disconnect Reader
            reader->disconnect();

            if (reader->lastError() == ErrorCode::Ok)
            {
                cout << "\n" << reader->info().readerTypeToString() << " disconnected." << endl;
            }
            delete reader;/*  */
        }
    }

    void onUsbEvent()
    {
        // Not used
    }

};

UsbManager UsbManagerConnect::usbManager;

int main()
{
    UsbManagerConnect::SampleMain();

    return 0;
}