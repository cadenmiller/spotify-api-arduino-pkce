#pragma once

#define SPOTIFY_DEBUG 1

// Prints the JSON received to serial (only use for debugging as it will be slow)
//#define SPOTIFY_PRINT_JSON_PARSE 1

// Fingerprint for "*.spotify.com", correct as of March 31, 2023
#define SPOTIFY_FINGERPRINT "9F 3F 7B C6 26 4C 97 06 A2 D4 D7 B2 35 45 D9 AA 8D BD CD 4D"

// Fingerprint for "*.scdn.co", correct as of March 31, 2023
#define SPOTIFY_IMAGE_SERVER_FINGERPRINT "8B 24 D0 B7 12 AC DB 03 75 09 45 95 24 FF BE D8 35 E6 EB DF"

#define SPOTIFY_HOST "api.spotify.com"
#define SPOTIFY_ACCOUNTS_HOST "accounts.spotify.com"

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

#define SPOTIFY_TIMEOUT 2000

#define SPOTIFY_ACCESS_TOKEN_LENGTH 309
#define SPOTIFY_PKCE_CODE_LENGTH 64 // Min of 32, Max of 96
#define SPOTIFY_PKCE_CODE_HASHED_LENGTH (SpotifyBase64::Length(32))
#define SPOTIFY_NAME_CHAR_LENGTH 100 //Increase if artists/song/album names are being cut off
#define SPOTIFY_URI_CHAR_LENGTH 40
#define SPOTIFY_URL_CHAR_LENGTH 70
#define SPOTIFY_DEVICE_ID_CHAR_LENGTH 45
#define SPOTIFY_DEVICE_NAME_CHAR_LENGTH 80
#define SPOTIFY_DEVICE_TYPE_CHAR_LENGTH 30
#define SPOTIFY_NUM_ALBUM_IMAGES 3 // Max spotify returns is 3, but the third one is probably too big for an ESP
#define SPOTIFY_MAX_NUM_ARTISTS 5
