#include <boost/python.hpp>
#include "rectangle.cpp"

BOOST_PYTHON_MODULE(rectangle) {
    using namespace boost::python;
    class_<Rectangle>("Rectangle", init<double, double>())
        .def("area", &Rectangle::area)
        .def("perimeter", &Rectangle::perimeter);
}

