// measureLatency - record RC_CHANNELS_PACKED inter-frame latency and link drops
//
// "Latency" here is the time gap between two successive RC channel frames
// from the receiver. At a steady packet rate it equals the rate's period
// (e.g. ~6.66 ms at 150 Hz, 2 ms at 500 Hz). Any spike above the period
// indicates the receiver missed an OTA packet or the link is degrading.
//
// A "connection drop" is counted every time isLinkUp() transitions from
// true to false, which happens after CRSF_FAILSAFE_STAGE1_MS (300 ms)
// without a channels packet.

#include <AlfredoCRSF.h>
#include <HardwareSerial.h>

#define PIN_RX 7
#define PIN_TX 8

#define PRINT_INTERVAL_MS 1000

HardwareSerial crsfSerial(1);
AlfredoCRSF crsf;

uint32_t prevPacketTime = 0;
uint32_t lastIntervalMs = 0;
uint32_t maxIntervalMs = 0;
uint32_t packetCount = 0;
uint32_t dropCount = 0;
bool prevLinkUp = false;
uint32_t lastPrintMs = 0;

void setup()
{
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("AlfredoCRSF latency monitor");

  crsfSerial.begin(CRSF_BAUDRATE, SERIAL_8N1, PIN_RX, PIN_TX);
  if (!crsfSerial) while (1) Serial.println("Invalid crsfSerial configuration");

  crsf.begin(crsfSerial);
}

void loop()
{
  crsf.update();

  // Detect a new RC_CHANNELS_PACKED frame by watching the timestamp
  // exposed by the library.
  uint32_t pktTime = crsf.getLastChannelsPacketTime();
  if (pktTime != prevPacketTime)
  {
    if (prevPacketTime != 0)
    {
      // Skip the first interval if the link was previously down: the
      // gap then reflects the outage, which is already counted as a
      // drop below.
      if (prevLinkUp)
      {
        lastIntervalMs = pktTime - prevPacketTime;
        if (lastIntervalMs > maxIntervalMs) maxIntervalMs = lastIntervalMs;
      }
    }
    prevPacketTime = pktTime;
    packetCount++;
  }

  // Count link-up -> link-down transitions as connection drops.
  bool linkUp = crsf.isLinkUp();
  if (prevLinkUp && !linkUp) dropCount++;
  prevLinkUp = linkUp;

  uint32_t now = millis();
  if (now - lastPrintMs >= PRINT_INTERVAL_MS)
  {
    lastPrintMs = now;
    Serial.print("link=");
    Serial.print(linkUp ? "UP  " : "DOWN");
    Serial.print(" pkts=");
    Serial.print(packetCount);
    Serial.print(" last=");
    Serial.print(lastIntervalMs);
    Serial.print("ms max=");
    Serial.print(maxIntervalMs);
    Serial.print("ms drops=");
    Serial.println(dropCount);
  }
}
