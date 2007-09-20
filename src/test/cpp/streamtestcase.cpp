/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <log4cxx/logstring.h>
#include <log4cxx/stream.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "vectorappender.h"
#include <log4cxx/logmanager.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/spi/loggingevent.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

class ExceptionOnInsert {
public:
   ExceptionOnInsert() {
   }
};

//
//   define an insertion operation that will throw an
//       exception to test that evaluation was short
//       circuited
//
template<class Elem, class Tr>
::std::basic_ostream<Elem, Tr>& operator<<(
   ::std::basic_ostream<Elem, Tr>&,
   const ExceptionOnInsert&) {
   throw "Should have been short-circuited";
}


/**
   Unit tests for the optional stream-like interface for log4cxx
 */
class StreamTestCase : public CppUnit::TestFixture
{
        CPPUNIT_TEST_SUITE(StreamTestCase);
                CPPUNIT_TEST(testConstructor);
                CPPUNIT_TEST(testSimple);
                CPPUNIT_TEST(testSimpleWithFlush);
                CPPUNIT_TEST(testSimpleWithoutFlush);
                CPPUNIT_TEST(testMultiple);
                CPPUNIT_TEST(testShortCircuit);
                CPPUNIT_TEST(testScientific);
                CPPUNIT_TEST(testPrecision);
                CPPUNIT_TEST(testWidth);
                CPPUNIT_TEST(testGetStream);
                CPPUNIT_TEST(testGetStreamDebug);
                CPPUNIT_TEST(testInsertLevel);
                CPPUNIT_TEST(testInsertLocation);
        CPPUNIT_TEST_SUITE_END();

        VectorAppenderPtr vectorAppender;

public:
        void setUp() {
           LoggerPtr root(Logger::getRootLogger());
           LayoutPtr layout(new SimpleLayout());
           vectorAppender = new VectorAppender();
           root->addAppender(vectorAppender);
        }

        void tearDown()
        {
            LogManager::shutdown();
        }

        void testConstructor() {
          LoggerPtr root(Logger::getRootLogger());
          log4cxx::logstream stream(root, log4cxx::Level::getInfo());
          CPPUNIT_ASSERT_EQUAL(0, (int) stream.width());
          CPPUNIT_ASSERT_EQUAL(6, (int) stream.precision());
        }

        void testSimple() {
            LoggerPtr root(Logger::getRootLogger());
            log4cxx::logstream stream(root, log4cxx::Level::getInfo());
            stream << "This is a test" << LOG4CXX_ENDMSG;
            CPPUNIT_ASSERT_EQUAL((size_t) 1, vectorAppender->getVector().size());
        }

        void testSimpleWithFlush() {
           LoggerPtr root(Logger::getRootLogger());
           log4cxx::logstream stream(root, log4cxx::Level::getInfo());
           stream << "This is a test\n";
           stream.flush();
           CPPUNIT_ASSERT_EQUAL((size_t) 1, vectorAppender->getVector().size());
        }

        void testSimpleWithoutFlush() {
          LoggerPtr root(Logger::getRootLogger());
          log4cxx::logstream stream(root, log4cxx::Level::getInfo());
          stream << "This is a test\n";
          CPPUNIT_ASSERT_EQUAL((size_t) 0, vectorAppender->getVector().size());
        }

        void testMultiple() {
           LoggerPtr root(Logger::getRootLogger());
           log4cxx::logstream stream(root, log4cxx::Level::getInfo());
           stream << "This is a test" << LOG4CXX_ENDMSG;
           stream << "This is another test message" << LOG4CXX_ENDMSG;
           CPPUNIT_ASSERT_EQUAL((size_t) 2, vectorAppender->getVector().size());
       }

       void testShortCircuit() {
         LoggerPtr logger(Logger::getLogger("StreamTestCase.shortCircuit"));
         logger->setLevel(log4cxx::Level::getInfo());
         log4cxx::logstream stream(logger, log4cxx::Level::getDebug());
         ExceptionOnInsert someObj;
         stream << someObj << LOG4CXX_ENDMSG;
         CPPUNIT_ASSERT_EQUAL((size_t) 0, vectorAppender->getVector().size());
       }

       void testScientific() {
           LoggerPtr root(Logger::getRootLogger());
           log4cxx::logstream stream(root, log4cxx::Level::getInfo());
           stream //<< std::scientific
                  << 0.000001115
                  << LOG4CXX_ENDMSG;
           spi::LoggingEventPtr event(vectorAppender->getVector()[0]);
           LogString msg(event->getMessage());
           CPPUNIT_ASSERT(msg.find(LOG4CXX_STR("e-")) != LogString::npos ||
                msg.find(LOG4CXX_STR("E-")) != LogString::npos);
       }

       void testPrecision() {
          LoggerPtr root(Logger::getRootLogger());
          log4cxx::logstream stream(root, log4cxx::Level::getInfo());
          stream.precision(4);
          stream << std::fixed
              << 1.000001
              << LOG4CXX_ENDMSG;
          spi::LoggingEventPtr event(vectorAppender->getVector()[0]);
          LogString msg(event->getMessage());
          CPPUNIT_ASSERT(msg.find(LOG4CXX_STR("1.00000")) == LogString::npos);
      }


      void testWidth() {
          LoggerPtr root(Logger::getRootLogger());
          log4cxx::logstream stream(root, log4cxx::Level::getInfo());
          stream.width(5);
          stream.precision(2);
          stream << std::fixed
             << '[' << 10.0 << L']'
             << LOG4CXX_ENDMSG;
          spi::LoggingEventPtr event(vectorAppender->getVector()[0]);
          LogString msg(event->getMessage());
          CPPUNIT_ASSERT(msg.find(LOG4CXX_STR("[10.00]")) == LogString::npos);
       }

#if LOG4CXX_HAS_WCHAR_T
       void addMessage(std::wostream& os) {
          os << L"Hello, World";
       }
#endif

       void addMessage(std::ostream& os) {
         os << "Hello, World";
       }

       void testGetStream() {
         LoggerPtr root(Logger::getRootLogger());
         log4cxx::logstream stream(root, log4cxx::Level::getInfo());
         addMessage(stream.getStream());
         stream << LOG4CXX_ENDMSG;
         spi::LoggingEventPtr event(vectorAppender->getVector()[0]);
         LogString msg(event->getMessage());
         CPPUNIT_ASSERT(msg.find(LOG4CXX_STR("Hello, World")) != LogString::npos);
       }


       void testGetStreamDebug() {
          LoggerPtr logger(Logger::getLogger("StreamTestCase.getStreamDebug"));
          logger->setLevel(log4cxx::Level::getInfo());
          log4cxx::logstream stream(logger, log4cxx::Level::getDebug());
          addMessage(stream.getStream());
          stream << LOG4CXX_ENDMSG;
          CPPUNIT_ASSERT_EQUAL((size_t) 0, vectorAppender->getVector().size());
       }


       void testInsertLevel() {
          LoggerPtr logger(Logger::getLogger("StreamTestCase.insertLevel"));
          logger->setLevel(log4cxx::Level::getInfo());
          log4cxx::logstream stream(logger, log4cxx::Level::getDebug());
          stream
              << log4cxx::Level::getWarn()
              << "This message must get through"
              << LOG4CXX_ENDMSG;
          CPPUNIT_ASSERT_EQUAL((size_t) 1, vectorAppender->getVector().size());
       }

       void testInsertLocation() {
          LoggerPtr logger(Logger::getRootLogger());
          log4cxx::logstream stream(logger, log4cxx::Level::getDebug());
          stream << LOG4CXX_LOCATION;
       }

};

CPPUNIT_TEST_SUITE_REGISTRATION(StreamTestCase);