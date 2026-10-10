#pragma once

#include "Transmitter.h"
#include "DisplayObject.h"

// ---- ReceiverStatus --------------------------------------------------------------------
// Displays whether a receiver is currently paired (and its name & icon, if so).

class ReceiverStatus : public DisplayObject
{
  public:
    ReceiverStatus(UserInterface& ui, Transmitter& transmitter);
    ~ReceiverStatus() override {};

    // From class DisplayObject
    void step(unsigned long dt) override;
    void draw(bool forceRedraw = false) override;

  private:
    // Draws the message lines next to the receiver's image: all of them, or only those whose bit
    // is set in myChangedLines.
    void drawMessages(bool changedOnly);

    // Reference to the transmitter
    Transmitter& myTransmitter;

    // false is a legitimate real starting value here (the device boots unpaired), so -
    // unlike myPeerDevice below - no artificial "impossible" sentinel is needed to
    // guarantee correct behavior on the very first step() call.
    bool myLastPaired = false;

    // eUndefined can never equal a real device value, so the very first step() call
    // always sees a "change" and triggers a redraw
    EDeviceType myPeerDevice = eUndefined;

    // Name and messages from peer (receiver)
    Transmitter::PeerData myPeerData;
    bool myShowMessages = false;
    uint32_t myChangedLines = 0; // bit i: message line i changed since it was last drawn
};