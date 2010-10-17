#include "StdAfx.h"
#include "CritterManager.h"
#include "Log.h"
#include "Names.h"
#include "ItemManager.h"

#ifdef FONLINE_SERVER
#include "MapManager.h"
#endif

CritterManager CrMngr;

#define CRPROTO_APP   "Critter proto"

//***************************************************************************************
//   CritterManager
//***************************************************************************************

bool CritterManager::Init()
{
	WriteLog("Critter manager initialization...\n");

	if(isActive)
	{
		WriteLog("already initialized.\n");
		return true;
	}

	if(!ItemMngr.IsInit())
	{
		WriteLog("Error, Items manager not init.\n");
		return false;
	}

	ZeroMemory(allProtos,sizeof(allProtos));
	Clear();

	isActive=true;
	WriteLog("Critter manager initialization complete.\n");
	return true;
}

void CritterManager::Finish()
{
	WriteLog("Critter manager finish...\n");

#ifdef FONLINE_SERVER
	CritterGarbager();
#endif

	ZeroMemory(allProtos,sizeof(allProtos));
	Clear();

	isActive=false;
	WriteLog("Critter manager finish complete.\n");
}

void CritterManager::Clear()
{
#ifdef FONLINE_SERVER
	for(CrMapIt it=allCritters.begin(),end=allCritters.end();it!=end;++it)
		SAFEREL((*it).second);
	allCritters.clear();
	crToDelete.clear();
	playersCount=0;
	npcCount=0;
	lastNpcId=NPC_START_ID;
#endif
}

bool CritterManager::LoadProtos()
{
	WriteLog("Loading critters prototypes...\n");

	if(!IsInit())
	{
		WriteLog("Critters manager is not init.\n");
		return false;
	}

	// Get names of proto
	if(!fileMngr.LoadFile("critters.lst",PT_SERVER_PRO_CRITTERS))
	{
		WriteLog("Cannot open \"critters.lst\".\n");
		return false;
	}

	char buf[256];
	StrVec fnames;
	while(fileMngr.GetLine(buf,sizeof(buf))) fnames.push_back(string(buf));
	fileMngr.UnloadFile();

	/*FILE* f=fopen("crproto.txt","wb");
	for(int i=0,k=fnames.size();i<k;i++)
	{
		if(!fileMngr.LoadFile(fnames[i].c_str(),PT_PRO_CRIT)) continue;
		LoadFromFileF2(&fileMngr,f);
	}
	fclose(f);
	return false;*/

	// Load protos
	IniParser protos_txt;
	int loaded_count=0;
	int errors=0;
	for(int i=0,k=fnames.size();i<k;i++)
	{
		const char* fname=fnames[i].c_str();
		if(protos_txt.LoadFile(fname,PT_SERVER_PRO_CRITTERS))
		{
			while(protos_txt.GotoNextApp(CRPROTO_APP))
			{
				int pid=-1;
				int params[MAX_PARAMS];
				ZeroMemory(params,sizeof(params));

				StrVec lines;
				protos_txt.GetAppLines(lines);

				for(int j=0;j<lines.size();j++)
				{
					char line[256];
					char indent[256];
					int value;
					StringCopy(line,lines[j].c_str());
					Str::Replacement(line,'=',' ');
					if(sscanf(line,"%s%d",indent,&value)==2)
					{
						if(!strcmp(indent,"Pid")) pid=value;
						else
						{
							int param_id=FONames::GetParamId(indent);
							if(param_id>=0 && param_id<MAX_PARAMS) params[param_id]=value;
							else
							{
								WriteLog("Unknown parameter<%s> in<%s>.\n",indent,fname);
								errors++;
							}
						}
					}
				}

				if(pid>0 && pid<MAX_CRIT_PROTOS)
				{
					CritData& proto=allProtos[pid];
					if(!proto.ProtoId) loaded_count++;
					//else WriteLog("Prototype is already parsed, pid<%u>. Rewrite.\n",pid);
					proto.ProtoId=pid;
					memcpy(proto.Params,params,sizeof(params));
				}
				else
				{
					WriteLog("Invalid pid<%d> in<%s>.\n",pid,fname);
					errors++;
				}
			}
		}
	}

	WriteLog("Loaded<%d> critter protos, errors<%d>.\n",loaded_count,errors);
	return errors==0;
}

CritData* CritterManager::GetProto(WORD proto_id)
{
	if(!proto_id || proto_id>=MAX_CRIT_PROTOS || !allProtos[proto_id].ProtoId) return NULL;
	return &allProtos[proto_id];
}

#ifdef FONLINE_SERVER
void CritterManager::SaveCrittersFile(void(*save_func)(void*,size_t))
{
	CrVec crits;
	for(CrMapIt it=allCritters.begin(),end=allCritters.end();it!=end;++it)
	{
		Critter* cr=(*it).second;
		if(cr->IsNpc()) crits.push_back(cr);
	}

	DWORD count=crits.size(); // npcCount
	save_func(&count,sizeof(count));
	for(CrVecIt it=crits.begin(),end=crits.end();it!=end;++it)
	{
		Critter* cr=*it;
		cr->Data.IsDataExt=(cr->DataExt?true:false);
		save_func(&cr->Data,sizeof(cr->Data));
		if(cr->Data.IsDataExt) save_func(cr->DataExt,sizeof(CritDataExt));
		DWORD te_count=cr->CrTimeEvents.size();
		save_func(&te_count,sizeof(te_count));
		if(te_count) save_func(&cr->CrTimeEvents[0],te_count*sizeof(Critter::CrTimeEvent));
	}
}

bool CritterManager::LoadCrittersFile(FILE* f)
{
	WriteLog("Load npc...\n");

	lastNpcId=0;

	DWORD count;
	fread(&count,sizeof(count),1,f);
	if(!count) return true;

	for(DWORD i=0;i<count;i++)
	{
		CritData data;
		fread(&data,sizeof(data),1,f);

		CritDataExt data_ext;
		if(data.IsDataExt) fread(&data_ext,sizeof(data_ext),1,f);

		Critter::CrTimeEventVec tevents;
		DWORD te_count;
		fread(&te_count,sizeof(te_count),1,f);
		if(te_count)
		{
			tevents.resize(te_count);
			fread(&tevents[0],te_count*sizeof(Critter::CrTimeEvent),1,f);
		}

		Npc* npc=CreateNpc(data.ProtoId,false);
		if(!npc)
		{
			WriteLog("Unable to create npc with id<%u>, pid<%u>.\n",data.Id,data.ProtoId);
			return false;
		}
		if(data.Id>lastNpcId) lastNpcId=data.Id;
		npc->Data=data;
		if(data.IsDataExt)
		{
			CritDataExt* pdata_ext=npc->GetDataExt();
			if(pdata_ext) *pdata_ext=data_ext;
		}
		if(te_count) npc->CrTimeEvents=tevents;

		npc->NextRefreshBagTick=Timer::GameTick()+(npc->Data.BagRefreshTime?npc->Data.BagRefreshTime:GameOpt.BagRefreshTime)*60*1000;
		npc->NameStr="Npc_";
		npc->NameStr+=Str::Format("%u",npc->GetId());
		AddCritter(npc);
	}

	WriteLog("Load npc complete, count<%u>.\n",count);
	return true;	
}

void CritterManager::RunInitScriptCritters()
{
	for(CrMapIt it=allCritters.begin(),end=allCritters.end();it!=end;++it)
	{
		Critter* cr=(*it).second;
		if(Script::PrepareContext(ServerFunctions.CritterInit,CALL_FUNC_STR,cr->GetInfo()))
		{
			Script::SetArgObject(cr);
			Script::SetArgBool(false);
			Script::RunPrepared();
		}
		if(cr->Data.ScriptId) cr->ParseScript(NULL,false);
	}
}

void CritterManager::CritterToGarbage(Critter* cr)
{
	SCOPE_LOCK(crLocker);
	crToDelete.push_back(cr->GetId());
}

void CritterManager::CritterGarbager()
{
	if(!crToDelete.empty())
	{
		crLocker.Lock();
		DwordVec to_del=crToDelete;
		crToDelete.clear();
		crLocker.Unlock();

		for(DwordVecIt it=to_del.begin(),end=to_del.end();it!=end;++it)
		{
			// Find and erase
			Critter* cr=NULL;

			crLocker.Lock();
			CrMapIt it_cr=allCritters.find(*it);
			if(it_cr!=allCritters.end()) cr=(*it_cr).second;
			if(!cr || !cr->IsNpc())
			{
				crLocker.Unlock();
				continue;
			}
			allCritters.erase(it_cr);
			npcCount--;
			crLocker.Unlock();

			SYNC_LOCK(cr);

			// Finish critter
			cr->LockMapTransfers++;

			Map* map=MapMngr.GetMap(cr->GetMap());
			if(map)
			{
				cr->ClearVisible();
				map->EraseCritter(cr);
				map->UnsetFlagCritter(cr->GetHexX(),cr->GetHexY(),cr->GetMultihex(),cr->IsDead());
			}

			cr->EventFinish(true);
			if(Script::PrepareContext(ServerFunctions.CritterFinish,CALL_FUNC_STR,cr->GetInfo()))
			{
				Script::SetArgObject(cr);
				Script::SetArgBool(true);
				Script::RunPrepared();
			}

			cr->LockMapTransfers--;

			// Erase from global
			if(cr->GroupMove)
			{
				GlobalMapGroup* group=cr->GroupMove;
				group->EraseCrit(cr);
				if(cr==group->Rule)
				{
					for(CrVecIt it_=group->CritMove.begin(),end_=group->CritMove.end();it_!=end_;++it_)
					{
						Critter* cr_=*it_;
						MapMngr.GM_GroupStartMove(cr_,true);
					}
				}
				else
				{
					for(CrVecIt it_=group->CritMove.begin(),end_=group->CritMove.end();it_!=end_;++it_)
					{
						Critter* cr_=*it_;
						cr_->Send_RemoveCritter(cr);
					}

					Item* car=cr->GetItemCar();
					if(car && car->GetId()==group->CarId)
					{
						group->CarId=0;
						MapMngr.GM_GroupSetMove(group,group->MoveX,group->MoveY,0); // Stop others
					}
				}
				cr->GroupMove=NULL;
			}

			cr->IsNotValid=true;
			cr->FullClear();
			Job::DeferredRelease(cr);
		}
	}
}

Npc* CritterManager::CreateNpc(WORD proto_id, DWORD params_count, int* params, DWORD items_count, int* items, const char* script, Map* map, WORD hx, WORD hy, BYTE dir, bool accuracy)
{
	if(!map || hx>=map->GetMaxHexX() || hy>=map->GetMaxHexY())
	{
		WriteLog(__FUNCTION__" - Wrong map values, hx<%u>, hy<%u>, map is nullptr<%s>.\n",hx,hy,!map?"true":"false");
		return NULL;
	}

	if(!map->IsHexPassed(hx,hy))
	{
		if(accuracy)
		{
			WriteLog(__FUNCTION__" - Accuracy position busy, map pid<%u>, hx<%u>, hy<%u>.\n",map->GetPid(),hx,hy);
			return NULL;
		}

		static THREAD int cur_step=0;
		short hx_=hx;
		short hy_=hy;
		bool odd=(hx&1)!=0;
		short* sx=(odd?SXOdd:SXEven);
		short* sy=(odd?SYOdd:SYEven);

		// Find in 2 hex radius
		for(int i=0;;i++)
		{
			if(i>=18)
			{
				WriteLog(__FUNCTION__" - All positions busy, map pid<%u>, hx<%u>, hy<%u>.\n",map->GetPid(),hx,hy);
				return NULL;
			}
			cur_step++;
			if(cur_step>=18) cur_step=0;
			if(hx_+sx[cur_step]<0 || hx_+sx[cur_step]>=map->GetMaxHexX()) continue;
			if(hy_+sy[cur_step]<0 || hy_+sy[cur_step]>=map->GetMaxHexY()) continue;
			if(!map->IsHexPassed(hx_+sx[cur_step],hy_+sy[cur_step])) continue;
			break;
		}

		hx_+=sx[cur_step];
		hy_+=sy[cur_step];
		hx=hx_;
		hy=hy_;
	}

	Npc* npc=CreateNpc(proto_id,true);
	if(!npc)
	{
		WriteLog(__FUNCTION__" - Create npc with pid <%u> failture.\n",proto_id);
		return NULL;
	}

	crLocker.Lock();
	npc->Data.Id=lastNpcId+1;
	lastNpcId++;
	crLocker.Unlock();

	// Flags and coords
	Location* loc=map->GetLocation(true);

	if(dir>5) dir=Random(0,5);
	npc->Data.Dir=dir;
	npc->Data.WorldX=(loc?loc->Data.WX:100);
	npc->Data.WorldY=(loc?loc->Data.WY:100);
	npc->SetHome(map->GetId(),hx,hy,dir);
	npc->SetMaps(map->GetId(),map->GetPid());
	npc->Data.HexX=hx;
	npc->Data.HexY=hy;
	npc->NameStr="Npc_";
	npc->NameStr+=Str::Format("%u",npc->GetId());

	for(DWORD i=0;i<params_count;i++)
	{
		WORD index=params[i*2];
		int value=params[i*2+1];
		if(index>=0 && index<MAX_PARAMS) npc->Data.Params[index]=value;
	}

	map->AddCritter(npc);
	AddCritter(npc);

	for(DWORD i=0;i<items_count;i++)
	{
		int pid=items[i*3];
		int count=items[i*3+1];
		int slot=items[i*3+2];
		if(pid>0 && count>0)
		{
			Item* item=ItemMngr.AddItemCritter(npc,pid,count);
			if(item && slot!=SLOT_INV) npc->MoveItem(SLOT_INV,slot,item->GetId(),item->GetCount());
		}
	}

	if(Script::PrepareContext(ServerFunctions.CritterInit,CALL_FUNC_STR,npc->GetInfo()))
	{
		Script::SetArgObject(npc);
		Script::SetArgBool(true);
		Script::RunPrepared();
	}
	if(script) npc->ParseScript(script,true);
	map->AddCritterEvents(npc);

	npc->RefreshBag();
	npc->ProcessVisibleCritters();
	npc->ProcessVisibleItems();
	return npc;
}

Npc* CritterManager::CreateNpc(WORD proto_id, bool copy_data)
{
	if(!IsInit())
	{
		WriteLog(__FUNCTION__" - Critter Manager is not initialized.\n");
		return false;
	}

	CritData* data=GetProto(proto_id);
	if(!data)
	{
		WriteLog(__FUNCTION__" - Critter data not found, proto id<%u>.\n",proto_id);
		return NULL;
	}

	Npc* npc=new Npc();
	if(!npc)
	{
		WriteLog(__FUNCTION__" - Allocation fail, proto id<%u>.\n",proto_id);
		return NULL;
	}

	if(!npc->SetDefaultItems(ItemMngr.GetProtoItem(ITEM_DEF_SLOT),
		ItemMngr.GetProtoItem(ITEM_DEF_SLOT),
		ItemMngr.GetProtoItem(ITEM_DEF_ARMOR)))
	{
		delete npc;
		WriteLog(__FUNCTION__" - Can't set default items of proto id<%u>.\n",proto_id);
		return NULL;
	}

	if(copy_data) npc->Data=*data;
	npc->Data.EnemyStackCount=MAX_ENEMY_STACK;
	npc->Data.ProtoId=proto_id;
	npc->Data.Cond=COND_LIFE;
	npc->Data.CondExt=COND_LIFE_NONE;
	npc->Data.Multihex=-1;

	SYNC_LOCK(npc);
	Job::PushBack(JOB_CRITTER,npc);
	return npc;
}

void CritterManager::AddCritter(Critter* cr)
{
	SCOPE_LOCK(crLocker);

	allCritters.insert(CrMapVal(cr->GetId(),cr));
	if(cr->IsPlayer()) playersCount++;
	else npcCount++;
}

void CritterManager::GetCopyNpcs(PcVec& npcs, bool sync_lock)
{
	crLocker.Lock();
	CrMap critters=allCritters;
	crLocker.Unlock();

	PcVec find_npcs;
	find_npcs.reserve(critters.size());
	for(CrMapIt it=critters.begin(),end=critters.end();it!=end;++it)
	{
		Critter* cr=(*it).second;
		if(cr->IsNpc()) find_npcs.push_back((Npc*)cr);
	}

	if(sync_lock && LogicMT)
	{
		// Synchronize
		for(PcVecIt it=find_npcs.begin(),end=find_npcs.end();it!=end;++it) SYNC_LOCK(*it);

		// Recheck
		crLocker.Lock();
		CrMap critters=allCritters;
		crLocker.Unlock();

		PcVec find_npcs2;
		find_npcs2.reserve(find_npcs.size());
		for(CrMapIt it=critters.begin(),end=critters.end();it!=end;++it)
		{
			Critter* cr=(*it).second;
			if(cr->IsNpc()) find_npcs2.push_back((Npc*)cr);
		}

		// Search again, if different
		if(!CompareContainers(find_npcs,find_npcs2))
		{
			GetCopyNpcs(npcs,sync_lock);
			return;
		}
	}

	npcs=find_npcs;
}

void CritterManager::GetCopyPlayers(ClVec& players, bool sync_lock)
{
	crLocker.Lock();
	CrMap critters=allCritters;
	crLocker.Unlock();

	ClVec find_players;
	find_players.reserve(critters.size());
	for(CrMapIt it=critters.begin(),end=critters.end();it!=end;++it)
	{
		Critter* cr=(*it).second;
		if(cr->IsPlayer()) find_players.push_back((Client*)cr);
	}

	if(sync_lock && LogicMT)
	{
		// Synchronize
		for(ClVecIt it=find_players.begin(),end=find_players.end();it!=end;++it) SYNC_LOCK(*it);

		// Recheck
		crLocker.Lock();
		CrMap critters=allCritters;
		crLocker.Unlock();

		ClVec find_players2;
		find_players2.reserve(find_players.size());
		for(CrMapIt it=critters.begin(),end=critters.end();it!=end;++it)
		{
			Critter* cr=(*it).second;
			if(cr->IsPlayer()) find_players2.push_back((Client*)cr);
		}

		// Search again, if different
		if(!CompareContainers(find_players,find_players2))
		{
			GetCopyPlayers(players,sync_lock);
			return;
		}
	}

	players=find_players;
}

Critter* CritterManager::GetCritter(DWORD crid, bool sync_lock)
{
	Critter* cr=NULL;

	crLocker.Lock();
	CrMapIt it=allCritters.find(crid);
	if(it!=allCritters.end()) cr=(*it).second;
	crLocker.Unlock();

	if(cr && sync_lock) SYNC_LOCK(cr);
	return cr;
}

Client* CritterManager::GetPlayer(DWORD crid, bool sync_lock)
{
	if(!IS_USER_ID(crid)) return NULL;

	Critter* cr=NULL;

	crLocker.Lock();
	CrMapIt it=allCritters.find(crid);
	if(it!=allCritters.end()) cr=(*it).second;
	crLocker.Unlock();

	if(!cr || !cr->IsPlayer()) return NULL;
	if(sync_lock) SYNC_LOCK(cr);
	return (Client*)cr;
}

Client* CritterManager::GetPlayer(const char* name, bool sync_lock)
{
	Client* cl=NULL;

	crLocker.Lock();
	for(CrMapIt it=allCritters.begin(),end=allCritters.end();it!=end;++it)
	{
		Critter* cr=(*it).second;
		if(cr->IsPlayer() && !_stricmp(name,cr->GetName()))
		{
			cl=(Client*)cr;
			break;
		}
	}
	crLocker.Unlock();

	if(cl && sync_lock) SYNC_LOCK(cl);
	return cl;
}

Npc* CritterManager::GetNpc(DWORD crid, bool sync_lock)
{
	if(!IS_NPC_ID(crid)) return NULL;

	Critter* cr=NULL;

	crLocker.Lock();
	CrMapIt it=allCritters.find(crid);
	if(it!=allCritters.end()) cr=(*it).second;
	crLocker.Unlock();

	if(!cr || !cr->IsNpc()) return NULL;
	if(sync_lock) SYNC_LOCK(cr);
	return (Npc*)cr;
}

void CritterManager::EraseCritter(Critter* cr)
{
	SCOPE_LOCK(crLocker);

	CrMapIt it=allCritters.find(cr->GetId());
	if(it!=allCritters.end())
	{
		if(cr->IsPlayer()) playersCount--;
		else npcCount--;
		allCritters.erase(it);
	}
}

void CritterManager::GetNpcIds(DwordSet& npc_ids)
{
	SCOPE_LOCK(crLocker);

	for(CrMapIt it=allCritters.begin(),end=allCritters.end();it!=end;++it)
	{
		Critter* cr=(*it).second;
		if(cr->IsNpc()) npc_ids.insert(cr->GetId());
	}
}

DWORD CritterManager::PlayersInGame()
{
	SCOPE_LOCK(crLocker);

	DWORD count=playersCount;
	return count;
}

DWORD CritterManager::NpcInGame()
{
	SCOPE_LOCK(crLocker);

	DWORD count=npcCount;
	return count;
}

DWORD CritterManager::CrittersInGame()
{
	SCOPE_LOCK(crLocker);

	DWORD count=allCritters.size();
	return count;
}

#endif
