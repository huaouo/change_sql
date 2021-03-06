//
// Created by huaouo on 2021/12/19.
//

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <unordered_map>
#include <functional>
#include <utility>

//int main() {
//    using namespace boost::interprocess;
//
//    //Remove shared memory on construction and destruction
//    struct shm_remove {
//        shm_remove() { shared_memory_object::remove("MySharedMemory"); }
//
//        ~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
//    } remover;
//
//    //Shared memory front-end that is able to construct objects
//    //associated with a c-string. Erase previous shared memory with the name
//    //to be used and create the memory segment at the specified address and initialize resources
//    managed_shared_memory segment
//            (create_only, "MySharedMemory" //segment name
//                    , 65536);          //segment size in bytes
//
//    //Note that map<Key, MappedType>'s value_type is std::pair<const Key, MappedType>,
//    //so the allocator must allocate that pair.
//    typedef std::pair<const int, float> ValueType;
//
//    //Alias an STL compatible allocator of for the map.
//    //This allocator will allow to place containers
//    //in managed shared memory segments
//    typedef allocator<ValueType, managed_shared_memory::segment_manager> ShmemAllocator;
//
//    //Alias a map of ints that uses the previous STL-like allocator.
//    //Note that the third parameter argument is the ordering function
//    //of the map, just like with std::map, used to compare the keys.
//    typedef std::unordered_map<int, float, std::hash<int>, std::equal_to<int>, ShmemAllocator> MyMap;
//
//    //Initialize the shared memory STL-compatible allocator
//    ShmemAllocator alloc_inst(segment.get_segment_manager());
//
//    //Construct a shared memory map.
//    //Note that the first parameter is the comparison function,
//    //and the second one the allocator.
//    //This the same signature as std::map's constructor taking an allocator
//    MyMap *mymap =
//            segment.construct<MyMap>("MyMap")   //object name
//                    (std::hash<int>(),
//                     std::equal_to<int>(),       //first  ctor parameter
//                     alloc_inst);      //second ctor parameter
//
//    //Insert data in the map
////    for (int i = 0; i < 100; ++i) {
////        mymap->insert(std::pair<const int, float>(i, (float) i));
////    }
//    return 0;
//}

#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <flat_hash_map.hpp>

#include <iostream>
#include <spdlog/spdlog.h>

#include <limits>

typedef std::numeric_limits< float > flt;

int main() {
    std::cout.precision(flt::max_digits10);
    std::cout << flt::max_digits10 << std::endl;
    std::cout << std::stof("10.135204300235847") << std::endl;
    std::cout << fmt::format("{:.18f}", std::stof("10.135204300235847")) << std::endl;
}
