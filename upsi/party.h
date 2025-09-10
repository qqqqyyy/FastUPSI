#ifndef PARTY_H
#define PARTY_H
#include "utils.h"
#include "ASE/ASE.h"
#include "vole.h"
#include "network.h"
#include "rbokvs/rb_okvs.h"
#include "oprf.h"
#include "data_util.h"

namespace upsi{

class Party{

    public: 

        int total_days;
        int current_day = 0;
        int max_data_size;
        int party; // 0 / 1

        Dataset dataset;

        VoleSender vole_sender;
        VoleReceiver vole_receiver;

        oc::Socket* chl;
        oc::PRNG my_prng;

        oc::block ro_seed;

        std::vector<Element> intersection;

        struct OPRFValueHash {
            size_t operator()(const OPRFValue& p) const noexcept {
                size_t h1 = std::hash<oc::block>{}(p.first);
                size_t h2 = std::hash<oc::block>{}(p.second);
                return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1<<6) + (h1>>2)); //TODO?
            }
        };
        struct OPRFData{
            
            std::unordered_map<Element, OPRFValue> key_value;
            std::unordered_map<OPRFValue, Element, OPRFValueHash> value_key;
            void insert(const Element& key, const OPRFValue& value) {
                key_value[key] = value;
                value_key[value] = key;
            }
            void insert(const BlockVec& keys, const OPRFValueVec& values) {
                int cnt = keys.size();
                for (int i = 0; i < cnt; ++i) insert(keys[i], values[i]);
            }
            void remove(const Element& key) {
                auto it = key_value.find(key);
                if(it == key_value.end()) return;
                OPRFValue value = it->second;
                key_value.erase(it);
                
                auto it2 = value_key.find(value);
                if(it2 == value_key.end()) throw std::runtime_error("oprf dataset deletion error (value_key)");
                value_key.erase(it2);
            }
            void remove(const BlockVec& keys) {
                for (const auto& key: keys) remove(key);
            }
            std::pair<bool, oc::block> find(const OPRFValue& value) {
                auto it = value_key.find(value);
                if(it == value_key.end()) return std::make_pair(false, oc::ZeroBlock);
                return std::make_pair(true, it->second);
            }
        }oprf_data;

        Party(int _party, oc::Socket* _chl, int _total_days, std::string fn);

        void setup() {
            addition(dataset.initial_set);
            intersection = dataset.intersection;
        }

        void run() {
            oc::Timer timer("party");
            timer.setTimePoint("begin");
            for (int i = 0; i < total_days; ++i) {
                one_day();
                timer.setTimePoint("day " + std::to_string(i));
            }
            std::cout << "Time: \n" << timer << "\n";
        }

        void one_day() {
            int cnt_del = deletion_part(dataset.daily_deletion[current_day]);
            int cnt_add = addition_part(dataset.daily_addition[current_day]);
            std::cout << "[Day " << current_day << "]: " << "-" << cnt_del << ", +" << cnt_add << std::endl;
            ++current_day;
        }

        int addition_part(const std::vector<Element>& addition_set);

        int deletion_part(const std::vector<Element>& deletion_set);

        virtual std::vector<Element> query(const std::vector<Element>& elems) = 0; // query for elems
        
        virtual void addition(const std::vector<Element>& elems) = 0;

        std::vector<Element> PSI_receiver(const std::vector<Element>& my_set);

        void PSI_sender(const std::vector<Element>& my_set);


        void merge_set(std::vector<Element>& X, const std::vector<Element>& Y) {
            std::unordered_map<oc::block, bool> map;
            for (const auto& x: X) map[x] = true;
            for (const auto& y: Y) if(!map[y]) X.push_back(y);
        }

        std::vector<Element> look_up(const std::vector<Element>& my_set, 
                const OPRFValueVec& my_values, const OPRFValueVec& other_values) {
            std::unordered_map<OPRFValue, Element, OPRFValueHash> map;
            int cnt = my_set.size();
            for (int i = 0; i < cnt; ++i) map[my_values[i]] = my_set[i];

            std::vector<Element> rs;
            for (auto& value: other_values) {
                auto it = map.find(value);
                if(it != map.end()) rs.push_back(it->second);
            }
            return rs;
        }

};

}
#endif