#pragma once

#include "RCProtocol.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ---- Transmitter -----------------------------------------------------------------------
// Contains the logic specific to the transmitter. It acts as the server that waits for
// a peer (receiver) to show up "on air".
//
// Pairing is deliberately simple: unencrypted, and with the first receiver that asks while the
// transmitter is not paired. ESP-NOW's encrypted peers (PMK/LMK) were judged not worth the setup.

class Transmitter : public ESPNowConnection
{
  public:
    // Telemetry data received from a peer.
    //
    // Each buffer is one byte wider than the wire field it mirrors, and every member is
    // zero-initialized. Copies in write exactly the wire width, so the final byte stays '\0':
    // the strings are NUL-terminated by construction, and a full-width label is not truncated.
    struct PeerData
    {
      char name[cNameLen + 1] = {};
      char switchLabels[cNumSwitches][cLabelLen + 1] = {};
      char btnLabels[cNumButtons][cLabelLen + 1] = {};
      char msgLabels[cNumMessages][cLabelLen + 1] = {};
      char msgInfo[cNumMessages][cLabelLen + 1] = {};
    };

  public:
    explicit Transmitter(EDeviceType device);
    ~Transmitter() override;

    // Returns a copy of the latest telemetry data received from the peer.
    // Access is thread-safe: onAppMsg() is invoked from the ESP-NOW receive
    // callback (WiFi task context), while this is typically called from loop().
    PeerData getPeerData();

  protected:
    // From class ESPNowConnection
    void onPairingRequestMsg(const PairingRequestData& pd, const uint8_t src[6]) override;
    void onAppMsg(uint8_t msgType, const void* data, size_t len, const PeerInfo& peer) override;

  private:
    EDeviceType myDevice; // eTransmitter1Joy or eTransmitter2Joy
    String myDeviceName;

    // Telemetry data received from the (single) paired peer.
    PeerData myPeerData;

    // True once telemetry has been stored in myPeerData. Only purpose is to make the clear-down in
    // getPeerData() happen once when the peer goes away, rather than on every call while unpaired.
    bool myPeerDataValid = false;

    // Protects myPeerData / myPeerDataValid, written from onAppMsg() (ESP-NOW
    // receive callback, WiFi task context) and read from getPeerData() (typically
    // called from loop()). Lock ordering rule: never call anything that takes the base
    // class's peer lock (isPaired(), getPeer(), setPeer(), send()) while holding this one.
    SemaphoreHandle_t myMutex = nullptr;
};
