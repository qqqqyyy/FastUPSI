#include "cryptoTools/Common/CLP.h"
#include "data_util.h"

using namespace upsi;

int main(int argc, char** argv)
{
    oc::CLP clp(argc, argv);
    clp.setDefault("days", 8);
    clp.setDefault("start_size", 0);
    clp.setDefault("add_size", 1024);
    // clp.setDefault("start_size", 49);
    // clp.setDefault("add_size", 4);
    clp.setDefault("del_size", 0);

    int days = clp.get<int>("days");
    int start_size = clp.get<int>("start_size");
    int add_size = clp.get<int>("add_size");
    int del_size = clp.get<int>("del_size");

    auto dataset = GenerateSets(start_size, days, add_size, del_size);
    dataset.first.Write("party0.txt");
    dataset.second.Write("party1.txt");

    return 0;
}