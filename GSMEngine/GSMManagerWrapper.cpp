#include <boost/python.hpp>
#include "GSMManager.hpp"
#include <string>

BOOST_PYTHON_MODULE(libGSMEngine)
{
    using namespace boost::python;
    class_<GSMManager, boost::noncopyable>("GSMManager", init<std::string>()).def("initialize", &GSMManager::Initilize).def("sendSMS", &GSMManager::SendSms);
}
