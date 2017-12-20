#include "EntityManager.h"
#include "ItemManager.h"
#include "CritterManager.h"
#include "MapManager.h"
#include "Script.h"

EntityManager EntityMngr;

EntityManager::EntityManager()
{
    currentId = 0;
    memzero( entitiesCount, sizeof( entitiesCount ) );
}

void EntityManager::RegisterEntity( Entity* entity )
{
    SCOPE_LOCK( entitiesLocker );

    if( !entity->GetId() )
        entity->SetId( ++currentId );

    auto it = allEntities.insert( std::make_pair( entity->GetId(), entity ) );
    RUNTIME_ASSERT( it.second );
    entitiesCount[ (int) entity->Type ]++;
}

void EntityManager::UnregisterEntity( Entity* entity )
{
    SCOPE_LOCK( entitiesLocker );

    auto it = allEntities.find( entity->GetId() );
    RUNTIME_ASSERT( it != allEntities.end() );
    allEntities.erase( it );
    entitiesCount[ (int) entity->Type ]--;

    Script::RemoveEventsEntity( entity );
}

Entity* EntityManager::GetEntity( uint id, EntityType type )
{
    SCOPE_LOCK( entitiesLocker );

    auto it = allEntities.find( id );
    if( it != allEntities.end() && it->second->Type == type )
        return it->second;
    return nullptr;
}

void EntityManager::GetEntities( EntityType type, EntityVec& entities )
{
    SCOPE_LOCK( entitiesLocker );

    entities.reserve( entities.size() + entitiesCount[ (int) type ] );
    for( auto it = allEntities.begin(); it != allEntities.end(); ++it )
    {
        if( it->second->Type == type )
            entities.push_back( it->second );
    }
}

uint EntityManager::GetEntitiesCount( EntityType type )
{
    SCOPE_LOCK( entitiesLocker );

    return entitiesCount[ (int) type ];
}

void EntityManager::GetItems( ItemVec& items )
{
    SCOPE_LOCK( entitiesLocker );

    items.reserve( items.size() + entitiesCount[ (int) EntityType::Item ] );
    for( auto it = allEntities.begin(); it != allEntities.end(); ++it )
    {
        if( it->second->Type == EntityType::Item )
            items.push_back( (Item*) it->second );
    }
}

void EntityManager::GetCritterItems( uint crid, ItemVec& items )
{
    SCOPE_LOCK( entitiesLocker );

    for( auto it = allEntities.begin(); it != allEntities.end(); ++it )
    {
        if( it->second->Type == EntityType::Item )
        {
            Item* item = (Item*) it->second;
            if( item->GetAccessory() == ITEM_ACCESSORY_CRITTER && item->GetCritId() == crid )
                items.push_back( item );
        }
    }
}

Critter* EntityManager::GetCritter( uint id )
{
    SCOPE_LOCK( entitiesLocker );

    auto it = allEntities.find( id );
    if( it != allEntities.end() && ( it->second->Type == EntityType::Npc || it->second->Type == EntityType::Client ) )
        return (Critter*) it->second;
    return nullptr;
}

void EntityManager::GetCritters( CrVec& critters )
{
    SCOPE_LOCK( entitiesLocker );

    critters.reserve( critters.size() + entitiesCount[ (int) EntityType::Npc ] + entitiesCount[ (int) EntityType::Client ] );
    for( auto it = allEntities.begin(); it != allEntities.end(); ++it )
    {
        if( it->second->Type == EntityType::Npc || it->second->Type == EntityType::Client )
            critters.push_back( (Critter*) it->second );
    }
}

Map* EntityManager::GetMapByPid( hash pid, uint skip_count )
{
    SCOPE_LOCK( entitiesLocker );

    for( auto it = allEntities.begin(); it != allEntities.end(); ++it )
    {
        if( it->second->Type == EntityType::Map )
        {
            Map* map = (Map*) it->second;
            if( map->GetProtoId() == pid )
            {
                if( !skip_count )
                    return map;
                else
                    skip_count--;
            }
        }
    }
    return nullptr;
}

void EntityManager::GetMaps( MapVec& maps )
{
    SCOPE_LOCK( entitiesLocker );

    maps.reserve( maps.size() + entitiesCount[ (int) EntityType::Map ] );
    for( auto it = allEntities.begin(); it != allEntities.end(); ++it )
    {
        if( it->second->Type == EntityType::Map )
            maps.push_back( (Map*) it->second );
    }
}

Location* EntityManager::GetLocationByPid( hash pid, uint skip_count )
{
    SCOPE_LOCK( entitiesLocker );

    for( auto it = allEntities.begin(); it != allEntities.end(); ++it )
    {
        if( it->second->Type == EntityType::Location )
        {
            Location* loc = (Location*) it->second;
            if( loc->GetProtoId() == pid )
            {
                if( !skip_count )
                    return loc;
                else
                    skip_count--;
            }
        }
    }
    return nullptr;
}

void EntityManager::GetLocations( LocVec& locs )
{
    SCOPE_LOCK( entitiesLocker );

    locs.reserve( locs.size() + entitiesCount[ (int) EntityType::Location ] );
    for( auto it = allEntities.begin(); it != allEntities.end(); ++it )
    {
        if( it->second->Type == EntityType::Location )
            locs.push_back( (Location*) it->second );
    }
}

void EntityManager::DumpEntity( IniParser& data, Entity* entity )
{
    const char* type_name;
    switch( entity->Type )
    {
    case EntityType::Location:
        type_name = "Location";
        break;
    case EntityType::Map:
        type_name = "Map";
        break;
    case EntityType::Npc:
        type_name = "Npc";
        break;
    case EntityType::Item:
        type_name = "Item";
        break;
    case EntityType::Custom:
        type_name = "Custom";
        break;
    case EntityType::Global:
        type_name = "Global";
        break;
    case EntityType::Client:
        type_name = "Client";
        break;
    default:
        RUNTIME_ASSERT( !"Unreachable place" );
        break;
    }

    StrMap& kv = data.SetApp( type_name );
    if( entity->Id )
        kv[ "$Id" ] = _str( "{}", entity->Id );
    if( entity->Proto )
        kv[ "$Proto" ] = _str().parseHash( entity->Proto->ProtoId );
    if( entity->Type == EntityType::Custom )
        kv[ "$ClassName" ] = ( (CustomEntity*) entity )->Props.GetClassName();
    entity->Props.SaveToText( kv, entity->Proto ? &entity->Proto->Props : nullptr );
}

void EntityManager::DumpEntities( IniParser& data )
{
    data.SetStr( "GeneralSettings", "LastEntityId", _str( "{}", currentId ) );

    EntityType query[] = { EntityType::Location, EntityType::Map, EntityType::Npc, EntityType::Item, EntityType::Custom };
    for( int q = 0; q < 5; q++ )
    {
        EntityType type = query[ q ];
        for( auto it = allEntities.begin(); it != allEntities.end(); ++it )
        {
            Entity* entity = it->second;
            if( type == entity->Type )
                DumpEntity( data, entity );
        }
    }
}

bool EntityManager::LoadEntities( IniParser& data )
{
    WriteLog( "Load entities...\n" );

    currentId = _str( data.GetStr( "GeneralSettings", "LastEntityId", "0" ) ).toUInt();

    uint       whole_count = 0;
    EntityType query[] = { EntityType::Location, EntityType::Map, EntityType::Npc, EntityType::Item, EntityType::Custom };
    for( int q = 0; q < 5; q++ )
    {
        EntityType type = query[ q ];
        PStrMapVec entities;
        if( type == EntityType::Location )
            data.GetApps( "Location", entities );
        else if( type == EntityType::Map )
            data.GetApps( "Map", entities );
        else if( type == EntityType::Npc )
            data.GetApps( "Npc", entities );
        else if( type == EntityType::Item )
            data.GetApps( "Item", entities );
        else if( type == EntityType::Custom )
            data.GetApps( "Custom", entities );

        uint count = 0;
        for( auto& pkv : entities )
        {
            auto& kv = *pkv;
            uint  id = _str( kv[ "$Id" ] ).toUInt();
            auto  proto_it = kv.find( "$Proto" );
            hash  proto_id = ( proto_it != kv.end() ? _str( proto_it->second ).toHash() : 0 );

            if( type == EntityType::Item )
            {
                if( !ItemMngr.RestoreItem( id, proto_id, kv ) )
                {
                    WriteLog( "Fail to restore item {}.\n", id );
                    continue;
                }
            }
            else if( type == EntityType::Npc )
            {
                if( !CrMngr.RestoreNpc( id, proto_id, kv ) )
                {
                    WriteLog( "Fail to restore npc {}.\n", id );
                    continue;
                }
            }
            else if( type == EntityType::Location )
            {
                if( !MapMngr.RestoreLocation( id, proto_id, kv ) )
                {
                    WriteLog( "Fail to restore location {}.\n", id );
                    continue;
                }
            }
            else if( type == EntityType::Map )
            {
                if( !MapMngr.RestoreMap( id, proto_id, kv ) )
                {
                    WriteLog( "Fail to restore map {}.\n", id );
                    continue;
                }
            }
            else if( type == EntityType::Custom )
            {
                const char* class_name = kv[ "$ClassName" ].c_str();
                if( !Script::RestoreEntity( class_name, id, kv ) )
                {
                    WriteLog( "Fail to restore entity {}.\n", id );
                    continue;
                }
            }
            else
            {
                RUNTIME_ASSERT( !"Unreachable place" );
            }

            count++;
        }

        if( count != (uint) entities.size() )
            return false;
        whole_count += count;
    }

    WriteLog( "Load entities complete, count {}.\n", whole_count );

    if( !LinkMaps() )
        return false;
    if( !LinkNpc() )
        return false;
    if( !LinkItems() )
        return false;
    InitAfterLoad();
    return true;
}

bool EntityManager::LinkMaps()
{
    WriteLog( "Link maps...\n" );

    // Link maps to locations
    int    errors = 0;
    MapVec maps;
    MapMngr.GetMaps( maps );
    for( auto& map : maps )
    {
        uint      loc_id = map->GetLocId();
        uint      loc_map_index = map->GetLocMapIndex();
        Location* loc = MapMngr.GetLocation( loc_id );
        if( !loc )
        {
            WriteLog( "Location {} for map '{}' ({}) not found.\n", loc_id, map->GetName(), map->GetId() );
            errors++;
            continue;
        }

        MapVec& maps = loc->GetMapsRaw();
        if( loc_map_index >= (uint) maps.size() )
            maps.resize( loc_map_index + 1 );
        maps[ loc_map_index ] = map;
        map->SetLocation( loc );
    }

    // Verify linkage result
    LocVec locs;
    MapMngr.GetLocations( locs );
    for( auto& loc : locs )
    {
        MapVec& maps = loc->GetMapsRaw();
        for( size_t i = 0; i < maps.size(); i++ )
        {
            if( !maps[ i ] )
            {
                WriteLog( "Location '{}' ({}) map index {} is empty.\n", loc->GetName(), loc->GetId(), i );
                errors++;
                continue;
            }
        }
    }

    WriteLog( "Link maps complete.\n" );
    return errors == 0;
}

bool EntityManager::LinkNpc()
{
    WriteLog( "Link npc...\n" );

    int   errors = 0;
    CrVec critters;
    CrMngr.GetCritters( critters );
    CrVec critters_groups;
    critters_groups.reserve( critters.size() );

    // Move all critters to local maps and global map leaders
    for( auto it = critters.begin(), end = critters.end(); it != end; ++it )
    {
        Critter* cr = *it;
        if( !cr->GetMapId() && cr->GetGlobalMapLeaderId() && cr->GetGlobalMapLeaderId() != cr->GetId() )
        {
            critters_groups.push_back( cr );
            continue;
        }

        Map* map = MapMngr.GetMap( cr->GetMapId() );
        if( cr->GetMapId() && !map )
        {
            WriteLog( "Map {} not found, critter '{}', hx {}, hy {}.\n", cr->GetMapId(), cr->GetName(), cr->GetHexX(), cr->GetHexY() );
            errors++;
            continue;
        }

        if( !MapMngr.CanAddCrToMap( cr, map, cr->GetHexX(), cr->GetHexY(), 0 ) )
        {
            WriteLog( "Error parsing npc '{}' ({}) to map {}, hx {}, hy {}.\n", cr->GetName(), cr->GetId(), cr->GetMapId(), cr->GetHexX(), cr->GetHexY() );
            errors++;
            continue;
        }
        MapMngr.AddCrToMap( cr, map, cr->GetHexX(), cr->GetHexY(), cr->GetDir(), 0 );

        if( !map )
            cr->SetGlobalMapTripId( cr->GetGlobalMapTripId() - 1 );
    }

    // Move critters to global groups
    for( auto it = critters_groups.begin(), end = critters_groups.end(); it != end; ++it )
    {
        Critter* cr = *it;
        if( !MapMngr.CanAddCrToMap( cr, nullptr, 0, 0, cr->GetGlobalMapLeaderId() ) )
        {
            WriteLog( "Error parsing npc to global group, critter '{}', rule id {}.\n", cr->GetName(), cr->GetGlobalMapLeaderId() );
            errors++;
            continue;
        }
        MapMngr.AddCrToMap( cr, nullptr, 0, 0, 0, cr->GetGlobalMapLeaderId() );
    }

    WriteLog( "Link npc complete.\n" );
    return errors == 0;
}

bool EntityManager::LinkItems()
{
    WriteLog( "Link items...\n" );

    int     errors = 0;
    CrVec   critters;
    CrMngr.GetCritters( critters );
    ItemVec game_items;
    ItemMngr.GetGameItems( game_items );
    for( auto& item : game_items )
    {
        switch( item->GetAccessory() )
        {
        case ITEM_ACCESSORY_CRITTER :
            {
                if( IS_CLIENT_ID( item->GetCritId() ) )
                    continue;                                                  // Skip player

                Critter* npc = CrMngr.GetNpc( item->GetCritId() );
                if( !npc )
                {
                    WriteLog( "Item '{}' ({}) npc not found, npc id {}.\n", item->GetName(), item->GetId(), item->GetCritId() );
                    errors++;
                    continue;
                }

                npc->SetItem( item );
            }
            break;
        case ITEM_ACCESSORY_HEX:
        {
            Map* map = MapMngr.GetMap( item->GetMapId() );
            if( !map )
            {
                WriteLog( "Item '{}' ({}) map not found, map id {}, hx {}, hy {}.\n", item->GetName(), item->GetId(), item->GetMapId(), item->GetHexX(), item->GetHexY() );
                errors++;
                continue;
            }

            if( item->GetHexX() >= map->GetWidth() || item->GetHexY() >= map->GetHeight() )
            {
                WriteLog( "Item '{}' ({}) invalid hex position, hx {}, hy {}.\n", item->GetName(), item->GetId(), item->GetHexX(), item->GetHexY() );
                errors++;
                continue;
            }

            if( item->IsScenery() )
            {
                WriteLog( "Item '{}' ({}) is scenery type {}.\n", item->GetName(), item->GetId(), item->GetType() );
                errors++;
                continue;
            }

            map->SetItem( item, item->GetHexX(), item->GetHexY() );
        }
        break;
        case ITEM_ACCESSORY_CONTAINER:
        {
            Item* cont = ItemMngr.GetItem( item->GetContainerId() );
            if( !cont )
            {
                WriteLog( "Item '{}' ({}) container not found, container id {}.\n", item->GetName(), item->GetId(), item->GetContainerId() );
                errors++;
                continue;
            }

            cont->ContSetItem( item );
        }
        break;
        default:
            WriteLog( "Unknown accessory id '{}' ({}), acc {}.\n", item->GetName(), item->Id, item->GetAccessory() );
            errors++;
            continue;
        }
    }

    WriteLog( "Link items complete.\n" );
    return errors == 0;
}

void EntityManager::InitAfterLoad()
{
    WriteLog( "Init entities after load...\n" );

    // Process visible
    CrVec critters;
    CrMngr.GetCritters( critters );
    for( auto& cr : critters )
    {
        cr->ProcessVisibleCritters();
        cr->ProcessVisibleItems();
    }

    // Other initialization
    EntityMap entities = allEntities;
    for( auto it = entities.begin(); it != entities.end(); ++it )
    {
        Entity* entity = it->second;
        if( entity->IsDestroyed )
            continue;

        if( entity->Type == EntityType::Location )
        {
            Location* loc = (Location*) entity;
            Script::RaiseInternalEvent( ServerFunctions.LocationInit, loc, false );
        }
        else if( entity->Type == EntityType::Map )
        {
            Map* map = (Map*) entity;
            Script::RaiseInternalEvent( ServerFunctions.MapInit, map, false );
            if( !map->IsDestroyed && map->GetScriptId() )
                map->SetScript( nullptr, false );
        }
        else if( entity->Type == EntityType::Npc )
        {
            Npc* npc = (Npc*) entity;
            npc->RefreshName();
            Script::RaiseInternalEvent( ServerFunctions.CritterInit, npc, false );
            if( !npc->IsDestroyed && npc->GetScriptId() )
                npc->SetScript( nullptr, false );
        }
        else if( entity->Type == EntityType::Item )
        {
            Item* item = (Item*) entity;
            if( item->GetIsRadio() )
                ItemMngr.RadioRegister( item, true );
            if( !item->IsDestroyed )
                Script::RaiseInternalEvent( ServerFunctions.ItemInit, item, false );
            if( !item->IsDestroyed && item->GetScriptId() )
                item->SetScript( nullptr, false );
        }
    }

    WriteLog( "Init entites after load complete.\n" );
}

void EntityManager::ClearEntities()
{
    for( auto it = allEntities.begin(); it != allEntities.end(); ++it )
    {
        it->second->IsDestroyed = true;
        Script::RemoveEventsEntity( it->second );
        entitiesCount[ (int) it->second->Type ]--;
        SAFEREL( it->second );
    }
    allEntities.clear();

    for( int i = 0; i < (int) EntityType::Max; i++ )
        RUNTIME_ASSERT( entitiesCount[ i ] == 0 );
}
