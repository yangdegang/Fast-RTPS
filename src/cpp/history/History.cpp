/*************************************************************************
 * Copyright (c) 2014 eProsima. All rights reserved.
 *
 * This copy of eProsima RTPS is licensed to you under the terms described in the
 * EPROSIMARTPS_LIBRARY_LICENSE file included in this distribution.
 *
 *************************************************************************/

/**
 * @file History.cpp
 *
 */


#include "eprosimartps/history/History.h"
#include "eprosimartps/Endpoint.h"
#include "eprosimartps/common/CacheChange.h"
#include "eprosimartps/utils/RTPSLog.h"

namespace eprosima {
namespace rtps {
//
//bool sort_CacheChanges_History_SeqNum (CacheChange_t* c1,CacheChange_t* c2)
//{
//	return(c1->sequenceNumber.to64long() < c2->sequenceNumber.to64long());
//}


History::History(Endpoint* endp,
					HistoryQosPolicy historypolicy,
					ResourceLimitsQosPolicy resourcelimits,
					uint32_t payload_max_size):
		mp_Endpoint(endp),
		m_historyQos(historypolicy),
		m_resourceLimitsQos(resourcelimits),
		m_isHistoryFull(false),
		m_changePool(resourcelimits.max_samples+1,payload_max_size)
{
	mp_invalidCache = m_changePool.reserve_Cache();
	mp_invalidCache->writerGUID = c_Guid_Unknown;
	mp_invalidCache->sequenceNumber = c_SequenceNumber_Unknown;

	pDebugInfo("History created"<<std::endl;);
}

History::~History()
{
	pDebugInfo("HistoryCache destructor"<<endl;);
}

//void History::sortCacheChangesBySeqNum()
//{
//	std::sort(m_changes.begin(),m_changes.end(),sort_CacheChanges_History_SeqNum);
//}


bool History::remove_all_changes()
{
	boost::lock_guard<History> guard(*this);
	if(!m_changes.empty())
	{
		for(std::vector<CacheChange_t*>::iterator it = m_changes.begin();it!=m_changes.end();++it)
		{
			m_changePool.release_Cache(*it);
		}
		m_keyedChanges.clear();
		m_changes.clear();
		m_isHistoryFull = false;
		updateMaxMinSeqNum();
		return true;
	}
	return false;
}

bool History::remove_change(CacheChange_t* ch)
{
	boost::lock_guard<History> guard(*this);
	if(mp_Endpoint->getTopic().topicKind == WITH_KEY)
	{
		for(std::vector<std::pair<InstanceHandle_t,std::vector<CacheChange_t*>>>::iterator kchit = m_keyedChanges.begin();
				kchit!=m_keyedChanges.end();++kchit)
		{
			if(kchit->first == ch->instanceHandle)
			{
				for(std::vector<CacheChange_t*>::iterator chit = kchit->second.begin();
						chit!=kchit->second.end();++chit)
				{
					if((*chit)->sequenceNumber == ch->sequenceNumber)
					{
						kchit->second.erase(chit);
						break;
					}
				}
				break;
			}
		}
	}
	for(std::vector<CacheChange_t*>::iterator chit = m_changes.begin();
			chit!=m_changes.end();++chit)
	{
		if((*chit)->sequenceNumber == ch->sequenceNumber)
		{
			m_changePool.release_Cache(ch);
			m_changes.erase(chit);
			return true;
		}
	}
	updateMaxMinSeqNum();
	return false;
}


} /* namespace rtps */
} /* namespace eprosima */
