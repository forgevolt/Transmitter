#include <Streaming.h>
#include "Transmitter.h"

// ---- Transmitter -----------------------------------------------------------------------

// A line printed from an ESP-NOW receive callback costs about 15 ms of wire time at 115200
// baud, and Serial blocks once its TX buffer fills - time the WiFi task spends not processing
// frames. The sites that can fire on every received frame report no more often than this.
static constexpr unsigned long cLogIntervalMs = 1000;

// True at most once per cLogIntervalMs, against the caller's own timestamp. The first call
// always reports, so a fault is never silent.
static bool logDue(unsigned long& lastMs)
{
  const unsigned long now = millis();

  if (lastMs != 0 && now - lastMs < cLogIntervalMs)
    return false;

  lastMs = now;
  return true;
}

// ----------------------------------------------------------------------------------------
Transmitter::Transmitter(EDeviceType device)
: myDevice(device)
{
  if (myDevice == eTransmitter1Joy)
    myDeviceName = "1-Transmitter";
  else if (myDevice == eTransmitter2Joy)
    myDeviceName = "2-Transmitter";
  else
    myDeviceName = "Transmitter";

  myMutex = xSemaphoreCreateMutex();
}

// ----------------------------------------------------------------------------------------
Transmitter::~Transmitter()
{
  vSemaphoreDelete(myMutex);
}

// ----------------------------------------------------------------------------------------
Transmitter::PeerData Transmitter::getPeerData()
{
  // isPaired() takes the base class's peer lock, so it is called before myMutex: the two are
  // never held at once.
  const bool paired = isPaired();

  xSemaphoreTake(myMutex, portMAX_DELAY);

  // No more peer -> reset old values
  if (paired == false && myPeerDataValid == true)
  {
    myPeerDataValid = false;
    myPeerData = PeerData(); // reset to default-constructed (empty) values
  }

  PeerData ret = myPeerData;

  xSemaphoreGive(myMutex);

  return ret;
}

// ----------------------------------------------------------------------------------------
void Transmitter::onPairingRequestMsg(const PairingRequestData& pd, const uint8_t src[6])
{
  // A pairing request arrives from a device we have no record of, so src - the frame's source
  // address - is all that identifies it.
  static unsigned long lastRequestLogMs = 0;
  if (logDue(lastRequestLogMs) == true)
  {
    Serial << __PRETTY_FUNCTION__  << " -> PairingRequestMsg received from "
           << mac2string(src)
           << " (" << pd.device << ":" << pd.name << ")" << endl;
  }

  PeerInfo existing;
  bool isCurrentPeer = getPeer(src, existing);

  // Single-peer design: reject a request from a different receiver while already
  // paired, rather than silently accumulating peers we'd never actually talk to.
  if (isPaired() == true && isCurrentPeer == false)
  {
    static unsigned long lastIgnoredLogMs = 0;
    if (logDue(lastIgnoredLogMs) == true)
    {
      Serial << __PRETTY_FUNCTION__ << " -> already paired -> ignoring request from "
             << mac2string(src) << endl;
    }
    return;
  }

  // Respond
  PairingResponseData response;
  response.device = myDevice;
  copyStringToBuffer(response.name, sizeof(response.name), myDeviceName);

  // pd.name is passed straight through: setPeer() takes the fixed-width wire field and copies it
  // bounded, so no NUL terminator is assumed.
  if (isCurrentPeer == false && setPeer(src, pd.device, pd.name) == false)
  {
    // Could not set the peer (e.g. esp_now_add_peer failed)
    Serial << __PRETTY_FUNCTION__ << " -> Failed to set peer " << mac2string(src) << endl;
    return;
  }

  send(response, src);
}

// ----------------------------------------------------------------------------------------
void Transmitter::onAppMsg(uint8_t msgType, const void* data, size_t len, const PeerInfo& peer)
{
  // dataRecvCB dispatches this only after confirming the frame came from our paired peer, and
  // hands over the PeerInfo it looked up. It knows nothing about this protocol's frames, so
  // the type, the length and the payload version are all checked here.
  if (msgType != eTelemetry)
    return;

  TelemetryData td;
  if (copyFrameTo(data, len, td) == false)
  {
    static unsigned long lastInvalidLengMs = 0;
    if (logDue(lastInvalidLengMs) == true)
    {
      Serial << __PRETTY_FUNCTION__ << " -> invalid len for TelemetryData: " << len
            << ", expecting: " << sizeof(TelemetryData) << endl;
    }
    return;
  }

  if (td.protocolVersion != cAppProtocolVersion)
  {
    static unsigned long lastVersionLogMs = 0;
    if (logDue(lastVersionLogMs) == true)
    {
      Serial << __PRETTY_FUNCTION__ << " -> app protocol version mismatch: received "
             << int(td.protocolVersion) << ", expected " << int(cAppProtocolVersion)
             << " - the two ends were built from different revisions of RCProtocol.h" << endl;
    }
    return;
  }

  xSemaphoreTake(myMutex, portMAX_DELAY);

  copyFieldToCString(myPeerData.name, sizeof(myPeerData.name), peer.name, cNameLen);

  for (size_t i = 0; i < cNumSwitches; i++)
    memcpy(myPeerData.switchLabels[i], td.switchLabels[i], cLabelLen);
  for (size_t i = 0; i < cNumButtons; i++)
    memcpy(myPeerData.btnLabels[i], td.btnLabels[i], cLabelLen);

  for (size_t i = 0; i < cNumMessages; i++)
  {
    memcpy(myPeerData.msgLabels[i], td.messages[i][0], cLabelLen);
    memcpy(myPeerData.msgInfo[i],   td.messages[i][1], cLabelLen);
  }

  myPeerDataValid = true;

  xSemaphoreGive(myMutex);
}
