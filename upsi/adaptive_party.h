#ifndef ADAPTIVE_PARTY_H
#define ADAPTIVE_PARTY_H

#include "party.h"
#include "adaptive.h"
#include "oprf.h"

namespace upsi{

template<typename BaseType>
class AdaptiveParty : public Party{
    static_assert(std::is_base_of<ASE, BaseType>::value, "AdaptiveParty: BaseType must derive from ASE");
    public: 
        Adaptive<PlainASE> my_adaptive; //elements in plaintext
        Adaptive<BaseType> my_adaptive_encrypted; //for deletions
        Adaptive<BaseType> my_vole; //for deletions
        Adaptive<BaseType> other_adaptive; //VOLE


        AdaptiveParty(int _party, oc::Socket* _chl, int _total_days, std::string fn, bool del = false, bool daily_vole = false):
                Party(_party, _chl, _total_days, fn, del, daily_vole){
            my_adaptive.setup(DEFAULT_ADAPTIVE_SIZE);
            other_adaptive.setup(DEFAULT_ADAPTIVE_SIZE);
            if(support_deletion) {
                my_adaptive_encrypted.setup(DEFAULT_ADAPTIVE_SIZE);
                my_vole.setup(DEFAULT_ADAPTIVE_SIZE);
            }
        }

        std::vector<Element> query(const std::vector<Element>& elems) override; // query for elems
        
        void addition(const std::vector<Element>& elems) override;

        void deletion(const std::vector<Element>& elems) override;
        
        void reset_all() override;

        void refresh_oprfs() override;
};

}

#endif