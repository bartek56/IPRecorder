#ifndef SERIALCONFIG_HPP
#define SERIALCONFIG_HPP

#if SIMULATOR == 1
static constexpr char SERIAL_PORT[] = "/dev/pts/2";
#endif

#if SIMULATOR == 0
static constexpr char SERIAL_PORT[] = "/dev/ttyAMA0";
#endif

#endif // SERIALCONFIG_HPP
