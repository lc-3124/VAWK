#ifndef _VA_ENTITY_HPP_
#define _VA_ENTITY_HPP_

#include "VaEvent.hpp"

#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>
#include <iostream>

namespace va{
class VaEntity
{
    public:
    inline void eventPush( std::shared_ptr< event::EventBase > event )
    {
        std::cout<<event.get()->id();
    };
};
}

#endif
