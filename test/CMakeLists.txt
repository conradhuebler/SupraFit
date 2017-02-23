find_package(Qt5Test REQUIRED)
find_package(Qt5Widgets REQUIRED)


set(test_additional
    models_SRC
)

add_test( test_additional json_test )

add_executable(json_test  test/testjson.cpp)
set_property(TARGET json_test PROPERTY CXX_STANDARD 14)
