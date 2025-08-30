#include "core/VaEntity.hpp"
#include "core/VaEventUpstream.hpp"

#include <iostream>

namespace va
{

// Subsribe and Unsubscribe
void VaEntity::subscribe( std::shared_ptr< VaEventUpstream > upstream , std::vector< size_t > event_ids )
{
    if ( !upstream )
        return;
    for ( auto id : event_ids )
    {
        upstream->Register( id, Sptr_from_this() );
    }
}

void VaEntity::unsubscribe( std::shared_ptr< VaEventUpstream > upstream , std::vector< size_t > event_ids )
{
    if ( !upstream )
        return;
    for ( auto id : event_ids )
    {
        upstream->UnRegister( Sptr_from_this(), id );
    }
}

void VaEntity::unsubscribe( std::shared_ptr< VaEventUpstream > upstream )
{
    if ( !upstream )
        return;
    upstream->UnRegister( Sptr_from_this() );
}

// Push an event into the entity's buffer and handle it
void VaEntity::eventPush( std::shared_ptr< event::EventBase > event )
{
    std::lock_guard< std::mutex > lock( mtx );
    this->EventBuffer.push( event );
}

int VaEntity::processOneEvent() {
    auto one_event = std::shared_ptr<event::EventBase>(nullptr);
    // 步骤1：先获取自身强引用，确保后续操作中entity不被销毁
    entity_Sptr self;
    try {
        self = Sptr_from_this(); // 若entity已销毁，会抛出异常
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] processOneEvent: entity already destroyed (" << e.what() << ")" << std::endl;
        return -2; // 标记entity已销毁
    }

    // 步骤2：加锁取出事件（与原逻辑一致）
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (this->EventBuffer.empty()) return -1;
        one_event = this->EventBuffer.front();
        this->EventBuffer.pop();
        if (!one_event) return 0;
    }

    // 步骤3：安全调用handleEvent（self持有强引用，this不会失效）
    this->handleEvent(one_event);
    return 1;
}
void VaEntity::Raii() {
    std::lock_guard<std::mutex> lock(upstream_entity_mtx);

    // 检查weak_this_是否初始化（避免Sptr_from_this()抛出空指针异常）
    if (dynamic_cast<enable_entity_shared_from_this*>(this)->weak_this_ == nullptr) {
        std::cerr << "[WARNING] Raii: weak_this_ not initialized (entity not managed by entity_Sptr)" << std::endl;
        // 直接清理资源，不调用UnRegister（无强引用可提供）
        Upstreams.clear();
        downEntitys.clear();
        return;
    }

    // 原逻辑：注销上游订阅
    for (auto& upstream : Upstreams) {
        if (upstream) {
            try {
                upstream->UnRegister(Sptr_from_this());
            } catch (const std::exception& e) {
                std::cerr << "[WARNING] Failed to unregister upstream: " << e.what() << std::endl;
            }
        }
    }
    Upstreams.clear();

    // 原逻辑：清理子实体
    for (auto& entity : downEntitys) {
        if (entity) {
            try {
                entity->Raii();
            } catch (const std::exception& e) {
                std::cerr << "[WARNING] Failed to clean downEntity: " << e.what() << std::endl;
            }
        }
    }
    downEntitys.clear();
}


};  // namespace va
