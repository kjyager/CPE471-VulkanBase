#ifndef KJY_MAP_MERGE_H_
#define KJY_MAP_MERGE_H_
#include <map>
#include <unordered_map>
#include <set>

/* Merging std::maps isn't supported until C++ 17, so I'm making my own.*/


/// merge 'aMapA' with 'aMapB' into a new map. If a key is present in both maps A and B,
/// the value from map B supercedes the value from map A. 
template<typename Key, typename T>
std::map<Key, T> merge(const std::map<Key,T>& aMapA, const std::map<Key,T>& aMapB){
    std::map<Key,T> merged(aMapB);
    merged.insert(aMapA.begin(), aMapA.end());
    return(std::move(merged));
}

/// merge 'aMapA' with 'aMapB' into a new map. If a key is present in both maps A and B,
/// the value from map B supercedes the value from map A. 
template<typename Key, typename T>
std::unordered_map<Key, T> merge(const std::unordered_map<Key,T>& aMapA, const std::unordered_map<Key,T>& aMapB){
    std::unordered_map<Key,T> merged(aMapB);
    merged.insert(aMapA.begin(), aMapA.end());
    return(std::move(merged));
}


/// merge 'aSetA' with 'aSetB' into a new set. Union of aSetA and aSetB
template<typename Key, typename T>
std::set<Key, T> merge(const std::set<Key,T>& aSetA, const std::set<Key,T>& aSetB){
    std::set<Key,T> merged(aSetB);
    merged.insert(aSetA.begin(), aSetA.end());
    return(std::move(merged));
}


#endif