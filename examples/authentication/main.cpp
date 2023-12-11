/*                                                                                                   
 * ========                                                                       
 * EXAMPLE: Authentication                                                        
 * ========                                                                       
 *                                                                                
 * Description:                                                                   
 *   This is an annotated version of the same minimal example as in the README    
 *   file. It will provide you valuable knowledge on how to get your device up and
 *   connected to Spotify. Feel free to copy this code and other examples like it 
 *   into your project and change them to your needs.                             
 *                                                                                
 *   Here is a basic graph of what happens:                                       
 *                                                                                
 *    ┌────────────────┐          ┌───────────────────────────┐                   
 *    │Start Web Server│       ┌─►│Spotify Authenticates User │                   
 *    └───────┬────────┘       │  └─────────────┬─────────────┘                   
 *            │                │                │                                 
 *     ┌──────▼──────┐         │     ┌──────────▼───────────┐                     
 *     │User Connects│         │     │Redirect to our Server│                     
 *     └──────┬──────┘         │     └──────────┬───────────┘                     
 *            │                │                │                                 
 * ┌──────────▼─────────────┐  │   ┌────────────▼─────────────┐                   
 * │Redirect to Auth Server ├──┘   │Use Code for Refresh Token│                   
 * └────────────────────────┘      └────────────┬─────────────┘                   
 *                                              │                                 
 *                                              │                                 
 *                                    ┌─────────▼───────────┐                     
 *                                    │Request User Info    │                     
 *                                    │ Currently Playing...│                     
 *                                    └─────────────────────┘                     
 *                                                                                
 *   This example uses the PKCE authentication method for users.      
 * 
 * 
 */   

#include <WiFiClientSecure.h> /* Secure networking on HTTPS. */
#include <ESPmDNS.h>          /* DNS server to get a name on the local network, "esp32.local"  */
#include <HTTPClient.h>       /* HTTP client for sending requests. */
#include <AsyncWebServer.h>   /* Async, fast web server. */
#include <SpotifyArduino.h>   /* Our beloved Spotify.  */

#define SPOTIFY_CLIENT_ID "AAAAAAAABBBBBBBBCCCCCCCCDDDDDDDD"

/* Spotify will automatically redirect the user back to our page when authentication is complete or incomplete. You can see that our MDNS name is inside this callback along with our Spotify callback URL. */
#define SPOTIFY_REDIRECT_CALLBACK "http%3A%2F%2Fesp32.local%2Fcallback"


WiFiClientSecure wifiClient;
HTTPClient httpClient;
AsyncWebServer server(80);
SpotifyArduino spotify(wifiClient, httpClient, SPOTIFY_CLIENT_ID);

setup() {

  /* CONNECT TO WIFI HERE. */

  MDNS.begin("esp32"); /* Creates our static name for Spotify redirect on network. */ 

  
  server.on("/", [](AsyncWebServerRequest* request){
    
    /* Build the redirect. */
    char url[SPOTIFY_REDIRECT_LENGTH] = {};
    spotify.generateRedirectForPKCE(url, sizeof(url), SPOTIFY_REDIRECT_CALLBACK);

    /* Redirects the user to the authentication page on Spotify's website. */
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
    refreshToken = spotify.requestAccessTokens(code.c_str(), SpotifyScope::eUserReadCurrentlyPlaying, SPOTIFY_REDIRECT_CALLBACK, true); /* true here means we're using PKCE for auth. */

    if (!refreshToken) { 
      request->send("text/plain", "Could not authenticate, try again.");
    } else {
      request->send("text/plain", "Successfully authenticated.");
      authenticated = true;
    }
  });

  server.begin();

  /* Wait until we are authenticated. */
  while(!authenticated) yield();

  server.end();
  MDNS.end();

  /* We should be authenticated now! */
  spotify.

}
