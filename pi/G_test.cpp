#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "include/fedm/ReaderModule.h"
#include "include/fedm/UsbManager.h"

using namespace FEDM;
using namespace FEDM::Utility;
using namespace std;

// vector<string> tagList;

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
            // RFID Tag Reading Functionality
            //********************************************************************************

            // Inventory tags
            state = reader->hm().inventory();
            cout << "inventory: " << reader->lastErrorStatusText() << endl;
            if (state != ErrorCode::Ok) { /* Add error-handling... */ }

            // Get tag count
            size_t count = reader->hm().itemCount();

            // Output number of tags found
            std::cout << "No. of tags: " << count << std::endl;

            // Save data to CSV file
            if (count > 0) {
                // Open file for appending
                ofstream file;
                file.open("tag_data.csv", std::ios::app);
                
                // Create TagItem for each tag and output IDD
                for (size_t itemIndex = 0; itemIndex < count; itemIndex++)
                {
                    // Create TagItem
                    TagItem tagItem = reader->hm().tagItem(itemIndex);
                    if (!tagItem.isValid()) { /* Add error handling */ cout << "Error: tag table empty"; return; }
                    
                    // Get IDD of TagItem
                    string iddString = tagItem.iddToHexString();
                    
                    // Display IDD on screen
                    std::cout << "IDD: " << iddString << std::endl;
                    
                    // Save data to CSV file
                    file << iddString << "," << time(0) << endl;
                }
                
                // Close file
                file.close();
            }

            // Disconnect Reader
            reader->disconnect();

            if (reader->lastError() == ErrorCode::Ok)
            {
                cout << "\n" << reader->info().readerTypeToString() << " disconnected." << endl;
            }
            delete reader;
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