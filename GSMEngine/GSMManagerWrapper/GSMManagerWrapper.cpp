#include <pybind11/pybind11.h>
#include "GSMManager.hpp"

namespace py = pybind11;

PYBIND11_MODULE(GSMEngine, m)
{
    py::class_<GSMManager>(m, "GSMManager")
            .def(py::init<const std::string &>())
            .def("initialize", &GSMManager::initilize)
            .def("sendSms", &GSMManager::sendSms)
            .def("sendSmsSync", &GSMManager::sendSmsSync)
            .def("isNewSms", &GSMManager::isNewSms)
            .def("getSms", &GSMManager::getSms);
}
