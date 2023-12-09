/*
SpotifyArduino - An Arduino library to wrap the Spotify API

Copyright (c) 2021  Brian Lough.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <mbedtls/sha256.h>

#define BASE64_SPOTIFY_ARDUINO_IMPLEMENTATION
#define BASE64_SPOTIFY_ARDUINO_URL
#include <SpotifyBase64.h>
#include "SpotifyArduino.h"

SpotifyArduino::SpotifyArduino(WiFiClient &wifiClient, HTTPClient &httpClient)
{
    this->_wifiClient = &wifiClient;
    this->_httpClient = &httpClient;
}

SpotifyArduino::SpotifyArduino(WiFiClient &wifiClient, HTTPClient &httpClient, char *bearerToken)
{
    this->_wifiClient = &wifiClient;
    this->_httpClient = &httpClient;
    sprintf(this->_bearerToken, "Bearer %s", bearerToken);
}

SpotifyArduino::SpotifyArduino(WiFiClient &wifiClient, HTTPClient &httpClient, const char *clientId)
{
    this->_wifiClient = &wifiClient;
    this->_httpClient = &httpClient;
    this->_clientId = clientId;
}

SpotifyArduino::SpotifyArduino(WiFiClient &wifiClient, HTTPClient &httpClient, const char *clientId, const char *clientSecret, const char *refreshToken)
{
    this->_wifiClient = &wifiClient;
    this->_httpClient = &httpClient;
    this->_clientId = clientId;
    this->_clientSecret = clientSecret;
    setRefreshToken(refreshToken);
}

void SpotifyArduino::setClientId(const char* clientId)
{
    _clientId = clientId;
}

const char* SpotifyArduino::generateCodeChallengeForPKCE()
{
    /* Reset any previous values. */
    memset(_verifier, 0, sizeof(_verifier));
    memset(_verifierChallenge, 0, sizeof(_verifierChallenge));

    /* PKCE can only contain letters, digits, underscores, periods, hyphens, or tildes*/
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

    /* Encode the hashed verifier to base64. */
    SpotifyBase64::encode(verifierHashed, sizeof(verifierHashed), _verifierChallenge);

    log_i("Verifier: %.64s", _verifier);
    log_i("Verifier Challenge: %.32s", verifierHashed);
    log_i("Verifier Challenge Encoded: %s", _verifierChallenge);

    /* Return the challenge that the user can submit to Spotify. */
    return (char*)_verifierChallenge;
}

int SpotifyArduino::makeRequestWithBody(const char *type, const char *command, const char *authorization, const char *body, const char *contentType, const char *host)
{
    /* Setup the HTTP client for the request. */
    _httpClient->setUserAgent("TALOS/1.0");
    _httpClient->setTimeout(SPOTIFY_TIMEOUT);
    _httpClient->setConnectTimeout(SPOTIFY_TIMEOUT);
    _httpClient->setReuse(false);
    _httpClient->useHTTP10(true);
    _httpClient->begin(*_wifiClient, host, 443, command);
    
#ifdef SPOTIFY_DEBUG
    log_i("%s", command);
#endif

    /* Give the esp a breather. */
    yield(); 

    /* Add the requests header values. */
    _httpClient->addHeader("Content-Type", contentType);

    if (authorization != NULL) _httpClient->addHeader("Authorization", authorization);
    /* _httpClient->addHeader("Cache-Control", "no-cache"); */

    /* Make the HTTP request. */
    return _httpClient->sendRequest(type, body);
}

int SpotifyArduino::makePutRequest(const char *command, const char *authorization, const char *body, const char *contentType, const char *host)
{
    return makeRequestWithBody("PUT", command, authorization, body, contentType);
}

int SpotifyArduino::makePostRequest(const char *command, const char *authorization, const char *body, const char *contentType, const char *host)
{
    return makeRequestWithBody("POST", command, authorization, body, contentType, host);
}

int SpotifyArduino::makeGetRequest(const char *command, const char *authorization, const char *accept, const char *host)
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

void SpotifyArduino::setRefreshToken(const char *refreshToken)
{
    int newRefreshTokenLen = strlen(refreshToken);
    if (_refreshToken == nullptr || strlen(_refreshToken) < newRefreshTokenLen)
    {
        delete _refreshToken;
        _refreshToken = new char[newRefreshTokenLen + 1]();
    }

    strncpy(_refreshToken, refreshToken, newRefreshTokenLen + 1);
}

bool SpotifyArduino::refreshAccessToken()
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
#ifdef SPOTIFY_DEBUG
            log_i("No JSON error, dealing with response");
#endif
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
#ifdef SPOTIFY_SERIAL_OUTPUT
                log_i("Problem with access_token (too long or null): %s", accessToken);
#endif
            }
        }
        else
        {
#ifdef SPOTIFY_SERIAL_OUTPUT
            log_i("deserializeJson() failed with code %s", error.c_str());
#endif
        }
    }
    else
    {
        parseError();
    }

    closeClient();
    return refreshed;
}

bool SpotifyArduino::checkAndRefreshAccessToken()
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

const char *SpotifyArduino::requestAccessTokens(const char *code, const char *redirectUrl, bool usingPKCE)
{
    char body[768];

    if (usingPKCE) log_d("Using PKCE for Spotify authorization.");

    if (usingPKCE) snprintf(body, sizeof(body), requestAccessTokensBodyPKCE, _clientId, redirectUrl, code, _verifier);
    else snprintf(body, sizeof(body), requestAccessTokensBody, code, redirectUrl, _clientId, _clientSecret);

#ifdef SPOTIFY_DEBUG
    log_i("%s", body);
#endif

    int statusCode = makePostRequest(SPOTIFY_TOKEN_ENDPOINT, NULL, body, "application/x-www-form-urlencoded", SPOTIFY_ACCOUNTS_HOST);
    
    unsigned long now = millis();

#ifdef SPOTIFY_DEBUG
    log_i("Status code: %d", statusCode);
#endif

    if (statusCode == 200)
    {
        DynamicJsonDocument doc(1000);
        // Parse JSON object
#ifndef SPOTIFY_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, _httpClient->getStream());
#else
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream);
#endif
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
#ifdef SPOTIFY_SERIAL_OUTPUT
            log_i("deserializeJson() failed with code %s", error.c_str());
#endif
        }
    }
    else
    {
        parseError();
    }

    closeClient();
    return _refreshToken;
}

bool SpotifyArduino::play(const char *deviceId)
{
    char command[100] = SPOTIFY_PLAY_ENDPOINT;
    return playerControl(command, deviceId);
}

bool SpotifyArduino::playAdvanced(char *body, const char *deviceId)
{
    char command[100] = SPOTIFY_PLAY_ENDPOINT;
    return playerControl(command, deviceId, body);
}

bool SpotifyArduino::pause(const char *deviceId)
{
    char command[100] = SPOTIFY_PAUSE_ENDPOINT;
    return playerControl(command, deviceId);
}

bool SpotifyArduino::setVolume(int volume, const char *deviceId)
{
    char command[125];
    sprintf(command, SPOTIFY_VOLUME_ENDPOINT, volume);
    return playerControl(command, deviceId);
}

bool SpotifyArduino::toggleShuffle(bool shuffle, const char *deviceId)
{
    char command[125];
    char shuffleState[10];
    if (shuffle)
    {
        strcpy(shuffleState, "true");
    }
    else
    {
        strcpy(shuffleState, "false");
    }
    sprintf(command, SPOTIFY_SHUFFLE_ENDPOINT, shuffleState);
    return playerControl(command, deviceId);
}

bool SpotifyArduino::setRepeatMode(SpotifyRepeatMode repeat, const char *deviceId)
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

bool SpotifyArduino::playerControl(char *command, const char *deviceId, const char *body)
{
    if (deviceId[0] != 0)
    {
        char *questionMarkPointer;
        questionMarkPointer = strchr(command, '?');
        char deviceIdBuff[50];
        if (questionMarkPointer == NULL)
        {
            sprintf(deviceIdBuff, "?device_id=%s", deviceId);
        }
        else
        {
            // params already started
            sprintf(deviceIdBuff, "&device_id=%s", deviceId);
        }
        strcat(command, deviceIdBuff);
    }

#ifdef SPOTIFY_DEBUG
    log_i("%s", command);
    log_i("%s", body);
#endif

    if (autoTokenRefresh)
    {
        checkAndRefreshAccessToken();
    }
    int statusCode = makePutRequest(command, _bearerToken, body);

    closeClient();
    //Will return 204 if all went well.
    return statusCode == 204;
}

bool SpotifyArduino::playerNavigate(char *command, const char *deviceId)
{
    if (deviceId[0] != 0)
    {
        char deviceIdBuff[50];
        sprintf(deviceIdBuff, "?device_id=%s", deviceId);
        strcat(command, deviceIdBuff);
    }

#ifdef SPOTIFY_DEBUG
    log_i("%s", command);
#endif

    if (autoTokenRefresh)
    {
        checkAndRefreshAccessToken();
    }
    int statusCode = makePostRequest(command, _bearerToken);

    closeClient();
    //Will return 204 if all went well.
    return statusCode == 204;
}

bool SpotifyArduino::nextTrack(const char *deviceId)
{
    char command[100] = SPOTIFY_NEXT_TRACK_ENDPOINT;
    return playerNavigate(command, deviceId);
}

bool SpotifyArduino::previousTrack(const char *deviceId)
{
    char command[100] = SPOTIFY_PREVIOUS_TRACK_ENDPOINT;
    return playerNavigate(command, deviceId);
}
bool SpotifyArduino::seek(int position, const char *deviceId)
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

#ifdef SPOTIFY_DEBUG
    log_i("%s", command);
    printStack();
#endif

    if (autoTokenRefresh)
    {
        checkAndRefreshAccessToken();
    }
    int statusCode = makePutRequest(command, _bearerToken);
    closeClient();
    //Will return 204 if all went well.
    return statusCode == 204;
}

bool SpotifyArduino::transferPlayback(const char *deviceId, bool play)
{
    char body[100];
    sprintf(body, "{\"device_ids\":[\"%s\"],\"play\":\"%s\"}", deviceId, (play ? "true" : "false"));

#ifdef SPOTIFY_DEBUG
    log_i("%s", SPOTIFY_PLAYER_ENDPOINT);
    log_i("%s", body);
    printStack();
#endif

    if (autoTokenRefresh)
    {
        checkAndRefreshAccessToken();
    }
    int statusCode = makePutRequest(SPOTIFY_PLAYER_ENDPOINT, _bearerToken, body);
    closeClient();
    //Will return 204 if all went well.
    return statusCode == 204;
}

int SpotifyArduino::getCurrentlyPlaying(SpotifyCallbackOnCurrentlyPlaying currentlyPlayingCallback, const char *market)
{
    char command[120] = SPOTIFY_CURRENTLY_PLAYING_ENDPOINT;
    if (market[0] != 0)
    {
        char marketBuff[15];
        sprintf(marketBuff, "&market=%s", market);
        strcat(command, marketBuff);
    }

#ifdef SPOTIFY_DEBUG
    log_i("%s", command);
    printStack();
#endif

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = currentlyPlayingBufferSize;

    if (autoTokenRefresh)
    {
        checkAndRefreshAccessToken();
    }
    int statusCode = makeGetRequest(command, _bearerToken);
#ifdef SPOTIFY_DEBUG
    log_i("%s", statusCode);
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
#ifdef SPOTIFY_DEBUG
                Serial.print(F("Num Images: "));
                log_i("%s", current.numImages);
                log_i("%s", numImages);
#endif

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
#ifdef SPOTIFY_DEBUG
                Serial.print(F("Num Images: "));
                log_i("%s", current.numImages);
                log_i("%s", numImages);
#endif

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
#ifdef SPOTIFY_SERIAL_OUTPUT
            Serial.print(F("deserializeJson() failed with code "));
            log_i("%s", error.c_str());
#endif
            statusCode = -1;
        }
    }

    closeClient();
    return statusCode;
}

int SpotifyArduino::getPlayerDetails(SpotifyCallbackOnPlayerDetails playerDetailsCallback, const char *market)
{
    char command[100] = SPOTIFY_PLAYER_ENDPOINT;
    if (market[0] != 0)
    {
        char marketBuff[30];
        sprintf(marketBuff, "?market=%s", market);
        strcat(command, marketBuff);
    }

#ifdef SPOTIFY_DEBUG
    log_i("%s", command);
    printStack();
#endif

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = playerDetailsBufferSize;
    if (autoTokenRefresh)
    {
        checkAndRefreshAccessToken();
    }

    int statusCode = makeGetRequest(command, _bearerToken);
#ifdef SPOTIFY_DEBUG
    log_i("Status Code: %s", statusCode);
#endif

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
#ifdef SPOTIFY_SERIAL_OUTPUT
            log_i("deserializeJson() failed with code %s", error.c_str());
#endif
            statusCode = -1;
        }
    }

    closeClient();
    return statusCode;
}

int SpotifyArduino::getDevices(SpotifyCallbackOnDevices devicesCallback)
{

#ifdef SPOTIFY_DEBUG
    log_i(SPOTIFY_DEVICES_ENDPOINT);
    printStack();
#endif

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = getDevicesBufferSize;
    if (autoTokenRefresh)
    {
        checkAndRefreshAccessToken();
    }

    int statusCode = makeGetRequest(SPOTIFY_DEVICES_ENDPOINT, _bearerToken);
#ifdef SPOTIFY_DEBUG
    log_i("Status Code: %s", statusCode);
#endif

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
#ifdef SPOTIFY_SERIAL_OUTPUT
            log_i("deserializeJson() failed with code %s", error.c_str());
#endif
            statusCode = -1;
        }
    }

    closeClient();
    return statusCode;
}

int SpotifyArduino::searchForSong(String query, int limit, SpotifyCallbackOnSearch searchCallback, SpotifySearchResult results[])
{

#ifdef SPOTIFY_DEBUG
    log_i(SPOTIFY_SEARCH_ENDPOINT);
    printStack();
#endif

    // Get from https://arduinojson.org/v6/assistant/
    const size_t bufferSize = searchDetailsBufferSize;
    if (autoTokenRefresh)
    {
        checkAndRefreshAccessToken();
    }

    int statusCode = makeGetRequest((SPOTIFY_SEARCH_ENDPOINT + query + "&limit=" + limit).c_str(), _bearerToken);
#ifdef SPOTIFY_DEBUG
    log_i("Status Code: %d", statusCode);
#endif

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
#ifdef SPOTIFY_SERIAL_OUTPUT
            log_i("deserializeJson() failed with code %s", error.c_str());
#endif
            statusCode = -1;
        }
    }

    closeClient();
    return statusCode;
}

int SpotifyArduino::commonGetImage(char *imageUrl)
{
#ifdef SPOTIFY_DEBUG
    log_i("Parsing image URL: %s", imageUrl);
#endif

    uint8_t lengthOfString = strlen(imageUrl);

    // We are going to just assume https, that's all I've
    // seen and I can't imagine a company will go back
    // to http

    if (strncmp(imageUrl, "https://", 8) != 0)
    {
#ifdef SPOTIFY_SERIAL_OUTPUT
        log_i("Url not in expected format: %s", imageUrl);
        log_i("(expected it to start with \"https://\")");
#endif
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

#ifdef SPOTIFY_DEBUG
    log_i("host: %s", host);
    log_i("len:host: %d", hostLength);
    log_i("path: %s", path);
    log_i("len:path: %d", strlen(path));
#endif

    int statusCode = makeGetRequest(path, NULL, "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8", host);
#ifdef SPOTIFY_DEBUG
    log_i("statusCode: %d", statusCode);
#endif
    if (statusCode == 200)
    {
        return getContentLength();
    }

    // Failed
    return -1;
}

bool SpotifyArduino::getImage(char *imageUrl, Stream *file)
{
    int totalLength = commonGetImage(imageUrl);

#ifdef SPOTIFY_DEBUG
    log_i("file length: %d", totalLength);
#endif
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
#ifdef SPOTIFY_DEBUG
        log_i("Finished getting image");
#endif
    }

    closeClient();

    return (totalLength > 0); //Probably could be improved!
}

bool SpotifyArduino::getImage(char *imageUrl, uint8_t **image, int *imageLength)
{
    int totalLength = commonGetImage(imageUrl);

#ifdef SPOTIFY_DEBUG
    log_i("file length: %d", totalLength);
#endif
    if (totalLength > 0)
    {
        uint8_t *imgPtr = (uint8_t *)malloc(totalLength);
        *image = imgPtr;
        *imageLength = totalLength;
        int remaining = totalLength;
        int amountRead = 0;

#ifdef SPOTIFY_DEBUG
        log_i("Fetching Image");
#endif

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
#ifdef SPOTIFY_DEBUG
        log_i("Finished getting image");
#endif
    }

    closeClient();

    return (totalLength > 0); //Probably could be improved!
}

int SpotifyArduino::getContentLength()
{
#ifdef SPOTIFY_DEBUG
        log_i("Content-Length: %d", _httpClient->getSize());
#endif

    return _httpClient->getSize();
}

void SpotifyArduino::parseError()
{
    //This method doesn't currently do anything other than print
#ifdef SPOTIFY_SERIAL_OUTPUT
    DynamicJsonDocument doc(1000);
    DeserializationError error = deserializeJson(doc, _httpClient->getStream());
    if (!error)
    {
        Serial.print(F("getAuthToken error"));
        serializeJson(doc, Serial);
    }
    else
    {
        Serial.print(F("Could not parse error"));
    }
#endif
}

void SpotifyArduino::lateInit(const char *clientId, const char *clientSecret, const char *refreshToken)
{
    this->_clientId = clientId;
    this->_clientSecret = clientSecret;
    setRefreshToken(refreshToken);
}

void SpotifyArduino::closeClient()
{
    if (_httpClient->connected())
    {
#ifdef SPOTIFY_DEBUG
        log_i("Closing client");
#endif
        _httpClient->end();
    }
}

#ifdef SPOTIFY_DEBUG
void SpotifyArduino::printStack()
{
    char stack;
    log_i("stack size %d", stack_start - &stack);
}
#endif
