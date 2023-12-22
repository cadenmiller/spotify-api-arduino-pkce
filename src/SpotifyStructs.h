#pragma once

#include <stdint.h>

#include <functional> /* std::function callbacks */

#include "SpotifyConfig.h"


enum class SpotifyResult : uint32_t
{
    eSuccess = 0,
    eInvalidGrant, /** @brief Your refresh token was revoked, probably not valid anymore. */
};


/** @brief Scopes provide Spotify users using third-party apps the confidence that only the information they choose to share will be shared, and nothing more. 
 *  @link https://developer.spotify.com/documentation/web-api/concepts/scopes 
 */
enum class SpotifyScopeFlagBits : uint32_t
{
    /* Images */
    eUgcImageUpload = (1 << 0), /** @brief Write access to user-provided images. */

    /* Spotify Connect */
    eUserReadPlaybackState = (1 << 1), /** @brief Read access to a user’s player state. */
    eUserModifyPlaybackState = (1 << 2), /** @brief Write access to a user’s playback state */
    eUserReadCurrentlyPlaying = (1 << 3), /** @brief 	Read access to a user’s currently playing content. */

    /* Playback*/
    eAppRemoteControl = (1 << 4), /** @brief Remote control playback of Spotify. This scope is currently available to Spotify iOS and Android SDKs. */
    eStreaming = (1 << 5), /** @brief Control playback of a Spotify track. This scope is currently available to the Web Playback SDK. The user must have a Spotify Premium account. */

    /* Playlists */
    ePlaylistReadPrivate = (1 << 6), /** @brief Read access to user's private playlists. */
    ePlaylistReadCollaborative = (1 << 7), /** @brief Include collaborative playlists when requesting a user's playlists. */
    ePlaylistModifyPrivate = (1 << 8), /** @brief Write access to a user's private playlists. */
    ePlaylistModifyPublic = (1 << 9), /** @brief Write access to a user's public playlists. */

    /* Follow */
    eUserFollowModify = (1 << 10), /** @brief Write/delete access to the list of artists and other users that the user follows. */
    eUserFollowRead = (1 << 11), /** @brief Read access to the list of artists and other users that the user follows. */

    /* Listening History */
    eUserReadPlaybackPosition = (1 << 12), /** @brief Read access to a user’s playback position in a content. */
    eUserTopRead = (1 << 13), /** @brief Read access to a user's top artists and tracks. */
    eUserReadRecentlyPlayed = (1 << 14), /** @brief Read access to a user’s recently played tracks. */

    /* Library */
    eUserLibraryModify = (1 << 15), /** @brief Write/delete access to a user's "Your Music" library. */
    eUserLibraryRead = (1 << 16), /** @brief Read access to a user's library. */

    /* Users */
    eUserReadEmail = (1 << 17), /** @brief Read access to user’s email address. */
    eUserReadPrivate = (1 << 18), /** @brief Read access to user’s subscription details (type of user account). */

    /* Open Access */
    eUserSoaLink = (1 << 19), /** @brief Link a partner user account to a Spotify user account. */
    eUserSoaUnlink = (1 << 20), /** @brief Unlink a partner user account from a Spotify account. */
    eUserManageEntitlements = (1 << 21), /** @brief Modify entitlements for linked users. */
    eUserManagePartner = (1 << 22), /** @brief Update partner information. */
    eUserCreatePartner = (1 << 23), /** @brief Create new partners, platform partners only. */

    eNone = 0x0000000, /** @brief None of the scopes. */
    eAll = 0xFFFFFFFF, /** @brief Every scope! Probably never want to use this, but it's there. */
};

using SpotifyScopeFlags = uint32_t;

/* I wish that C++ was a better language and I didn't have to write the operators by hand. */

inline constexpr SpotifyScopeFlags operator&(SpotifyScopeFlags x, SpotifyScopeFlagBits y) { return static_cast<SpotifyScopeFlags>(static_cast<int>(x) & static_cast<int>(y)); }
inline constexpr SpotifyScopeFlags operator|(SpotifyScopeFlags x, SpotifyScopeFlagBits y) { return static_cast<SpotifyScopeFlags>(static_cast<int>(x) | static_cast<int>(y)); }
inline constexpr bool operator==(SpotifyScopeFlags x, SpotifyScopeFlagBits y) { return static_cast<SpotifyScopeFlags>(static_cast<int>(x) == static_cast<int>(y)); }
inline constexpr bool operator!=(SpotifyScopeFlags x, SpotifyScopeFlagBits y) { return !(x == y); }


/** @brief Authorization code flows, depending on circumstance one is recommended over another.
 * 
 *  @link https://developer.spotify.com/documentation/web-api/concepts/authorization 
 * 
 *  The code flow is determines by which constructor you use for the SpotifyESP object.
 * 
 */
enum class SpotifyCodeFlow {
    eAuthorizationCode, /** @brief Useful for server sided applications and where the client secret can be stored safely. */
    eAuthorizationCodeWithPKCE,     /** @brief RECOMMENDED: Useful when storing the client secret isn't safe, (e.g. desktop, apps and websites) */
    /* The other flows are not implemented yet. */
};

/** @brief Different repeat modes.  
 *  @link https://developer.spotify.com/documentation/web-api/reference/get-the-users-currently-playing-track
 */
enum class SpotifyRepeatMode {
    eTrack, /** @brief Will repeat the current track. */
    eContext, /** @brief Will repeat the current context (playlists, album, etc).  */
    eOff /** @brief Will turn repeat off. */
};

/** @brief What type of audio item the user is playing. 
 *  @link https://developer.spotify.com/documentation/web-api/reference/get-the-users-currently-playing-track
 */
enum class SpotifyPlayingType {
    eTrack,
    eEpisode,
    eUnknown
};

/** @brief An for album art, profile images or covers of any kind.
 *  @link https://developer.spotify.com/documentation/web-api/reference/get-the-users-currently-playing-track
 *  @link https://developer.spotify.com/documentation/web-api/reference/get-an-album
 */
struct SpotifyImage {
    int height;
    int width;
    char url[SPOTIFY_URL_CHAR_LENGTH];
};

/** @brief Any controllable spotify playback device. 
 *  @link https://developer.spotify.com/documentation/web-api/reference/get-a-users-available-devices
 */
struct SpotifyDevice {
    char id[SPOTIFY_DEVICE_ID_CHAR_LENGTH];
    char name[SPOTIFY_DEVICE_NAME_CHAR_LENGTH];
    char type[SPOTIFY_DEVICE_TYPE_CHAR_LENGTH];
    bool isActive;
    bool isRestricted;
    bool isPrivateSession;
    int volumePercent;
};

/** @brief Playback information of player details.  */
struct SpotifyPlayerDetails {
    SpotifyDevice device;
    long progressMs;
    bool isPlaying;
    SpotifyRepeatMode repeatState;
    bool shuffleState;
};

/** @brief An artist on Spotify. 
 *  @url https://developer.spotify.com/documentation/web-api/reference/get-an-artist
 */
struct SpotifyArtist {
    char artistName[SPOTIFY_NAME_CHAR_LENGTH];
    char artistUri[SPOTIFY_URI_CHAR_LENGTH];
};

/** @brief Results from a search of the Spotify catalogue. 
 *  @link https://developer.spotify.com/documentation/web-api/reference/search
*/
struct SpotifySearchResult {
    char albumName[SPOTIFY_NAME_CHAR_LENGTH];
    char albumUri[SPOTIFY_URI_CHAR_LENGTH];
    char trackName[SPOTIFY_NAME_CHAR_LENGTH];
    char trackUri[SPOTIFY_URI_CHAR_LENGTH];
    SpotifyArtist artists[SPOTIFY_MAX_NUM_ARTISTS];
    SpotifyImage albumImages[SPOTIFY_NUM_ALBUM_IMAGES];
    int numArtists;
    int numImages;
};

/** @brief Retrieves results from the currently playing track. 
 *  @url https://developer.spotify.com/documentation/web-api/reference/get-the-users-currently-playing-track
 */
struct SpotifyCurrentlyPlaying {
    SpotifyArtist artists[SPOTIFY_MAX_NUM_ARTISTS];
    int numArtists;
    char albumName[SPOTIFY_NAME_CHAR_LENGTH];
    char albumUri[SPOTIFY_URI_CHAR_LENGTH];
    char trackName[SPOTIFY_NAME_CHAR_LENGTH];
    char trackUri[SPOTIFY_URI_CHAR_LENGTH];
    SpotifyImage albumImages[SPOTIFY_NUM_ALBUM_IMAGES];
    int numImages;
    bool isPlaying;
    long progressMs;
    long durationMs;
    char contextUri[SPOTIFY_URI_CHAR_LENGTH];
    SpotifyPlayingType currentlyPlayingType;
};

using SpotifyCallbackOnCurrentlyPlaying = std::function<void(SpotifyCurrentlyPlaying currentlyPlaying)>;
using SpotifyCallbackOnPlaybackState = std::function<void(SpotifyPlayerDetails playerDetails)>;
using SpotifyCallbackOnDevices = std::function<bool(SpotifyDevice device, int index, int numDevices)>;
using SpotifyCallbackOnSearch = std::function<bool(SpotifySearchResult result, int index, int numResults)>;
