#include <EtherCard.h>

// your variable

#define PATH    "/api/rooms/Arduino/states"

const int pirPin = 7;

// ethernet interface mac address, must be unique on the LAN
byte mymac[] = { 0x74, 0x69, 0x69, 0x2D, 0x38, 0x31 };

const char website[] PROGMEM = "http://192.168.0.14:8765";

bool isOccupied = false;
byte Ethernet::buffer[700];
uint32_t timer;
Stash stash;

void setup () {
  Serial.begin(57600);
  Serial.println("\n[webClient]");

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
    Serial.println( "Failed to access Ethernet controller");
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

  //if (!ether.dnsLookup("google.com"))
  // Serial.println("DNS failed");

  ether.printIp("SRV: ", ether.hisip);
  //  Serial.println("hisport: %D", ether.hisport);

  pinMode(pirPin, INPUT);
}

void loop () {
  ether.packetLoop(ether.packetReceive());

  if (millis() > timer) {
    timer = millis() + 10000;

    isOccupied = digitalRead(pirPin);

    byte sd = stash.create();
    stash.print("{\"isOccupied\":\"");
    stash.print(isOccupied ? "true" : "false");
    stash.print("\"}");
    stash.save();

    // generate the header with payload - note that the stash size is used,
    // and that a "stash descriptor" is passed in as argument using "$H"
    Stash::prepare(PSTR("POST http://192.168.0.14:8765$F HTTP/1.1" "\r\n"
                        "Host: $F" "\r\n"
                        "Content-Length: $D" "\r\n"
                        "Content-Type: application/json" "\r\n"
                        "\r\n"
                        "$H"),
                   PSTR(PATH), website, stash.size(), sd);

    Serial.println("Package ready!");
    Serial.println(sd);
    // send the packet - this also releases all stash buffers once done
    ether.tcpSend();
    Serial.println("Sent the package!");
  }
}
