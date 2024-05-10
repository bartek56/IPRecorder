#include <boost/python.hpp>
#include "GSMManager.hpp"
#include <string>

BOOST_PYTHON_MODULE(libGSMEngine)
{
    using namespace boost::python;

    class_<Sms>("Sms").def_readwrite("dateAndTime", &Sms::dateAndTime).def_readwrite("number", &Sms::number).def_readwrite("msg", &Sms::msg);

    class_<GSMManager, boost::noncopyable>("GSMManager", init<std::string>())
            .def("initialize", &GSMManager::Initilize)
            .def("sendSms", &GSMManager::SendSms)
            .def("isNewSms", &GSMManager::isNewSms)
            .def("getSms", &GSMManager::getSms);
}
