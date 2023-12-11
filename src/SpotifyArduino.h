
/* Spotify ESP32*/

#pragma once

#define SPOTIFY_DEBUG 1

// Comment out if you want to disable any serial output from this library (also comment out DEBUG and PRINT_JSON_PARSE)
#define SPOTIFY_SERIAL_OUTPUT 1

// Prints the JSON received to serial (only use for debugging as it will be slow)
//#define SPOTIFY_PRINT_JSON_PARSE 1

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

#include "SpotifyBase64.h"
#include "SpotifyStructs.h"
#include "SpotifyCert.h"

#ifdef SPOTIFY_PRINT_JSON_PARSE
#include <StreamUtils.h>
#endif

#define SPOTIFY_HOST "api.spotify.com"
#define SPOTIFY_ACCOUNTS_HOST "accounts.spotify.com"

// Fingerprint for "*.spotify.com", correct as of March 31, 2023
#define SPOTIFY_FINGERPRINT "9F 3F 7B C6 26 4C 97 06 A2 D4 D7 B2 35 45 D9 AA 8D BD CD 4D"

// Fingerprint for "*.scdn.co", correct as of March 31, 2023
#define SPOTIFY_IMAGE_SERVER_FINGERPRINT "8B 24 D0 B7 12 AC DB 03 75 09 45 95 24 FF BE D8 35 E6 EB DF"

#define SPOTIFY_ACCESS_TOKEN_LENGTH 309
#define SPOTIFY_PKCE_CODE_LENGTH 64 // Min of 32, Max of 96
#define SPOTIFY_PKCE_CODE_HASHED_LENGTH (SpotifyBase64::Length(32))

#define SPOTIFY_TIMEOUT 2000

#define SPOTIFY_CURRENTLY_PLAYING_ENDPOINT "/v1/me/player/currently-playing?additional_types=episode"
#define SPOTIFY_PLAYER_ENDPOINT "/v1/me/player"
#define SPOTIFY_DEVICES_ENDPOINT "/v1/me/player/devices"
#define SPOTIFY_PLAY_ENDPOINT "/v1/me/player/play"
#define SPOTIFY_SEARCH_ENDPOINT "/v1/search"
#define SPOTIFY_PAUSE_ENDPOINT "/v1/me/player/pause"
#define SPOTIFY_VOLUME_ENDPOINT "/v1/me/player/volume?volume_percent=%d"
#define SPOTIFY_SHUFFLE_ENDPOINT "/v1/me/player/shuffle?state=%s"
#define SPOTIFY_REPEAT_ENDPOINT "/v1/me/player/repeat?state=%s"
#define SPOTIFY_NEXT_TRACK_ENDPOINT "/v1/me/player/next"
#define SPOTIFY_PREVIOUS_TRACK_ENDPOINT "/v1/me/player/previous"
#define SPOTIFY_SEEK_ENDPOINT "/v1/me/player/seek"
#define SPOTIFY_TOKEN_ENDPOINT  "/api/token"

class SpotifyArduino {
public:
    SpotifyArduino(WiFiClient &wifiClient, HTTPClient &httpClient);
    SpotifyArduino(WiFiClient &wifiClient, HTTPClient &httpClient, char *bearerToken);
    SpotifyArduino(WiFiClient &wifiClient, HTTPClient &httpClient, const char *clientId);
    SpotifyArduino(WiFiClient &wifiClient, HTTPClient &httpClient, const char *clientId, const char *clientSecret, const char *refreshToken = "");

    void setClientId(const char* clientId);

// ==============
// Authentication
// ==============

    /** @brief Generates a PKCE authentication code challenge.
     *  
     *  Uses the onboard hardware random number generator to create a secure
     *  PKCE code for authentication with Spotify's servers. You may want to
     *  use @ref generateRedirectForPKCE because it does this job for you 
     *  while creating the redirect.
     * 
     *  @param buffer Must be at least SPOTIFY_PKCE_CODE_HASHED_LENGTH or longer.
     * 
     *  @note This function does not append a null-terminator to @ref buffer.
     * 
     *  @example
     *  @code{cpp}
     *  char codeChallenge[SPOTIFY_PKCE_CODE_HASHED_LENGTH+1];
     *  memset(codeChallenge, 0, sizeof(codeChallenge));
     *
     *  generateCodeChallengeForPKCE(codeChallenge);
     *  @endcode
     * 
    */
    void generateCodeChallengeForPKCE(char* buffer);

    /** @brief Generates a redirect for a Spotify PKCE authentication URL. 
     * 
     * Uses the scopes and redirect provided to create an auth URL. Redirect
     * your users to this link when they access your login page to begin the
     * authentication process. Uses @ref generateCodeChallengeForPKCE to create
     * the auth code.
     * 
     * @param[in]  scopes Scopes to use, allows the API to access more of users data.
     * @param[in]  redirect Spotify will redirect the users to a callback when the user is done authenticating.
     * @param[out] buffer Redirect is filled into this buffer. Depending on the amount 
     *  of scopes and redirect length you might want to make the buffer size larger.
     * 
     * @note The function will ensure a null-terminator is placed at the end of the buffer.
     * 
    */
    void generateRedirectForPKCE(
        SpotifyScopeFlags scopes, 
        const char *redirect, 
        char *buffer,
        size_t bufferLength);

    /** @brief Generates a redirect for a Spotify PKCE authentication URL.
     * 
     * Uses the scopes and callback redirect provided to create an auth URL.
     * Redirect your users to this link when they access your login page to 
     * begin the authentication process. Uses @ref generateCodeChallengeForPKCE
     * to create the auth code.
     * 
     * @param[in] scopes Scopes to use, how to access user data.
     * @param[in] redirect Spotify will redirect the users to a callback when the user is done authentication.
     * @param[out] buffer The full redirect if filled into this buffer. Depending on the size of the 
     *  scopes string and the redirect length you should adjust the size accordingly.
     * 
     * @note The function will ensure a null-terminator is placed at the end of the buffer.
     * 
    */
    void generateRedirectForPKCE(
        const char *scopes,
        const char *redirect, 
        char *buffer,
        size_t bufferLength);

    /** @brief Refreshes the access token if it's going to expire soon. 
     * 
     * Spotify uses a volatile access token that only lasts for 3600 seconds.
     * The application has to request Spotify for another token using the old
     * token from an hour ago.
     *  
     * @return bool True on -- token is still stable OR successfully received a new token.
     * @return bool False on -- a new token wasn't received.
    */
    bool checkAndRefreshAccessToken();

    /** @brief Determines if our refresh token should be refreshed or not. 
     * 
     * @return bool True on -- the token should be refreshed now.
    */
    bool shouldRefresh();

    /** @brief Forces a refresh on the refresh token.
     * 
     * Requests a new refresh token from Spotify's servers.
     * 
     * @return bool True on -- successfully received a new refresh token.  
     */
    bool refreshAccessToken();

    
    const char *requestAccessTokens(
        const char *code, 
        const char *redirectUrl, 
        bool usingPKCE = false);

    /** @brief Sets the refresh token that was previously created.
     * 
     * If you have already went through the authentication process then you may
     * already have a refresh token to use.
     * 
     * @param[in] refreshToken A refresh token previously created from authentication.
     * 
     */
    void setRefreshToken(const char *refreshToken);

// ==============
// Authentication
// ==============

    int getCurrentlyPlaying(SpotifyCallbackOnCurrentlyPlaying currentlyPlayingCallback, const char *market = "");
    int getPlayerDetails(SpotifyCallbackOnPlayerDetails playerDetailsCallback, const char *market = "");
    int getDevices(SpotifyCallbackOnDevices devicesCallback);
    bool play(const char *deviceId = "");
    bool playAdvanced(char *body, const char *deviceId = "");
    bool pause(const char *deviceId = "");
    bool setVolume(int volume, const char *deviceId = "");
    bool toggleShuffle(bool shuffle, const char *deviceId = "");
    bool setRepeatMode(SpotifyRepeatMode repeat, const char *deviceId = "");
    bool nextTrack(const char *deviceId = "");
    bool previousTrack(const char *deviceId = "");
    bool playerControl(char *command, const char *deviceId = "", const char *body = "");
    bool playerNavigate(char *command, const char *deviceId = "");
    bool seek(int position, const char *deviceId = "");
    bool transferPlayback(const char *deviceId, bool play = false);

    //Search
    int searchForSong(String query, int limit, SpotifyCallbackOnSearch searchCallback, SpotifySearchResult results[]);

    // Image methods
    bool getImage(char *imageUrl, Stream *file);
    bool getImage(char *imageUrl, uint8_t **image, int *imageLength);

    int portNumber = 443;
    int currentlyPlayingBufferSize = 3000;
    int playerDetailsBufferSize = 2000;
    int getDevicesBufferSize = 3000;
    int searchDetailsBufferSize = 3000;
    bool autoTokenRefresh = true;
    WiFiClient* _wifiClient;
    HTTPClient* _httpClient;
    void lateInit(const char *clientId, const char *clientSecret, const char *refreshToken = "");

#ifdef SPOTIFY_DEBUG
    char *stack_start;
#endif

private:
    char _bearerToken[SPOTIFY_ACCESS_TOKEN_LENGTH+10]; //10 extra is for "bearer " at the start
    unsigned char _verifier[SPOTIFY_PKCE_CODE_LENGTH+1];
    unsigned char _verifierChallenge[SpotifyBase64::Length(32)+1];
    char* _refreshToken;
    const char* _clientId;
    const char* _clientSecret;
    unsigned int timeTokenRefreshed;
    unsigned int tokenTimeToLiveMs;
    
    // Generic Request Methods
    int makeGetRequest(const char *command, const char *authorization, const char *accept = "application/json", const char *host = SPOTIFY_HOST);
    int makeRequestWithBody(const char *type, const char *command, const char *authorization, const char *body = "", const char *contentType = "application/json", const char *host = SPOTIFY_HOST);
    int makePostRequest(const char *command, const char *authorization, const char *body = "", const char *contentType = "application/json", const char *host = SPOTIFY_HOST);
    int makePutRequest(const char *command, const char *authorization, const char *body = "", const char *contentType = "application/json", const char *host = SPOTIFY_HOST);

    int commonGetImage(char *imageUrl);
    int getContentLength();
    void closeClient();
    void parseError();

    const char *requestAccessTokensBody = R"(grant_type=authorization_code&code=%s&redirect_uri=%s&client_id=%s&client_secret=%s)";
    const char *requestAccessTokensBodyPKCE = R"(client_id=%s&grant_type=authorization_code&redirect_uri=%s&code=%s&code_verifier=%s)";
    const char *refreshAccessTokensBody = R"(grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s)";

#ifdef SPOTIFY_DEBUG
    void printStack();
#endif
};

