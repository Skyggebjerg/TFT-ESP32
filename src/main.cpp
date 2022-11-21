
//====================================================================================
//                                  Libraries
//====================================================================================
// Call up the SPIFFS FLASH filing system this is part of the ESP Core
#define FS_NO_GLOBALS
#include <FS.h>
//#include <Arduino_GFX_Library.h>
#include "Free_Fonts.h" // Include the header file attached to this sketch
//#include "Orbitron_50_Bold.h"

#ifdef ESP32
  #include "SPIFFS.h" // ESP32 only
#endif

// JPEG decoder library
#include <JPEGDecoder.h>

#include <TFT_eSPI.h>      // Hardware-specific library

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// Return the minimum of two values a and b
#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

//====================================================================================
//   Decode and render the Jpeg image onto the TFT screen
//====================================================================================
void jpegRender(int xpos, int ypos) {

  // retrieve infomration about the image
  uint16_t  *pImg;
  int16_t mcu_w = JpegDec.MCUWidth;
  int16_t mcu_h = JpegDec.MCUHeight;
  int32_t max_x = JpegDec.width;
  int32_t max_y = JpegDec.height;

  // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
  // Typically these MCUs are 16x16 pixel blocks
  // Determine the width and height of the right and bottom edge image blocks
  int32_t min_w = minimum(mcu_w, max_x % mcu_w);
  int32_t min_h = minimum(mcu_h, max_y % mcu_h);

  // save the current image block size
  int32_t win_w = mcu_w;
  int32_t win_h = mcu_h;

  // record the current time so we can measure how long it takes to draw an image
  uint32_t drawTime = millis();

  // save the coordinate of the right and bottom edges to assist image cropping
  // to the screen size
  max_x += xpos;
  max_y += ypos;

  // read each MCU block until there are no more
  while ( JpegDec.readSwappedBytes()) { // Swapped byte order read

    // save a pointer to the image block
    pImg = JpegDec.pImage;

    // calculate where the image block should be drawn on the screen
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;  // Calculate coordinates of top left corner of current MCU
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    // check if the image block size needs to be changed for the right edge
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;

    // check if the image block size needs to be changed for the bottom edge
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;

    // copy pixels into a contiguous block
    if (win_w != mcu_w)
    {
      for (int h = 1; h < win_h-1; h++)
      {
        memcpy(pImg + h * win_w, pImg + (h + 1) * mcu_w, win_w << 1);
      }
    }

    // draw image MCU block only if it will fit on the screen
    if ( mcu_x < tft.width() && mcu_y < tft.height())
    {
      // Now push the image block to the screen
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    }

    else if ( ( mcu_y + win_h) >= tft.height()) JpegDec.abort();

  }

  // calculate how long it took to draw the image
  drawTime = millis() - drawTime; // Calculate the time it took

  // print the results to the serial port
  Serial.print  ("Total render time was    : "); Serial.print(drawTime); Serial.println(" ms");
  Serial.println("=====================================");

}

//====================================================================================
//   Print information decoded from the Jpeg image
//====================================================================================
void jpegInfo() {

  Serial.println("===============");
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print  ("Width      :"); Serial.println(JpegDec.width);
  Serial.print  ("Height     :"); Serial.println(JpegDec.height);
  Serial.print  ("Components :"); Serial.println(JpegDec.comps);
  Serial.print  ("MCU / row  :"); Serial.println(JpegDec.MCUSPerRow);
  Serial.print  ("MCU / col  :"); Serial.println(JpegDec.MCUSPerCol);
  Serial.print  ("Scan type  :"); Serial.println(JpegDec.scanType);
  Serial.print  ("MCU width  :"); Serial.println(JpegDec.MCUWidth);
  Serial.print  ("MCU height :"); Serial.println(JpegDec.MCUHeight);
  Serial.println("===============");
  Serial.println("");
}


/*====================================================================================
  This sketch contains support functions to render the Jpeg images.
  Created by Bodmer 15th Jan 2017
  Updated by Bodmer to support ESP32 with SPIFFS Jan 2018
  ==================================================================================*/


//====================================================================================
//   Opens the image file and prime the Jpeg decoder
//====================================================================================
void drawJpeg(const char *filename, int xpos, int ypos) {

  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");

  // Open the named file (the Jpeg decoder library will close it after rendering image)
  fs::File jpegFile = SPIFFS.open( filename, "r");    // File handle reference for SPIFFS
  //  File jpegFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library

  //ESP32 always seems to return 1 for jpegFile so this null trap does not work
  if ( !jpegFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  // Use one of the three following methods to initialise the decoder,
  // the filename can be a String or character array type:

  //boolean decoded = JpegDec.decodeFsFile(jpegFile); // Pass a SPIFFS file handle to the decoder,
  //boolean decoded = JpegDec.decodeSdFile(jpegFile); // or pass the SD file handle to the decoder,
  boolean decoded = JpegDec.decodeFsFile(filename);  // or pass the filename (leading / distinguishes SPIFFS files)

  if (decoded) {
    // print information about the image to the serial port
    jpegInfo();

    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

//====================================================================================
//   Open a Jpeg file and send it to the Serial port in a C array compatible format
//====================================================================================
void createArray(const char *filename) {

  // Open the named file
  fs::File jpgFile = SPIFFS.open( filename, "r");    // File handle reference for SPIFFS
  //  File jpgFile = SD.open( filename, FILE_READ);  // or, file handle reference for SD library

  if ( !jpgFile ) {
    Serial.print("ERROR: File \""); Serial.print(filename); Serial.println ("\" not found!");
    return;
  }

  uint8_t data;
  byte line_len = 0;
  Serial.println("");
  Serial.println("// Generated by a JPEGDecoder library example sketch:");
  Serial.println("// https://github.com/Bodmer/JPEGDecoder");
  Serial.println("");
  Serial.println("#if defined(__AVR__)");
  Serial.println("  #include <avr/pgmspace.h>");
  Serial.println("#endif");
  Serial.println("");
  Serial.print  ("const uint8_t ");
  while (*filename != '.') Serial.print(*filename++);
  Serial.println("[] PROGMEM = {"); // PROGMEM added for AVR processors, it is ignored by Due

  while ( jpgFile.available()) {

    data = jpgFile.read();
    Serial.print("0x"); if (abs(data) < 16) Serial.print("0");
    Serial.print(data, HEX); Serial.print(",");// Add value and comma
    line_len++;
    if ( line_len >= 32) {
      line_len = 0;
      Serial.println();
    }

  }

  Serial.println("};\r\n");
  jpgFile.close();
}
//====================================================================================

/*====================================================================================
  This sketch contains support functions for the ESP6266 SPIFFS filing system
  Created by Bodmer 15th Jan 2017
  Updated by Bodmer to support ESP32 with SPIFFS Jan 2018
  ==================================================================================*/

//====================================================================================
//                 Print a SPIFFS directory list (root directory)
//====================================================================================
#ifdef ESP8266
void listFiles(void) {
  Serial.println();
  Serial.println("SPIFFS files found:");

  fs::Dir dir = SPIFFS.openDir("/"); // Root directory
  String  line = "=====================================";

  Serial.println(line);
  Serial.println("  File name               Size");
  Serial.println(line);

  while (dir.next()) {
    String fileName = dir.fileName();
    Serial.print(fileName);
    int spaces = 21 - fileName.length(); // Tabulate nicely
    while (spaces--) Serial.print(" ");

    fs::File f = dir.openFile("r");
    String fileSize = (String) f.size();
    spaces = 10 - fileSize.length(); // Tabulate nicely
    while (spaces--) Serial.print(" ");
    Serial.println(fileSize + " bytes");
  }

  Serial.println(line);
  Serial.println();
  delay(1000);
}
#endif

//====================================================================================

#ifdef ESP32



void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {

  Serial.println();
  Serial.println("SPIFFS files found:");

  Serial.printf("Listing directory: %s\n", "/");
  String  line = "=====================================";

  Serial.println(line);
  Serial.println("  File name               Size");
  Serial.println(line);

  fs::File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  fs::File file = root.openNextFile();
  while (file) {

    if (file.isDirectory()) {
      Serial.print("DIR : ");
      String fileName = file.name();
      Serial.print(fileName);
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      String fileName = file.name();
      Serial.print("  " + fileName);
      int spaces = 20 - fileName.length(); // Tabulate nicely
      while (spaces--) Serial.print(" ");
      String fileSize = (String) file.size();
      spaces = 10 - fileSize.length(); // Tabulate nicely
      while (spaces--) Serial.print(" ");
      Serial.println(fileSize + " bytes");
    }

    file = root.openNextFile();
  }

  Serial.println(line);
  Serial.println();
  delay(1000);
}

void listFiles(void) {
  listDir(SPIFFS, "/", 0);
}
#endif


//====================================================================================
//                                    Setup
//====================================================================================
void setup()
{
  Serial.begin(115200); // Used for messages and the C array generator

  delay(10);
  Serial.println("NodeMCU decoder test!");

  tft.begin();
  tft.setRotation(0);  // 0 & 2 Portrait. 1 & 3 landscape
  tft.fillScreen(TFT_BLACK);

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nInitialisation done.");
  listFiles(); // Lists the files so you can see what is in the SPIFFS

}

//====================================================================================
//                                    Loop
//====================================================================================
void loop()
{
  // Note the / before the SPIFFS file name must be present, this means the file is in
  // the root directory of the SPIFFS, e.g. "/tiger.jpg" for a file called "tiger.jpg"

  tft.setRotation(3);  // portrait
  tft.fillScreen(TFT_WHITE);

    // Set text colour to orange with black background
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  tft.fillScreen(TFT_BLACK);            // Clear screen
  tft.setFreeFont(&Orbitron_Bold_45);                 // Select the font
  tft.drawString("Hejsa", 20, 50, 1);// Print the string name of the font
//display.setCursor(20, 50);
delay(2000);
  drawJpeg("/DAPI2.jpg", 0 , 0);     // 240 x 320 image
  //drawJpeg("/Baboon40.jpg", 0, 0); // 320 x 480 image
  delay(2000);
  drawJpeg("/AO2.jpg", 0 , 0);     // 240 x 320 image
delay(2000);
  drawJpeg("/Via2.jpg", 0 , 0);     // 240 x 320 image
delay(2000);
  drawJpeg("/AOandDAPI2.jpg", 0 , 0);     // 240 x 320 image
delay(2000);
  drawJpeg("/Nuclei.jpg", 0 , 0);     // 240 x 320 image
delay(2000);
  drawJpeg("/Nucleocolor.jpg", 0 , 0);     // 240 x 320 image
delay(2000);

  //createArray("/tiger.jpg");
  //delay(2000);
  //while(1) yield(); // Stay here
}
//====================================================================================
