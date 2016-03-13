#include <EtherCard.h>


#define PATH "api/rooms/Arduino/states"

// ethernet interface mac address, must be unique on the LAN
byte mymac[] = { 0x74, 0x69, 0x69, 0x2D, 0x38, 0x31 };

const char website[] PROGMEM = "192.168.0.14:8765";

byte Ethernet::buffer[700];
uint32_t timer;
Stash stash;
byte session;

const int pirPin = 7;
bool pirSensorState = false;


void setup() {
    Serial.begin(57600);
    Serial.println("\n[BusyRoom-Arduino]");

    initializeEthernet();

    pinMode(pirPin, INPUT);
}

void loop() {
    ether.packetLoop(ether.packetReceive());

    if (millis() > timer) {
        timer = millis() + 10000;

        pirSensorState = digitalRead(pirPin);

        sendSensorReadings(pirSensorState);
    }

    getAndPrintReply();
}

void initializeEthernet()
{
    if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
        Serial.println("Failed to access Ethernet controller");
    if (!ether.dhcpSetup())
        Serial.println("DHCP failed");

    ether.printIp("IP:  ", ether.myip);
    ether.printIp("GW:  ", ether.gwip);
    ether.printIp("DNS: ", ether.dnsip);

    ether.hisip[0] = 192;
    ether.hisip[1] = 168;
    ether.hisip[2] = 0;
    ether.hisip[3] = 14;

    ether.hisport = 8765;

    ether.printIp("Target IP: ", ether.hisip);
    Serial.print("Target port: ");
    Serial.println(ether.hisport);
}

void sendSensorReadings(bool isOccupied)
{
    byte sd = stash.create();
    stash.print("{\"isOccupied\":\"");
    stash.print(isOccupied ? "true" : "false");
    stash.print("\"}");
    stash.save();

    // generate the header with payload - note that the stash size is used,
    // and that a "stash descriptor" is passed in as argument using "$H"
    Stash::prepare(
        PSTR(
            "POST http://$F/$F HTTP/1.1" "\r\n"
            "Host: $F" "\r\n"
            "Content-Length: $D" "\r\n"
            "Content-Type: application/json" "\r\n"
            "\r\n"
            "$H"),
        website, PSTR(PATH), website, stash.size(), sd);

    // send the packet - this also releases all stash buffers once done
    session = ether.tcpSend();
    Serial.println("Sent the package!");
}

void getAndPrintReply()
{
    const char* reply = ether.tcpReply(session);

    if (reply != 0) {
        Serial.println("");
        Serial.println("Reply from the server:");
        Serial.println(reply);
        Serial.println("");
    }
}