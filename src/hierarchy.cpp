/***************************************************************************
                          hierarchy.cpp  -  description
                             -------------------
    begin                : jeu avr 17 2003
    copyright            : (C) 2003 by Michael CATANZARITI
    email                : mcatan@free.fr
 ***************************************************************************/

/***************************************************************************
 * Copyright (C) The Apache Software Foundation. All rights reserved.      *
 *                                                                         *
 * This software is published under the terms of the Apache Software       *
 * License version 1.1, a copy of which has been included with this        *
 * distribution in the LICENSE.txt file.                                   *
 ***************************************************************************/

#include <log4cxx/spi/loggerfactory.h>
#include <log4cxx/hierarchy.h>
#include <log4cxx/defaultcategoryfactory.h>
#include <log4cxx/logger.h>
#include <log4cxx/spi/hierarchyeventlistener.h>
#include <log4cxx/level.h>
#include <algorithm>
#include <log4cxx/helpers/loglog.h>
#include <log4cxx/appender.h>

using namespace log4cxx;
using namespace log4cxx::spi;
using namespace log4cxx::helpers;

IMPLEMENT_LOG4CXX_OBJECT(Hierarchy)

namespace {
    bool startsWith(const String& teststr, const String& substr)
	{
        bool val = false;
        if(teststr.length() > substr.length()) {
            val = teststr.substr(0, substr.length()) == substr;
        }

        return val;
    }
}

Hierarchy::Hierarchy(LoggerPtr root) : root(root),
emittedNoAppenderWarning(false), emittedNoResourceBundleWarning(false)
{
	// Enable all level levels by default.
	setThreshold(Level::ALL);
	this->root->setHierarchy(this);
	defaultFactory = new DefaultCategoryFactory();
}

Hierarchy::~Hierarchy()
{
}

void Hierarchy::addHierarchyEventListener(spi::HierarchyEventListenerPtr listener)
{
	if (std::find(listeners.begin(), listeners.end(), listener) != listeners.end())
	{
		LogLog::warn(_T("Ignoring attempt to add an existent listener."));
	} 
	else
	{
		listeners.push_back(listener);
	}
}

void Hierarchy::clear()
{
	mapCs.lock();

	loggers.clear();
	
	mapCs.unlock();
}

void Hierarchy::emitNoAppenderWarning(LoggerPtr logger)
{
	// No appenders in hierarchy, warn user only once.
	if(!this->emittedNoAppenderWarning)
	{
		LogLog::warn(_T("No appenders could be found for logger (") +
			logger->getName() + _T(")."));
		LogLog::warn(_T("Please initialize the log4cxx system properly."));
		this->emittedNoAppenderWarning = true;
	}
}


LoggerPtr Hierarchy::exists(const String& name)
{
	mapCs.lock();
	
	LoggerMap::iterator it = loggers.find(name);
	return (it != loggers.end()) ? it->second : 0;

	mapCs.unlock();
}
	
void Hierarchy::setThreshold(const Level& l)
{
	thresholdInt = l.level;
	threshold = &l;
}

void Hierarchy::setThreshold(const String& levelStr)

{
	const Level& l = Level::toLevel(levelStr, Level::OFF);

	if(&l != &Level::OFF)
	{
		setThreshold(l);
	} 
	else
	{
		LogLog::warn(_T("Could not convert [")+levelStr+_T("] to Level."));
	}
}

void Hierarchy::fireAddAppenderEvent(LoggerPtr logger, AppenderPtr appender)
{
    HierarchyEventListenerList::iterator it, itEnd = listeners.end();
    HierarchyEventListenerPtr listener;

    for(it = listeners.begin(); it != itEnd; it++)
	{
		listener = *it;
		listener->addAppenderEvent(logger, appender);
	}
}

void Hierarchy::fireRemoveAppenderEvent(LoggerPtr logger, AppenderPtr appender)

{
    HierarchyEventListenerList::iterator it, itEnd = listeners.end();
    HierarchyEventListenerPtr listener;

    for(it = listeners.begin(); it != itEnd; it++)
	{
		listener = *it;
		listener->removeAppenderEvent(logger, appender);
	}
}

const Level& Hierarchy::getThreshold()
{
	return *threshold;
}

LoggerPtr Hierarchy::getLogger(const String& name)
{
	return getLogger(name, defaultFactory);
}

LoggerPtr Hierarchy::getLogger(const String& name, spi::LoggerFactoryPtr factory)
{
	// Synchronize to prevent write conflicts. Read conflicts (in
	// getEffectiveLevel method) are possible only if variable
	// assignments are non-atomic.
	LoggerPtr logger;

	mapCs.lock();

	LoggerMap::iterator it = loggers.find(name);
	
	if (it != loggers.end())
	{
		logger = it->second;
	}
	else
	{
		logger = factory->makeNewLoggerInstance(name);

		logger->setHierarchy(this);
		loggers.insert(LoggerMap::value_type(name, logger));

		ProvisionNodeMap::iterator it2 = provisionNodes.find(name);
		if (it2 != provisionNodes.end())
		{
			updateChildren(it2->second, logger);
			provisionNodes.erase(it2);
		}

		updateParents(logger);
	}

	mapCs.unlock();

	return logger;
}

LoggerList Hierarchy::getCurrentLoggers()
{
	mapCs.lock();

	LoggerList v;
	LoggerMap::iterator it, itEnd = loggers.end();

	for (it = loggers.begin(); it != itEnd; it++)
	{
		v.push_back(it->second);
	}

	mapCs.unlock();

	return v;
}

LoggerPtr Hierarchy::getRootLogger()
{
	return root;
}

bool Hierarchy::isDisabled(int level)
{
	return thresholdInt > level;
}


void Hierarchy::resetConfiguration()
{
	mapCs.lock();
	
	getRootLogger()->setLevel(Level::DEBUG);
	//root->setResourceBundle(0);
	setThreshold(Level::ALL);
	
	shutdown(); // nested locks are OK
	
	LoggerList loggers = getCurrentLoggers();
	LoggerList::iterator it, itEnd = loggers.end();

	for (it = loggers.begin(); it != itEnd; it++)
	{
		LoggerPtr& logger = *it;
		logger->setLevel(Level::OFF);
		logger->setAdditivity(true);
		//logger->setResourceBundle(0);
	}

	//rendererMap.clear();

	mapCs.unlock();
}

void Hierarchy::shutdown()
{
	LoggerPtr root = getRootLogger();
	
	// begin by closing nested appenders
	root->closeNestedAppenders();
	
	LoggerList loggers = getCurrentLoggers();
	LoggerList::iterator it, itEnd = loggers.end();

	for (it = loggers.begin(); it != itEnd; it++)
	{
		LoggerPtr& logger = *it;
		logger->closeNestedAppenders();
	}

	// then, remove all appenders
	root->removeAllAppenders();
	for (it = loggers.begin(); it != itEnd; it++)
	{
		LoggerPtr& logger = *it;
		logger->removeAllAppenders();
	}
}


void Hierarchy::updateParents(LoggerPtr logger)
{
	const String& name = logger->name;
	int length = name.size();
	bool parentFound = false;

	//tcout << _T("UpdateParents called for ") << name << std::endl;

	// if name = "w.x.y.z", loop thourgh "w.x.y", "w.x" and "w", but not "w.x.y.z"
	for(int i = name.find_last_of(_T('.'), length-1); i != String::npos;
	i = name.find_last_of(_T('.'), i-1))
	{
		String substr = name.substr(0, i);
		//tcout << _T("UpdateParents processing ") << substr << std::endl;

        LoggerMap::iterator it = loggers.find(substr);
		if(it != loggers.end())
		{
			parentFound = true;
			logger->parent = it->second;
			break; // no need to update the ancestors of the closest ancestor
		}
		else
		{
			ProvisionNodeMap::iterator it2 = provisionNodes.find(name);
			if (it2 != provisionNodes.end())
			{
				it2->second.push_back(logger);
			}
			else
			{
				//tcout << _T("Inserting ProvisionNode for ") << substr << std::endl;
				ProvisionNode node(logger);
				provisionNodes.insert(
					ProvisionNodeMap::value_type(substr, node));
			}
		}
	}

	// If we could not find any existing parents, then link with root.
	if(!parentFound)
	{
		logger->parent = root;
	}
}

void Hierarchy::updateChildren(ProvisionNode& pn, LoggerPtr logger)
{
	//tcout << _T("updateChildren called for ") << logger->name << std::endl;

	ProvisionNode::iterator it, itEnd = pn.end();
	
	for(it = pn.begin(); it != itEnd; it++)
	{
		LoggerPtr& l = *it;
		//tcout << _T("Updating child ") << l->name << std::endl;
		
		// Unless this child already points to a correct (lower) parent,
		// make cat.parent point to l.parent and l.parent to cat.
		if(!startsWith(l->parent->name, logger->name))
		{
			logger->parent = l->parent;
			l->parent = logger;
		}
	}
}
