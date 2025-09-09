#ifndef PARTY_H
#define PARTY_H
#include "utils.h"
#include "ASE/ASE.h"
#include "vole.h"
#include "network.h"
#include "rbokvs/rb_okvs.h"
#include "oprf.h"

namespace upsi{

class Party{

    public: 

        int total_days;
        int current_day = 0;
        int max_data_size;
        int party; // 0 / 1

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

        Party(int _party, oc::Socket* _chl, int _total_days, int _max_data_size) {

            this->party = _party;
            this->chl = _chl;
            this->total_days = _total_days;
            this->max_data_size = _max_data_size;
            
            my_prng.SetSeed(oc::sysRandomSeed());
            vole_sender.setup(chl, &my_prng);
            vole_receiver.setup(chl, &my_prng);
            
            // TODO: max_vole_size?
            size_t max_vole_size = max_data_size * (4 * (oc::log2ceil(max_data_size) + 1) + 2) + 500 * (total_days + 1);
            if(party == 0) {
                vole_sender.generate(max_vole_size);
                vole_receiver.generate(max_vole_size);
            }
            else {
                vole_receiver.generate(max_vole_size);
                vole_sender.generate(max_vole_size);
            }
        }


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
                return rs;
            }
        }

        template<typename type> void random_shuffle(std::vector<type>& vec) {
            std::mt19937_64 rng{std::random_device{}()};
            std::shuffle(vec.begin(), vec.end(), rng);
        }

        std::vector<Element> PSI_receiver(const std::vector<Element>& my_set) {
            int cnt = my_set.size();
            rb_okvs okvs(cnt);
            okvs.build(my_set, ro_seed);

            auto vole = vole_receiver.get(okvs.n);
            ASE c = okvs - vole.second;
            oc::cp::sync_wait(send_ASE(c, chl));
            rb_okvs a = rb_okvs(std::move(vole.first));
            OPRFValueVec oprf_values;
            OPRF<rb_okvs> oprf_okvs;
            oprf_okvs.receiver(my_set, 0, a, oprf_values, ro_seed);

            OPRFValueVec other_oprf_values = oc::cp::sync_wait(recv_OPRF(chl));

            return look_up(my_set, oprf_values, other_oprf_values);
        }

        void PSI_sender(const std::vector<Element>& my_set) {
            ASE diff = oc::cp::sync_wait(recv_ASE(chl));
            diff *= vole_sender.delta;
            diff += vole_sender.get(diff.n);
            rb_okvs b = rb_okvs(std::move(diff));
            
            OPRFValueVec oprf_values;
            OPRF<rb_okvs> oprf_okvs;
            oprf_okvs.sender(my_set, 0, b, vole_sender.delta, oprf_values, ro_seed);
            
            std::mt19937_64 rng{std::random_device{}()};
            std::shuffle(oprf_values.begin(), oprf_values.end(), rng);
            oc::cp::sync_wait(send_OPRF(oprf_values, chl));
        }

};

}
#endif