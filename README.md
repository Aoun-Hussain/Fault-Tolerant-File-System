# Building the gtfs library and tests
This project is built using cmake. Cmake 3.10 is already installed on advos. To install on your local machine
```shell
sudo apt-get install cmake
cmake --version #Make sure >= 3.10
```
To build the library and tests:
```shell
mkdir build
cd build
cmake ..
make
```
To run the provided tests:
```shell
cd build #the same directory we just made above
make run_tests #Run the tests on normal debug level
make run_tests_verbose #Run the tests with extra debug info
```
