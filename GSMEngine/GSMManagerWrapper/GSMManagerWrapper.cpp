#include <pybind11/pybind11.h>
#include "GSMManager.hpp"
#include "spdlog/spdlog.h"

namespace py = pybind11;

PYBIND11_MODULE(GSMEngine, m)
{
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#-%!()] %v");
    spdlog::set_level(spdlog::level::trace);
    py::class_<Sms>(m, "Sms")
            .def(py::init<>())
            .def_readwrite("number", &Sms::number)
            .def_readwrite("dateAndTime", &Sms::dateAndTime)
            .def_readwrite("msg", &Sms::msg);
    py::class_<Call>(m, "Call").def(py::init<>()).def_readwrite("number", &Call::number);

    py::class_<GSMManager>(m, "GSMManager")
            .def(py::init<const std::string &>())
            .def("initialize", &GSMManager::initilize)
            .def("sendSms", &GSMManager::sendSms)
            .def("sendSmsSync", &GSMManager::sendSmsSync)
            .def("isNewSms", &GSMManager::isNewSms)
            .def("getSms", &GSMManager::getSms)
            .def("isNewCall", &GSMManager::isNewCall)
            .def("getCall", &GSMManager::getCall);
}
