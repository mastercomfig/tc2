//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Agones SDK C++ REST integration for Source Engine
//
//=============================================================================

#ifndef AGONES_SDK_H
#define AGONES_SDK_H
#ifdef _WIN32
#pragma once
#endif

#include "steam/steam_api.h"
#include <string>

// Forward declarations
struct HTTPRequestCompleted_t;

namespace Agones
{
	class CSDK
	{
	public:
		CSDK();
		~CSDK();

		// Initialization
		bool Init();
		void Shutdown();

		// State management
		bool Ready();
		bool Health();
		bool Reserve( int seconds );
		bool Allocate();

		// Metadata
		bool SetLabel( const char *key, const char *value );
		bool SetAnnotation( const char *key, const char *value );

		// Player Tracking
		bool PlayerConnect( const char *playerID );
		bool PlayerDisconnect( const char *playerID );
		bool SetPlayerCapacity( int capacity );
		bool GetPlayerCapacity();
		bool GetPlayerCount();
		bool GetConnectedPlayers();

		// GameServer Data
		bool GetGameServer();

	private:
		bool SendEmptyPostRequest( const char *endpoint );
		bool SendJsonPutRequest( const char *endpoint, const char *jsonBody );
		bool SendJsonPostRequest( const char *endpoint, const char *jsonBody );
		bool SendGetRequest( const char *endpoint );

		ISteamHTTP *GetSteamHTTP() const;
		const char *GetBaseURL() const;

		class CAgonesRequest
		{
		public:
			CAgonesRequest( CSDK *pSDK, HTTPRequestHandle hRequest );
			void OnResponseReceived( HTTPRequestCompleted_t *pInfo, bool bIOFailure );

			HTTPRequestHandle m_hRequest;
			CCallResult< CAgonesRequest, HTTPRequestCompleted_t > m_CallbackCompleted;

		private:
			void Cleanup();
			CSDK *m_pSDK;
		};

		friend class CAgonesRequest;
		void RemoveRequest( CAgonesRequest *pRequest );
		CUtlVector< CAgonesRequest * > m_vecRequests;

		bool m_bInitialized;
		std::string m_strBaseURL;
	};

	// Global accessor
	CSDK *SDK();
}

#endif // AGONES_SDK_H
