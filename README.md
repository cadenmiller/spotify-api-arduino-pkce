# Spotify API for the ESP32

![License](https://img.shields.io/github/license/cadenmiller/spotify-api-esp32)
![Release stable](https://badgen.net/github/release/cadenmiller/spotify-api-esp32/stable)  
Arduino library for integrating with a subset of the [Spotify Web-API](https://developer.spotify.com/documentation/web-api/reference/) (Does not play music)

**🚧 Work in Progress! Expect Changes 🚧**

## Forked
This is a fork of https://github.com/witnessmenow/spotify-api-arduino.<br> Check out the original library if you are interested it.

#### Improvements
 * Better documentation (still working on this).
 * Added more functions for ease of use.
 * Using HTTPClient instead of TCPClient, makes it much easier to maintain.
 * Binary sizes may decrease by multiple percent.
 * Changed logging, oriented to ESP32.
 * Simplifications.

#### Why Fork?

Here are my points on why I forked:
 * I wanted PKCE authentication support
 * I really liked the groundwork this library laid
 * My changes were far too much for a pull request
 * More up-to-date HTTPClient/WiFiClient
 * I wanted to change lots more things...

## Boards

This library was designed to work on ESP32 based boards.
Through Platform IO however, it's possible that this library will work on your board! Please feel free to check and add it to the list below. These names are from the platform io board registry.

* ESP32
  * featheresp32
  * adafruit_feather_esp32_v2

## Library Features

The Library supports the following features:

- Get Authentication Tokens
- Getting your currently playing track
- Player Controls:
  - Next
  - Previous
  - Seek
  - Play (basic version, basically resumes a paused track)
  - Play Advanced (play given song, album, artist)
  - Pause
  - Set Volume (doesn't seem to work on my phone, works on desktop though)
  - Set Repeat Modes
  - Toggle Shuffle
- Get Devices
- Search Spotify Library

### What needs to be added:

- Better instructions for how to set up your refresh token.
- Example where refresh token and full operation are handled in same sketch.

## Setup Instructions

### Spotify Account

- Sign into the [Spotify Developer page](https://developer.spotify.com/dashboard/login)
- Create a new application. (name it whatever you want)
- You will need to use the "client ID" and "client secret" from this page in your sketches
- You will also need to add a callback URI for authentication process by clicking "Edit Settings", what URI to add will be mentioned in further instructions

### Getting Your Refresh Token

Spotify **requires** the use of a webserver if you want a complete self contained device that can authenticate properly. Luckily the ESP32 and other boards can run webservers and other things.

This library makes it much easier to authenticate with Spotify and send requests from your device to the Spotify API.

#### Minimal Example

```C++

#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <AsyncWebServer.h>
#include <SpotifyArduino.h>

#define SPOTIFY_CLIENT_ID "AAAAAAAABBBBBBBBCCCCCCCCDDDDDDDD"
#define SPOTIFY_REDIRECT_CALLBACK "http%3A%2F%2Fesp32.local%2Fcallback"


WiFiClientSecure wifiClient;
HTTPClient httpClient;
AsyncWebServer server(80);
SpotifyArduino spotify(wifiClient, httpClient, SPOTIFY_CLIENT_ID);

setup() {
  /* ... CONNECT TO WIFI HERE ... */
  
  MDNS.begin("esp32");

  server.on("/", [](AsyncWebServerRequest* request){
    char url[SPOTIFY_REDIRECT_LENGTH] = {};
    spotify.generateRedirectForPKCE(url, sizeof(url), SPOTIFY_REDIRECT_CALLBACK);

    request->redirect(url);
  });
  
  
  std::atomic<bool> authenticated(false);
  server.on("/callback", [&authenticated](AsyncWebServerRequest* request){
    String code = "";
    for (uint8_t i = 0; i < request->args(); i++) {
      if (request->argName(i) == "code") {
          code = request->arg(i);
          log_i("Recieved code from spotify: %s", code.c_str());
      }
    }
    
    const char *refreshToken = NULL;
    refreshToken = spotify.requestAccessTokens(code.c_str(), SpotifyScope::eUserReadCurrentlyPlaying, SPOTIFY_REDIRECT_CALLBACK, true);

    if (!refreshToken) { 
      request->send("text/plain", "Could not authenticate, try again.");
    } else {
      request->send("text/plain", "Successfully authenticated.");
      authenticated = true;
    }
  });

  server.begin();
  while(!authenticated) yeild();

  /* We are authenticated now! */
  server.end();
  MDNS.end();

  /* ... SPOTIFY API FUNCTIONS HERE ...  */
}

```

**Wow! That's a lot,**

and you'd be right if you thought that just now. Spotify requires a lot just to authenticate. If you are thinking about building you're own project and this turns you off because of requirements or it's too much, I don't blame you! Spotify is a secure but difficult API to use and manage.

Spotify's Authentication flow requires a webserver to complete, but it's only needed once to get your refresh token. Your refresh token can then be used in all future sketches to authenticate.

Because the webserver is only needed once, I decided to seperate the logic for getting the Refresh token to it's own example.

Follow the instructions in the [getRefreshToken example](examples/getRefreshToken/getRefreshToken.ino) to get your token.

Note: Once you have a refresh token, you can use it on either platform in your sketches, it is not tied to any particular device.

### Running

Take one of the included examples and update it with your WiFi creds, Client ID, Client Secret and the refresh token you just generated.

### Scopes

By default the getRefreshToken examples will include the required scopes, but if you want change them the following info might be useful.

put a `%20` between the ones you need.

| Feature                   | Required Scope             |
| ------------------------- | -------------------------- |
| Current Playing Song Info | user-read-playback-state   |
| Player Controls           | user-modify-playback-state |

## Installation

Download zip from Github and install to the Arduino IDE using that.

#### Dependancies

- V6 of Arduino JSON - can be installed through the Arduino Library manager.

## Compile flag configuration

There are some flags that you can set in the `SpotifyArduino.h` that can help with debugging

```

#define SPOTIFY_DEBUG 1
// Enables extra debug messages on the serial.
// Will be disabled by default when library is released.
// NOTE: Do not use this option on live-streams, it will reveal your private tokens!

//#define SPOTIFY_PRINT_JSON_PARSE 1
// Prints the JSON received to serial (only use for debugging as it will be slow)
// Requires the installation of ArduinoStreamUtils (https://github.com/bblanchon/ArduinoStreamUtils)

```
