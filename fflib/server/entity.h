#ifndef _FF_ENTITY_H_
#define _FF_ENTITY_H_

#include <string>
#include <vector>
#include <list>
#include <map>

#include "base/fftype.h"
#include "base/smart_ptr.h"
#include "server/script.h"
#include "server/prop.h"

namespace ff
{

#define GAME_LOG "GAME_LOG"
#define CFG_DB  "cfgDB"
#define USER_DB "userDB"

class EntityField;

template <typename T>
struct FieldIndexUtil_t{
    static int FIELD_INDEX;
    static int getFieldIndex();
};


class Entity
{
private:
    Entity(int ntype, userid_t id, userid_t sid = 0):m_type(0), m_uid(id), m_sessionID(sid){}
public:
    //!factory method
    static SharedPtr<Entity> genEntity(int ntype, userid_t id, userid_t sid = 0);
    static SharedPtr<Entity> newEntity(int ntype, userid_t id, userid_t sid = 0);
    static SharedPtr<Entity> toEntity(long p);
    virtual ~Entity();
    
    template<typename T>
    T* get(){
        int index = FieldIndexUtil_t<T>::getFieldIndex();
        if (index == -1)
            return NULL;
        if (index >= (int)m_fields.size()){
            for (int i = 0; i <= index; ++i){
                m_fields.push_back(NULL);
            }
        }
        
        EntityField* ret = m_fields[index];
        if (ret == NULL){
            ret = new T();
            initField(ret, TYPE_NAME(T));
            m_fields[index] = ret;
        }
        return (T*)ret;
    }
    EntityField* getFieldByName(const std::string& name);
    void clearField();
    
    void setUid(userid_t id){ m_uid = id;  };
    userid_t getUid() const { return m_uid;}
    
    void setName(const std::string& s) {  m_name = s;  }
    const std::string& getName() const { return m_name;}
    
    int getType() const { return m_type;}
    void setType(int t) { m_type = t;   }
    
    void setSession(userid_t id){ m_sessionID = id;  };
    userid_t getSession() const { return m_sessionID;}
    bool sendMsg(uint16_t cmd, const std::string& msg);
    bool sessionClose();// 断开连接

    SharedPtr<Entity> toPtr();
    
    
private:
    void initField(EntityField* ret, const std::string& name);
public:
    static std::map<long, WeakPtr<Entity> >     EntityPtr2Ref;
    int                                         m_type;
    userid_t                                    m_uid;
    userid_t                                    m_sessionID;
    std::string                                 m_name;
    std::vector<EntityField*>                   m_fields;
};
typedef SharedPtr<Entity> EntityPtr;
typedef WeakPtr<Entity>   EntityRef;

#define ALLOC_ENTITY(ntype, id, sessionId) Entity::newEntity(ntype, id, sessionId)
#define NEW_ENTITY(ntype, id, sessionId) Entity::genEntity(ntype, id, sessionId)
#define TO_ENTITY(ptr) Entity::toEntity(long(ptr))
#define UID_TO_ENTITY(ntype, id) Singleton<EntityMgr>::instance().get(ntype, id)

template<typename T> struct EntityFieldTool{ static T* getField(EntityPtr e){ return e->get<T>();} };
#define PROP_MGR Singleton<PropCommonMgr<EntityPtr> >::instance()

class EntityField//!entity的字段基类
{
public:
    virtual ~EntityField(){}
    const std::string& getFieldName(){ return m_strFieldName; }
    void setFiledName(const std::string& s){ m_strFieldName = s; }
    
    EntityPtr getOwner(){ return m_owner.lock(); }
    void setOwner(EntityPtr ref) { m_owner = ref;}

public:
    std::string   m_strFieldName;
    EntityRef     m_owner;
};
class EntityFieldReg{
public:
    EntityFieldReg():m_index(0){
        
    }
    template<typename T>
    EntityFieldReg& reg(){
        int& index = FieldIndexUtil_t<T>::FIELD_INDEX;
        if (index == -1){
            index = m_index++;
        }
        
        return *this;
    }
public:
    int m_index;
};

template <typename T>
int FieldIndexUtil_t<T>::FIELD_INDEX = -1;
template <typename T>
int FieldIndexUtil_t<T>::getFieldIndex(){
    if (FIELD_INDEX == -1){
        Singleton<EntityFieldReg>::instance().reg<T>();
    }
    return FIELD_INDEX;
}
class EntityMgr{
public:
    EntityMgr(){}
    virtual ~EntityMgr(){}
    
    void add(EntityPtr p);
    bool del(int ntype, userid_t id);
    EntityPtr get(int ntype, userid_t id);
    size_t size(int ntype) { return m_allEntity[ntype].size(); }
    
    template <typename T>
    void foreach(int ntype, T f){
        std::map<userid_t, EntityPtr>& allEntity = m_allEntity[ntype];
        std::map<userid_t, EntityPtr>::iterator it = allEntity.begin();
        for (; it != allEntity.end(); ++it){
            f(it->second);
        }
    }

    EntityPtr getEntityBySession(userid_t sessionId){
        std::map<userid_t, EntityPtr>::iterator it = m_session2entity.find(sessionId);
        if (it != m_session2entity.end()){
            return it->second;
        }
        return NULL;
    }

protected:
    std::map<int/*entity type*/, std::map<userid_t, EntityPtr> >   m_allEntity;
    std::map<userid_t, EntityPtr>                                  m_session2entity;
};
#define ENTITY_MGR Singleton<EntityMgr>::instance()

#define DELETE_ENTITY(X) Singleton<EntityMgr>::instance().del(X->getType(), X->getUid());

template<>
struct CppScriptValutil<EntityPtr>{
    static void toScriptVal(ScriptArgObjPtr retVal, EntityPtr a){
        retVal->toInt((long)(a.get()));
    }
    static void toCppVal(ScriptArgObjPtr argVal, EntityPtr &a){
        long ptr = (long)(argVal->getInt());
        a = TO_ENTITY(ptr);
    }
};

struct EntityModule{
    static bool init();
};

}

#endif
