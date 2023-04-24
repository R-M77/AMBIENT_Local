#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>
#include <set>
#include "include/fedm/ReaderModule.h"
#include "include/fedm/UsbManager.h"

using namespace FEDM;
using namespace std;

class Inventory_TagItem_withRSSI_Sample
{
public:
    static void run(ReaderModule &reader)
    {
        // Inventory with default UsageMode = UseTable
        // reader.hm().setUsageMode(UsageMode.UseTable);

        InventoryParam inventoryParam;

        // Set antenna to ANT1 and ANT2
        // Antenna must be set to set mode to "Request with antenna number" for RSSI
        inventoryParam.setAntennas((uint8_t)0x03);

        int state = reader.hm().inventory(true, inventoryParam);
        //cout << "inventory: " << reader.lastErrorStatusText() << endl;
        cout << "inventory: " << state << endl;
        if (state != ErrorCode::Ok) { /* Add error-handling... */ }

        // Number of read tags
        size_t count = reader.hm().itemCount();
        std::cout << "No. of tags: " << count << std::endl;

        // Create TagItem for each tag and output IDD and RSSI values
        for (size_t itemIndex = 0; itemIndex < count; itemIndex++)
        {
            // Create TagItem
            TagItem tagItem = reader.hm().tagItem(itemIndex);
            if (!tagItem.isValid()) { /* Add error handling */ cout << "Error: tag table empty"; return; }
            // Output IDD of TagItem
            string iddString = tagItem.iddToHexString();
            std::cout << "IDD: " << iddString << std::endl;

            // Create List for RSSI values of TagItem
            vector<RssiItem> rssiListe = tagItem.rssiValues();
            vector<RssiItem>::iterator itor;

            for (RssiItem rssiItem : rssiListe)
            {
                // Antenna Number
                int antennaNumber = rssiItem.antennaNumber();
                std::cout << "Ant: " << antennaNumber;

                // RSSI Value
                int rssi = rssiItem.rssi() * -1;
                std::cout << " | RSSI: " << rssi << std::endl;
            }
        }
    }
};

int main()
{
    // Initialize UsbManager
    UsbManager usbManager;

    // Check for connected readers
    // if (usbManager.getReaderCount() == 0) { /* Add error-handling... */ }

    // Get first connected reader
    // ReaderModule reader = usbManager.getReader(0);

    // Open reader
    // if (reader.open() != ErrorCode::Ok) { /* Add error-handling... */ }

    // Run inventory and output results
    // Inventory_TagItem_withRSSI_Sample::run(reader);

    // Close reader
    // reader.close();

    return 0;
}
