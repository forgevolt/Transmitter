#include "ReceiverStatus.h"

#include <ESPNowUtilities.h>
#include "UserInterface.h"
#include <TFT_eSPI.h>
#include "images/imgHexapod.h"
#include "images/imgUnknownReceiver.h"

#include <Streaming.h>

// ---- ReceiverStatus --------------------------------------------------------------------

namespace
{

const uint32_t cPairedCol    = 0x4CBB; 
const uint32_t cNotPairedCol = 0xE0E3;

const float cPosX = 430;
const float cPosY = 40; 

const int32_t cIconRectPosX   = 80;
const int32_t cIconRectPosY   = cMainAreaPosY+55;
const int32_t cIconRectHeight = 120;
const int32_t cIconRectWidth  = cTFTWidth-2*cIconRectPosX;

// Message lines: label left-aligned at cMsgLeftX, value right-aligned at cMsgRightX, font 2.
const int32_t cMsgLeftX  = cIconRectPosX+cIconRectWidth-130;
const int32_t cMsgRightX = cIconRectPosX+cIconRectWidth-10;

static_assert(cNumMessages <= 32, "myChangedLines has one bit per message line");

} // namespace

// ----------------------------------------------------------------------------------------
ReceiverStatus::ReceiverStatus(UserInterface& ui, Transmitter& transmitter)
: DisplayObject(ui),
  myTransmitter(transmitter)
{}

// ----------------------------------------------------------------------------------------
void ReceiverStatus::step(unsigned long dt)
{
  if (isActive() == false)
    return;

  bool paired = myTransmitter.isPaired();

  if (myLastPaired != paired)
  {
    markRedraw();
    myLastPaired = paired; 
  } 

  ESPNowConnection::PeerInfo pi;
  bool havePeer = paired && myTransmitter.getPeerInfo(pi);

  if (havePeer == true && myPeerDevice != static_cast<EDeviceType>(pi.device))
  {
    markRedraw();
    myPeerDevice = static_cast<EDeviceType>(pi.device);     
  }

  // Updated messages from peer (receiver)? Only meaningful while paired, so this is
  // skipped entirely (rather than run and discarded) when there's no peer.
  if (havePeer == true)
  {
    Transmitter::PeerData peerData = myTransmitter.getPeerData(); 

    bool showMessages = false;
    for (int i=0; i<int(cNumMessages); i++)
    {
      if (strncmp(myPeerData.msgLabels[i], peerData.msgLabels[i], cLabelLen) != 0 ||
          strncmp(myPeerData.msgInfo[i],   peerData.msgInfo[i],   cLabelLen) != 0)
      {
        myChangedLines |= (1u << i);
      }

      if (strlen(peerData.msgLabels[i]) > 0)
        showMessages = true;
    }

    // Messages appearing or disappearing move the image: redraw the panel. A changed text only
    // needs its line redrawn - values such as the robot's height change several times a second,
    // and redrawing the whole panel each time would flicker.
    if (showMessages != myShowMessages)
      markRedraw();
    else if (myChangedLines != 0)
      markPartialRedraw();

    myShowMessages = showMessages;
    myPeerData = peerData;
  }  
}

// ----------------------------------------------------------------------------------------
void ReceiverStatus::draw(bool forceRedraw)
{
  if (isActive() == false)
    return;

  if (forceRedraw == false && needsRedraw() == false && needsPartialRedraw() == false)
    return;

  // ---- Partial redraw: only the message lines changed
  if (forceRedraw == false && needsRedraw() == false)
  {
    clearRedrawFlags();

    if (myLastPaired == true && myShowMessages == true)
      drawMessages(true);

    myChangedLines = 0;
    return;
  }

  // ---- Full redraw
  clearRedrawFlags();
  myChangedLines = 0;

  // Clear before redrawing, i.e. the Wi-Fi symbol, the device name and the icon
  myUI.tft().fillRect(cPosX-25, cStatusAreaPosY, 50, cStatusAreaHeight, cStatusAreaCol);
  myUI.tft().fillRect(myUI.width()/2-80, cStatusAreaPosY, 160, cStatusAreaHeight, cStatusAreaCol);
  myUI.tft().fillRect(cIconRectPosX-1, cIconRectPosY-1, cIconRectWidth+3, cIconRectHeight+3, cMainAreaCol);

  myUI.tft().drawSmoothArc(cPosX, cPosY,  10,  8, 180-43, 180+43, cPairedCol, TFT_BLACK, false);
  myUI.tft().drawSmoothArc(cPosX, cPosY,  17, 15, 180-43, 180+43, cPairedCol, TFT_BLACK, false);
  myUI.tft().drawSmoothArc(cPosX, cPosY,  24, 22, 180-43, 180+43, cPairedCol, TFT_BLACK, false);

  if (myLastPaired == true)
  {
    myUI.tft().drawSpot(cPosX, cPosY, 3, cPairedCol, cPairedCol);

    // Display name and icon of receiver
    myUI.tft().setTextDatum(MC_DATUM);
    myUI.tft().setTextColor(cBorderCol, cStatusAreaCol);
    myUI.tft().drawString(myPeerData.name, myUI.width()/2, 32, 4);

    myUI.tft().fillRoundRect(cIconRectPosX, cIconRectPosY, cIconRectWidth, cIconRectHeight, 5, TFT_BLACK);
    switch (myPeerDevice)
    {
      case eHexapod:
        {
          int32_t posX = (myShowMessages == true) ? cIconRectPosX + 20 : cIconRectPosX+cIconRectWidth/2  - imgHexapodWidth/2;
          myUI.tft().pushImage(posX, cIconRectPosY+cIconRectHeight/2 - imgHexapodHeight/2, 
                              imgHexapodWidth, imgHexapodHeight, imgHexapod);
        }
        break;
      
      // Hint: add an image for each receiver type
      // case eAnotherDevice:
      //   break;

      default:
        myUI.tft().pushImage(cIconRectPosX+cIconRectWidth/2  - imgUnknownReceiverWidth/2, 
                             cIconRectPosY+cIconRectHeight/2 - imgUnknownReceiverHeight/2, 
                             imgUnknownReceiverWidth, imgUnknownReceiverHeight, imgUnknownReceiver);

        break;
    }
    myUI.tft().drawSmoothRoundRect(cIconRectPosX-1, cIconRectPosY-1, 5, 5, cIconRectWidth+2, cIconRectHeight+2, cBorderCol, TFT_BLACK);

    // Draw messages from peer (receiver)?
    if (myShowMessages == true)
      drawMessages(false);
  }
  else
  {
    myUI.tft().drawSpot(cPosX, cPosY, 3, cNotPairedCol, cNotPairedCol);
    myUI.tft().drawWedgeLine(cPosX, cPosY-8, cPosX, cPosY-23, 3, 4, cNotPairedCol, TFT_BLACK);
  }
}

// ----------------------------------------------------------------------------------------
void ReceiverStatus::drawMessages(bool changedOnly)
{
  myUI.tft().setTextColor(cBorderCol, TFT_BLACK);
  myUI.tft().setTextPadding(0);

  for (int i=0; i<int(cNumMessages); i++)
  {
    if (changedOnly == true && (myChangedLines & (1u << i)) == 0)
      continue;

    const int32_t y = cIconRectPosY + i*20 + 20;

    // Clear the line first: label and value share it from both ends, so text padding on one
    // would erase the other.
    myUI.tft().fillRect(cMsgLeftX, y, cMsgRightX-cMsgLeftX, myUI.tft().fontHeight(2), TFT_BLACK);

    myUI.tft().setTextDatum(TL_DATUM);
    myUI.tft().drawString(myPeerData.msgLabels[i], cMsgLeftX, y, 2);
    myUI.tft().setTextDatum(TR_DATUM);
    myUI.tft().drawString(myPeerData.msgInfo[i], cMsgRightX, y, 2);
  }
}
