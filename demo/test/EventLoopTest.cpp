#include "../../inc/core/VaEventRouter.hpp"
#include <cstdio>
#include <iostream>

auto eventrouter = new va::VaEventRouter();

// Event types definition
VA_EVENT_DEFINE(event_1)
    std::string label = "null";
    void func_printid(){std::cout<< "A: "<< this->id() << std::endl;};
VA_EVENT_DEFINE_END

VA_EVENT_DEFINE(event_2)
    std::string label = "null";
    void func_printid(){std::cout<< "B: "<< this->id() << std::endl;};
VA_EVENT_DEFINE_END

VA_EVENT_DEFINE(event_3)
    std::string label = "null";
    void func_printid(){std::cout<< "C: "<< this->id() << std::endl;};
VA_EVENT_DEFINE_END

VA_EVENT_DEFINE(event_4)
    std::string label = "null";
    void func_printid(){std::cout<< "D: "<< this->id() << std::endl;};
VA_EVENT_DEFINE_END

VA_EVENT_DEFINE(event_5)
    std::string label = "null";
    void func_printid(){std::cout<< "E: "<< this->id() << std::endl;};
VA_EVENT_DEFINE_END

// Entities types  definition

class EntityDemo : public va::VaEntity
{
    public:
        std::string analyzeEventLabel ( std::shared_ptr< va::event::EventBase > _event )
        {
            std::string one;
            // switch doesn't allow case with a non-const-value
            if( _event->id() == event_type_id< event_1 >()) one = va::event::getIf<event_1>(*_event.get())->label;
            else if( _event->id() == event_type_id< event_2 >()) one = va::event::getIf<event_2>(*_event.get())->label;
            else if( _event->id() == event_type_id< event_3 >()) one = va::event::getIf<event_3>(*_event.get())->label;
            else if( _event->id() == event_type_id< event_4 >()) one = va::event::getIf<event_4>(*_event.get())->label;
            else if( _event->id() == event_type_id< event_5 >()) one = va::event::getIf<event_5>(*_event.get())->label;
            else {
                std::cerr<< " bad event type in EntityDemo::analyzeEventLabel() "<<std::endl;
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
            std::cout << "Entity type A\n "
                <<"ins ptr is: "<<(unsigned long )this
                <<"\ngot a event which type is "
                <<  event->id() << std::endl;
        };
};

class EntityB : public EntityDemo 

{
    protected:
        void handleEvent(std::shared_ptr< va::event::EventBase > event)
        {
            std::cout << "Entity type B\n"
                <<"ins ptr is: "<<(unsigned long )this
                <<"\ngot a event which type is "
                <<  event->id() << std::endl;
        };
};

class EntityC : public  EntityDemo 

{
    protected:
        void handleEvent(std::shared_ptr< va::event::EventBase > event)
        {
            std::cout << "Entity type C\n"
                <<"ins ptr is: "<<(unsigned long )this
                <<"\ngot a event which type is "
                <<  event->id() << std::endl;
        };
};

std::unordered_map< std::string , std::shared_ptr<va::event::EventBase> > EventContainer;
std::unordered_map< std::string , std::shared_ptr<va::VaEntity> > EntityContainer;

void make_event ( std::string name , char type );
void make_entity( std::string name , char   type );
void bond( std::string nameevent , std::string nameentity);
void unbond( std::string nameevent , std::string nameentity);
void list_con( std::string name );
void start_loop();
void stop_loop();


int main(int argc , char** argv)
{

    while( true )
    {
        /*
         * User Input Command
         * make -> event <type> <name>          : Add an event instant in event Container
         *      -> entity <type> <name>         : .......
         * bond <event type> <entity name>
         * unbond <event type> <entiey name>
         * list <container>  
         * shot <name>                          : Push a event into eventloop 
         * start                                : start the loop
         * stop                                 : stop the loop
         * exit                                 : exit this function test
         * help
         * clear
         * ls
         */

        std::string cmd , arg1 , arg2 , arg3;
        std::cout<<">> ";
        std::cin>> cmd;

        if ( cmd == "make" )
        {

        }
        else if ( cmd == "bond" )
        {

        }
        else if ( cmd == "unbond" )
        {

        }
        else if ( cmd == "list ")
        {

        }
        else if ( cmd == "shot" )
        {

        }
        else if ( cmd == "start")
        {

        }
        else if ( cmd == "stop" )
        {

        }
        else if ( cmd == "exit"){ break;}
        else if ( cmd == "clear"){ system("clear");}
        else if ( cmd == "help" )
        {
            std::cout <<
                "User Input Command"<< 
                "make -> event <type> <name>          \n"<<
                "      -> entity <type> <name>        \n"<< 
                "bond <event type> <entity name>\n"      << 
                "unbond <event type> <entiey name>\n"    << 
                "list <container>  \n"<< 
                "shot <name>                          \n"<< 
                "start                                \n"<< 
                "stop                                 \n"<< 
                "exit                                 \n"<< 
                "clear                                \n"<<
                "help\n"<<std::endl;
        }
        else if ( cmd == "ls")
        { 
            std::cout<<
                "event:  A,B,C,D,E \n"<<
                "entity: A,B,C\n";
        }
        else std::cout <<"Bad command ! "<<std::endl;

    }
    return EXIT_SUCCESS;
}

void make_event ( std::string name , size_t type )
{
    // First, check is a unique name? 


    std::shared_ptr<va::event::EventBase> temp_event;
    switch (type) {
        case 'A':temp_event = std::make_shared<event_1>();break;
        case 'B':temp_event = std::make_shared<event_2>();break;
        case 'C':temp_event = std::make_shared<event_3>();break;
        case 'D':temp_event = std::make_shared<event_4>();break;
        case 'E':temp_event = std::make_shared<event_5>();break;
        default:std::cout << "Bad type index!" << std::endl;break;
    }    
    // Insert
    EventContainer.insert( std::pair< std::string , std::shared_ptr<va::event::EventBase>>( name , temp_event )); 
}
