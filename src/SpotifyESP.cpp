#include <mbedtls/sha256.h>

#include "SpotifyESP.h"

SpotifyESP::SpotifyESP(WiFiClient &wifiClient, HTTPClient &httpClient)
{
    this->_wifiClient = &wifiClient;
    this->_httpClient = &httpClient;
}

SpotifyESP::SpotifyESP(WiFiClient &wifiClient, HTTPClient &httpClient, char *bearerToken)
{
    this->_wifiClient = &wifiClient;
    this->_httpClient = &httpClient;
    sprintf(this->_bearerToken, "Bearer %s", bearerToken);
}

SpotifyESP::SpotifyESP(WiFiClient &wifiClient, HTTPClient &httpClient, const char *clientId)
{
    this->_wifiClient = &wifiClient;
    this->_httpClient = &httpClient;
    this->_clientId = clientId;
}

SpotifyESP::SpotifyESP(WiFiClient &wifiClient, HTTPClient &httpClient, const char *clientId, const char *clientSecret, const char *refreshToken)
{
    this->_wifiClient = &wifiClient;
    this->_httpClient = &httpClient;
    this->_clientId = clientId;
    this->_clientSecret = clientSecret;
    setRefreshToken(refreshToken);
}

void SpotifyESP::setClientId(const char* clientId)
{
    _clientId = clientId;
}

void SpotifyESP::generateCodeChallengeForPKCE(char* buffer)
{
    /* Reset any previous values. */
    memset(_verifier, 0, sizeof(_verifier));
    memset(_verifierChallenge, 0, sizeof(_verifierChallenge));
    memset(buffer, 0, SPOTIFY_PKCE_CODE_HASHED_LENGTH);

    /* PKCE can only contain letters, digits, underscores, periods, hyphens, or tildes. */
    char verifierDict[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";

    /* Generate a random verifier using the hardware randomizer. */
    for (int i = 0; i < SPOTIFY_PKCE_CODE_LENGTH; i++)
        _verifier[i] = verifierDict[random(sizeof(verifierDict)-1)];

    /* Hash the verifier using SHA256. */
    unsigned char verifierHashed[32];
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, false);
    mbedtls_sha256_update(&ctx, _verifier, SPOTIFY_PKCE_CODE_LENGTH);
    mbedtls_sha256_finish(&ctx, verifierHashed);
    mbedtls_sha256_free(&ctx);

    /* Encode the hashed verifier to base64. 
        The data is encoded into 'buffer'. */
    SpotifyBase64::encode(verifierHashed, sizeof(verifierHashed), (uint8_t*)buffer);

    log_i("Verifier: %.64s", _verifier);
    log_i("Verifier Challenge: %.32s", verifierHashed);
    // log_i("Verifier Challenge Encoded: %s", buffer); /* We don't know how long the user's string actually is. */
}

void SpotifyESP::generateRedirectForPKCE(
        SpotifyScopeFlags scopes, 
        const char *redirect, 
        char *buffer,
        size_t bufferLength)
{

    /* Generate the code challenge. */
    char codeChallenge[SPOTIFY_PKCE_CODE_HASHED_LENGTH+1];
    memset(codeChallenge, 0, sizeof(codeChallenge));

    generateCodeChallengeForPKCE(codeChallenge);

    /* Write most of the buffer string. */
    snprintf(buffer, bufferLength,
        "https://accounts.spotify.com/authorize/?"
        "response_type=code"
        "&client_id=%s"
        "&redirect_uri=%s"
        "&code_challenge_method=S256"
        "&code_challenge=%s",
        "&scope=", _clientId, redirect, codeChallenge);

    /* Append the scope flags onto the end. */
    auto appendScope = [&scopes, &buffer, &bufferLength](SpotifyScopeFlagBits bit, const char* str){ 
        if (scopes & bit) 
            strncat(buffer, str, bufferLength); 
    };

    if (scopes != SpotifyScopeFlagBits::eNone) 
    {
        appendScope(SpotifyScopeFlagBits::eUgcImageUpload, "ugc-image-upload+");
        appendScope(SpotifyScopeFlagBits::eUserReadPlaybackState, "user-read-playback-state+");
        appendScope(SpotifyScopeFlagBits::eUserModifyPlaybackState, "user-modify-playback-state+");
        appendScope(SpotifyScopeFlagBits::eUserReadCurrentlyPlaying, "user-read-currently-playing+");
        appendScope(SpotifyScopeFlagBits::eAppRemoteControl, "app-remote-control+");
        appendScope(SpotifyScopeFlagBits::eStreaming, "streaming+");
        appendScope(SpotifyScopeFlagBits::ePlaylistReadPrivate, "playlist-read-private+");
        appendScope(SpotifyScopeFlagBits::ePlaylistReadCollaborative, "playlist-read-collaborative+");
        appendScope(SpotifyScopeFlagBits::ePlaylistModifyPrivate, "playlist-modify-private+");
        appendScope(SpotifyScopeFlagBits::ePlaylistModifyPublic, "playlist-modify-public+");
        appendScope(SpotifyScopeFlagBits::eUserFollowModify, "user-follow-modify+");
        appendScope(SpotifyScopeFlagBits::eUserFollowRead, "user-follow-read+");
        appendScope(SpotifyScopeFlagBits::eUserReadPlaybackPosition, "user-read-playback-position+");
        appendScope(SpotifyScopeFlagBits::eUserTopRead, "user-top-read+");
        appendScope(SpotifyScopeFlagBits::eUserReadRecentlyPlayed, "user-read-recently-played+");
        appendScope(SpotifyScopeFlagBits::eUserLibraryModify, "user-library-modify+");
        appendScope(SpotifyScopeFlagBits::eUserLibraryRead, "user-library-read+");
        appendScope(SpotifyScopeFlagBits::eUserReadEmail, "user-read-email+");
        appendScope(SpotifyScopeFlagBits::eUserReadPrivate, "user-read-private+");
        appendScope(SpotifyScopeFlagBits::eUserSoaLink, "user-soa-link+");
        appendScope(SpotifyScopeFlagBits::eUserSoaUnlink, "user-soa-unlink+");
        appendScope(SpotifyScopeFlagBits::eUserManageEntitlements, "user-manage-entitlements+");
        appendScope(SpotifyScopeFlagBits::eUserManagePartner, "user-manage-partner+");
        appendScope(SpotifyScopeFlagBits::eUserCreatePartner, "user-create-partner+");

        /* Remove the last '+' from the scopes. */
        char* lastPlus = strrchr(buffer, '+');
        if (lastPlus) *lastPlus = '\0';
    }
}

void SpotifyESP::generateRedirectForPKCE(
    const char *scopes,
    const char *redirect, 
    char *buffer,
    size_t bufferLength)
{
    /* Generate the code challenge. */
    char codeChallenge[SPOTIFY_PKCE_CODE_HASHED_LENGTH+1];
    memset(codeChallenge, 0, sizeof(codeChallenge));

    generateCodeChallengeForPKCE(codeChallenge);

    /* Write most of the buffer string. */
    snprintf(buffer, bufferLength,
        "https://accounts.spotify.com/authorize/?"
        "response_type=code"
        "&client_id=%s"
        "&scope=%s"
        "&redirect_uri=%s"
        "&code_challenge_method=S256"
        "&code_challenge=%s",
        _clientId, scopes, redirect, codeChallenge);
}

int SpotifyESP::makeRequestWithBody(const char *type, const char *command, const char *authorization, const char *body, const char *contentType, const char *host)
{
    /* Setup the HTTP client for the request. */
    _httpClient->setUserAgent("TALOS/1.0");
    _httpClient->setTimeout(SPOTIFY_TIMEOUT);
    _httpClient->setConnectTimeout(SPOTIFY_TIMEOUT);
    _httpClient->setReuse(false);
    _httpClient->useHTTP10(true);
    _httpClient->begin(*_wifiClient, host, 443, command);
    
    log_d("%s", command);

    /* Give the esp a breather. */
    yield(); 

    /* Add the requests header values. */
    _httpClient->addHeader("Content-Type", contentType);

    if (authorization != NULL) _httpClient->addHeader("Authorization", authorization);
    /* _httpClient->addHeader("Cache-Control", "no-cache"); */

    /* Make the HTTP request. */
    return _httpClient->sendRequest(type, body);
}

int SpotifyESP::makePutRequest(const char *command, const char *authorization, const char *body, const char *contentType, const char *host)
{
    return makeRequestWithBody("PUT", command, authorization, body, contentType);
}

int SpotifyESP::makePostRequest(const char *command, const char *authorization, const char *body, const char *contentType, const char *host)
{
    return makeRequestWithBody("POST", command, authorization, body, contentType, host);
}

int SpotifyESP::makeGetRequest(const char *command, const char *authorization, const char *accept, const char *host)
{
    _httpClient->setUserAgent("TALOS/1.0");
    _httpClient->setTimeout(SPOTIFY_TIMEOUT);
    _httpClient->setConnectTimeout(SPOTIFY_TIMEOUT);
    _httpClient->setReuse(false);
    _httpClient->useHTTP10(true);
    _httpClient->begin(*_wifiClient, command);
    
    log_i("%s", command);

    // give the esp a breather
    yield();

    if (accept) _httpClient->addHeader("Accept", accept);
    if (authorization)  _httpClient->addHeader("Authorization", authorization);

    _httpClient->addHeader("Cache-Control", "no-cache");
    
    return _httpClient->GET();
}

void SpotifyESP::setRefreshToken(const char *refreshToken)
{
    int newRefreshTokenLen = strlen(refreshToken);
    if (_refreshToken == nullptr || strlen(_refreshToken) < newRefreshTokenLen)
    {
        delete _refreshToken;
        _refreshToken = new char[newRefreshTokenLen + 1]();
    }

    strncpy(_refreshToken, refreshToken, newRefreshTokenLen + 1);
}

bool SpotifyESP::refreshAccessToken()
{
    char body[300];
    sprintf(body, refreshAccessTokensBody, _refreshToken, _clientId, _clientSecret);

#ifdef SPOTIFY_DEBUG
    log_i("%s", body);
    printStack();
#endif

    int statusCode = makePostRequest(SPOTIFY_TOKEN_ENDPOINT, NULL, body, "application/x-www-form-urlencoded", SPOTIFY_ACCOUNTS_HOST);

    unsigned long now = millis();

#ifdef SPOTIFY_DEBUG
    log_i("status code: %d", statusCode);
#endif

    bool refreshed = false;
    if (statusCode == 200)
    {
        StaticJsonDocument<48> filter;
        filter["access_token"] = true;
        filter["token_type"] = true;
        filter["expires_in"] = true;

        DynamicJsonDocument doc(512);

        // Parse JSON object
#ifndef SPOTIFY_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, _httpClient->getStream(), DeserializationOption::Filter(filter));
#else
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
#endif
        if (!error)
        {
            log_d("No JSON error, dealing with response");
            
            const char *accessToken = doc["access_token"].as<const char *>();
            if (accessToken != NULL && (SPOTIFY_ACCESS_TOKEN_LENGTH >= strlen(accessToken)))
            {
                sprintf(this->_bearerToken, "Bearer %s", accessToken);
                int tokenTtl = doc["expires_in"];             // Usually 3600 (1 hour)
                tokenTimeToLiveMs = (tokenTtl * 1000) - 2000; // The 2000 is just to force the token expiry to check if its very close
                timeTokenRefreshed = now;
                refreshed = true;
            }
            else
            {
                log_e("Problem with access_token (too long or null): %s", accessToken);
            }
        }
        else
        {
            log_e("deserializeJson() failed with code %s", error.c_str());
        }
    }
    else
    {
        parseError();
    }

    closeClient();
    return refreshed;
}

bool SpotifyESP::checkAndRefreshAccessToken()
{
    unsigned long timeSinceLastRefresh = millis() - timeTokenRefreshed;
    if (timeSinceLastRefresh >= tokenTimeToLiveMs)
    {
#ifdef SPOTIFY_SERIAL_OUTPUT
        log_i("Refresh of the Access token is due, doing that now.");
#endif
        return refreshAccessToken();
    }

    // Token is still valid
    return true;
}

const char *SpotifyESP::requestAccessTokens(const char *code, const char *redirectUrl, bool usingPKCE)
{
    char body[768];

    if (usingPKCE) log_d("Using PKCE for Spotify authorization.");
    if (usingPKCE)
        snprintf(body, sizeof(body), requestAccessTokensBodyPKCE, _clientId, redirectUrl, code, _verifier);
    else 
        snprintf(body, sizeof(body), requestAccessTokensBody, code, redirectUrl, _clientId, _clientSecret);

    log_d("%s", body);

    int statusCode = makePostRequest(SPOTIFY_TOKEN_ENDPOINT, NULL, body, "application/x-www-form-urlencoded", SPOTIFY_ACCOUNTS_HOST);
    
    unsigned long now = millis();

    log_d("Status code: %d", statusCode);

    if (statusCode == 200)
    {
        DynamicJsonDocument doc(1000);
        // Parse JSON object
// #ifndef SPOTIFY_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, _httpClient->getStream());
// #else
//         ReadLoggingStream loggingStream(*client, Serial);
//         DeserializationError error = deserializeJson(doc, loggingStream);
// #endif
        if (!error)
        {
            sprintf(this->_bearerToken, "Bearer %s", doc["access_token"].as<const char *>());
            setRefreshToken(doc["refresh_token"].as<const char *>());
            int tokenTtl = doc["expires_in"];             // Usually 3600 (1 hour)
            tokenTimeToLiveMs = (tokenTtl * 1000) - 2000; // The 2000 is just to force the token expiry to check if its very close
            timeTokenRefreshed = now;
        }
        else
        {
            log_i("deserializeJson() failed with code %s", error.c_str());
        }
    }
    else
    {
        parseError();
    }

    closeClient();
    return _refreshToken;
}

bool SpotifyESP::play(const char *deviceId)
{
    char command[100] = SPOTIFY_PLAY_ENDPOINT;
    return playerControl(command, deviceId);
}

bool SpotifyESP::playAdvanced(char *body, const char *deviceId)
{
    char command[100] = SPOTIFY_PLAY_ENDPOINT;
    return playerControl(command, deviceId, body);
}

bool SpotifyESP::pause(const char *deviceId)
{
    char command[100] = SPOTIFY_PAUSE_ENDPOINT;
    return playerControl(command, deviceId);
}

bool SpotifyESP::setVolume(int volume, const char *deviceId)
{
    char command[125];
    sprintf(command, SPOTIFY_VOLUME_ENDPOINT, volume);
    return playerControl(command, deviceId);
}

bool SpotifyESP::toggleShuffle(bool shuffle, const char *deviceId)
{
    char command[125];
    char shuffleState[10];

    strcpy(shuffleState, shuffle ? "true" : "false");

    sprintf(command, SPOTIFY_SHUFFLE_ENDPOINT, shuffleState);
    return playerControl(command, deviceId);
}

bool SpotifyESP::setRepeatMode(SpotifyRepeatMode repeat, const char *deviceId)
{
    char command[125];
    char repeatState[10];
    switch (repeat)
    {
    case SpotifyRepeatMode::eTrack:
        strcpy(repeatState, "track");
        break;
    case SpotifyRepeatMode::eContext:
        strcpy(repeatState, "context");
        break;
    case SpotifyRepeatMode::eOff:
        strcpy(repeatState, "off");
        break;
    }

    sprintf(command, SPOTIFY_REPEAT_ENDPOINT, repeatState);
    return playerControl(command, deviceId);
}

bool SpotifyESP::playerControl(char *command, const char *deviceId, const char *body)
{
    if (deviceId[0] != 0)
    {
        char *questionMarkPointer;
        questionMarkPointer = strchr(command, '?');
        char deviceIdBuff[50];

        if (questionMarkPointer == NULL)
            sprintf(deviceIdBuff, "?device_id=%s", deviceId);
        else // params already started
            sprintf(deviceIdBuff, "&device_id=%s", deviceId);

        strcat(command, deviceIdBuff);
    }

    log_d("%s", command);
    log_d("%s", body);

    if (autoTokenRefresh)
        checkAndRefreshAccessToken();

    int statusCode = makePutRequest(command, _bearerToken, body);
    closeClient();

    /* Will return 204 if all went well. */
    return statusCode == 204;
}

bool SpotifyESP::playerNavigate(char *command, const char *deviceId)
{
    if (deviceId[0] != 0)
    {
        char deviceIdBuff[50];
        sprintf(deviceIdBuff, "?device_id=%s", deviceId);
        strcat(command, deviceIdBuff);
    }

    log_d("%s", command);

    if (autoTokenRefresh)
        checkAndRefreshAccessToken();

    int statusCode = makePostRequest(command, _bearerToken);
    closeClient();

    /* Will return 204 if all went well. */
    return statusCode == 204;
}

bool SpotifyESP::skipToNext(const char *deviceId)
{
    char command[100] = SPOTIFY_NEXT_TRACK_ENDPOINT;
    return playerNavigate(command, deviceId);
}

bool SpotifyESP::skipToPrevious(const char *deviceId)
{
    char command[100] = SPOTIFY_PREVIOUS_TRACK_ENDPOINT;
    return playerNavigate(command, deviceId);
}

bool SpotifyESP::seek(int position, const char *deviceId)
{
    char command[100] = SPOTIFY_SEEK_ENDPOINT;
    char tempBuff[100];
    sprintf(tempBuff, "?position_ms=%d", position);
    strcat(command, tempBuff);
    if (deviceId[0] != 0)
    {
        sprintf(tempBuff, "?device_id=%s", deviceId);
        strcat(command, tempBuff);
    }

    log_d("%s", command);

#ifdef SPOTIFY_DEBUG
    printStack();
#endif

    if (autoTokenRefresh)
        checkAndRefreshAccessToken();

    int statusCode = makePutRequest(command, _bearerToken);
    closeClient();

    return statusCode == 204; /* Will return 204 if all went well. */
}

bool SpotifyESP::transferPlayback(const char *deviceId, bool play)
{
    char body[100];
    sprintf(body, "{\"device_ids\":[\"%s\"],\"play\":\"%s\"}", deviceId, (play ? "true" : "false"));

    log_d("%s", SPOTIFY_PLAYER_ENDPOINT);
    log_d("%s", body);

#ifdef SPOTIFY_DEBUG
    printStack();
#endif

    if (autoTokenRefresh)
        checkAndRefreshAccessToken();

    int statusCode = makePutRequest(SPOTIFY_PLAYER_ENDPOINT, _bearerToken, body);
    closeClient();
    
    return statusCode == 204; /* Will return 204 if all went well. */
}

int SpotifyESP::getCurrentlyPlayingTrack(SpotifyCallbackOnCurrentlyPlaying currentlyPlayingCallback, const char *market)
{
    char command[120] = SPOTIFY_CURRENTLY_PLAYING_ENDPOINT;
    if (market[0] != 0)
    {
        char marketBuff[15];
        sprintf(marketBuff, "&market=%s", market);
        strcat(command, marketBuff);
    }

    log_d("%s", command);

#ifdef SPOTIFY_DEBUG
    printStack();
#endif

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = currentlyPlayingBufferSize;

    if (autoTokenRefresh)
        checkAndRefreshAccessToken();

    int statusCode = makeGetRequest(command, _bearerToken);
    log_d("%s", statusCode);

#ifdef SPOTIFY_DEBUG
    printStack();
#endif

    if (statusCode == 200)
    {
        SpotifyCurrentlyPlaying current;

        //Apply Json Filter: https://arduinojson.org/v6/example/filter/
        StaticJsonDocument<464> filter;
        filter["is_playing"] = true;
        filter["currently_playing_type"] = true;
        filter["progress_ms"] = true;
        filter["context"]["uri"] = true;

        JsonObject filter_item = filter.createNestedObject("item");
        filter_item["duration_ms"] = true;
        filter_item["name"] = true;
        filter_item["uri"] = true;

        JsonObject filter_item_artists_0 = filter_item["artists"].createNestedObject();
        filter_item_artists_0["name"] = true;
        filter_item_artists_0["uri"] = true;

        JsonObject filter_item_album = filter_item.createNestedObject("album");
        filter_item_album["name"] = true;
        filter_item_album["uri"] = true;

        JsonObject filter_item_album_images_0 = filter_item_album["images"].createNestedObject();
        filter_item_album_images_0["height"] = true;
        filter_item_album_images_0["width"] = true;
        filter_item_album_images_0["url"] = true;

        // Podcast filters
        JsonObject filter_item_show = filter_item.createNestedObject("show");
        filter_item_show["name"] = true;
        filter_item_show["uri"] = true;

        JsonObject filter_item_images_0 = filter_item["images"].createNestedObject();
        filter_item_images_0["height"] = true;
        filter_item_images_0["width"] = true;
        filter_item_images_0["url"] = true;

        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
#ifndef SPOTIFY_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, _httpClient->getStream(), DeserializationOption::Filter(filter));
#else
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
#endif
        if (!error)
        {
#ifdef SPOTIFY_DEBUG
            serializeJsonPretty(doc, Serial);
#endif
            JsonObject item = doc["item"];

            const char *currently_playing_type = doc["currently_playing_type"];

            current.isPlaying = doc["is_playing"].as<bool>();

            current.progressMs = doc["progress_ms"].as<long>();
            current.durationMs = item["duration_ms"].as<long>();

            // context may be null
            if (!doc["context"].isNull())
            {
                strncpy(current.contextUri, doc["context"]["uri"].as<const char *>(), sizeof(current.contextUri)-1);   
            }
            else
            {
                memset(current.contextUri, 0, sizeof(current.contextUri));
            }

            // Check currently playing type
            if (strcmp(currently_playing_type, "track") == 0)
            {
                current.currentlyPlayingType = SpotifyPlayingType::eTrack;
            }
            else if (strcmp(currently_playing_type, "episode") == 0)
            {
                current.currentlyPlayingType = SpotifyPlayingType::eEpisode;
            }
            else
            {
                current.currentlyPlayingType = SpotifyPlayingType::eUnknown;
            }

            // If it's a song/track
            if (current.currentlyPlayingType == SpotifyPlayingType::eTrack)
            {
                int numArtists = item["artists"].size();
                if (numArtists > SPOTIFY_MAX_NUM_ARTISTS)
                {
                    numArtists = SPOTIFY_MAX_NUM_ARTISTS;
                }
                current.numArtists = numArtists;

                for (int i = 0; i < current.numArtists; i++)
                {
                    strncpy(current.artists[i].artistName, item["artists"][i]["name"].as<const char *>(), sizeof(current.artists[i].artistName)-1);
                    strncpy(current.artists[i].artistUri, item["artists"][i]["uri"].as<const char *>(), sizeof(current.artists[i].artistUri)-1);
                }

                strncpy(current.albumName, item["album"]["name"].as<const char *>(), sizeof(current.albumName)-1);
                strncpy(current.albumUri, item["album"]["uri"].as<const char *>(), sizeof(current.albumUri)-1);

                JsonArray images = item["album"]["images"];

                // Images are returned in order of width, so last should be smallest.
                int numImages = images.size();
                int startingIndex = 0;
                if (numImages > SPOTIFY_NUM_ALBUM_IMAGES)
                {
                    startingIndex = numImages - SPOTIFY_NUM_ALBUM_IMAGES;
                    current.numImages = SPOTIFY_NUM_ALBUM_IMAGES;
                }
                else
                {
                    current.numImages = numImages;
                }

                log_d("Num Images: %s", current.numImages);
                log_d("%s", numImages);

                for (int i = 0; i < current.numImages; i++)
                {
                    int adjustedIndex = startingIndex + i;
                    current.albumImages[i].height = images[adjustedIndex]["height"].as<int>();
                    current.albumImages[i].width = images[adjustedIndex]["width"].as<int>();
                    strncpy(current.albumImages[i].url, images[adjustedIndex]["url"].as<const char *>(), sizeof(current.albumImages[i].url)-1);
                }

                strncpy(current.trackName, item["name"].as<const char *>(), sizeof(current.trackName)-1);
                strncpy(current.trackUri, item["uri"].as<const char *>(), sizeof(current.trackUri)-1);
            }
            else if (current.currentlyPlayingType == SpotifyPlayingType::eEpisode) // Podcast
            {
                current.numArtists = 1;

                // Save Podcast as the "track"
                strncpy(current.trackName, item["name"].as<const char *>(), sizeof(current.trackName)-1);
                strncpy(current.trackUri, item["uri"].as<const char *>(), sizeof(current.trackUri)-1);

                // Save Show name as the "artist"
                strncpy(current.artists[0].artistName, item["show"]["name"].as<const char *>(), sizeof(current.artists[0].artistName)-1);
                strncpy(current.artists[0].artistUri, item["show"]["uri"].as<const char *>(), sizeof(current.artists[0].artistUri)-1);

                // Leave "album" name blank
                char blank[1] = "";
                strncpy(current.albumName, blank, sizeof(current.albumName)-1);
                strncpy(current.albumUri, blank, sizeof(current.albumUri)-1);

                // Save the episode images as the "album art"
                JsonArray images = item["images"];
                // Images are returned in order of width, so last should be smallest.
                int numImages = images.size();
                int startingIndex = 0;
                if (numImages > SPOTIFY_NUM_ALBUM_IMAGES)
                {
                    startingIndex = numImages - SPOTIFY_NUM_ALBUM_IMAGES;
                    current.numImages = SPOTIFY_NUM_ALBUM_IMAGES;
                }
                else
                {
                    current.numImages = numImages;
                }

                log_d("Num images in current: %s", current.numImages);
                log_d("Num images%s", numImages);

                for (int i = 0; i < current.numImages; i++)
                {
                    int adjustedIndex = startingIndex + i;
                    current.albumImages[i].height = images[adjustedIndex]["height"].as<int>();
                    current.albumImages[i].width = images[adjustedIndex]["width"].as<int>();
                    strncpy(current.albumImages[i].url, images[adjustedIndex]["url"].as<const char *>(), sizeof(current.albumImages[i].url)-1);
                }
            }

            currentlyPlayingCallback(current);
        }
        else
        {
            log_e("deserializeJson() failed with code %s", error.c_str());
            statusCode = -1;
        }
    }

    closeClient();
    return statusCode;
}

int SpotifyESP::getPlaybackState(SpotifyCallbackOnPlaybackState playerDetailsCallback, const char *market)
{
    char command[100] = SPOTIFY_PLAYER_ENDPOINT;
    if (market[0] != 0)
    {
        char marketBuff[30];
        sprintf(marketBuff, "?market=%s", market);
        strcat(command, marketBuff);
    }

    log_d("%s", command);

#ifdef SPOTIFY_DEBUG
    printStack();
#endif

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = playerDetailsBufferSize;
    if (autoTokenRefresh)
        checkAndRefreshAccessToken();

    int statusCode = makeGetRequest(command, _bearerToken);
    log_d("Status Code: %s", statusCode);

    if (statusCode == 200)
    {

        StaticJsonDocument<192> filter;
        JsonObject filter_device = filter.createNestedObject("device");
        filter_device["id"] = true;
        filter_device["name"] = true;
        filter_device["type"] = true;
        filter_device["is_active"] = true;
        filter_device["is_private_session"] = true;
        filter_device["is_restricted"] = true;
        filter_device["volume_percent"] = true;
        filter["progress_ms"] = true;
        filter["is_playing"] = true;
        filter["shuffle_state"] = true;
        filter["repeat_state"] = true;

        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
#ifndef SPOTIFY_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, _httpClient->getStream(), DeserializationOption::Filter(filter));
#else
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
#endif
        if (!error)
        {
            SpotifyPlayerDetails playerDetails;

            JsonObject device = doc["device"];
            // Copy into buffer and make the last character a null just incase we went over.
            strncpy(playerDetails.device.id, device["id"].as<const char *>(), sizeof(playerDetails.device.id)-1);
            strncpy(playerDetails.device.name, device["name"].as<const char *>(), sizeof(playerDetails.device.name)-1);
            strncpy(playerDetails.device.type, device["type"].as<const char *>(), sizeof(playerDetails.device.type)-1);

            playerDetails.device.isActive = device["is_active"].as<bool>();
            playerDetails.device.isPrivateSession = device["is_private_session"].as<bool>();
            playerDetails.device.isRestricted = device["is_restricted"].as<bool>();
            playerDetails.device.volumePercent = device["volume_percent"].as<int>();

            playerDetails.progressMs = doc["progress_ms"].as<long>();
            playerDetails.isPlaying = doc["is_playing"].as<bool>();

            playerDetails.shuffleState = doc["shuffle_state"].as<bool>();

            const char *repeat_state = doc["repeat_state"];

            if (strncmp(repeat_state, "eTrack", 5) == 0)
            {
                playerDetails.repeatState = SpotifyRepeatMode::eTrack;
            }
            else if (strncmp(repeat_state, "context", 7) == 0)
            {
                playerDetails.repeatState = SpotifyRepeatMode::eContext;
            }
            else
            {
                playerDetails.repeatState = SpotifyRepeatMode::eOff;
            }

            playerDetailsCallback(playerDetails);
        }
        else
        {
            log_e("deserializeJson() failed with code %s", error.c_str());
            statusCode = -1;
        }
    }

    closeClient();

    return statusCode;
}

int SpotifyESP::getAvailableDevices(SpotifyCallbackOnDevices devicesCallback)
{
    log_i(SPOTIFY_DEVICES_ENDPOINT);

#ifdef SPOTIFY_DEBUG
    printStack();
#endif

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = getDevicesBufferSize;
    if (autoTokenRefresh)
        checkAndRefreshAccessToken();

    int statusCode = makeGetRequest(SPOTIFY_DEVICES_ENDPOINT, _bearerToken);
    log_d("Status Code: %s", statusCode);

    if (statusCode == 200)
    {

        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
#ifndef SPOTIFY_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, _httpClient->getStream());
#else
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream);
#endif
        if (!error)
        {

            uint8_t totalDevices = doc["devices"].size();

            SpotifyDevice spotifyDevice;
            for (int i = 0; i < totalDevices; i++)
            {
                JsonObject device = doc["devices"][i];
                strncpy(spotifyDevice.id, device["id"].as<const char *>(), sizeof(spotifyDevice.id)-1);
                strncpy(spotifyDevice.name, device["name"].as<const char *>(), sizeof(spotifyDevice.name)-1);
                strncpy(spotifyDevice.type, device["type"].as<const char *>(), sizeof(spotifyDevice.type)-1);

                spotifyDevice.isActive = device["is_active"].as<bool>();
                spotifyDevice.isPrivateSession = device["is_private_session"].as<bool>();
                spotifyDevice.isRestricted = device["is_restricted"].as<bool>();
                spotifyDevice.volumePercent = device["volume_percent"].as<int>();

                if (!devicesCallback(spotifyDevice, i, totalDevices))
                {
                    //User has indicated they are finished.
                    break;
                }
            }
        }
        else
        {
            log_e("deserializeJson() failed with code %s", error.c_str());
            statusCode = -1;
        }
    }

    closeClient();

    return statusCode;
}

int SpotifyESP::searchForSong(String query, int limit, SpotifyCallbackOnSearch searchCallback, SpotifySearchResult results[])
{

    log_i(SPOTIFY_SEARCH_ENDPOINT);

#ifdef SPOTIFY_DEBUG
    printStack();
#endif

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = searchDetailsBufferSize;
    if (autoTokenRefresh)
        checkAndRefreshAccessToken();

    int statusCode = makeGetRequest((SPOTIFY_SEARCH_ENDPOINT + query + "&limit=" + limit).c_str(), _bearerToken);
    log_d("Status Code: %d", statusCode);

    if (statusCode == 200)
    {

        // Allocate DynamicJsonDocument
        DynamicJsonDocument doc(bufferSize);

        // Parse JSON object
#ifndef SPOTIFY_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, _httpClient->getStream());
#else
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream);
#endif
        if (!error)
        {

            uint8_t totalResults = doc["tracks"]["items"].size();

            log_i("Total Results: %d", totalResults);

            SpotifySearchResult searchResult;
            for (int i = 0; i < totalResults; i++)
            {
                //Polling track information
                JsonObject result = doc["tracks"]["items"][i];
                strncpy(searchResult.trackUri, result["uri"].as<const char *>(), sizeof(searchResult.trackUri)-1);
                strncpy(searchResult.trackName, result["name"].as<const char *>(), sizeof(searchResult.trackName)-1);
                strncpy(searchResult.albumUri, result["album"]["uri"].as<const char *>(), sizeof(searchResult.albumUri)-1);
                strncpy(searchResult.albumName, result["album"]["name"].as<const char *>(), sizeof(searchResult.albumName)-1);

                //Pull artist Information for the result
                uint8_t totalArtists = result["artists"].size();
                searchResult.numArtists = totalArtists;

                SpotifyArtist artist;
                for (int j = 0; j < totalArtists; j++)
                {
                    JsonObject artistResult = result["artists"][j];
                    strncpy(artist.artistName, artistResult["name"].as<const char *>(), sizeof(artist.artistName)-1);
                    strncpy(artist.artistUri, artistResult["uri"].as<const char *>(), sizeof(artist.artistUri)-1);
                    searchResult.artists[j] = artist;
                }

                uint8_t totalImages = result["album"]["images"].size();
                searchResult.numImages = totalImages;

                SpotifyImage image;
                for (int j = 0; j < totalImages; j++)
                {
                    JsonObject imageResult = result["album"]["images"][j];
                    image.height = imageResult["height"].as<int>();
                    image.width = imageResult["width"].as<int>();
                    strncpy(image.url, imageResult["url"].as<const char *>(), sizeof(image.url)-1);
                    searchResult.albumImages[j] = image;
                }

                //log_i(searchResult.trackName);
                results[i] = searchResult;

                if (i >= limit || !searchCallback(searchResult, i, totalResults))
                {
                    //Break at the limit or when indicated
                    break;
                }
            }
        }
        else
        {
            log_e("deserializeJson() failed with code %s", error.c_str());
            statusCode = -1;
        }
    }

    closeClient();

    return statusCode;
}

int SpotifyESP::commonGetImage(char *imageUrl)
{
    log_d("Parsing image URL: %s", imageUrl);

    uint8_t lengthOfString = strlen(imageUrl);

    // We are going to just assume https, that's all I've
    // seen and I can't imagine a company will go back
    // to http

    if (strncmp(imageUrl, "https://", 8) != 0)
    {
        log_e("Url not in expected format: %s", imageUrl);
        log_e("(expected it to start with \"https://\")");

        return false;
    }

    uint8_t protocolLength = 8;

    char *pathStart = strchr(imageUrl + protocolLength, '/');
    uint8_t pathIndex = pathStart - imageUrl;
    uint8_t pathLength = lengthOfString - pathIndex;
    char path[pathLength + 1];
    strncpy(path, pathStart, pathLength);
    path[pathLength] = '\0';

    uint8_t hostLength = pathIndex - protocolLength;
    char host[hostLength + 1];
    strncpy(host, imageUrl + protocolLength, hostLength);
    host[hostLength] = '\0';

    log_i("host: %s", host);
    log_i("len:host: %d", hostLength);
    log_i("path: %s", path);
    log_i("len:path: %d", strlen(path));

    int statusCode = makeGetRequest(path, NULL, "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8", host);
    log_d("statusCode: %d", statusCode);

    if (statusCode == 200)
    {
        return getContentLength();
    }

    // Failed
    return -1;
}

bool SpotifyESP::getImage(char *imageUrl, Stream *file)
{
    int totalLength = commonGetImage(imageUrl);

    log_d("file length: %d", totalLength);

    if (totalLength > 0)
    {
        int remaining = totalLength;
        // This section of code is inspired but the "Web_Jpg"
        // example of TJpg_Decoder
        // https://github.com/Bodmer/TJpg_Decoder
        // -----------
        uint8_t buff[128] = {0};
        while (_httpClient->connected() && (remaining > 0 || remaining == -1))
        {
            // Get available data size
            size_t size = _httpClient->getStream().available();

            if (size)
            {
                // Read up to 128 bytes
                int c = _httpClient->getStream().readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                // Write it to file
                file->write(buff, c);

                // Calculate remaining bytes
                if (remaining > 0)
                {
                    remaining -= c;
                }
            }
            yield();
        }
// ---------
        log_d("Finished getting image");
    }

    closeClient();

    return (totalLength > 0); //Probably could be improved!
}

bool SpotifyESP::getImage(char *imageUrl, uint8_t **image, int *imageLength)
{
    int totalLength = commonGetImage(imageUrl);

    log_d("file length: %d", totalLength);

    if (totalLength > 0)
    {
        uint8_t *imgPtr = (uint8_t *)malloc(totalLength);
        *image = imgPtr;
        *imageLength = totalLength;
        int remaining = totalLength;
        int amountRead = 0;

        log_d("Fetching Image");

        // This section of code is inspired but the "Web_Jpg"
        // example of TJpg_Decoder
        // https://github.com/Bodmer/TJpg_Decoder
        // -----------
        uint8_t buff[128] = {0};
        while (_httpClient->connected() && (remaining > 0 || remaining == -1))
        {
            // Get available data size
            size_t size = _httpClient->getStream().available();

            if (size)
            {
                // Read up to 128 bytes
                int c = _httpClient->getStream().readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                // Write it to file
                memcpy((uint8_t *)imgPtr + amountRead, (uint8_t *)buff, c);

                // Calculate remaining bytes
                if (remaining > 0)
                {
                    amountRead += c;
                    remaining -= c;
                }
            }
            yield();
        }
// ---------
        log_d("Finished getting image");
    }

    closeClient();

    return (totalLength > 0); //Probably could be improved!
}

int SpotifyESP::getContentLength()
{
#ifdef SPOTIFY_DEBUG
        log_i("Content-Length: %d", _httpClient->getSize());
#endif

    return _httpClient->getSize();
}

void SpotifyESP::parseError()
{
    //This method doesn't currently do anything other than print
    DynamicJsonDocument doc(1000);
    DeserializationError error = deserializeJson(doc, _httpClient->getStream());
    if (!error)
    {
        log_i("getAuthToken error");
        serializeJson(doc, Serial);
    }
    else
    {
        log_i("Could not parse error");
    }
}

void SpotifyESP::lateInit(const char *clientId, const char *clientSecret, const char *refreshToken)
{
    this->_clientId = clientId;
    this->_clientSecret = clientSecret;
    setRefreshToken(refreshToken);
}

void SpotifyESP::closeClient()
{
    if (_httpClient->connected())
    {
        log_d("Closing client");
        _httpClient->end();
    }
}

#ifdef SPOTIFY_DEBUG
void SpotifyESP::printStack()
{
    char stack;
    log_i("stack size %d", stack_start - &stack);
}
#endif
