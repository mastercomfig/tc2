#include "cbase.h"
#include "gcsdk/gcsdk_auto.h"

#include "gcsdk_gcmessages.pb.h"
#include "comtress_gc_websocket.h"

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

	void CGCClient::RemoveLocalSOCache(CGCClientSharedObjectCache* pSOCache) {
		NotifySOCacheUnsubscribed(pSOCache->GetOwner());
	}
}