#include "cbase.h"
#include "gcsdk/gcsdk_auto.h"

#include "gcsdk_gcmessages.pb.h"
#include "comtress_gc_websocket.h"
#include "gcsystemmsgs.pb.h"

namespace GCSDK {
	CGCClient::CGCClient(ISteamGameCoordinator* _pSteamGameCoordinator, bool bGameserver):
		// this sucks
		m_mapSOCache(DefLessFunc(CSteamID))
	{}

	CGCClient::~CGCClient() {
		Uninit();
	}

	bool CGCClient::BInit(ISteamGameCoordinator* _pSteamGameCoordinator) {
		ComtressGCWebsocket()->Init();
		m_JobMgr.SetThreadPoolSize(GetCPUInformation()->m_nLogicalProcessors);
		return true;
	}

	void CGCClient::Uninit() {
		ComtressGCWebsocket()->Shutdown();
		FOR_EACH_MAP_FAST(m_mapSOCache, i) {
			auto cache = m_mapSOCache[i];
			if (cache->BIsSubscribed()) {
				cache->NotifyUnsubscribe();
			}

			delete cache; // FIXME: Is this correct?
		}

		m_mapSOCache.RemoveAll();
	}

	bool CGCClient::BMainLoop(uint64 ulLimitMicroseconds, uint64 ulFrameTimeMicroseconds) {
		ComtressGCWebsocket()->Update();

		CJobTime::UpdateJobTime(ulFrameTimeMicroseconds != 0 ? ulFrameTimeMicroseconds : 50000);

		CLimitTimer timer(ulLimitMicroseconds);
		bool ret = false;
		ret |= m_JobMgr.BFrameFuncRunSleepingJobs(timer);
		ret |= m_JobMgr.BFrameFuncRunYieldingJobs(timer);
		// it's not even used...
		return ret;
	}

	bool CGCClient::BSendMessage(uint32 unMsgType, const uint8* pubData, uint32 cubData) {
		return ComtressGCWebsocket()->BSendMessage(unMsgType, pubData, cubData);
	}

	bool CGCClient::BSendMessage(const CGCMsgBase& msg) {
		return ComtressGCWebsocket()->BSendMessage(msg);
	}

	bool CGCClient::BSendMessage(const CProtoBufMsgBase& msg) {
		return ComtressGCWebsocket()->BSendMessage(msg);
	}

	CSharedObject* CGCClient::FindSharedObject(const CSteamID& ownerID, const CSharedObject& soIndex) {
		if (auto cache = FindSOCache(ownerID, false)) {
			return cache->FindSharedObject(soIndex);
		}

		return nullptr;
	};

	CGCClientSharedObjectCache* CGCClient::FindSOCache(const CSteamID& steamID, bool bCreateIfMissing) {
		auto index = m_mapSOCache.Find(steamID);
		if (index != m_mapSOCache.InvalidIndex()) {
			return m_mapSOCache.Element(index);
		}
		
		// Valve does this too, sometimes bots hit this path and we have an invalid Steam ID. There might be other cases too.
		if (!steamID.IsValid()) {
			Warning("Invalid SteamID passed to FindSOCache: %s\n", steamID.Render());
			return nullptr;
		}

		if (bCreateIfMissing) {
			auto newCache = new CGCClientSharedObjectCache(steamID);
			m_mapSOCache.Insert(steamID, newCache);
			return newCache;
		}

		return nullptr;
	}

	void CGCClient::AddSOCacheListener(const CSteamID& ownerID, ISharedObjectListener* pListener) {
		FindSOCache(ownerID, true)->AddListener(pListener);
	}

	bool CGCClient::RemoveSOCacheListener(const CSteamID& ownerID, ISharedObjectListener* pListener) {
		if (auto cache = FindSOCache(ownerID, false)) {
			return cache->RemoveListener(pListener);
		}

		return false;
	}

	void CGCClient::NotifySOCacheUnsubscribed(const CSteamID& ownerID) {
		auto cache = FindSOCache(ownerID, false);
		if (cache && cache->BIsSubscribed()) {
			cache->NotifyUnsubscribe();
		}
	}

	void CGCClient::Dump() {
		FOR_EACH_MAP(m_mapSOCache, i) {
			m_mapSOCache[i]->Dump();
		}
	}

	CGCClientSharedObjectCache* CGCClient::AddLocalSOCache(const CSteamID& ownerID, void* pubData, uint32 cubData) {
		CMsgSOCacheSubscribed cacheMessage;
		if (!cacheMessage.ParseFromArray(pubData, cubData)) {
			return nullptr;
		}

		auto cache = FindSOCache(ownerID, true);
		if (!cache) {
			return nullptr;
		}

		// Valve does not check the return value, despite the function returning a bool.
		cache->BParseCacheSubscribedMsg(cacheMessage, true);
		// This does absolutely nothing.
		Test_CacheSubscribed(cache->GetOwner());

		return cache;
	}

	void CGCClient::RemoveLocalSOCache(CGCClientSharedObjectCache* pSOCache)
	{
		NotifySOCacheUnsubscribed(pSOCache->GetOwner());
	}

	class CGCSOCacheSubscribedJob : public CGCClientJob
	{
	public:
		CGCSOCacheSubscribedJob( CGCClient* pGCClient ) : CGCClientJob( pGCClient ) {}
		virtual bool BYieldingRunJobFromMsg( IMsgNetPacket *pNetPacket ) override
		{
			CProtoBufMsg<CMsgSOCacheSubscribed> msg( pNetPacket );
			CGCClientSharedObjectCache *pSOCache = m_pGCClient->FindSOCache( CSteamID( msg.Body().owner() ), true );
			if ( pSOCache )
			{
				pSOCache->BParseCacheSubscribedMsg( msg.Body() );
			}

			return true;
		}
	};
	GC_REG_JOB( CGCClient, CGCSOCacheSubscribedJob, "CGCSOCacheSubscribedJob", k_ESOMsg_CacheSubscribed, k_EServerTypeGCClient );

	class CGCSOCacheUnsubscribedJob : public CGCClientJob
	{
	public:
		CGCSOCacheUnsubscribedJob( CGCClient* pGCClient ) : CGCClientJob( pGCClient ) {}
		virtual bool BYieldingRunJobFromMsg( IMsgNetPacket *pNetPacket ) override {
			CProtoBufMsg<CMsgSOCacheUnsubscribed> msg( pNetPacket );
			m_pGCClient->NotifySOCacheUnsubscribed( msg.Body().owner() );
			return true;
		}
	};
	GC_REG_JOB( CGCClient, CGCSOCacheUnsubscribedJob, "CGCSOCacheUnsubscribedJob", k_ESOMsg_CacheUnsubscribed, k_EServerTypeGCClient );

	class CGCSOCreateJob : public CGCClientJob
	{
	public:
		CGCSOCreateJob( CGCClient* pGCClient ) : CGCClientJob( pGCClient ) {}
		virtual bool BYieldingRunJobFromMsg( IMsgNetPacket *pNetPacket ) override {
			CProtoBufMsg<CMsgSOSingleObject> msg( pNetPacket );
			if ( CGCClientSharedObjectCache *pCache = m_pGCClient->FindSOCache( CSteamID( msg.Body().owner() ) ) ) {
				pCache->BCreateFromMsg( msg.Body().type_id(), msg.Body().object_data().data(), msg.Body().object_data().size() );
				pCache->SetVersion( msg.Body().version() );
			}
			return true;
		}
	};
	GC_REG_JOB( CGCClient, CGCSOCreateJob, "CGCSOCreateJob", k_ESOMsg_Create, k_EServerTypeGCClient );

	class CGCSOUpdateJob : public CGCClientJob
	{
	public:
		CGCSOUpdateJob( CGCClient* pGCClient ) : CGCClientJob( pGCClient ) {}
		virtual bool BYieldingRunJobFromMsg( IMsgNetPacket *pNetPacket ) override {
			CProtoBufMsg<CMsgSOSingleObject> msg( pNetPacket );
			if ( CGCClientSharedObjectCache *pCache = m_pGCClient->FindSOCache( CSteamID( msg.Body().owner() ), false ) ) {
				pCache->BUpdateFromMsg( msg.Body().type_id(), msg.Body().object_data().data(), msg.Body().object_data().size() );
				pCache->SetVersion( msg.Body().version() );
			}
			return true;
		}
	};
	GC_REG_JOB( CGCClient, CGCSOUpdateJob, "CGCSOUpdateJob", k_ESOMsg_Update, k_EServerTypeGCClient );

	class CGCSODestroyJob : public CGCClientJob
	{
	public:
		CGCSODestroyJob( CGCClient* pGCClient ) : CGCClientJob( pGCClient ) {}
		virtual bool BYieldingRunJobFromMsg( IMsgNetPacket *pNetPacket ) override {
			CProtoBufMsg<CMsgSOSingleObject> msg( pNetPacket );
			if ( CGCClientSharedObjectCache *pCache = m_pGCClient->FindSOCache( CSteamID( msg.Body().owner() ), false ) ) {
				pCache->BDestroyFromMsg( msg.Body().type_id(), msg.Body().object_data().data(), msg.Body().object_data().size() );
				pCache->SetVersion( msg.Body().version() );
			}
			return true;
		}
	};
	GC_REG_JOB( CGCClient, CGCSODestroyJob, "CGCSODestroyJob", k_ESOMsg_Destroy, k_EServerTypeGCClient );

	class CGCSOUpdateMultipleJob : public CGCClientJob
	{
	public:
		CGCSOUpdateMultipleJob( CGCClient* pGCClient ) : CGCClientJob( pGCClient ) {}
		virtual bool BYieldingRunJobFromMsg( IMsgNetPacket *pNetPacket ) override {
			CProtoBufMsg<CMsgSOMultipleObjects> msg( pNetPacket );
			if ( CGCClientSharedObjectCache *pCache = m_pGCClient->FindSOCache( CSteamID( msg.Body().owner() ), false ) ) {
				pCache->m_context.PreSOUpdate( eSOCacheEvent_Incremental );
				for ( int i = 0; i < msg.Body().objects_size(); i++ ) {
					const CMsgSOMultipleObjects_SingleObject &obj = msg.Body().objects(i);
					pCache->BUpdateFromMsg( obj.type_id(), obj.object_data().data(), obj.object_data().size() );
				}
				pCache->m_context.PostSOUpdate( eSOCacheEvent_Incremental );
				pCache->SetVersion( msg.Body().version() );
			}
			return true;
		}
	};
	GC_REG_JOB( CGCClient, CGCSOUpdateMultipleJob, "CGCSOUpdateMultipleJob", k_ESOMsg_UpdateMultiple, k_EServerTypeGCClient );

	class CGCSOCacheSubscriptionCheck : public CGCClientJob
	{
	public:
		CGCSOCacheSubscriptionCheck( CGCClient* pGCClient ) : CGCClientJob( pGCClient ) {}
		virtual bool BYieldingRunJobFromMsg( IMsgNetPacket *pNetPacket ) override {
			return true; // TODO(mcoms): Ignored for now
		}
	};
	GC_REG_JOB( CGCClient, CGCSOCacheSubscriptionCheck, "CGCSOCacheSubscriptionCheck", k_ESOMsg_CacheSubscriptionCheck, k_EServerTypeGCClient );

}