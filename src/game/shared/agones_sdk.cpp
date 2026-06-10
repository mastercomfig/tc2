//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Agones SDK C++ REST integration for Source Engine
//
//=============================================================================

#include "cbase.h"
#include "agones_sdk.h"
#include "fmtstr.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace Agones
{
	static CSDK s_AgonesSDK;
	CSDK *SDK() { return &s_AgonesSDK; }

	CSDK::CAgonesRequest::CAgonesRequest( CSDK *pSDK, HTTPRequestHandle hRequest )
	{
		m_pSDK = pSDK;
		m_hRequest = hRequest;
	}

	void CSDK::CAgonesRequest::OnResponseReceived( HTTPRequestCompleted_t *pInfo, bool bIOFailure )
	{
		if ( !pInfo )
		{
			DevWarning( "Agones request failed (bIOFailure=%d).\n", bIOFailure );
			m_pSDK->GetSteamHTTP()->ReleaseHTTPRequest( m_hRequest );
			Cleanup();
			return;
		}

		if ( !pInfo->m_bRequestSuccessful || ( pInfo->m_eStatusCode != k_EHTTPStatusCode200OK && pInfo->m_eStatusCode != k_EHTTPStatusCode204NoContent ) )
		{
			DevWarning( "Agones request failed with HTTP status %d\n", pInfo->m_eStatusCode );
		}
		else
		{
			// Optionally read the response if needed
		}

		m_pSDK->GetSteamHTTP()->ReleaseHTTPRequest( pInfo->m_hRequest );
		Cleanup();
	}

	void CSDK::CAgonesRequest::Cleanup()
	{
		m_pSDK->RemoveRequest( this );
		delete this;
	}

	CSDK::CSDK()
	{
		m_bInitialized = false;
		// Default port for Agones REST is 9358
		m_strBaseURL = "http://localhost:9358";
	}

	CSDK::~CSDK()
	{
		Shutdown();
	}

	bool CSDK::Init()
	{
		if ( CommandLine()->FindParm( "-agones" ) )
		{
			m_bInitialized = true;
			return true;
		}
		return false;
	}

	void CSDK::Shutdown()
	{
		if ( !m_bInitialized )
			return;

		// Post a shutdown request to Agones before cleaning up
		SendEmptyPostRequest( "/shutdown" );

		m_vecRequests.PurgeAndDeleteElements();
		m_bInitialized = false;
	}

	ISteamHTTP *CSDK::GetSteamHTTP() const
	{
#ifdef GAME_DLL
		if ( engine->IsDedicatedServer() )
		{
			return SteamGameServerHTTP();
		}
#endif
		return SteamHTTP();
	}

	const char *CSDK::GetBaseURL() const
	{
		return m_strBaseURL.c_str();
	}

	void CSDK::RemoveRequest( CAgonesRequest *pRequest )
	{
		m_vecRequests.FindAndRemove( pRequest );
	}

	bool CSDK::SendEmptyPostRequest( const char *endpoint )
	{
		if ( !m_bInitialized || !GetSteamHTTP() )
			return false;

		CFmtStr url( "%s%s", GetBaseURL(), endpoint );
		HTTPRequestHandle hRequest = GetSteamHTTP()->CreateHTTPRequest( k_EHTTPMethodPOST, url.Get() );
		if ( hRequest == INVALID_HTTPREQUEST_HANDLE )
			return false;

		GetSteamHTTP()->SetHTTPRequestRawPostBody( hRequest, "application/json", ( uint8* )"{}", 2 );

		SteamAPICall_t callResult;
		if ( !GetSteamHTTP()->SendHTTPRequest( hRequest, &callResult ) )
		{
			GetSteamHTTP()->ReleaseHTTPRequest( hRequest );
			return false;
		}

		CAgonesRequest *pRequest = new CAgonesRequest( this, hRequest );
		m_vecRequests.AddToTail( pRequest );
		pRequest->m_CallbackCompleted.Set( callResult, pRequest, &CAgonesRequest::OnResponseReceived );

		return true;
	}

	bool CSDK::SendJsonPutRequest( const char *endpoint, const char *jsonBody )
	{
		if ( !m_bInitialized || !GetSteamHTTP() )
			return false;

		CFmtStr url( "%s%s", GetBaseURL(), endpoint );
		HTTPRequestHandle hRequest = GetSteamHTTP()->CreateHTTPRequest( k_EHTTPMethodPUT, url.Get() );
		if ( hRequest == INVALID_HTTPREQUEST_HANDLE )
			return false;

		GetSteamHTTP()->SetHTTPRequestRawPostBody( hRequest, "application/json", ( uint8* )jsonBody, Q_strlen(jsonBody) );

		SteamAPICall_t callResult;
		if ( !GetSteamHTTP()->SendHTTPRequest( hRequest, &callResult ) )
		{
			GetSteamHTTP()->ReleaseHTTPRequest( hRequest );
			return false;
		}

		CAgonesRequest *pRequest = new CAgonesRequest( this, hRequest );
		m_vecRequests.AddToTail( pRequest );
		pRequest->m_CallbackCompleted.Set( callResult, pRequest, &CAgonesRequest::OnResponseReceived );

		return true;
	}
	
	bool CSDK::SendJsonPostRequest( const char *endpoint, const char *jsonBody )
	{
		if ( !m_bInitialized || !GetSteamHTTP() )
			return false;

		CFmtStr url( "%s%s", GetBaseURL(), endpoint );
		HTTPRequestHandle hRequest = GetSteamHTTP()->CreateHTTPRequest( k_EHTTPMethodPOST, url.Get() );
		if ( hRequest == INVALID_HTTPREQUEST_HANDLE )
			return false;

		GetSteamHTTP()->SetHTTPRequestRawPostBody( hRequest, "application/json", ( uint8* )jsonBody, Q_strlen(jsonBody) );

		SteamAPICall_t callResult;
		if ( !GetSteamHTTP()->SendHTTPRequest( hRequest, &callResult ) )
		{
			GetSteamHTTP()->ReleaseHTTPRequest( hRequest );
			return false;
		}

		CAgonesRequest *pRequest = new CAgonesRequest( this, hRequest );
		m_vecRequests.AddToTail( pRequest );
		pRequest->m_CallbackCompleted.Set( callResult, pRequest, &CAgonesRequest::OnResponseReceived );

		return true;
	}

	bool CSDK::SendGetRequest( const char *endpoint )
	{
		if ( !m_bInitialized || !GetSteamHTTP() )
			return false;

		CFmtStr url( "%s%s", GetBaseURL(), endpoint );
		HTTPRequestHandle hRequest = GetSteamHTTP()->CreateHTTPRequest( k_EHTTPMethodGET, url.Get() );
		if ( hRequest == INVALID_HTTPREQUEST_HANDLE )
			return false;

		SteamAPICall_t callResult;
		if ( !GetSteamHTTP()->SendHTTPRequest( hRequest, &callResult ) )
		{
			GetSteamHTTP()->ReleaseHTTPRequest( hRequest );
			return false;
		}

		CAgonesRequest *pRequest = new CAgonesRequest( this, hRequest );
		m_vecRequests.AddToTail( pRequest );
		pRequest->m_CallbackCompleted.Set( callResult, pRequest, &CAgonesRequest::OnResponseReceived );

		return true;
	}

	bool CSDK::Ready()
	{
		return SendEmptyPostRequest( "/ready" );
	}

	bool CSDK::Health()
	{
		return SendEmptyPostRequest( "/health" );
	}

	bool CSDK::Allocate()
	{
		return SendEmptyPostRequest( "/allocate" );
	}

	bool CSDK::Reserve( int seconds )
	{
		CFmtStr jsonBody( "{\"seconds\": %d}", seconds );
		return SendJsonPostRequest( "/reserve", jsonBody.Get() );
	}

	bool CSDK::SetLabel( const char *key, const char *value )
	{
		CFmtStr jsonBody( "{\"key\": \"%s\", \"value\": \"%s\"}", key, value );
		return SendJsonPutRequest( "/metadata/label", jsonBody.Get() );
	}

	bool CSDK::SetAnnotation( const char *key, const char *value )
	{
		CFmtStr jsonBody( "{\"key\": \"%s\", \"value\": \"%s\"}", key, value );
		return SendJsonPutRequest( "/metadata/annotation", jsonBody.Get() );
	}

	bool CSDK::PlayerConnect( const char *playerID )
	{
		CFmtStr jsonBody( "{\"playerID\": \"%s\"}", playerID );
		return SendJsonPutRequest( "/player/connect", jsonBody.Get() );
	}

	bool CSDK::PlayerDisconnect( const char *playerID )
	{
		CFmtStr jsonBody( "{\"playerID\": \"%s\"}", playerID );
		return SendJsonPutRequest( "/player/disconnect", jsonBody.Get() );
	}

	bool CSDK::SetPlayerCapacity( int capacity )
	{
		CFmtStr jsonBody( "{\"capacity\": %d}", capacity );
		return SendJsonPutRequest( "/player/capacity", jsonBody.Get() );
	}

	bool CSDK::GetPlayerCapacity()
	{
		return SendGetRequest( "/player/capacity" );
	}

	bool CSDK::GetPlayerCount()
	{
		return SendGetRequest( "/player/count" );
	}

	bool CSDK::GetConnectedPlayers()
	{
		return SendGetRequest( "/player/connected" );
	}

	bool CSDK::GetGameServer()
	{
		return SendGetRequest( "/gameserver" );
	}
}
