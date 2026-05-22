#ifndef SUBNET_H
#define SUBNET_H

#include "layer.h"

struct Subnet
{
    HiddenLayer<512 * 2, 16> hidden1;
    HiddenLayer<16, 32> hidden2;
    HiddenLayer<32, 1> hidden3;

    float Forward(const Accumulator& us, const Accumulator& them) const;
};

#endif