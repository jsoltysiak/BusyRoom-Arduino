#include <DHT.h>
#include <EtherCard.h>

const int PIN_PIR = 7;
const int PIN_DHT = 2;

// ethernet interface mac address, must be unique on the LAN
const byte mymac[] = { 0x74, 0x69, 0x69, 0x2D, 0x38, 0x31 };

const uint8_t IP[] = { 192,168,0,14 };
const uint16_t PORT = 3333;
const char ADDRESS[] PROGMEM = "192.168.0.14:3333";
const char PATH[] PROGMEM = "api/rooms/Arduino/states";

byte Ethernet::buffer[700];
uint32_t timer;
Stash stash;
byte session;

DHT dht;
float temperature;
float humidity;
bool pirSensorState = false;


void setup() {
    Serial.begin(57600);
    Serial.println("\n[BusyRoom-Arduino]");

    initializeEthernet();

    pinMode(PIN_PIR, INPUT);
    dht.setup(2, DHT::DHT11);

    timer = millis() + 3000;
}

void loop() {
    ether.packetLoop(ether.packetReceive());

    if (millis() > timer) {
        timer = millis() + dht.getMinimumSamplingPeriod();

        pirSensorState = digitalRead(PIN_PIR);

        temperature = dht.getTemperature();
        humidity = dht.getHumidity();

        Serial.print("pirSensorState: ");
        Serial.println(pirSensorState);
        Serial.print("temperature: ");
        Serial.println(temperature);
        Serial.print("humidity: ");
        Serial.println(humidity);

        sendSensorReadings(pirSensorState, temperature, humidity);
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

    ether.hisip[0] = IP[0];
    ether.hisip[1] = IP[1];
    ether.hisip[2] = IP[2];
    ether.hisip[3] = IP[3];

    ether.hisport = PORT;

    ether.printIp("Target IP: ", ether.hisip);
    Serial.print("Target port: ");
    Serial.println(ether.hisport);
}

void sendSensorReadings(bool isOccupied, float temperature, float humidity)
{
    byte sd = stash.create();
    stash.print("{");
    stash.print("\"isOccupied\":\"");
    stash.print(isOccupied ? "true" : "false");
    stash.print(",");
    stash.print("\"temperature\":\"");
    stash.print(temperature);
    stash.print("\",");
    stash.print("\"humidity\":\"");
    stash.print(humidity);
    stash.print("\"");
    stash.print("}");
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
        ADDRESS, PATH, ADDRESS, stash.size(), sd);

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