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

    // Load the input layer parameters
    params.read(reinterpret_cast<char*>(&inputLayer), sizeof(inputLayer));

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

void NNUE::Add(Accumulator& acc, int index) const
{
    for (int i = 0; i < 512; i++)
    {
        acc.data[i] += inputLayer.weights[i][index];
    }
    for (int i = 0; i < 8; i++)
    {
        acc.psqt[i] += inputLayer.weights[i + 512][index];
    }
}

void NNUE::Remove(Accumulator& acc, int index) const
{
    for (int i = 0; i < 512; i++)
    {
        acc.data[i] -= inputLayer.weights[i][index];
    }
    for (int i = 0; i < 8; i++)
    {
        acc.psqt[i] -= inputLayer.weights[i + 512][index];
    }
}

void NNUE::Update(Accumulator& acc, int oldIndex, int newIndex) const
{
    for (int i = 0; i < 512; i++)
    {
        acc.data[i] += inputLayer.weights[i][newIndex] - inputLayer.weights[i][oldIndex];
    }
    for (int i = 0; i < 8; i++)
    {
        acc.psqt[i] += inputLayer.weights[i + 512][newIndex] - inputLayer.weights[i + 512][oldIndex];
    }
}

void NNUE::Reset(Accumulator& acc) const
{
    for (int i = 0; i < 512; i++)
    {
        acc.data[i] = inputLayer.biases[i];
    }
    for (int i = 0; i < 8; i++)
    {
        acc.psqt[i] = inputLayer.biases[i + 512];
    }
}