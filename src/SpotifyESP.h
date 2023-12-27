
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
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

    /** @brief Minimal Initialization Constructor
     * 
     * Only really use this construction when you don't know much, not even the
     * client id, in reality you should always know you're client ID. I suggest
     * never using this constructor.
     * 
     * @param wifiClient Your secure Wi-Fi client for sending HTTPS requests.
     * @param httpClient Your HTTP client object for building the requests.
     * @param flow How the user will be authenticated.
     * 
     */
    SpotifyESP(WiFiClientSecure &wifiClient, HTTPClient &httpClient, SpotifyCodeFlow flow);
    
    /** @brief PKCE Initialization Constructor 
     * 
     * Use this constructor when using the Authentication code with PKCE
     * method. A client secret is not needed but a client id is required.
     *  
    */
    SpotifyESP(WiFiClientSecure &wifiClient, HTTPClient &httpClient, const char *clientId, const char* refreshToken = "");

    /** @brief Code Authentication Constructor 
     * 
     * Use this constructor when using the Authentication code method. 
     * 
    */
    SpotifyESP(WiFiClientSecure &wifiClient, HTTPClient &httpClient, const char *clientId, const char *clientSecret, const char *refreshToken = "");

   
// ========================================
// Authentication API
// ========================================

    void setClientId(const char* id);
    void setClientSecret(const char* secret);

    /** @brief Sets the refresh token that was previously created.
     * 
     * If you have already went through the authentication process then you may
     * already have a refresh token to use. By using this function, you can 
     * bypass all the authentication API because you've already done it!
     * 
     * @param[in] refreshToken A refresh token previously created from authentication.
     * 
     */
    void setRefreshToken(const char *refreshToken);

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

    /** @brief Refreshes the refresh token.
     * 
     * Requests a new refresh token from Spotify's servers.
     * 
     * @return bool True on -- successfully received a new refresh token.  
     */
    bool refreshAccessToken();

    /** @brief Determines if our refresh token should be refreshed or not. 
     * 
     * @return bool True on -- the token should be refreshed now.
     */
    bool shouldRefresh();

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
     * @return The number of characters copied into the string.
     *          Check for truncation to ensure you have a correct URL. (if result >= bufferLength)
     * 
     * @note The function will ensure a null-terminator is placed at the end of the buffer.
     * 
     * @note The other version of this function doesn't use scope flags, that may be easier in some circumstances.
     */
    int generateRedirectForPKCE(
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
     * @return The number of characters copied into the string.
     *          Check for truncation to ensure you have a correct URL. (if result >= bufferLength)
     * 
     * @note The function will ensure a null-terminator is placed at the end of the buffer.
     * 
     */
    int generateRedirectForPKCE(
        const char *scopes,
        const char *redirect, 
        char *buffer,
        size_t bufferLength);
    
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

    /** @brief Requests a refresh token using the authentication code provided. 
     * 
     * In order to get yourself authenticated you have use one of the 
     * authentication methods. Using the auth code Spotify sent in the callback
     * you can request for a refresh token and use that for Spotify's Web API.
     * 
     * @param[in] code The code from Spotify's HTTP callback.
     * @param[in] redirectUrl Spotify requires the redirect for checking purposes.
     *  
     * @return NULL on -- failed to request refresh token from Spotify.
     * @return Valid on -- successfully requested the refresh token from Spotify. 
     */
    SpotifyResult requestAccessTokens(
        const char *code, 
        const char *redirectUrl);

    /** @brief Returns the current refresh token. 
     * 
     * Use this function to save refresh tokens to preferences or any config
     * system that you desire. When recreating the Spotify device, on restart
     * you can use this refresh token along with @ref setRefreshToken to easily
     * get started again without fully authenticating with the user.
     * 
     * @return The current refresh token.
     */
    const String& getRefreshToken();


// ========================================
// User API
// ========================================

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
    SpotifyResult getCurrentlyPlayingTrack(SpotifyCallbackOnCurrentlyPlaying callback, const char *market = "");

    /** @brief Requests for what the users playback state is like. 
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/get-information-about-the-users-current-playback
     * 
     * Retrieves more more broad information more geared towards the player
     * than the track as seen in @ref getCurrentlyPlayingTrack. You may want
     * to use this function when retrieving if the user is playing the track
     * track titles, artists names, and less about the track itself.
     * 
     * @param[in] callback Callback provides the playback state of the user.
     * @param[in] market Market specific info about the player. Must be a valid ISO 3166-1 alpha-2 code if used. (United States is 'US')
     * 
     * @return A HTTP status code of the request.
     * @return 200 on -- successful HTTP status of request. 
     * 
    */
    SpotifyResult getPlaybackState(SpotifyCallbackOnPlaybackState callback, const char *market = "");

    /** @brief Retrieves the devices available for Spotify audio playback.
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/get-a-users-available-devices
     * 
     * Retrieves all the available playback devices that Spotify has access to.
     * Useful for when you want to see the devices current volume setting, 
     * which one is currently playing, its type and id.
     * 
     * @param[in] callback Callback can run multiple times providing info about all devices.
     *
     * @return A HTTP status code of the request.
     * @return 200 on -- successful HTTP status of request. 
     * 
     */
    SpotifyResult getAvailableDevices(SpotifyCallbackOnDevices callback);

    /** @brief Starts or resumes playback on a device.
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/start-a-users-playback
     * 
     * Begins playback on a device by resuming a previous session or starting
     * a new one. The device can be left blank to play from the latest used 
     * device.
     * 
     * @param[in] deviceId optional, a device to continue or create a new session on.
     * 
     * @return True on -- successfully started or resumed a session.
     * 
     * @note Requires Spotify premium.
     * 
     * @deprecated Both play members will combine later to become one.
     * 
     */
    SpotifyResult play(const char *deviceId = "");

    /** @brief Pauses playback on the users account. 
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/pause-a-users-playback
     * 
     * Pauses playback of the users account, you can specify a device or leave
     * the parameter blank. The end result may not change if you specify a 
     * device or not.
     * 
     * @param[in] deviceId optional, a device to pause playback from.
     * 
     * @return True on -- successfully paused the session.
     * 
     * @note Requires Spotify premium.
     * 
    */
    SpotifyResult pause(const char *deviceId = "");

    /** @brief Starts or resumes playback on a device with a body for track choosing.
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/start-a-users-playback
     * 
     * Similar to the other @ref play but allows to specify a body of the 
     * request. This lets you choose a track to play or an album. Later this
     * function will be changed and merged with the other simpler one.
     * 
     * @param[in] deviceId optional, a device to continue or create a new session on.
     * 
     * @return True on -- successfully started or resumed a session.
     * 
     * @note Requires Spotify premium.
     * 
     * @deprecated Both play members will combine later to become one.
     * 
     */
    SpotifyResult playAdvanced(char *body, const char *deviceId = "");

    /** @brief Sets the volume of a device.
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/set-volume-for-users-playback
     * 
     * Sets the volume of the player from 0 to 100, will be clamped. If the 
     * device is not specified the users currently active device is the target.
     * 
     * @param[in] volume Volume of the device from 0 to 100 inclusive.
     * @param[in] deviceId optional, a device to set the volume of.
     * 
     * @return True on -- successfully set the volume.
     * 
     * @note Requires Spotify premium.
     *  
     */
    SpotifyResult setVolume(int volume, const char *deviceId = "");
    
    /** @brief Toggle the users shuffle mode to on or off.
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/toggle-shuffle-for-users-playback
     * 
     * The player will mix the songs in a psudo-random way instead of playing
     * straight through linearly. If a device is not specified
     * it automatically uses the currently active device as the target.
     * 
     * @param[in] shuffle True for -- shuffle on.
     * @param[in] deviceId optional, a device to toggle the shuffle on.
     * 
     * @return True on -- successfully toggled the players shuffle.
     * 
     * @note Requires Spotify premium.
     * 
     */
    SpotifyResult toggleShuffle(bool shuffle, const char *deviceId = "");
    
    /** @brief Set the users repeat mode for tracks and contexts. 
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/set-repeat-mode-on-users-playback
     * 
     * Sets the users repeat mode for tracks, to context and off. Track: means 
     * that the currently track will be repeated. Context: means that the 
     * current album, or playlist will repeat. Off: will disable repeat modes.
     * If a device is not specified it automatically uses the currently active
     * device as the target.
     * 
     * @param[in] mode The mode to switch to.
     * @param[in] deviceId optional, a device to set the players repeat mode on. 
     * 
     * @return True on -- successfully changed the players repeat mode.
     *
     * @note Requires Spotify premium.
     * 
     */
    SpotifyResult setRepeatMode(SpotifyRepeatMode mode, const char *deviceId = "");
    
    /** @brief Skips to the next track in the queue. 
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/skip-users-playback-to-next-track
     * 
     * Skips the current song playing onto the next song in the queue of the
     * player. If a device is not specified it automatically uses the currently
     * active device as the target.
     * 
     * @param[in] deviceId optional, a device to skip the player queue.
     * 
     * 
     * @return True on -- successfully skipped to the next song in queue.
     * 
     * @note Requires Spotify premium.
    */
    SpotifyResult skipToNext(const char *deviceId = "");

    /** @brief Skips to the previous track in the queue. 
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/skip-users-playback-to-next-track
     * 
     * Returns the queue to the previous song in the queue of the
     * player. If a device is not specified it automatically uses the currently
     * active device as the target.
     * 
     * @param[in] deviceId optional, a device to skip the player queue.
     *
     * @return True on -- successfully skipped to the previous song in queue.
     * 
     * @note Requires Spotify premium.
    */
    SpotifyResult skipToPrevious(const char *deviceId = "");
    
    /** @brief Control method for spotify player commands.
     * 
     * Sends a request to spotify using one of its player control commands.
     * Can be used to do easily PUT request any Spotify player option. Has a
     * device id parameter to specify devices player to change. Can be left 
     * blank to change the currently active device as the target.
     * 
     * @param[in] command The command you want to run, see Spotify's documentation.
     * @param[in] deviceId optional, Device you want to run the command on.
     * @param[in] body The body of the PUT request. 
     *  
     * @warning You might not want to use this function normally and use one of
     * the specified functions for controlling tracks above.
    */
    SpotifyResult playerControl(char *command, const char *deviceId = "", const char *body = "");

    /** @brief Seeks to a position in the current track.
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/seek-to-position-in-currently-playing-track
     * 
     * Sets the play head to a given position on the track that is currently
     * playing. Can choose a device id to change the player seek of. The device
     * id can be empty meaning that it uses the current player device. If the 
     * position parameter is greater than the length of the song, the next 
     * track will begin playing.
     * 
     * @param[in] position Position in milliseconds to seek to. Must be Positive.
     * @param[in] deviceId optional, Device you want to seek the player of.
     * 
     * @return True on -- successfully set the players head.
     * 
     * @note Requires Spotify premium.    
     * 
    */
    SpotifyResult seekToPosition(int position, const char *deviceId = "");
    
    /** @brief Transfers playback to a new device and optionally begins playback. 
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/transfer-a-users-playback
     * 
     * Transfers playback from one device that may currently be playing audio
     * and can start playing audio on that device too. The device id must be a
     * valid device id from @ref getAvailableDevices.
     * 
     * @param[in] deviceId An id of a playback device.
     * @param[in] play optional, True for -- plays on the device transferred to.
     * 
     * @return True on -- successfully transferred playback to the device.
     * 
     * @note Requires Spotify premium. 
    */
    SpotifyResult transferPlayback(const char *deviceId, bool play = false);

    /** @brief Send custom spotify navigation commands to its API.
     * 
     * Unlike the above functions you can send any commands using this 
     * function. However it is suggested that you use one of the above
     * methods for simplicity and ease of use.
     * 
     * @param[in] command The command to run on the spotify navigation API.
     * @param[in] deviceId The device to act this command upon. 
     * 
     * @return True on -- successful response from Spotify's API.
     * 
     * @note Requires Spotify premium. 
     * 
     */
    SpotifyResult playerNavigate(char *command, const char *deviceId = "");
    
// ========================================
// Search API
// ========================================

    /** @brief Search Spotify's library for track.
     * 
     * @url https://developer.spotify.com/documentation/web-api/reference/search
     * 
     * Uses Spotify's search API to find a track. Will return a limited number 
     * of items resulted items from the search. The results parameter is 
     * optional if you don't need an array of your search results. When results 
     * is a valid value, it must be an array size of limit!
     * 
     * @param[in] query Your query for you are looking for, see the spotify docs link for more info.
     * @param[in] limit Max items listed by the search results.
     * @param[in] callback A callback ran for every search result found.
     * @param[out] results optional, All search results, must be size of the limit parameter. 
     * 
     * @return The http code returned from the get request. 
     * 
     */
    SpotifyResult searchForSong(String query, int limit, SpotifyCallbackOnSearch searchCallback, SpotifySearchResult* results);

// ========================================
// Image API
// ========================================

    /** @brief Downloads an image from Spotify's image server and streams it.
     *
     * Downloads cover art, user image and other images from Spotify's image 
     * server. This function uses a stream to place the image file into. You
     * can use the stream to retrieve some amount of data when required. The 
     * image files format is JPEG.
     * 
     * @param[in] imageUrl The image url from one of the above Spotify requests like @ref getCurrentlyPlayingTrack.
     * @param[out] length The length of the image in bytes before we stream it in.
     * 
     * @return True on -- image length is greater than 0.
     * 
     * @note It might make it easier on the system to make this an insecure 
     * request. The request isn't connected to your account in any way 
     * because its just an image link.
     * 
     */
    SpotifyResult requestImage(char *imageUrl, size_t* length);


    /** @brief Pipes the Spotify image data into a stream. 
     * 
     * 
     * 
     */
    SpotifyResult getImage(Stream* stream);

    /** @brief Reads the image data into a buffer.
     *
     * 
     *  
     */
    SpotifyResult getImage(uint8_t* buffer);

    /** @brief Downloads an image from Spotify's image server and saves it to a buffer. 
     * 
     * Downloads cover art, user images and other Spotify images from the 
     * server. This function creates a large buffer to save the image in. 
     * However that buffer may be too large for the system at any given moment
     * so it will return false if the buffer could not be allocated.
     * 
     * @param[in] imageUrl The image url from one of the above requests, like @ref getCurrentlyPlayingTrack.
     * @param[out] image The newly created jpeg image as an array of bytes. 
     * @param[out] imageLength The amount of bytes in the jpeg image array from @ref image.
     * 
     * @return True on -- successfully allocated a buffer and downloaded the jpeg image.
     * 
    */
    // bool getImage(char *imageUrl, uint8_t *image, int imageLength);

    int portNumber = 443;
    int currentlyPlayingBufferSize = 3000;
    int playerDetailsBufferSize = 2000;
    int getDevicesBufferSize = 3000;
    int searchDetailsBufferSize = 3000;
    bool autoTokenRefresh = true;

private:

    /** @brief Default Constructor.
     * 
     *  This constructor only clears the memory of this object. We shouldn't
     *  have to use it in the public space because there is another constructor
     *  for your purpose whether using PKCE or code authentication.
     *   
     */
    SpotifyESP();

    SpotifyCodeFlow _flow;
    char _bearerToken[SPOTIFY_ACCESS_TOKEN_LENGTH+10]; // 10 extra is for "bearer " at the start
    char _verifier[SPOTIFY_PKCE_CODE_LENGTH+1];
    String _refreshToken;
    const char* _clientId;
    const char* _clientSecret;
    unsigned int timeTokenRefreshed;
    unsigned int tokenTimeToLiveMs;
    WiFiClientSecure* _wifiClient;
    HTTPClient* _httpClient;
    int _imageLength;
    
    // Generic Request Methods
    int makeGetRequest(const char *command, const char *authorization, const char *accept = "application/json", const char *host = SPOTIFY_HOST);
    int makeRequestWithBody(const char *type, const char *command, const char *authorization, const char *body = "", const char *contentType = "application/json", const char *host = SPOTIFY_HOST);
    int makePostRequest(const char *command, const char *authorization, const char *body = "", const char *contentType = "application/json", const char *host = SPOTIFY_HOST);
    int makePutRequest(const char *command, const char *authorization, const char *body = "", const char *contentType = "application/json", const char *host = SPOTIFY_HOST);

    SpotifyResult processJsonError(DeserializationError error);
    SpotifyResult processAuthenticationError();
    SpotifyResult processRegularError(int code);


    const char *requestAccessTokensBody = R"(grant_type=authorization_code&code=%s&redirect_uri=%s&client_id=%s&client_secret=%s)";
    const char *requestAccessTokensBodyPKCE = R"(client_id=%s&grant_type=authorization_code&redirect_uri=%s&code=%s&code_verifier=%s)";
    const char *refreshAccessTokensBody = R"(grant_type=refresh_token&refresh_token=%s&client_id=%s&client_secret=%s)";
    const char *refreshAccessTokensBodyPKCE = R"(grant_type=refresh_token&refresh_token=%s&client_id=%s)";

};

