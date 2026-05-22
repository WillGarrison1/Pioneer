#include "subnet.h"

float Subnet::Forward(const Accumulator& us, const Accumulator& them) const
{
    int32_t hidden1_output[16];
    int32_t hidden2_output[32];
    hidden1.Forward(us, them, hidden1_output);
    hidden2.Forward(hidden1_output, hidden2_output);
    float output;
    hidden3.Forwardf(hidden2_output, &output);
    return output;
}