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
        bool support_deletion = false;
        bool refresh_seeds = false;
        bool daily_vole = false;

        Dataset dataset;

        VoleSender vole_sender;
        VoleReceiver vole_receiver;

        oc::Socket* chl;
        oc::PRNG my_prng;
        oc::PRNG prng_del;

        oc::block ro_seed;

        size_t cur_vole_size;

        std::unordered_map<Element, bool> intersection;

        struct OPRFValueHash {
            size_t operator()(const OPRFValue& p) const noexcept {
                return std::hash<std::string_view>{}(
                    std::string_view(reinterpret_cast<const char*>(p.data()), p.size()));
            }
        };
        struct OPRFData{
            
            std::unordered_map<Element, OPRFValue> key_value;
            std::unordered_map<OPRFValue, Element, OPRFValueHash> value_key;
            void clear() {
                key_value.clear();
                value_key.clear();
            }
            std::vector<Element> get_keys() {
                std::vector<Element> rs;
                rs.reserve(key_value.size());
                for (const auto& kv : key_value) rs.push_back(kv.first);
                return rs;
            }
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

        Party(int _party, oc::Socket* _chl, int _total_days, std::string fn, bool deletion = false, bool daily_vole = false);

        virtual void setup() {
            if(dataset.start_size) {
                addition(dataset.initial_set);
                // intersection = dataset.intersection;
                for (const auto& cur_elem: dataset.intersection) intersection[cur_elem] = true;
                std::cout << "[VOLE] setup used: " << vole_receiver.idx << "\n";
            }
        }

        void run() {
            size_t vole_idx = vole_receiver.idx;
            size_t max_daily_vole = 0;
            size_t sum_vole = 0;
            size_t last_comm = chl->bytesSent() + chl->bytesReceived();
            size_t max_comm = 0;
            size_t sum_comm = 0;
            oc::Timer timer("party");
            timer.setTimePoint("begin");
            for (int i = 0; i < total_days; ++i) {

                if(daily_vole) cur_vole_size = 0;

                one_day();

                if(total_days <= 8) timer.setTimePoint("day " + std::to_string(i));

                if(!daily_vole) cur_vole_size = vole_receiver.idx - vole_idx;
                max_daily_vole = std::fmax(max_daily_vole, cur_vole_size);
                sum_vole += cur_vole_size;
                vole_idx += cur_vole_size;

                size_t cur_comm = chl->bytesSent() + chl->bytesReceived() - last_comm;
                max_comm = std::fmax(max_comm, cur_comm);
                sum_comm += cur_comm;
                last_comm += cur_comm;
            }

            if(total_days > 8) timer.setTimePoint("end");

            std::cout << "\n[VOLE] (receiver) used = " << sum_vole << ", amortized = " << sum_vole / total_days + 1 << ", max = " << max_daily_vole << "\n\n"; 
            std::cout << "[Time] \n" << timer << "\n\n";
            std::cout << "[Comm.(MB)] (both parties) total = " << sum_comm / 1024.0 / 1024.0
                << ", amortized = " << sum_comm / total_days / 1024.0 / 1024.0
                << ", max = " << max_comm / 1024.0 / 1024.0 << "\n\n";
        }

        void one_day() {
            if(refresh_seeds) {
                if(party == 0) {
                    ro_seed = oc::sysRandomSeed();
                    coproto::sync_wait(chl->send(ro_seed));
                }
                else {
                    coproto::sync_wait(chl->recv(ro_seed));
                }
            }
            int cnt_del = 0;
            if(support_deletion) cnt_del = deletion_part(dataset.daily_deletion[current_day]);
            int cnt_add = addition_part(dataset.daily_addition[current_day]);
            if(total_days <= 8) std::cout << "[Day " << current_day << "]: " << "-" << cnt_del << ", +" << cnt_add << std::endl;
            ++current_day;
        }

        int addition_part(const std::vector<Element>& addition_set);

        int deletion_part(const std::vector<Element>& deletion_set);

        virtual std::vector<Element> query(const std::vector<Element>& elems) {
            throw std::runtime_error("query not implemented");
        }
        
        virtual void addition(const std::vector<Element>& elems) {
            throw std::runtime_error("addition not implemented");
        }

        virtual void deletion(const std::vector<Element>& elems) {
            throw std::runtime_error("deletion not supported");
        }

        virtual void reset_all() {
            throw std::runtime_error("reset_all not supported");
        }

        virtual void refresh_oprfs() {
            throw std::runtime_error("refresh_oprfs not supported");

        }

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