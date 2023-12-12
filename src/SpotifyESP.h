
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

#include "SpotifyConfig.h"
#include "SpotifyBase64.h"
#include "SpotifyStructs.h"
#include "SpotifyCert.h"

#ifdef SPOTIFY_PRINT_JSON_PARSE
#include <StreamUtils.h>
#endif

class SpotifyESP {
public:
    SpotifyESP(WiFiClient &wifiClient, HTTPClient &httpClient);
    SpotifyESP(WiFiClient &wifiClient, HTTPClient &httpClient, char *bearerToken);
    SpotifyESP(WiFiClient &wifiClient, HTTPClient &httpClient, const char *clientId);
    SpotifyESP(WiFiClient &wifiClient, HTTPClient &httpClient, const char *clientId, const char *clientSecret, const char *refreshToken = "");

    void setClientId(const char* clientId);

// ==============
// Authentication API
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

    /** @brief Requests a refresh token using the authentication code provided. 
     * 
     * In order to get yourself authenticated you have use one of the 
     * authentication methods. Using the auth code Spotify sent in the callback
     * you can request for a refresh token and use that for Spotify's Web API.
     * 
     * @param[in] code The code from Spotify's HTTP callback.
     * @param[in] redirectUrl Spotify requires the redirect for checking purposes.
     * @param[in] usingPKCE True on -- we are preforming 'Authorization code with PKCE' flow, otherwise it does the 'Authorization code' flow.
     *  
     * @return NULL on -- failed to request refresh token from Spotify.
     * @return Valid on -- successfully requested the refresh token from Spotify. 
    */
    const char *requestAccessTokens(
        const char *code, 
        const char *redirectUrl, 
        bool usingPKCE = false);

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
// User API
// ==============

    /** @brief Requests for the track the user is currently playing.
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/get-the-users-currently-playing-track
     * 
     * Uses Spotify playback API to retrieve the track the user is currently 
     * playing. Using the @ref SpotifyCallbackOnCurrentlyPlaying you can
     * obtain information about the track and how far along the user is in it.
     * 
     * @param callback Callback for the currently playing track info.
     * @param market Market specific info about the player. Must be a valid ISO 3166-1 alpha-2 code if used. (United States is 'US')
     * 
     * @return A HTTP status code of the request.
     * @return 200 on -- successful HTTP status of request. 
     */
    int getCurrentlyPlayingTrack(SpotifyCallbackOnCurrentlyPlaying callback, const char *market = "");

    /** @brief Requests for what the users playback state is like. 
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/get-information-about-the-users-current-playback
     * 
     * Retrieves more more broad information more geared towards the player
     * than the track as seen in @ref getCurrentlyPlayingTrack. You may want
     * to use this function when retrieving if the user is playing the track
     * track titles, artists names, and less about the track itself.
     * 
     * @param callback Callback provides the playback state of the user.
     * @param market Market specific info about the player. Must be a valid ISO 3166-1 alpha-2 code if used. (United States is 'US')
     * 
     * @return A HTTP status code of the request.
     * @return 200 on -- successful HTTP status of request. 
     * 
    */
    int getPlaybackState(SpotifyCallbackOnPlaybackState callback, const char *market = "");

    /** @brief Retrieves the devices available for Spotify audio playback.
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/get-a-users-available-devices
     * 
     * Retrieves all the available playback devices that Spotify has access to.
     * Useful for when you want to see the devices current volume setting, 
     * which one is currently playing, its type and id.
     * 
     * @param callback Callback can run multiple times providing info about all devices.
     *
     * @return A HTTP status code of the request.
     * @return 200 on -- successful HTTP status of request. 
     * 
     */
    int getAvailableDevices(SpotifyCallbackOnDevices callback);
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

