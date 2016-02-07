#pragma once
#include <mutex>
#include <string>
#include <unordered_map>

#include "onut/forward_declaration.h"
OForwardDeclare(ContentManager);
OForwardDeclare(Resource);

namespace onut
{
    /*!
        Thread safe resource management class.
    */
    class ContentManager : public std::enable_shared_from_this<ContentManager>
    {
    public:
        static OContentManagerRef create();

        ContentManager();
        virtual ~ContentManager();

        // Resource
        void addResource(const std::string& name, const OResourceRef& pResource);
        void removeResource(const OResourceRef& pResource);
        size_t size();
        void clear();
        OResourceRef getResource(const std::string& name);
        template<typename Tresource> std::shared_ptr<Tresource> getResourceAs(const std::string& name);

        // Search Paths
        void addDefaultSearchPaths();
        void addSearchPath(const std::string& path);
        void clearSearchPaths();
        std::string findResourceFile(const std::string& name);

    private:
        using ResourceMap = std::unordered_map<std::string, OResourceRef>;
        using SearchPaths = std::vector<std::string>;

        ResourceMap m_resources;
        SearchPaths m_searchPaths;
        std::mutex m_mutex;
    };

    template<typename Tresource>
    inline std::shared_ptr<Tresource> ContentManager::getResourceAs(const std::string& name)
    {
        auto pRet = std::dynamic_pointer_cast<Tresource>(getResource(name));
        if (!pRet)
        {
            auto filename = findResourceFile(name);
            pRet = Tresource::createFromFile(filename, shared_from_this());
            pRet->setName(name);
            pRet->setFilename(filename);
            addResource(name, pRet);
        }
        return pRet;
    }
}

extern OContentManagerRef oContentManager;