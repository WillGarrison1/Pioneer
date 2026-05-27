#include "nnue.h"
#include "../board.h"
#include "../color.h"
#include <fstream>
#include <iostream>

NNUE* nnue = new NNUE;

bool NNUE::Load(const std::string& filename)
{
    std::ifstream params(filename, std::ios::binary);

    if (!params)
    {
        std::cerr << "Error: Could not open NNUE parameters file: " << filename << std::endl;
        return false;
    }

    // Load using old [520][NUM_FEATURES] layout
    using OldInputLayer = Layer<int16_t, int16_t, NUM_FEATURES, 520, 64, 127, false>;
    auto* temp = new OldInputLayer;
    params.read(reinterpret_cast<char*>(temp->weights), sizeof(temp->weights));
    params.read(reinterpret_cast<char*>(inputLayer.biases), sizeof(inputLayer.biases));

    // Transpose into inputLayer (IS_INPUT=true → weights[NUM_FEATURES][520])
    for (int i = 0; i < 520; i++)
        for (int j = 0; j < NUM_FEATURES; j++)
            inputLayer.weights[j][i] = temp->weights[i][j];

    delete temp;

    for (int i = 0; i < 8; i++)
    {
        auto& subnet = subnets[i];
        params.read(reinterpret_cast<char*>(subnet.hidden1.weights), sizeof(subnet.hidden1.weights));
    }
    for (int i = 0; i < 8; i++)
    {
        auto& subnet = subnets[i];
        params.read(reinterpret_cast<char*>(subnet.hidden1.biases), sizeof(subnet.hidden1.biases));
    }

    for (int i = 0; i < 8; i++)
    {
        auto& subnet = subnets[i];
        params.read(reinterpret_cast<char*>(subnet.hidden2.weights), sizeof(subnet.hidden2.weights));
    }
    for (int i = 0; i < 8; i++)
    {
        auto& subnet = subnets[i];
        params.read(reinterpret_cast<char*>(subnet.hidden2.biases), sizeof(subnet.hidden2.biases));
    }

    for (int i = 0; i < 8; i++)
    {
        auto& subnet = subnets[i];
        params.read(reinterpret_cast<char*>(subnet.hidden3.weights), sizeof(subnet.hidden3.weights));
    }
    for (int i = 0; i < 8; i++)
    {
        auto& subnet = subnets[i];
        params.read(reinterpret_cast<char*>(subnet.hidden3.biases), sizeof(subnet.hidden3.biases));
    }

    size_t paramsSize = params.tellg();
    params.seekg(0, std::ios::end);
    size_t fileSize = params.tellg();
    std::cout << "NNUE parameters: " << paramsSize << " bytes loaded from " << filename << " - file size: " << fileSize
              << std::endl;

    if (paramsSize != fileSize)
    {
        std::cerr << "Warning: file size does not match expected parameters size. The file may be corrupted or in an "
                     "incorrect format."
                  << std::endl;
    }

    return true;
}

float NNUE::Evaluate(const Board& board, const Accumulator& us, const Accumulator& them) const
{
    int numPieces = popCount(board.getBB(ALL_PIECES));
    int index = (numPieces - 2) / 4;

    assert(index >= 0 && index < 8); // Ensure index is within bounds

    float psqt = (us.psqt[index] - them.psqt[index]) / 2.0f;
    float output = Forward(index, us, them);
    return (output + psqt) * 500.0f / 127.0f;
}

float NNUE::Forward(int subnet, const Accumulator& us, const Accumulator& them) const
{
    // Feed the accumulators through the specified subnet and return the final evaluation
    return subnets[subnet].Forward(us, them);
}

float NNUE::FastEvaluate(const Board& board, const Accumulator& us, const Accumulator& them) const
{
    int numPieces = popCount(board.getBB(ALL_PIECES));
    int index = (numPieces - 2) / 4;

    assert(index >= 0 && index < 8); // Ensure index is within bounds

    float psqt = (us.psqt[index] - them.psqt[index]) / 2.0f;
    return psqt * 500.0f / 127.0f;
}

void NNUE::Add(Accumulator& acc, int add) const
{
    const int16_t* addW = inputLayer.weights[add];
    for (int i = 0; i < 512; i++)
    {
        acc.data[i] += addW[i];
    }
    for (int i = 0; i < 8; i++)
    {
        acc.psqt[i] += addW[i + 512];
    }
}

void NNUE::Sub(Accumulator& acc, int sub) const
{
    const int16_t* subW = inputLayer.weights[sub];
    for (int i = 0; i < 512; i++)
    {
        acc.data[i] -= subW[i];
    }
    for (int i = 0; i < 8; i++)
    {
        acc.psqt[i] -= subW[i + 512];
    }
}

void NNUE::AddSub(Accumulator& acc, int add, int sub) const
{
    const int16_t* addW = inputLayer.weights[add];
    const int16_t* subW = inputLayer.weights[sub];
    for (int i = 0; i < 512; i++)
    {
        acc.data[i] += addW[i] - subW[i];
    }
    for (int i = 0; i < 8; i++)
    {
        acc.psqt[i] += addW[i + 512] - subW[i + 512];
    }
}

void NNUE::AddSubSub(Accumulator& acc, int add, int sub1, int sub2) const
{
    const int16_t* addW = inputLayer.weights[add];
    const int16_t* sub1W = inputLayer.weights[sub1];
    const int16_t* sub2W = inputLayer.weights[sub2];
    for (int i = 0; i < 512; i++)
    {
        acc.data[i] += addW[i] - sub1W[i] - sub2W[i];
    }
    for (int i = 0; i < 8; i++)
    {
        acc.psqt[i] += addW[i + 512] - sub1W[i + 512] - sub2W[i + 512];
    }
}

void NNUE::AddAddSubSub(Accumulator& acc, int add1, int add2, int sub1, int sub2) const
{
    const int16_t* add1W = inputLayer.weights[add1];
    const int16_t* add2W = inputLayer.weights[add2];
    const int16_t* sub1W = inputLayer.weights[sub1];
    const int16_t* sub2W = inputLayer.weights[sub2];
    for (int i = 0; i < 512; i++)
    {
        acc.data[i] += add1W[i] + add2W[i] - sub1W[i] - sub2W[i];
    }
    for (int i = 0; i < 8; i++)
    {
        acc.psqt[i] += add1W[i + 512] + add2W[i + 512] - sub1W[i + 512] - sub2W[i + 512];
    }
}

void NNUE::Reset(Accumulator& acc) const
{
    std::memcpy(acc.data, inputLayer.biases, sizeof(acc.data));

    for (int i = 0; i < 8; i++)
    {
        acc.psqt[i] = inputLayer.biases[i + 512];
    }
}