#include "GroupModule.h"
#include "GroupImp.h"
#include <log4cplus/loggingmacros.h>
#include <memory>
#include "../tinyxml2/tinyxml2.h"

// GroupModule

namespace chilli {
namespace Group {

	// Constructor of the GroupModule 
	GroupModule::GroupModule(const std::string & id) :ProcessModule(id)
	{
		log = log4cplus::Logger::getInstance("chilli.GroupModule");
		LOG4CPLUS_DEBUG(log, "Constuction a  module.");
	}

	// Destructor of GroupModule
	GroupModule::~GroupModule()
	{
		if (m_bRunning) {
			Stop();
		}

		for (auto & it : m_Extensions) {
			g_Extensions.erase(it.first);
		}

		LOG4CPLUS_DEBUG(log, "Destruction a module.");
	}

	int GroupModule::Start()
	{
		if (!m_bRunning) {
			m_bRunning = true;

			for (auto & it : m_Extensions) {
				it.second->Start();
			}
		}
		else {
			LOG4CPLUS_WARN(log, "already running for this module.");
		}
		return 0;
	}

	int GroupModule::Stop()
	{
		if (m_bRunning) {
			m_bRunning = false;
			chilli::model::EventType_t stopEvent(Json::nullValue);
			this->PushEvent(stopEvent);

			for (auto & it : m_Extensions) {
				it.second->Stop();
			}
		}
		return 0;
	}

	bool GroupModule::LoadConfig(const std::string & configContext)
	{
		using namespace tinyxml2;
		tinyxml2::XMLDocument config;
		if (config.Parse(configContext.c_str()) != XMLError::XML_SUCCESS)
		{
			LOG4CPLUS_ERROR(log, "load config error:" << config.ErrorName() << ":" << config.GetErrorStr1());
			return false;
		}

		tinyxml2::XMLElement * groups = config.FirstChildElement();
		//groups 
		for (XMLElement *child = groups->FirstChildElement("Group");
			child != nullptr;
			child = child->NextSiblingElement("Group"))
		{
			const char * num = child->Attribute("ExtensionNumber");
			const char * sm = child->Attribute("StateMachine");
			num = num ? num : "";
			sm = sm ? sm : "";
			if (this->g_Extensions.find(num) == this->g_Extensions.end())
			{
				model::ExtensionPtr ext(new GroupImp(this, num, sm));
				this->g_Extensions[num] = ext;
				this->m_Extensions[num] = ext;
				ext->setVar("_extension.Extension", num);
				Json::Value extensions;
				for (XMLElement *extptr = child->FirstChildElement("Extension"); extptr!=nullptr;
					extptr = extptr->NextSiblingElement("Extension"))
				{
					std::string extension = extptr->GetText() ? extptr->GetText() : "";
					extensions.append(extension);
				}
				ext->setVar("_extension.Extensions", extensions);
			}
			else {
				LOG4CPLUS_ERROR(log, "alredy had extension:" << num);
			}
		}

		return true;
	}

	const model::ExtensionMap & GroupModule::GetExtension()
	{
		// TODO: insert return statement here
		return m_Extensions;
	}

	void GroupModule::fireSend(const std::string & strContent, const void * param)
	{
		LOG4CPLUS_DEBUG(log, strContent);
		LOG4CPLUS_WARN(log, "fireSend not implement.");
	}
}
}
