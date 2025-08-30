#include "../../inc/core/VaEventRouter.hpp"
#include <cstdio>
#include <iostream>
#include <cstdlib>

// ANSI color codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

auto eventrouter = new va::VaEventRouter();

// it should be in line 110 + , but I need it be called by `EntityDemo`
void stop_loop();

// Event types definition
VA_EVENT_DEFINE(event_1)
    std::string label = "null";
    void func_printid(){std::cout<< "A: "<< this->id() << std::endl;};
    event_1( std::string name ){label = name;};
VA_EVENT_DEFINE_END

VA_EVENT_DEFINE(event_2)
    std::string label = "null";
    void func_printid(){std::cout<< "B: "<< this->id() << std::endl;};
    event_2( std::string name ){label = name;};
VA_EVENT_DEFINE_END

VA_EVENT_DEFINE(event_3)
    std::string label = "null";
    void func_printid(){std::cout<< "C: "<< this->id() << std::endl;};
    event_3( std::string name ){label = name;};
VA_EVENT_DEFINE_END

VA_EVENT_DEFINE(event_4)
    std::string label = "null";
    void func_printid(){std::cout<< "D: "<< this->id() << std::endl;};
    event_4( std::string name ){label = name;};
VA_EVENT_DEFINE_END

VA_EVENT_DEFINE(event_5)
    std::string label = "null";
    void func_printid(){std::cout<< "E: "<< this->id() << std::endl;};
    event_5( std::string name ){label = name;};
VA_EVENT_DEFINE_END

// Entities types  definition

class EntityDemo : public va::VaEntity
{
    public:
        std::string analyzeEventLabel ( std::shared_ptr< va::event::EventBase > _event )
        {
            std::string one;
            if( _event->id() == event_type_id< event_1 >()) one = va::event::getIf<event_1>(*_event.get())->label;
            else if( _event->id() == event_type_id< event_2 >()) one = va::event::getIf<event_2>(*_event.get())->label;
            else if( _event->id() == event_type_id< event_3 >()) one = va::event::getIf<event_3>(*_event.get())->label;
            else if( _event->id() == event_type_id< event_4 >()) one = va::event::getIf<event_4>(*_event.get())->label;
            else if( _event->id() == event_type_id< event_5 >()) one = va::event::getIf<event_5>(*_event.get())->label;
            else {
                std::cerr<< RED << "Error: bad event type in EntityDemo::analyzeEventLabel()" << RESET << std::endl;
                stop_loop();
                exit(EXIT_FAILURE);
            }
            return one;
        };
};

class EntityA : public EntityDemo 
{
    protected:
        void handleEvent(std::shared_ptr< va::event::EventBase > event)
        {
            std::cout << CYAN << "Entity type A" << RESET << "\n"
                <<"Instance pointer: "<<(unsigned long )this
                <<"\nReceived event with type ID: "
                <<  event->id() 
                <<"\nAnd ins label: "<<analyzeEventLabel(event)<<std::endl;
        };
};

class EntityB : public EntityDemo 
{
    protected:
        void handleEvent(std::shared_ptr< va::event::EventBase > event)
        {
            std::cout << CYAN << "Entity type B" << RESET << "\n"
                <<"Instance pointer: "<<(unsigned long )this
                <<"\nReceived event with type ID: "
                <<  event->id()
                <<"\nAnd ins label: "<<analyzeEventLabel(event)<<std::endl;
        };
};

class EntityC : public  EntityDemo 
{
    protected:
        void handleEvent(std::shared_ptr< va::event::EventBase > event)
        {
            std::cout << CYAN << "Entity type C" << RESET << "\n"
                <<"Instance pointer: "<<(unsigned long )this
                <<"\nReceived event with type ID: "
                <<  event->id() 
                <<"\nAnd ins label: "<<analyzeEventLabel(event)<<std::endl;
        };
};

std::unordered_map< std::string , std::shared_ptr<va::event::EventBase> > EventContainer;
std::unordered_map< std::string , va::entity_Sptr > EntityContainer;

void make_event ( std::string name , char type );
void make_entity( std::string name , char   type );
void bond( std::string nameevent , std::string nameentity);
void unbond( std::string nameevent , std::string nameentity);
void list_con( std::string name );
void start_loop();
void go_process();


int main(int argc , char** argv)
{

    while( true )
    {
        std::string cmd, arg1, arg2, arg3;
        
        // Prompt for command input
        std::cout << BLUE << ">> " << RESET;
        std::cin >> cmd;

        if ( cmd == "make" )
        {
            std::cout << YELLOW << "Enter type (event/entity): " << RESET;
            std::cin >> arg1;
            
            if (arg1 == "event") 
            {
                std::cout << YELLOW << "Enter event type (A-E): " << RESET;
                std::cin >> arg2;
                
                std::cout << YELLOW << "Enter event name: " << RESET;
                std::cin >> arg3;
                
                if (arg2.size() == 1) {
                    make_event(arg3, arg2[0]);
                } else {
                    std::cout << RED << "Invalid event type format!" << RESET << std::endl;
                }
            }
            else if (arg1 == "entity") 
            {
                std::cout << YELLOW << "Enter entity type (A-C): " << RESET;
                std::cin >> arg2;
                
                std::cout << YELLOW << "Enter entity name: " << RESET;
                std::cin >> arg3;
                
                if (arg2.size() == 1) {
                    make_entity(arg3, arg2[0]);
                } else {
                    std::cout << RED << "Invalid entity type format!" << RESET << std::endl;
                }
            }
            else {
                std::cout << RED << "Invalid make subcommand!" << RESET << std::endl;
            }
        }
        else if ( cmd == "bond" )
        {
            std::cout << YELLOW << "Enter event type (A-E): " << RESET;
            std::cin >> arg1;
            
            std::cout << YELLOW << "Enter entity name: " << RESET;
            std::cin >> arg2;
            
            bond(arg1, arg2);
        }
        else if ( cmd == "unbond" )
        {
            std::cout << YELLOW << "Enter event type (A-E): " << RESET;
            std::cin >> arg1;
            
            std::cout << YELLOW << "Enter entity name: " << RESET;
            std::cin >> arg2;
            
            unbond(arg1, arg2);
        }
        else if ( cmd == "list" )
        {
            std::cout << YELLOW << "Enter container to list (event/entity): " << RESET;
            std::cin >> arg1;
            
            list_con(arg1);
        }
        else if ( cmd == "shot" )
        {
            std::cout << YELLOW << "Enter event name to trigger: " << RESET;
            std::cin >> arg1;

            auto it = EventContainer.find(arg1);
            if (it != EventContainer.end()) {
                eventrouter->Push(it->second);
                EventContainer.erase( arg1 );
                std::cout << GREEN << "Event '" << arg1 << "' triggered!" << RESET << std::endl;
            } else {
                std::cout << RED << "Event '" << arg1 << "' not found!" << RESET << std::endl;
            }
        }
        else if ( cmd == "start")
        {
            start_loop();
        }
        else if ( cmd == "stop" )
        {
            stop_loop();
        }
        else if ( cmd == "exit")
        { 
            stop_loop();
            break;
        }
        else if ( cmd == "clear")
        { 
            system("clear");
        }
        else if ( cmd == "help" )
        {
            std::cout << MAGENTA <<
                "Available commands:\n"<<
                "make   - Create event or entity\n"<<
                "bond   - Bind event to entity\n"<<
                "unbond - Unbind event from entity\n"<<
                "list   - List contents of container\n"<<
                "shot   - Trigger specified event\n"<<
                "start  - Start event loop\n"<<
                "stop   - Stop event loop\n"<<
                "exit   - Exit program\n"<<
                "clear  - Clear screen\n"<<
                "help   - Show this help message\n"<<
                "go     - Tell to process all event\n"<< //TODO
                "ls     - Show available types\n"<< RESET << std::endl;
        }
        else if ( cmd == "ls")
        { 
            std::cout << MAGENTA <<
                "Event types:  A,B,C,D,E \n"<<
                "Entity types: A,B,C\n" << RESET;
        }
        else if ( cmd == "go")
        {
            std::cout<<YELLOW<<" Event process go!"<<RESET<<std::endl;
            go_process();
        }
        else 
        {
            std::cout << RED << "Unknown command! Enter help for available commands" << RESET << std::endl;
        }

    }

    delete eventrouter;
    return EXIT_SUCCESS;
}

void make_event ( std::string name , char type )
{
    if (EventContainer.find(name) != EventContainer.end()) {
        std::cout << RED << "Event '" << name << "' already exists!" << RESET << std::endl;
        return;
    }

    std::shared_ptr<va::event::EventBase> temp_event;
    switch (type) {
        case 'A': temp_event = std::make_shared<event_1>(name); break;
        case 'B': temp_event = std::make_shared<event_2>(name); break;
        case 'C': temp_event = std::make_shared<event_3>(name); break;
        case 'D': temp_event = std::make_shared<event_4>(name); break;
        case 'E': temp_event = std::make_shared<event_5>(name); break;
        default: std::cout << RED << "Invalid event type! Must be A-E" << RESET << std::endl; return;
    }    
    EventContainer.insert({name, temp_event});
    std::cout << GREEN << "Created event '" << name << "' (type " << type << ")" << RESET << std::endl;
}

void make_entity( std::string name , char type )
{
    if (EntityContainer.find(name) != EntityContainer.end()) {
        std::cout << RED << "Entity '" << name << "' already exists!" << RESET << std::endl;
        return;
    }

    va::entity_Sptr temp_entity;
    switch (type) {
        case 'A': temp_entity = va::make_entity_sptr<EntityA>(); break;
        case 'B': temp_entity = va::make_entity_sptr<EntityB>(); break;
        case 'C': temp_entity = va::make_entity_sptr<EntityC>(); break;
        default: std::cout << RED << "Invalid entity type! Must be A-C" << RESET << std::endl; return;
    }
    temp_entity->setLabel(name);
    EntityContainer.insert({name, temp_entity});
    std::cout << GREEN << "Created entity '" << name << "' (type " << type << ")" << RESET << std::endl;
}

void bond( std::string event_type , std::string nameentity)
{
    size_t event_id;
    switch (event_type[0]) {
        case 'A': event_id = event_type_id<event_1>(); break;
        case 'B': event_id = event_type_id<event_2>(); break;
        case 'C': event_id = event_type_id<event_3>(); break;
        case 'D': event_id = event_type_id<event_4>(); break;
        case 'E': event_id = event_type_id<event_5>(); break;
        default: std::cout << RED << "Bond failed: Invalid event type!" << RESET << std::endl; return;
    }

    auto ent_it = EntityContainer.find(nameentity);
    if (ent_it == EntityContainer.end()) {
        std::cout << RED << "Bond failed: Entity '" << nameentity << "' not found!" << RESET << std::endl;
        return;
    }

    eventrouter->Register(event_id, ent_it->second);
    std::cout << GREEN << "Bonded event type " << event_type << " to entity '" << nameentity << "'" << RESET << std::endl;
}

void unbond( std::string event_type , std::string nameentity)
{
    size_t event_id;
    switch (event_type[0]) {
        case 'A': event_id = event_type_id<event_1>(); break;
        case 'B': event_id = event_type_id<event_2>(); break;
        case 'C': event_id = event_type_id<event_3>(); break;
        case 'D': event_id = event_type_id<event_4>(); break;
        case 'E': event_id = event_type_id<event_5>(); break;
        default: std::cout << RED << "Unbond failed: Invalid event type!" << RESET << std::endl; return;
    }

    auto ent_it = EntityContainer.find(nameentity);
    if (ent_it == EntityContainer.end()) {
        std::cout << RED << "Unbond failed: Entity '" << nameentity << "' not found!" << RESET << std::endl;
        return;
    }

    eventrouter->UnRegister(ent_it->second, event_id);
    std::cout << GREEN << "Unbonded event type " << event_type << " from entity '" << nameentity << "'" << RESET << std::endl;
}

void list_con( std::string name )
{
    if (name == "event") {
        std::cout << WHITE << "Event Container (" << EventContainer.size() << " items):" << RESET << std::endl;
        for (const auto& pair : EventContainer) {
            std::cout << "  " << pair.first << std::endl;
        }
    }
    else if (name == "entity") {
        std::cout << WHITE << "Entity Container (" << EntityContainer.size() << " items):" << RESET << std::endl;
        for (const auto& pair : EntityContainer) {
            std::cout << "  " << pair.first << std::endl;
        }
    }
    else {
        std::cout << RED << "Unknown container! Use 'event' or 'entity'" << RESET << std::endl;
    }
}

void start_loop()
{
    if (eventrouter->IsRunning()) {
        std::cout << YELLOW << "Event loop is already running!" << RESET << std::endl;
    } else {
        eventrouter->eventloopStart();
        std::cout << GREEN << "Event loop started" << RESET << std::endl;
    }
}

void stop_loop()
{
    if (!eventrouter->IsRunning()) {
        std::cout << YELLOW << "Event loop is not running!" << RESET << std::endl;
    } else {
        eventrouter->eventloopStop();
        std::cout << GREEN << "Event loop stopped" << RESET << std::endl;
    }
}
    
void go_process()
{
    for(  auto itr_ent : EntityContainer )
    {
        while( true )
        {
            int fi = itr_ent.second->processOneEvent();
            if ( fi == -1 )break;
        }
    }
}
