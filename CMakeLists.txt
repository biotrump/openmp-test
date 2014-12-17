cmake_minimum_required(VERSION 2.8.5)
PROJECT( test1 C)
#openMP to avoid link error : omp_xxx error!
# undefined reference to `GOMP_loop_dynamic_start
find_package(OpenMP)
if(OPENMP_FOUND)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

PROJECT( test1 )

ADD_EXECUTABLE( test1
	test1.c
	)

#dynamic or static link
#TARGET_LINK_LIBRARIES( test1 ${OpenCV_LIBS} )
