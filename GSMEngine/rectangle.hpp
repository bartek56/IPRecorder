#pragma once
class Rectangle {
private:
    double length;
    double width;
public:
    Rectangle(double l, double w) : length(l), width(w) {}
    double area() {
        return length * width;
    }
    double perimeter() {
        return 2 * (length + width);
    }
};


