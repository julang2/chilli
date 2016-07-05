#include <stdint.h>
#include <esl.h>
#include "FreeSwitchModule.h"
#include <log4cplus/loggingmacros.h>
#include "../tinyxml2/tinyxml2.h"
#include <json/json.h>


namespace chilli{
namespace FreeSwitch{


FreeSwtichModule::FreeSwtichModule(void) :SMInstance(this), bRunning(false), m_Port(0)
{
	log = log4cplus::Logger::getInstance("chilli.FreeSwtichModule");
	LOG4CPLUS_DEBUG(log, "Constuction a FreeSwitch module.");
}


FreeSwtichModule::~FreeSwtichModule(void)
{
	if (bRunning){
		Stop();
	}

	LOG4CPLUS_DEBUG(log, "Destruction a FreeSwitch module.");
}

int FreeSwtichModule::Stop(void)
{
	LOG4CPLUS_DEBUG(log, "Stop  FreeSwitch module");
	bRunning = false;

	int result = m_Thread.size();
	for (auto it : this->m_Thread){
		if (it->joinable()){
			it->join();
		}
	}

	m_Thread.clear();
	return result;
}

int FreeSwtichModule::Start()
{
	LOG4CPLUS_DEBUG(log, "Start  FreeSwitch module");
	while (!bRunning)
	{
		bRunning = true;
		for (int i = 0; i < 1; i++)
		{
			std::shared_ptr<std::thread> th(new std::thread(&FreeSwtichModule::Listen, this));
			m_Thread.push_back(th);
		}
	}
	return m_Thread.size();
}

bool FreeSwtichModule::LoadConfig(const std::string & configContext)
{
	using namespace tinyxml2;
	tinyxml2::XMLDocument config;
	if (config.Parse(configContext.c_str()) != XMLError::XML_SUCCESS)
	{
		LOG4CPLUS_ERROR(log, "load config error:" << config.ErrorName() << ":" << config.GetErrorStr1());
		return false;
	}
	tinyxml2::XMLElement * eConfig = config.FirstChildElement();
	m_Host = eConfig->Attribute("host") ? eConfig->Attribute("host"):"";
	m_Port = eConfig->IntAttribute("port");
	m_User = eConfig->Attribute("user") ? eConfig->Attribute("user") : "";
	m_Password = eConfig->Attribute("password") ? eConfig->Attribute("password") : "";

	return true;
}

const std::map<std::string, model::ExtensionPtr> FreeSwtichModule::GetExtension()
{
	return m_Extension;
}


void FreeSwtichModule::OnTimerExpired(unsigned long timerId, const std::string & attr)
{
	LOG4CPLUS_DEBUG(log, __FUNCTION__ << "," << timerId << ":" << attr);
	Json::Value jsonEvent;
	jsonEvent["event"] = "timer";
	std::string sessionId = attr.substr(0, attr.find_first_of(":"));
	jsonEvent["sessionid"] = sessionId;

	this->PushEvent(jsonEvent.toStyledString());
}

void FreeSwtichModule::Listen()
{
	while (bRunning)
	{
		esl_handle_t handle = { { 0 } };

		esl_status_t status = esl_connect_timeout(&handle, m_Host.c_str(), m_Port, m_User.c_str(), m_Password.c_str(),30*1000);

		if (!handle.connected){
			LOG4CPLUS_ERROR(log, "connect freeswitch error");
		}

		esl_send_recv(&handle, "api status\n\n");

		while (esl_recv(&handle) != ESL_FAIL){
			if (handle.last_sr_event && handle.last_sr_event->body) {
				LOG4CPLUS_DEBUG(log, handle.last_sr_event->body);
			}
			else {
				// this is unlikely to happen with api or bgapi (which is hardcoded above) but prefix but may be true for other commands
				LOG4CPLUS_DEBUG(log, handle.last_sr_reply);
			}
		}

		esl_disconnect(&handle);
	}

}
}
}

